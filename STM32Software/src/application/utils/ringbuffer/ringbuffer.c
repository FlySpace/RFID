/*
 * ringbuffer.c
 *
 *  Created on: 2013-8-30
 *      Author: Fly
 */

#include "ringbuffer.h"

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

RING_BUFFER_INDEX_TYPE ringBufferPut(struct RingBuffer * ringBuffer, const unsigned char * dataPtr,
		RING_BUFFER_INDEX_TYPE length)
{
	RING_BUFFER_INDEX_TYPE emptySize = ringBufferEmptySize(ringBuffer);
	if (length > emptySize)
	{
		length = emptySize;
	}
	if ((ringBuffer->readIndex ^ ringBuffer->writeIndex) & ringBuffer->_indexMSBMask)
	{
		memcpy(ringBuffer->pool + ringBuffer->writeIndex, dataPtr, length);
	}
	else
	{
		RING_BUFFER_INDEX_TYPE length1 = (-ringBuffer->writeIndex) & ringBuffer->_indexSizeMask;
		if (length > length1)
		{
			memcpy(ringBuffer->pool + ringBuffer->writeIndex, dataPtr, length1);
			memcpy(ringBuffer->pool, dataPtr + length1, length - length1);
		}
		else
		{
			memcpy(ringBuffer->pool + ringBuffer->writeIndex, dataPtr, length);
		}
	}
	ringBuffer->writeIndex = (ringBuffer->writeIndex + length) & ringBuffer->_indexMask;
	return (length);
}

RING_BUFFER_INDEX_TYPE ringBufferGet(struct RingBuffer * ringBuffer, unsigned char * dataPtr,
		RING_BUFFER_INDEX_TYPE length)
{
	RING_BUFFER_INDEX_TYPE dataSize = ringBufferDataSize(ringBuffer);
	if (length > dataSize)
	{
		length = dataSize;
	}
	if ((ringBuffer->readIndex ^ ringBuffer->writeIndex) & ringBuffer->_indexMSBMask)
	{
		RING_BUFFER_INDEX_TYPE length1 = (-ringBuffer->readIndex) & ringBuffer->_indexSizeMask;
		if (length > length1)
		{
			memcpy(dataPtr, ringBuffer->pool + ringBuffer->readIndex, length1);
			memcpy(dataPtr + length1, ringBuffer->pool, length - length1);
		}
		else
		{
			memcpy(dataPtr, ringBuffer->pool + ringBuffer->readIndex, length);
		}
	}
	else
	{
		memcpy(dataPtr, ringBuffer->pool + ringBuffer->readIndex, length);
	}
	ringBuffer->readIndex = (ringBuffer->readIndex + length) & ringBuffer->_indexMask;
	return (length);
}