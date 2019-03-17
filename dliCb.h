// The class which handles the frame received callback
// from the Decklink API

#include "spark.h"
#include "decklink/mac/DeckLinkAPI.h"

struct cbctrl_t {
	char *frontbuf, *backbuf;
	bool streaming;
};

class dliCb: public IDeckLinkInputCallback {
  public:
	HRESULT	VideoInputFormatChanged(BMDVideoInputFormatChangedEvents e, IDeckLinkDisplayMode* dm, BMDDetectedVideoInputFormatFlags f);
	HRESULT	VideoInputFrameArrived(IDeckLinkVideoInputFrame* f, IDeckLinkAudioInputPacket* a);
	HRESULT QueryInterface(REFIID iid, LPVOID *ppv);
	ULONG AddRef();
	ULONG Release();
};
