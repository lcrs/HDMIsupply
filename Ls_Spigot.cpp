#include <iostream>
#include "decklink/mac/DeckLinkAPI.h"
using namespace std;

int main(int argc, char **argv) {
	IDeckLinkIterator *dli = CreateDeckLinkIteratorInstance();
	IDeckLink *dl;

	while(dli->Next(&dl) == S_OK) {
		CFStringRef mns;				
		dl->GetModelName(&mns);

		char mn[99];		
		CFStringGetCString(mns, mn, sizeof(mn), kCFStringEncodingUTF8);
		cout << mn << endl;
	}
}
