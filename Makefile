Ls_Spigot: *.cpp *.h Makefile
	g++ -o Ls_Spigot Ls_Spigot.cpp spigotCb.cpp decklink/mac/DeckLinkAPIDispatch.cpp -framework CoreFoundation
