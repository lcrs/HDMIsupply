#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "spark.h"
#include "half.h"
#include "decklink/mac/DeckLinkAPI.h"
#include "dliCb.h"
using namespace std;

/*  TODO:
		funny business when called in low res for reels thumbnail...
		debug toggle
		visual frame drop indicator
		yuv headroom
		ram record n playback
		could we uhhh... do this as an OFX plugin, and on the GPU?
		note in readme that building on macos requires older xcode
		note in readme referring to robert hodgkin's code apologia

	PERF TODO:
		_cvtss_sh for half float conversion + cpu feature detection
		outputting 12bit and letting flame convert to half float might be faster
		check gcc vs clang
		pixfc's v210-to-r210 then our 10bit-to-half conversion?
		increase the ref count of the IDeckLinkVideoInputFrame instead of memcpy()?
*/

IDeckLinkInput *dlin = NULL;
dliCb cb;
int threadcount, w, h, v210rowbytes;
struct cbctrl_t *cbctrl;
const char *shmfile = NULL;

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

int SparkClips(void) {
	return 0;
}

void SparkMemoryTempBuffers(void) {
}

void startAnew(void) {
	// Create new control struct and buffers
	cout << "HDMIsupply: starting new instance" << endl;
	cbctrl = (cbctrl_t *)malloc(sizeof(cbctrl_t));
	cbctrl->frontbuf = (char *)malloc(v210rowbytes * h);
	cbctrl->backbuf = (char *)malloc(v210rowbytes * h);
	cbctrl->streaming = false;

	// Share pointer to control struct for future instances to pick up
	int shmfd = shm_open(shmfile, O_CREAT | O_RDWR, 0700);
	if(shmfd == -1) {
		cout << "HDMIsupply: new instance shm_open() returned " << errno << endl;
	}
	ftruncate(shmfd, sizeof(cbctrl_t *));
	void *shmptr = mmap(0, 8, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if(shmptr == MAP_FAILED) {
		cout << "HDMIsupply: new instance shm mmap() returned " << errno << endl;
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
		sparkError("failed to find DeckLink device!");
		return;
	}
	dl->QueryInterface(IID_IDeckLinkInput, (void **)&dlin);
	r = dlin->EnableVideoInput(dm, bmdFormat10BitYUV, bmdVideoInputFlagDefault);
	if(r != S_OK) {
		sparkError("failed to enable DeckLink video input!");
		return;
	}
	dlin->SetCallback(&cb);
	r = dlin->StartStreams();
	if(r != S_OK) {
		sparkError("failed to start DeckLink streams!");
		return;
	}
	cbctrl->streaming = true;
	cout << "HDMIsupply: input started at " << fps << "fps" << endl;
}

void stopIt(void) {
	if(dlin != NULL) {
		dlin->StopStreams();
		dlin->DisableVideoInput();
		cbctrl->streaming = false;
		cout << "HDMIsupply: streams stopped and input disabled" << endl;
	}
	if(shmfile != NULL) {
		shm_unlink(shmfile);
		cout << "HDMIsupply: shm file removed" << endl;
	}
}

unsigned int SparkInitialise(SparkInfoStruct si) {
	cout << "HDMIsupply: initialising" << endl;

	threadcount = si.NumProcessors;
	cout << "HDMIsupply: using " << threadcount << " threads" << endl;
	w = si.FrameWidth;
	h = si.FrameHeight;
	v210rowbytes = w * 8 / 3;
	cout << "HDMIsupply: resolution is " << w << "x" << h << endl;
	if(w != 1920 || h != 1080) {
		sparkError("resolution is not 1920x1080, cannot start!");
		return SPARK_MODULE;
	}

	// Check for existing instance
	ostringstream s;
	s << "HDMIsupply" << getpid();
	shmfile = strdup(s.str().c_str());
	cout << "HDMIsupply: using shm file " << shmfile << endl;
	int shmfd = shm_open(shmfile, O_RDONLY, 0700);
	if(shmfd == -1) {
		cout << "HDMIsupply: shm_open() returned " << errno << ", no instance found" << endl;
		startAnew();
	} else {
		cout << "HDMIsupply: found existing instance" << endl;
		void *shmptr = mmap(0, 8, PROT_READ, MAP_SHARED, shmfd, 0);
		if(shmptr == MAP_FAILED) {
			cout << "HDMIsupply: shm mmap() returned " << errno << endl;
			shmptr = NULL;
		}
		if(shmptr) {
			// Use buffers of existing instance
			cbctrl = *(cbctrl_t **)shmptr;
			cout << "HDMIsupply: found pointer to control struct at " << cbctrl << endl;
		}
	}
	close(shmfd);

	return SPARK_MODULE;
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
		half *rgb = (half *)((char *)to->Buffer + row * to->Stride);
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
			y0 = y0 - 64.0;
			y1 = y1 - 64.0;
			y2 = y2 - 64.0;
			y3 = y3 - 64.0;
			y4 = y4 - 64.0;
			y5 = y5 - 64.0;
			cr0 = cr0 - 512.0;
			cr2 = cr2 - 512.0;
			cr4 = cr4 - 512.0;
			cr6 = cr6 - 512.0;
			cb0 = cb0 - 512.0;
			cb2 = cb2 - 512.0;
			cb4 = cb4 - 512.0;
			cb6 = cb6 - 512.0;

			// Interpolate missing chroma samples from those either side
			float cr1 = (cr0 + cr2) * 0.5;
			float cr3 = (cr2 + cr4) * 0.5;
			float cr5 = (cr4 + cr6) * 0.5;
			float cb1 = (cb0 + cb2) * 0.5;
			float cb3 = (cb2 + cb4) * 0.5;
			float cb5 = (cb4 + cb6) * 0.5;

			// Apply Rec709 YCbCr to RGB matrix
			rgb[0] = (y0 * 1.164 + cb0 *  0.000 + cr0 *  1.793) / 1023.0;
			rgb[1] = (y0 * 1.164 + cb0 * -0.213 + cr0 * -0.533) / 1023.0;
			rgb[2] = (y0 * 1.164 + cb0 *  2.112 + cr0 *  0.000) / 1023.0;

			rgb[3] = (y1 * 1.164 + cb1 *  0.000 + cr1 *  1.793) / 1023.0;
			rgb[4] = (y1 * 1.164 + cb1 * -0.213 + cr1 * -0.533) / 1023.0;
			rgb[5] = (y1 * 1.164 + cb1 *  2.112 + cr1 *  0.000) / 1023.0;

			rgb[6] = (y2 * 1.164 + cb2 *  0.000 + cr2 *  1.793) / 1023.0;
			rgb[7] = (y2 * 1.164 + cb2 * -0.213 + cr2 * -0.533) / 1023.0;
			rgb[8] = (y2 * 1.164 + cb2 *  2.112 + cr2 *  0.000) / 1023.0;

			rgb[9]  = (y3 * 1.164 + cb3 *  0.000 + cr3 *  1.793) / 1023.0;
			rgb[10] = (y3 * 1.164 + cb3 * -0.213 + cr3 * -0.533) / 1023.0;
			rgb[11] = (y3 * 1.164 + cb3 *  2.112 + cr3 *  0.000) / 1023.0;

			rgb[12] = (y4 * 1.164 + cb4 *  0.000 + cr4 *  1.793) / 1023.0;
			rgb[13] = (y4 * 1.164 + cb4 * -0.213 + cr4 * -0.533) / 1023.0;
			rgb[14] = (y4 * 1.164 + cb4 *  2.112 + cr4 *  0.000) / 1023.0;

			rgb[15] = (y5 * 1.164 + cb5 *  0.000 + cr5 *  1.793) / 1023.0;
			rgb[16] = (y5 * 1.164 + cb5 * -0.213 + cr5 * -0.533) / 1023.0;
			rgb[17] = (y5 * 1.164 + cb5 *  2.112 + cr5 *  0.000) / 1023.0;

			// Move to next 32-byte v210 chunk
			v210 += 4;
			rgb += 6 * 3;
		}
	}
}

unsigned long *SparkProcess(SparkInfoStruct si) {
	if(!cbctrl->streaming) {
		cout << "HDMIsupply: streams have stopped, starting again..." << endl;
		startAnew();
	}

	if(w != 1920 || h != 1080) {
		sparkError("resolution is not 1920x1080, cannot process!");
		return 0;
	}

	static struct timespec s, e, last;
	last = s;
	clock_gettime(CLOCK_REALTIME, &s);
	float ms = (s.tv_nsec - last.tv_nsec) / 1000000.0;
	while(ms < 0.0) ms += 1000.0;
	cout << "HDMIsupply: " << ms << "ms since last process call   ";

	SparkMemBufStruct buf;
	sparkBuf(1, &buf);
	sparkMpFork((void(*)())threadProc, 2, cbctrl->frontbuf, &buf);

	clock_gettime(CLOCK_REALTIME, &e);
	ms = (e.tv_nsec - s.tv_nsec) / 1000000.0;
	cout << ms << "ms to convert buffer" << endl;

	return buf.Buffer; // N.B. this is some bullshit, the pointer returned is rudely ignored
}

void SparkUnInitialise(SparkInfoStruct si) {
	cout << "HDMIsupply: uninitialising" << endl;
	stopIt();
}
