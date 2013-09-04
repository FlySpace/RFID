/*
 * thread_card.c
 *
 *  Created on: 2013-8-19
 *      Author: Fly
 */

#include "rtthread.h"
#include "threads.h"
#include "ringbuffer.h"
#include "usart.h"

void thread_card(void * param)
{

}

rt_err_t matchArray(struct RingBuffer * ringBuffer, unsigned char * array, rt_size_t length, void * resultPtr)
{
	struct RingBuffer tempBuffer = *ringBuffer;
	RING_BUFFER_INDEX_TYPE len = ringBufferGet(&tempBuffer, resultPtr, length);
	if (len == length)
	{
		while (len--)
		{
			if (*(array++) != *(resultPtr++))
			{
				return (RT_ERROR);
			}
		}
		return (RT_EOK);
	}
	else
	{
		return (RT_ERROR);
	}
}

void waitRingBufferData(struct RingBuffer * pRingBuffer, struct UARTDevice * pUart, rt_size_t size,
		rt_tick_t delayPeriod)
{
	while (1)
	{
		rt_base_t lv = rt_hw_interrupt_disable();
		pRingBuffer->writeIndex = pUart->pRxBuffer.writeIndex;
		rt_hw_interrupt_enable(lv);
		if (ringBufferDataSize(pRingBuffer) < size)
		{
			rt_thread_delay(delayPeriod);
		}
		else
		{
			break;
		}
	}
}

rt_size_t findFirstPacket(struct UARTDevice * pUart, rt_uint8_t messageType, rt_uint8_t code)
{
	rt_uint8_t cmd3[3] =
	{ 0xBB, messageType, code };
	rt_uint8_t temp[3];
	struct RingBuffer tempRingBuffer;
	while (1)
	{
		tempRingBuffer = *(pUart->pRxBuffer);
		//0xBB-MessageType-Code
		waitUARTData(&tempRingBuffer, pUart, 3, 5);
		if (matchArray(pUart->pRxBuffer, cmd3, 3, temp) != RT_EOK)
		{
			rt_device_read(pUart->parent, 0, temp, 1);
			continue;
		}
		ringBufferGet(&tempRingBuffer, temp, 3);
		//PayloadLength
		waitUARTData(&tempRingBuffer, pUart, 2, 5);
		ringBufferGet(&tempRingBuffer, temp, 2);
		rt_uint16_t payloadLen = 0; //TODO temp[0]<<8
		//Payload-EndMask-CRC16
		waitUARTData(&tempRingBuffer, pUart, payloadLen + 3, 5);
		return (3 + 2 + payloadLen + 3);
	}
}

