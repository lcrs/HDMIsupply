CC = clang
CFLAGS = -std=c++11 -march=westmere -mtune=native -mf16c -fPIC -DDL_LITTLE_ENDIAN -O3 -Wno-deprecated-register -Wno-multichar
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

all: HDMIsupply.$(EXT)

HDMIsupply.$(EXT): HDMIsupply.o dliCb.o DeckLinkAPIDispatch.o Makefile
	$(CC) $(LDFLAGS) HDMIsupply.o dliCb.o DeckLinkAPIDispatch.o -o HDMIsupply.$(EXT)

HDMIsupply.o: HDMIsupply.cpp *.h Makefile spark.h
	$(CC) $(CFLAGS) -c HDMIsupply.cpp

dliCb.o: dliCb.cpp *.h Makefile
	$(CC) $(CFLAGS) -c dliCb.cpp

DeckLinkAPIDispatch.o: decklink/mac/DeckLinkAPIDispatch.cpp *.h Makefile
	$(CC) $(CFLAGS) -c decklink/mac/DeckLinkAPIDispatch.cpp

spark.h: Makefile
	ln -sf `ls /opt/Autodesk/presets/*/sparks/spark.h | head -n1` spark.h

clean:
	rm -f HDMIsupply.spark HDMIsupply.spark_x86_64 *.o spark.h
