// The class which handles the frame received callback
// from the Decklink API

#include <iostream>
#include "spigotCb.h"
using namespace std;

HRESULT spigotCb::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents e, IDeckLinkDisplayMode* dm, BMDDetectedVideoInputFormatFlags f) {
	return 0;
}

HRESULT	spigotCb::VideoInputFrameArrived(IDeckLinkVideoInputFrame* f, IDeckLinkAudioInputPacket* a) {
	void *b;
	f->GetBytes((void **)&b);
	cout << b << endl;
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
