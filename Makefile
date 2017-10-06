Ls_Spigot: Ls_Spigot.cpp Makefile
	g++ -o Ls_Spigot Ls_Spigot.cpp decklink/mac/DeckLinkAPIDispatch.cpp -lm -ldl -lpthread -framework CoreFoundation


#CC=g++
#SDK_PATH=../../include
#CFLAGS=-Wno-multichar -I $(SDK_PATH) -fno-rtti
#LDFLAGS=-lm -ldl -lpthread
#
#DeviceList: main.cpp $(SDK_PATH)/DeckLinkAPIDispatch.cpp
#	$(CC) -o DeviceList main.cpp $(SDK_PATH)/DeckLinkAPIDispatch.cpp $(CFLAGS) $(LDFLAGS)
#
#clean:
#	rm -f DeviceList
#
