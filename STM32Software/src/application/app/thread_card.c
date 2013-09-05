/*
 * thread_card.c
 *
 *  Created on: 2013-8-19
 *      Author: Fly
 */

#include "rtthread.h"
#include "threads.h"
#include "rthw.h"
#include "ringbuffer.h"
#include "usart.h"

static const rt_uint8_t cmdStartAutoRead2[] =
{ 0xBB, 0x00, 0x36, 0x00, 0x05, 0x02, 0x01, 0x00, 0x00, 0x00, 0x7E, 0x88, 0x5C };

static const rt_uint8_t rspStartAutoRead2CompleteHeader[] =
{ 0xBB, 0x01, 0x36 };

static const rt_uint8_t rspReadTypeCU2Header[] =
{ 0xBB, 0x02, 0x22 };

static const rt_uint8_t rspReadTypeCU2CompleteHeader[] =
{ 0xBB, 0x02, 0x36 };

static const rt_uint8_t cmdStopAutoRead2[] =
{ 0xBB, 0x00, 0x37, 0x00, 0x00, 0x7E, 0xF3, 0x91 };

static const rt_uint8_t rspStopAutoRead2CompleteHeader[] =
{ 0xBB, 0x01, 0x28 };

extern struct RingBuffer cardData;
rt_size_t findPacket(struct UARTDevice * pUart, rt_uint8_t messageType, rt_uint8_t code);

void thread_card(void * param)
{
	struct UARTDevice * pUart = (struct UARTDevice *) rt_device_find("uart2");
	rt_device_t uart2 = &pUart->parent;
	rt_size_t packetLen;
	rt_uint8_t * tempBuffer;

	//
	tempBuffer = rt_malloc(20);
	do
	{
		rt_thread_delay(10);
	} while (rt_device_read(uart2, 0, tempBuffer, 20) > 0);
	rt_free(tempBuffer);

	//
	rt_device_write(uart2, 0, cmdStartAutoRead2, sizeof(cmdStartAutoRead2));
	packetLen = findPacket(pUart, rspStartAutoRead2CompleteHeader[1], rspStartAutoRead2CompleteHeader[2]);
	tempBuffer = rt_malloc(packetLen);
	rt_device_read(uart2, 0, tempBuffer, packetLen);
	rt_free(tempBuffer);
	packetLen = findPacket(pUart, rspReadTypeCU2Header[1], rspReadTypeCU2Header[2]);
	tempBuffer = rt_malloc(packetLen);
	rt_device_read(uart2, 0, tempBuffer, packetLen);
	ringBufferPut(&cardData, tempBuffer, packetLen);
	rt_free(tempBuffer);
	packetLen = findPacket(pUart, rspReadTypeCU2CompleteHeader[1], rspReadTypeCU2CompleteHeader[2]);
	tempBuffer = rt_malloc(packetLen);
	rt_device_read(uart2, 0, tempBuffer, packetLen);
	rt_free(tempBuffer);
	rt_device_write(uart2, 0, cmdStopAutoRead2, sizeof(cmdStopAutoRead2));
	packetLen = findPacket(pUart, rspStopAutoRead2CompleteHeader[1], rspStopAutoRead2CompleteHeader[2]);
	tempBuffer = rt_malloc(packetLen);
	rt_device_read(uart2, 0, tempBuffer, packetLen);
	rt_free(tempBuffer);

}

rt_err_t matchArray(struct RingBuffer * ringBuffer, unsigned char * array, rt_size_t length, unsigned char * resultPtr)
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

rt_size_t findPacket(struct UARTDevice * pUart, rt_uint8_t messageType, rt_uint8_t code)
{
	rt_uint8_t cmd3[3] =
	{ 0xBB, messageType, code };
	rt_uint8_t temp[3];
	struct RingBuffer tempRingBuffer;
	while (1)
	{
		tempRingBuffer = pUart->pRxBuffer;
		//0xBB-MessageType-Code
		waitRingBufferData(&tempRingBuffer, pUart, 3, 5);
		if (matchArray(&pUart->pRxBuffer, cmd3, 3, temp) != RT_EOK)
		{
			rt_device_read(&pUart->parent, 0, temp, 1);
			continue;
		}
		ringBufferGet(&tempRingBuffer, temp, 3);
		//PayloadLength
		waitRingBufferData(&tempRingBuffer, pUart, 2, 5);
		ringBufferGet(&tempRingBuffer, temp, 2);
		rt_uint16_t payloadLen = temp[0] << 8 + temp[1];
		//Payload-EndMask-CRC16
		waitRingBufferData(&tempRingBuffer, pUart, payloadLen + 3, 5);
		return (3 + 2 + payloadLen + 3);
	}
}

