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

	half *rgb = (half *)buf.Buffer;
	int *v210 = (int *)fbuf;

	/* TODO:
		flip
		double buffering
		re-entrancy
		chroma interpolation
		speed
		safety
	*/
	while(rgb < (half *)buf.Buffer + (1920 * 1080 * 3)) {
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

		y0 = y0 - 64.0;
		y1 = y1 - 64.0;
		y2 = y2 - 64.0;
		y3 = y3 - 64.0;
		y4 = y4 - 64.0;
		y5 = y5 - 64.0;
		cr0 = cr0 - 512.0;
		cr2 = cr2 - 512.0;
		cr4 = cr4 - 512.0;
		cb0 = cb0 - 512.0;
		cb2 = cb2 - 512.0;
		cb4 = cb4 - 512.0;

		rgb[0] = (y0 * 1.164 + cb0 *  0.000 + cr0 *  1.793) / 1023.0;
		rgb[1] = (y0 * 1.164 + cb0 * -0.213 + cr0 * -0.533) / 1023.0;
		rgb[2] = (y0 * 1.164 + cb0 *  2.112 + cr0 *  0.000) / 1023.0;

		rgb[3] = (y1 * 1.164 + cb0 *  0.000 + cr0 *  1.793) / 1023.0;
		rgb[4] = (y1 * 1.164 + cb0 * -0.213 + cr0 * -0.533) / 1023.0;
		rgb[5] = (y1 * 1.164 + cb0 *  2.112 + cr0 *  0.000) / 1023.0;

		rgb[6] = (y2 * 1.164 + cb2 *  0.000 + cr2 *  1.793) / 1023.0;
		rgb[7] = (y2 * 1.164 + cb2 * -0.213 + cr2 * -0.533) / 1023.0;
		rgb[8] = (y2 * 1.164 + cb2 *  2.112 + cr2 *  0.000) / 1023.0;

		rgb[9]  = (y3 * 1.164 + cb2 *  0.000 + cr2 *  1.793) / 1023.0;
		rgb[10] = (y3 * 1.164 + cb2 * -0.213 + cr2 * -0.533) / 1023.0;
		rgb[11] = (y3 * 1.164 + cb2 *  2.112 + cr2 *  0.000) / 1023.0;

		rgb[12] = (y4 * 1.164 + cb4 *  0.000 + cr4 *  1.793) / 1023.0;
		rgb[13] = (y4 * 1.164 + cb4 * -0.213 + cr4 * -0.533) / 1023.0;
		rgb[14] = (y4 * 1.164 + cb4 *  2.112 + cr4 *  0.000) / 1023.0;

		rgb[15] = (y5 * 1.164 + cb4 *  0.000 + cr4 *  1.793) / 1023.0;
		rgb[16] = (y5 * 1.164 + cb4 * -0.213 + cr4 * -0.533) / 1023.0;
		rgb[17] = (y5 * 1.164 + cb4 *  2.112 + cr4 *  0.000) / 1023.0;

		v210 += 4;
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
