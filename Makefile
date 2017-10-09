CFLAGS = -march=core2 -mtune=native -fPIC -DDL_LITTLE_ENDIAN
LDFLAGS = -fPIC

ifeq ($(shell uname), Darwin)
	CFLAGS += -D_DARWIN_USE_64_BIT_INODE
	LDFLAGS += -dynamiclib -undefined dynamic_lookup -framework CoreFoundation
	EXT = spark
endif
ifeq ($(shell uname), Linux)
	CFLAGS += -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
	LDFLAGS += -shared -Bsymbolic
	EXT = spark_x86_64
endif

all: Ls_Spigot.$(EXT)

Ls_Spigot.$(EXT): Ls_Spigot.o spigotCb.o DeckLinkAPIDispatch.o Makefile
	g++ $(LDFLAGS) Ls_Spigot.o spigotCb.o DeckLinkAPIDispatch.o -o Ls_Spigot.$(EXT)

Ls_Spigot.o: Ls_Spigot.cpp *.h Makefile
	g++ $(CFLAGS) -c Ls_Spigot.cpp

spigotCb.o: spigotCb.cpp *.h Makefile
	g++ $(CFLAGS) -c spigotCb.cpp

DeckLinkAPIDispatch.o: decklink/mac/DeckLinkAPIDispatch.cpp *.h Makefile
	g++ $(CFLAGS) -c decklink/mac/DeckLinkAPIDispatch.cpp

spark.h: Makefile
	ln -sf `ls /usr/discreet/presets/*/sparks/spark.h | head -n1` spark.h

clean:
	rm -f Ls_Spigot.spark Ls_Spigot.spark_x86_64 *.o spark.h
