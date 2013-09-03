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
	if (length > emptySize)
	{
		length = emptySize;
	}
	if ((ringBuffer->readIndex ^ ringBuffer->writeIndex) & ringBuffer->_indexMSBMask)
	{
		memcpy(ringBuffer->pool + (ringBuffer->writeIndex & ringBuffer->_indexSizeMask), dataPtr, length);
	}
	else
	{
		RING_BUFFER_INDEX_TYPE length1 = ringBuffer->_indexMSBMask
				- (ringBuffer->writeIndex & ringBuffer->_indexSizeMask);
		if (length > length1)
		{
			memcpy(ringBuffer->pool + (ringBuffer->writeIndex & ringBuffer->_indexSizeMask), dataPtr, length1);
			memcpy(ringBuffer->pool, dataPtr + length1, length - length1);
		}
		else
		{
			memcpy(ringBuffer->pool + (ringBuffer->writeIndex & ringBuffer->_indexSizeMask), dataPtr, length);
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
		RING_BUFFER_INDEX_TYPE length1 = ringBuffer->_indexMSBMask
				- (ringBuffer->readIndex & ringBuffer->_indexSizeMask);
		if (length > length1)
		{
			memcpy(dataPtr, ringBuffer->pool + (ringBuffer->readIndex & ringBuffer->_indexSizeMask), length1);
			memcpy(dataPtr + length1, ringBuffer->pool, length - length1);
		}
		else
		{
			memcpy(dataPtr, ringBuffer->pool + (ringBuffer->readIndex & ringBuffer->_indexSizeMask), length);
		}
	}
	else
	{
		memcpy(dataPtr, ringBuffer->pool + (ringBuffer->readIndex & ringBuffer->_indexSizeMask), length);
	}
	ringBuffer->readIndex = (ringBuffer->readIndex + length) & ringBuffer->_indexMask;
	return (length);
}
