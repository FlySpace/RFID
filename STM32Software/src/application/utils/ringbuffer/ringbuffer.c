/*
 * ringbuffer.c
 *
 *  Created on: 2013-8-30
 *      Author: Fly
 */

#include "ringbuffer.h"
#include "string.h"

void ringBufferInit(struct RingBuffer * ringBuffer, unsigned char * poolPtr, unsigned char sizeBitCount)
{
	ringBuffer->pool = poolPtr;
	ringBuffer->bufferSizeBitCount = sizeBitCount;
	ringBuffer->writeIndex = 0;
	ringBuffer->readIndex = 0;

	ringBuffer->_indexMask = ((RING_BUFFER_INDEX_TYPE) 1 << (sizeBitCount + 1)) - (RING_BUFFER_INDEX_TYPE) 1;
	ringBuffer->_indexMSBMask = (RING_BUFFER_INDEX_TYPE) 1 << sizeBitCount;
	ringBuffer->_indexSizeMask = ((RING_BUFFER_INDEX_TYPE) 1 << sizeBitCount) - (RING_BUFFER_INDEX_TYPE) 1;
}

RING_BUFFER_INDEX_TYPE ringBufferPut(struct RingBuffer * ringBuffer, unsigned char * dataPtr,
		RING_BUFFER_INDEX_TYPE length)
{
	RING_BUFFER_INDEX_TYPE emptySize = ringBufferEmptySize(ringBuffer);
	RING_BUFFER_INDEX_TYPE wi = ringBuffer->writeIndex & ringBuffer->_indexSizeMask;
	if (length > emptySize)
	{
		length = emptySize;
	}
	if ((ringBuffer->readIndex ^ ringBuffer->writeIndex) & ringBuffer->_indexMSBMask)
	{
		memcpy(ringBuffer->pool + wi, dataPtr, length);
	}
	else
	{
		RING_BUFFER_INDEX_TYPE length1 = ringBuffer->_indexMSBMask - wi;
		if (length > length1)
		{
			memcpy(ringBuffer->pool + wi, dataPtr, length1);
			memcpy(ringBuffer->pool, dataPtr + length1, length - length1);
		}
		else
		{
			memcpy(ringBuffer->pool + wi, dataPtr, length);
		}
	}
	ringBuffer->writeIndex = (ringBuffer->writeIndex + length) & ringBuffer->_indexMask;
	return (length);
}

RING_BUFFER_INDEX_TYPE ringBufferGet(struct RingBuffer * ringBuffer, unsigned char * dataPtr,
		RING_BUFFER_INDEX_TYPE length)
{
	RING_BUFFER_INDEX_TYPE dataSize = ringBufferDataSize(ringBuffer);
	RING_BUFFER_INDEX_TYPE ri = ringBuffer->readIndex & ringBuffer->_indexSizeMask;
	if (length > dataSize)
	{
		length = dataSize;
	}
	if ((ringBuffer->readIndex ^ ringBuffer->writeIndex) & ringBuffer->_indexMSBMask)
	{
		RING_BUFFER_INDEX_TYPE length1 = ringBuffer->_indexMSBMask - ri;
		if (length > length1)
		{
			memcpy(dataPtr, ringBuffer->pool + ri, length1);
			memcpy(dataPtr + length1, ringBuffer->pool, length - length1);
		}
		else
		{
			memcpy(dataPtr, ringBuffer->pool + ri, length);
		}
	}
	else
	{
		memcpy(dataPtr, ringBuffer->pool + ri, length);
	}
	ringBuffer->readIndex = (ringBuffer->readIndex + length) & ringBuffer->_indexMask;
	return (length);
}
