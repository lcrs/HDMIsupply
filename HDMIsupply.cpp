#include <iostream>
#include <sstream>
#include <initializer_list>
#include <sys/time.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <emmintrin.h>
#include "spark.h"
#include "half.h"
#include "decklink/mac/DeckLinkAPI.h"
#include "dliCb.h"
using namespace std;

/*  TODO:
		first processed frame usually black because callback thread hasn't run yet
		at least one call to free() would be polite
		visual frame drop indicator
		yuv headroom
		ram record n playback
		could we uhhh... do this as an OFX plugin, and on the GPU?
		note in readme that building on macos requires older xcode
		note in readme referring to robert hodgkin's code apologia

	PERF TODO:
		_cvtss_sh for half float conversion
		outputting 12bit and letting flame convert to half float might be faster
		check gcc vs clang
		pixfc's v210-to-r210 then our 10bit-to-half conversion?
		increase the ref count of the IDeckLinkVideoInputFrame instead of memcpy()?
*/

IDeckLinkInput *dlin = NULL;
dliCb cb;
int threadcount, w, h, v210rowbytes;
struct cbctrl_t *cbctrl = NULL;
char *shmfile = NULL;
bool debuglog = false;
bool avx = false;

void say(initializer_list<string> v) {
	if(!debuglog) return;
	cout << "HDMIsupply: ";
	initializer_list<string>::iterator i;
	for(i = v.begin(); i != v.end(); i++) {
		cout << *i;
	}
	cout << endl;
}

void err(initializer_list<string> v) {
	ostringstream s;
	s << "HDMIsupply: ";
	initializer_list<string>::iterator i;
	for(i = v.begin(); i != v.end(); i++) {
		s << *i;
	}
	s << endl;
	cout << s.str();
	sparkError(s.str().c_str());
}

int sparkBuf(int n, SparkMemBufStruct *b) {
	if(!sparkMemGetBuffer(n, b)) {
		cout << "HDMIsupply: sparkMemGetBuffer() failed: " << n << endl;
		return 0;
	}
	if(!(b->BufState & MEMBUF_LOCKED)) {
		cout << "HDMIsupply: buffer " << n << " not locked" << endl;
		return 0;
	}
	return 1;
}

int SparkIsInputFormatSupported(SparkPixelFormat fmt) {
	switch(fmt) {
		case SPARKBUF_RGB_48_3x16_FP:
			return 1;
		default:
			return 0;
	}
}

void threadProc(char *from, SparkMemBufStruct *to) {
	unsigned long offset, pixels;
	sparkMpInfo(&offset, &pixels);
	int thread = round(threadcount * (float)offset / (w * h));
	int rowcount = h / threadcount;
	int rowstart = thread * rowcount;

	// Last thread also gets remaining rows when height is not divisible by threadcount
	if(thread == threadcount - 1) rowcount += h - (rowcount * threadcount);

	for(int row = rowstart; row < rowstart + rowcount; row++) {
		unsigned short *rgb = (unsigned short *)((char *)to->Buffer + row * to->Stride);
		int *v210 = (int *)((from + v210rowbytes * h) - (row + 1) * v210rowbytes);

		for(int chunk = 0; chunk < w / 6; chunk++) {
			// Unpack 6 10bit YCbCr pixels from this 4:2:2 v210 format 32-byte chunk
			float y0 = (v210[0] >> 10) & 0x000003ff;
			float y1 = (v210[1] >>  0) & 0x000003ff;
			float y2 = (v210[1] >> 20) & 0x000003ff;
			float y3 = (v210[2] >> 10) & 0x000003ff;
			float y4 = (v210[3] >>  0) & 0x000003ff;
			float y5 = (v210[3] >> 20) & 0x000003ff;
			float cr0 = (v210[0] >> 20) & 0x000003ff;
			float cr2 = (v210[2] >>  0) & 0x000003ff;
			float cr4 = (v210[3] >> 10) & 0x000003ff;
			float cb0 = (v210[0] >> 00) & 0x000003ff;
			float cb2 = (v210[1] >> 10) & 0x000003ff;
			float cb4 = (v210[2] >> 20) & 0x000003ff;

			// We need the next two chroma samples for interpolation
			float cr6, cb6;
			if(chunk == (w / 6) - 1) {
				// ...but not if we would read beyond this row
				cr6 = cr4;
				cb6 = cb4;
			} else {
				cr6 = (v210[4] >> 20) & 0x000003ff;
				cb6 = (v210[4] >> 00) & 0x000003ff;
			}

			// Remove offsets, gains are handled in the matrix below
			y0 = y0 - 64.0f;
			y1 = y1 - 64.0f;
			y2 = y2 - 64.0f;
			y3 = y3 - 64.0f;
			y4 = y4 - 64.0f;
			y5 = y5 - 64.0f;
			cr0 = cr0 - 512.0f;
			cr2 = cr2 - 512.0f;
			cr4 = cr4 - 512.0f;
			cr6 = cr6 - 512.0f;
			cb0 = cb0 - 512.0f;
			cb2 = cb2 - 512.0f;
			cb4 = cb4 - 512.0f;
			cb6 = cb6 - 512.0f;

			// Interpolate missing chroma samples from those either side
			float cr1 = (cr0 + cr2) * 0.5f;
			float cr3 = (cr2 + cr4) * 0.5f;
			float cr5 = (cr4 + cr6) * 0.5f;
			float cb1 = (cb0 + cb2) * 0.5f;
			float cb3 = (cb2 + cb4) * 0.5f;
			float cb5 = (cb4 + cb6) * 0.5f;

			// Apply Rec709 YCbCr to RGB matrix
			// Could use: unsigned short _cvtss_sh(float x, int imm)
			rgb[0] = _cvtss_sh((y0 * 1.164f + cb0 *  0.000f + cr0 *  1.793f) / 1023.0f, 0);
			rgb[1] = _cvtss_sh((y0 * 1.164f + cb0 * -0.213f + cr0 * -0.533f) / 1023.0f, 0);
			rgb[2] = _cvtss_sh((y0 * 1.164f + cb0 *  2.112f + cr0 *  0.000f) / 1023.0f, 0);

			rgb[3] = _cvtss_sh((y1 * 1.164f + cb1 *  0.000f + cr1 *  1.793f) / 1023.0f, 0);
			rgb[4] = _cvtss_sh((y1 * 1.164f + cb1 * -0.213f + cr1 * -0.533f) / 1023.0f, 0);
			rgb[5] = _cvtss_sh((y1 * 1.164f + cb1 *  2.112f + cr1 *  0.000f) / 1023.0f, 0);

			rgb[6] = _cvtss_sh((y2 * 1.164f + cb2 *  0.000f + cr2 *  1.793f) / 1023.0f, 0);
			rgb[7] = _cvtss_sh((y2 * 1.164f + cb2 * -0.213f + cr2 * -0.533f) / 1023.0f, 0);
			rgb[8] = _cvtss_sh((y2 * 1.164f + cb2 *  2.112f + cr2 *  0.000f) / 1023.0f, 0);

			rgb[9]  = _cvtss_sh((y3 * 1.164f + cb3 *  0.000f + cr3 *  1.793f) / 1023.0f, 0);
			rgb[10] = _cvtss_sh((y3 * 1.164f + cb3 * -0.213f + cr3 * -0.533f) / 1023.0f, 0);
			rgb[11] = _cvtss_sh((y3 * 1.164f + cb3 *  2.112f + cr3 *  0.000f) / 1023.0f, 0);

			rgb[12] = _cvtss_sh((y4 * 1.164f + cb4 *  0.000f + cr4 *  1.793f) / 1023.0f, 0);
			rgb[13] = _cvtss_sh((y4 * 1.164f + cb4 * -0.213f + cr4 * -0.533f) / 1023.0f, 0);
			rgb[14] = _cvtss_sh((y4 * 1.164f + cb4 *  2.112f + cr4 *  0.000f) / 1023.0f, 0);

			rgb[15] = _cvtss_sh((y5 * 1.164f + cb5 *  0.000f + cr5 *  1.793f) / 1023.0f, 0);
			rgb[16] = _cvtss_sh((y5 * 1.164f + cb5 * -0.213f + cr5 * -0.533f) / 1023.0f, 0);
			rgb[17] = _cvtss_sh((y5 * 1.164f + cb5 *  2.112f + cr5 *  0.000f) / 1023.0f, 0);

			// Move to next 32-byte v210 chunk
			v210 += 4;
			rgb += 6 * 3;
		}
	}
}

void stopHDMI(void) {
	say({"stopping streams..."});
	if(cbctrl != NULL) cbctrl->streaming = false;
	if(dlin != NULL) {
		dlin->StopStreams();
		dlin->DisableVideoInput();
		say({"streams stopped and input disabled"});
		if(shmfile != NULL) {
			shm_unlink(shmfile);
			shmfile = NULL;
			say({"shm file removed"});
		}
	}
}

void startHDMI(void) {
	// Create new control struct and buffers
	say({"starting new instance"});
	cbctrl = (cbctrl_t *)malloc(sizeof(cbctrl_t));
	cbctrl->frontbuf = (char *)malloc(v210rowbytes * h);
	cbctrl->backbuf = (char *)malloc(v210rowbytes * h);
	cbctrl->streaming = false;

	// Share pointer to control struct for future instances to pick up
	int shmfd = shm_open(shmfile, O_CREAT | O_RDWR, 0700);
	if(shmfd == -1) {
		say({"new instance shm_open() returned: ", strerror(errno)});
	}
	ftruncate(shmfd, sizeof(cbctrl_t *));
	void *shmptr = mmap(0, 8, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if(shmptr == MAP_FAILED) {
		say({"new instance shm mmap() returned: ", strerror(errno)});
		shmptr = NULL;
		return;
	}
	*(cbctrl_t **)shmptr = cbctrl;
	close(shmfd);

	// Set up DeckLink device and start its streams
	double fps = sparkFrameRate();
	BMDDisplayMode dm = bmdModeHD1080p2398;
	if(fps == 24.0) dm = bmdModeHD1080p24;
	if(fps == 25.0) dm = bmdModeHD1080p25;
	if(abs(fps - 29.97) < 0.01) dm = bmdModeHD1080p2997;
	if(fps == 30.0) dm = bmdModeHD1080p30;
	if(fps == 50.0) dm = bmdModeHD1080p50;
	if(abs(fps - 59.94) < 0.01) dm = bmdModeHD1080p5994;
	if(fps == 60.0) dm = bmdModeHD1080p6000;

	IDeckLink *dl;
	IDeckLinkIterator *dli = CreateDeckLinkIteratorInstance();
	HRESULT r;
	r = dli->Next(&dl);
	if(r != S_OK) {
		err({"failed to find DeckLink device!"});
		stopHDMI();
		return;
	}
	dl->QueryInterface(IID_IDeckLinkInput, (void **)&dlin);
	r = dlin->EnableVideoInput(dm, bmdFormat10BitYUV, bmdVideoInputFlagDefault);
	if(r != S_OK) {
		err({"failed to enable DeckLink video input!"});
		stopHDMI();
		return;
	}
	dlin->SetCallback(&cb);
	r = dlin->StartStreams();
	if(r != S_OK) {
		err({"failed to start DeckLink streams!"});
		stopHDMI();
		return;
	}
	cbctrl->streaming = true;
	say({"input started at ", to_string(fps), "fps"});
}

unsigned int SparkInitialise(SparkInfoStruct si) {
	if(getenv("HDMISUPPLY_DEBUG")) debuglog = true;
	say({"initialising"});

	threadcount = si.NumProcessors;
	say({"using ", to_string(threadcount), " threads"});
	avx = __builtin_cpu_supports("avx");
	if(avx) say({"CPU supports AVX"});

	w = si.FrameWidth;
	h = si.FrameHeight;
	say({"resolution is ",  to_string(w), "x", to_string(h)});
	if(w != 1920 || h != 1080) {
		err({"resolution is not 1920x1080, cannot start!"});
		return SPARK_MODULE;
	}
	v210rowbytes = w * 8 / 3;

	// Check for existing instance
	ostringstream s;
	s << "HDMIsupply" << getpid();
	shmfile = strdup(s.str().c_str());
	say({"using shm file ", shmfile});
	int shmfd = shm_open(shmfile, O_RDONLY, 0700);
	if(shmfd == -1) {
		say({"shm_open() returned ", strerror(errno), ", no instance found"});
		startHDMI();
	} else {
		say({"found existing instance"});
		void *shmptr = mmap(0, 8, PROT_READ, MAP_SHARED, shmfd, 0);
		if(shmptr == MAP_FAILED) {
			say({"shm mmap() returned ", strerror(errno)});
			shmptr = NULL;
		}
		if(shmptr) {
			// Use buffers of existing instance
			cbctrl = *(cbctrl_t **)shmptr;
			say({"found pointer to control struct at ", to_string((long)cbctrl)});
		}
	}
	close(shmfd);

	return SPARK_MODULE;
}

unsigned long *SparkProcess(SparkInfoStruct si) {
	if(w != 1920 || h != 1080) {
		err({"resolution is not 1920x1080, cannot process!"});
		return 0;
	}

	if(!cbctrl->streaming) {
		say({"streams have stopped, starting again..."});
		SparkInitialise(si);
	}

	static struct timespec s, e, last;
	last = s;
	clock_gettime(CLOCK_REALTIME, &s);
	float msp = (s.tv_nsec - last.tv_nsec) / 1000000.0;
	if(msp < 0.0) msp += 1000.0;

	SparkMemBufStruct buf;
	sparkBuf(1, &buf);
	sparkMpFork((void(*)())threadProc, 2, cbctrl->frontbuf, &buf);

	clock_gettime(CLOCK_REALTIME, &e);
	float msc = (e.tv_nsec - s.tv_nsec) / 1000000.0;
	if(msc < 0.0) msc += 1000.0;
	say({to_string(msp), "ms since last call ", to_string(msc), "ms to convert buffer"});

	return buf.Buffer; // N.B. this is some bullshit, the pointer returned is rudely ignored
}

void SparkUnInitialise(SparkInfoStruct si) {
}

int SparkClips(void) {
	return 0;
}

void SparkMemoryTempBuffers(void) {
}
