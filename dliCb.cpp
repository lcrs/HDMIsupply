// The class which handles the frame received callback
// from the Decklink API

#include <iostream>
#include "dliCb.h"
using namespace std;

extern cbctrl_t *cbctrl;
extern int w, h, v210rowbytes;

HRESULT dliCb::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents e, IDeckLinkDisplayMode *dm, BMDDetectedVideoInputFormatFlags f) {
	return 0;
}

HRESULT	dliCb::VideoInputFrameArrived(IDeckLinkVideoInputFrame *f, IDeckLinkAudioInputPacket *a) {
	void *b;
	f->GetBytes(&b);

	// Copy DeckLink-provided buffer to our own
	memcpy(cbctrl->backbuf, b, v210rowbytes * h);

	// Set this new buffer as the "front" one to be picked up by the main thread
	char *t;
	t = cbctrl->frontbuf;
	cbctrl->frontbuf = cbctrl->backbuf;
	cbctrl->backbuf = t;

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
