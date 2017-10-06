#include <iostream>
#include "decklink/mac/DeckLinkAPI.h"
using namespace std;

int main(int argc, char **argv) {
	IDeckLinkIterator *dli = CreateDeckLinkIteratorInstance();
	IDeckLink *dl;
	IDeckLinkInput *dlin;
	spigotCb cb;

	dli->Next(&dl);
	dl->QueryInterface(IID_IDeckLinkInput, (void **)&dlin);
	dlin->EnableVideoInput(bmdModeHD1080p25, bmdFormat10BitYUV, bmdVideoInputFlagDefault);
	dlin->SetCallback(&cb);
}
