// The class which handles the frame received callback
// from the Decklink API

#include <iostream>
#include "spigotCb.h"
using namespace std;

extern char *fb1, *fb2;
extern int readyfb;

HRESULT spigotCb::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents e, IDeckLinkDisplayMode *dm, BMDDetectedVideoInputFormatFlags f) {
	return 0;
}

HRESULT	spigotCb::VideoInputFrameArrived(IDeckLinkVideoInputFrame *f, IDeckLinkAudioInputPacket *a) {
	void *b;
	f->GetBytes(&b);

	char *fbuf;
	if(readyfb == 1) {
		fbuf = fb2;
	} else {
		fbuf = fb1;
	}

	memcpy(fbuf, b, 5120 * 1080);

	if(readyfb == 1) {
		readyfb = 2;
	} else {
		readyfb = 1;
	}

	return 0;
}

HRESULT spigotCb::QueryInterface(REFIID iid, LPVOID *ppv) {
	return 0;
}

ULONG spigotCb::AddRef() {
	return 0;
}

ULONG spigotCb::Release() {
	return 0;
}
