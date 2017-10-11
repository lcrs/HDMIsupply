#include <iostream>
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
			cout << "Ls_Spigot: unhandled pixel depth " << fmt <<", failing!" << endl;
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
	SparkMemBufStruct buf;
	sparkBuf(1, &buf);

	for(int row = 0; row < 1080; row++) {
		for(int col = 0; col < 1920; col+=6) {
			char *v210_addr = (char *)fbuf + (1080 - row) * 5120 + ((col / 6) * 16);
			char *rgb_addr = (char *)buf.Buffer + row * buf.Stride + col * buf.Inc;
			uint32_t *v210 = (uint32_t *)v210_addr;
			half *rgb = (half *)rgb_addr;
			uint16_t y0 = (*v210 >> 10) & 0x000003ff;
			rgb[0] = rgb[3] = rgb[6] = rgb[9] = rgb[12] = rgb[15] = y0 / 1024.0;
		}
	}

	return buf.Buffer; // N.B. this is some bullshit, the pointer returned is rudely ignored
}

void SparkUnInitialise(SparkInfoStruct si) {
	dlin->StopStreams();
	dlin->DisableVideoInput();
	cout << "streams stopped" << endl << flush;
}
