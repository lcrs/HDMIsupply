
// Test how aggressive Flame's caching of node results is
// lewis@lewissaunders.com

#include "spark.h"
#include "half.h"
#include <iostream>
using namespace std;


float uniqueseed = 0.0;
/*
SparkFloatStruct SparkFloat7 = {
	0.0,						// Initial
	-INFINITY,					// Min
	+INFINITY,					// Max
	0.001,						// Increment
	0,							// Flags
	(char *) "Unique seed %f",	// Name
	NULL						// Callback
};
*/

int getbuf(int n, SparkMemBufStruct *b) {
	if(!sparkMemGetBuffer(n, b)) {
		printf("sparkMemGetBuffer() failed: %d\n", n);
		return(0);
	}
	if(!(b->BufState & MEMBUF_LOCKED)) {
		printf("buffer %d not locked\n", n);
		return(0);
	}
	return(1);
}

unsigned long *SparkProcess(SparkInfoStruct si) {
	SparkMemBufStruct result;

	if(!getbuf(1, &result)) return(NULL);
	half *buf = (half *)result.Buffer;
	memset(result.Buffer, 0, 1280*100);
	buf[(int)uniqueseed*3] = 1.0;
	uniqueseed++;

/*
	// Increment unqiue seed
	// This is an attempt to defeat Flame's caching, which presumably
	// relies on hashing this value
	uniqueseed += 1.0;
	SparkFloat7.Value = uniqueseed;
	cout << si.FrameNo << endl;
	sparkSetCurveKey(SPARK_UI_CONTROL, 7, si.FrameNo + 1, uniqueseed);
	sparkControlUpdate(7);
*/
	return(result.Buffer);
}

unsigned int SparkInitialise(SparkInfoStruct si) {
	return(SPARK_MODULE);
}

void SparkUnInitialise(SparkInfoStruct sparkInfo) {
}

int SparkIsInputFormatSupported(SparkPixelFormat fmt) {
	switch(fmt) {
		case SPARKBUF_RGB_48_3x16_FP:
			return(1);
		default:
			return(0);
	}
}

int SparkClips(void) {
	return(0);
}

void SparkMemoryTempBuffers(void) { /* Must be defined to keep Batch happy */ }
