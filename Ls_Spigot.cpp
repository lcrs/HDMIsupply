#include <iostream>
#include <unistd.h>
#include "spark.h"
#include "decklink/mac/DeckLinkAPI.h"
#include "spigotCb.h"
using namespace std;

IDeckLinkInput *dlin;
spigotCb cb;
IDeckLinkVideoInputFrame *fNext = NULL;

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
		case SPARKBUF_RGB_24_3x8:
		case SPARKBUF_RGB_48_3x10:
		case SPARKBUF_RGB_48_3x12:
		case SPARKBUF_RGB_48_3x16_FP:
			return 1;
		break;
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
	
	if(fNext == NULL) {
		usleep(10 * 1000);
		return buf.Buffer;
	}

	void *b;
	fNext->GetBytes(&b);
	int stride = fNext->GetRowBytes();
	int h = fNext->GetHeight();

	memcpy(buf.Buffer, b, h * stride);

	fNext->Release();
	fNext = NULL;

	return buf.Buffer; // N.B. this is some bullshit, the pointer returned is rudely ignored
}

void SparkUnInitialise(SparkInfoStruct si) {
	dlin->StopStreams();
	dlin->DisableVideoInput();
	cout << "streams stopped" << endl << flush;
}
