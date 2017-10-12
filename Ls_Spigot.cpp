#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "spark.h"
#include "half.h"
#include "decklink/mac/DeckLinkAPI.h"
#include "spigotCb.h"
using namespace std;

IDeckLinkInput *dlin;
spigotCb cb;
void *fbuf;

int sparkBuf(int n, SparkMemBufStruct *b) {
	if(!sparkMemGetBuffer(n, b)) {
		cout << "Ls_Spigot: sparkMemGetBuffer() failed: " << n << endl;
		return(0);
	}
	if(!(b->BufState & MEMBUF_LOCKED)) {
		cout << "Ls_Spigot: buffer " << n << " not locked" << endl;
		return(0);
	}
	return(1);
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
	return(0);
}

void SparkMemoryTempBuffers(void) {
}

unsigned int SparkInitialise(SparkInfoStruct si) {
	IDeckLinkIterator *dli;
	IDeckLink *dl;

	fbuf = malloc(5120 * 1080);

	dli = CreateDeckLinkIteratorInstance();
	dli->Next(&dl);
	dl->QueryInterface(IID_IDeckLinkInput, (void **)&dlin);
	dlin->EnableVideoInput(bmdModeHD1080p25, bmdFormat10BitYUV, bmdVideoInputFlagDefault);
	dlin->SetCallback(&cb);
	dlin->StartStreams();
	cout << "streams started" << endl << flush;

	return(SPARK_MODULE);
}

unsigned long *SparkProcess(SparkInfoStruct si) {
	static struct timespec s, e, last;
	last = s;
	clock_gettime(CLOCK_REALTIME, &s);
	float ms = (s.tv_nsec - last.tv_nsec) / 1000000.0;
	cout << ms << "ms since last process call" << endl;
	
	SparkMemBufStruct buf;
	sparkBuf(1, &buf);

	uint16_t *rgb = (uint16_t *)buf.Buffer;
	uint32_t *v210 = (uint32_t *)fbuf;


	while(rgb < (uint16_t *)buf.Buffer + (1920 * 1080 * 3)) {
		uint16_t y0 = (v210[0] >> 10) & 0x000003ff;
		uint16_t y1 = (v210[1] >>  0) & 0x000003ff;
		uint16_t y2 = (v210[1] >> 20) & 0x000003ff;
		uint16_t y3 = (v210[2] >> 10) & 0x000003ff;
		uint16_t y4 = (v210[3] >>  0) & 0x000003ff;
		uint16_t y5 = (v210[3] >> 20) & 0x000003ff;
		v210 += 4;

		rgb[0] = y0 << 4;
		rgb[3] = y1 << 4;
		rgb[6] = y2 << 4;
		rgb[9] = y3 << 4;
		rgb[12] = y4 << 4;
		rgb[15] = y5 << 4;
		rgb += 6 * 3;
	}

	clock_gettime(CLOCK_REALTIME, &e);
	ms = (e.tv_nsec - s.tv_nsec) / 1000000.0;
	cout << ms << "ms to convert buffer" << endl;


	return buf.Buffer; // N.B. this is some bullshit, the pointer returned is rudely ignored
}

void SparkUnInitialise(SparkInfoStruct si) {
	dlin->StopStreams();
	dlin->DisableVideoInput();
	cout << "streams stopped" << endl << flush;
}
