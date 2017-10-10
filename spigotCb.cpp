// The class which handles the frame received callback
// from the Decklink API

#include <iostream>
#include "spigotCb.h"
using namespace std;

extern IDeckLinkVideoInputFrame *fNext;

HRESULT spigotCb::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents e, IDeckLinkDisplayMode *dm, BMDDetectedVideoInputFormatFlags f) {
	return 0;
}

HRESULT	spigotCb::VideoInputFrameArrived(IDeckLinkVideoInputFrame *f, IDeckLinkAudioInputPacket *a) {
	f->AddRef();
	fNext = f;
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
