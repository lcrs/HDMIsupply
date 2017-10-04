CFLAGS = -march=core2 -mtune=native -fPIC -DDL_LITTLE_ENDIAN
LDFLAGS = -fPIC

ifeq ($(shell uname), Darwin)
	CFLAGS += -D_DARWIN_USE_64_BIT_INODE
	LDFLAGS += -dynamiclib -undefined dynamic_lookup
	EXT = spark
endif
ifeq ($(shell uname), Linux)
	CFLAGS += -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
	LDFLAGS += -shared -Bsymbolic
	EXT = spark_x86_64
endif

all: batchcachetest.$(EXT)

batchcachetest.$(EXT): batchcachetest.o Makefile
	g++ $(LDFLAGS) batchcachetest.o -o batchcachetest.$(EXT)

batchcachetest.o: batchcachetest.cpp half.h halfExport.h spark.h Makefile
	g++ $(CFLAGS) -c batchcachetest.cpp -o batchcachetest.o

spark.h: Makefile
	ln -sf `ls /usr/discreet/presets/*/sparks/spark.h | head -n1` spark.h

clean:
	rm -f batchcachetest.spark batchcachetest.spark_x86_64 batchcachetest.o spark.h
