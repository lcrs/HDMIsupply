# Remember some people do still have Core2Duos or whatever...
CFLAGS = -march=native -mtune=native -fPIC -DDL_LITTLE_ENDIAN -O3
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

Ls_Spigot.o: Ls_Spigot.cpp *.h Makefile spark.h
	g++ $(CFLAGS) -c Ls_Spigot.cpp

spigotCb.o: spigotCb.cpp *.h Makefile
	g++ $(CFLAGS) -c spigotCb.cpp

DeckLinkAPIDispatch.o: decklink/mac/DeckLinkAPIDispatch.cpp *.h Makefile
	g++ $(CFLAGS) -c decklink/mac/DeckLinkAPIDispatch.cpp

spark.h: Makefile
	ln -sf `ls /opt/Autodesk/presets/*/sparks/spark.h | head -n1` spark.h

clean:
	rm -f Ls_Spigot.spark Ls_Spigot.spark_x86_64 *.o spark.h
