// The class which handles the frame received callback
// from the Decklink API

#include "decklink/mac/DeckLinkAPI.h"

class spigotCb : public IDeckLinkInputCallback {
  public:
	HRESULT	VideoInputFormatChanged(BMDVideoInputFormatChangedEvents e, IDeckLinkDisplayMode* dm, BMDDetectedVideoInputFormatFlags f);
	HRESULT	VideoInputFrameArrived(IDeckLinkVideoInputFrame* f, IDeckLinkAudioInputPacket* a);
	HRESULT QueryInterface(REFIID iid, LPVOID *ppv);
	ULONG AddRef();
	ULONG Release();
};
