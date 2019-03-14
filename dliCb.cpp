// The class which handles the frame received callback
// from the Decklink API

#include <iostream>
#include "dliCb.h"
using namespace std;

extern char *frontbuf, *backbuf;
extern int w, h, v210rowbytes;

HRESULT dliCb::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents e, IDeckLinkDisplayMode *dm, BMDDetectedVideoInputFormatFlags f) {
	return 0;
}

HRESULT	dliCb::VideoInputFrameArrived(IDeckLinkVideoInputFrame *f, IDeckLinkAudioInputPacket *a) {
	void *b;
	f->GetBytes(&b);

	memcpy(backbuf, b, v210rowbytes * h);

	char *t;
	t = frontbuf;
	frontbuf = backbuf;
	backbuf = t;

	return 0;
}

HRESULT dliCb::QueryInterface(REFIID iid, LPVOID *ppv) {
	return 0;
}

ULONG dliCb::AddRef() {
	return 0;
}

ULONG dliCb::Release() {
	return 0;
}
