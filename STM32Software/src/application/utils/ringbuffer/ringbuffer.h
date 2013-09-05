/*
 * RingBuffer.h
 *
 *  Created on: 2013-8-30
 *      Author: Fly
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include "ringbuffer-config.h"

#ifndef RING_BUFFER_INDEX_TYPE
#define RING_BUFFER_INDEX_TYPE unsigned int
#endif

struct RingBuffer
{
	unsigned char * pool;
	RING_BUFFER_INDEX_TYPE writeIndex;
	RING_BUFFER_INDEX_TYPE readIndex;
	unsigned char bufferSizeBitCount;

	RING_BUFFER_INDEX_TYPE _indexMask;		//count+1
	RING_BUFFER_INDEX_TYPE _indexMSBMask;	//count+1
	RING_BUFFER_INDEX_TYPE _indexSizeMask;	//count
};

#define ringBufferDataSize(ringBuffer) (((ringBuffer)->writeIndex - (ringBuffer)->readIndex) & (ringBuffer)->_indexMask)
#define ringBufferEmptySize(ringBuffer) ((ringBuffer)->_indexMSBMask - ringBufferDataSize(ringBuffer))

void ringBufferInit(struct RingBuffer * ringBuffer, unsigned char * poolPtr, unsigned char sizeBitCount);
RING_BUFFER_INDEX_TYPE ringBufferPut(struct RingBuffer * ringBuffer, unsigned char * dataPtr,
		RING_BUFFER_INDEX_TYPE length);
RING_BUFFER_INDEX_TYPE ringBufferGet(struct RingBuffer * ringBuffer, unsigned char * dataPtr,
		RING_BUFFER_INDEX_TYPE length);

#endif /* RINGBUFFER_H_ */
