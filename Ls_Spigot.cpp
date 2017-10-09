#include <iostream>
#include <unistd.h>
#include "spark.h"
#include "decklink/mac/DeckLinkAPI.h"
#include "spigotCb.h"
using namespace std;

int bufid;
SparkMemBufStruct buf;
IDeckLinkInput *dlin;
spigotCb cb;
char *dlbuf;

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
	bufid = sparkMemRegisterBuffer();
}

unsigned int SparkInitialise(SparkInfoStruct si) {
	IDeckLinkIterator *dli;
	IDeckLink *dl;

	dli = CreateDeckLinkIteratorInstance();
	cout << "dli " << dli << endl;
	dli->Next(&dl);
	cout << "dl " << dl << endl;
	dl->QueryInterface(IID_IDeckLinkInput, (void **)&dlin);
	cout << "dlin " << dlin << endl << flush;
	dlin->EnableVideoInput(bmdModeHD1080p25, bmdFormat10BitYUV, bmdVideoInputFlagDefault);
	dlin->SetCallback(&cb);
	dlin->StartStreams();

	dlbuf = (char *) malloc(20 * 1024 * 1024);
	return(SPARK_MODULE);
}

unsigned long *SparkProcess(SparkInfoStruct si) {
	sparkBuf(1, &buf);
	memcpy(buf.Buffer, dlbuf, 2 * 1024 * 1024);
	return buf.Buffer; // N.B. this is some bullshit, the pointer returned is rudely ignored
}

void SparkUnInitialise(SparkInfoStruct si) {
	dlin->StopStreams();
	dlin->DisableVideoInput();
	cout << "streams stopped" << endl << flush;
}
