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
#include "string.h"

rt_err_t findPacket(rt_size_t * packetLen, struct UARTDevice * pUart, rt_uint8_t messageType, rt_uint8_t code,
		rt_tick_t timeOut);
void mallocAfterFree(rt_size_t size, uint8_t ** mem, uint8_t * memFlag);
void * mlc(rt_size_t size, uint8_t * memFlag);
rt_err_t fr(void * mem, uint8_t * memFlag);
uint16_t mallocReadAreaCmd(uint8_t ** buffer, uint8_t * memFlag, uint32_t ap, uint8_t * epc, uint16_t epcLen,
		uint8_t mb, uint16_t sa, uint16_t dl);
uint16_t mallocWriteAreaCmd(uint8_t ** buffer, uint8_t * bufferFlag, uint32_t ap, uint8_t * epc, uint16_t epcLen,
		uint8_t mb, uint16_t sa, uint8_t * data, uint16_t dl);
void thread_autoLookAndDeleteCardData(void * param);

static void rspTimeOut(void * param);

struct rt_semaphore cardDataSem;
enum CardOperation operation = NO;
uint16_t writeOffset;
uint16_t writeLength;
uint8_t * writeBuffer;
uint8_t writeBufferFlag;

static const rt_uint8_t cmdReset[] =
{ 0xBB, 0x00, 0x08, 0x00, 0x00, 0x7E, 0x0B, 0x96 };

static const rt_uint8_t cmdSetRegionChina2[] =
{ 0xBB, 0x00, 0x07, 0x00, 0x01, 0x52, 0x7E, 0xA9, 0x70 };

static const rt_uint8_t rspSetRegionChina2CompleteHeader[] =
{ 0xBB, 0x01, 0x07 };

static const rt_uint8_t cmdSetRFChannel10[] =
{ 0xBB, 0x00, 0x12, 0x00, 0x02, 0x0A, 0x00, 0x7E, 0xD5, 0xFA };

static const rt_uint8_t rspSetRFChannel10[] =
{ 0xBB, 0x01, 0x12 };

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

static const rt_uint8_t rspReadArea[] =
{ 0xBB, 0x01, 0x29 };

static const rt_uint8_t rspWriteArea[] =
{ 0xBB, 0x01, 0x46 };

static uint8_t isRspTimeOut;
static struct rt_timer rspTimeOutTimer;
static struct rt_thread threadAutoLookAndDeleteCardData;
static uint8_t threadAutoLookAndDeleteCardDataStack[THREAD_AUTO_LOOK_AND_DELETE_CARD_DATA_STACK_SIZE];

static rt_uint8_t * tempBuffer;
static uint8_t tempBufferFlag = 0;
static rt_uint8_t * autoReadPacket;
static uint8_t autoReadPacketFlag = 0;
static rt_uint8_t * reservedArea;
static uint8_t reservedAreaFlag = 0;
static rt_uint8_t * epcArea;
static uint8_t epcAreaFlag = 0;
static rt_uint8_t * tidArea;
static uint8_t tidAreaFlag = 0;
static rt_uint8_t * userArea;
static uint8_t userAreaFlag = 0;
void thread_card(void * param)
{
	rt_device_t uart2 = rt_device_find("uart2");
	struct UARTDevice * pUart = (struct UARTDevice *) uart2;
	rt_timer_init(&rspTimeOutTimer, "rspt", rspTimeOut, RT_NULL, 500, RT_TIMER_FLAG_ONE_SHOT);
	rt_sem_init(&cardDataSem, "cdSem", 0, RT_IPC_FLAG_FIFO);
	rt_thread_init(&threadAutoLookAndDeleteCardData, "autoLk", thread_autoLookAndDeleteCardData, RT_NULL,
			threadAutoLookAndDeleteCardDataStack, THREAD_AUTO_LOOK_AND_DELETE_CARD_DATA_STACK_SIZE, 8, 10);

	rt_size_t packetLen;
	rt_size_t autoReadPacketLen;
	uint8_t lastTypeCU2CRC[2] =
	{ 0, 0 };
	rt_tick_t lastTypeCU2Tick = rt_tick_get();

	rt_device_open(uart2, RT_DEVICE_OFLAG_RDWR);
	while (1)
	{
		//Reset
		rt_device_write(uart2, 0, cmdReset, sizeof(cmdReset));
		rt_thread_delay(1000);
		mallocAfterFree(20, &tempBuffer, &tempBufferFlag);
		do
		{
			rt_thread_delay(10);
		} while (rt_device_read(uart2, 0, tempBuffer, 20) > 0);
		//Set Region to China2
		rt_device_write(uart2, 0, cmdSetRegionChina2, sizeof(cmdSetRegionChina2));
		if (findPacket(&packetLen, pUart, rspSetRegionChina2CompleteHeader[1], rspSetRegionChina2CompleteHeader[2],
				500)!=RT_EOK)
		{
			continue;
		}
		mallocAfterFree(packetLen, &tempBuffer, &tempBufferFlag);
		rt_device_read(uart2, 0, tempBuffer, packetLen);
		//Set RF Channel to 10
		rt_device_write(uart2, 0, cmdSetRFChannel10, sizeof(cmdSetRFChannel10));
		if (findPacket(&packetLen, pUart, rspSetRFChannel10[1], rspSetRFChannel10[2], 500) != RT_EOK)
		{
			continue;
		}
		mallocAfterFree(packetLen, &tempBuffer, &tempBufferFlag);
		rt_device_read(uart2, 0, tempBuffer, packetLen);
		//Start Auto Read
		rt_device_write(uart2, 0, cmdStartAutoRead2, sizeof(cmdStartAutoRead2));
		if (findPacket(&packetLen, pUart, rspStartAutoRead2CompleteHeader[1], rspStartAutoRead2CompleteHeader[2],
				500)!=RT_EOK)
		{
			continue;
		}
		mallocAfterFree(packetLen, &tempBuffer, &tempBufferFlag);
		rt_device_read(uart2, 0, tempBuffer, packetLen);
		//Read TypeC U2
		if (findPacket(&autoReadPacketLen, pUart, rspReadTypeCU2Header[1], rspReadTypeCU2Header[2], 1000) != RT_EOK)
		{
			continue;
		}
		mallocAfterFree(autoReadPacketLen, &autoReadPacket, &autoReadPacketFlag);
		rt_device_read(uart2, 0, autoReadPacket, autoReadPacketLen);
		// Read TypeC U2 Complete
		if (findPacket(&packetLen, pUart, rspReadTypeCU2CompleteHeader[1], rspReadTypeCU2CompleteHeader[2],
				500)!=RT_EOK)
		{
			continue;
		}
		mallocAfterFree(packetLen, &tempBuffer, &tempBufferFlag);
		rt_device_read(uart2, 0, tempBuffer, packetLen);
		//Stop Auto Read
		rt_device_write(uart2, 0, cmdStopAutoRead2, sizeof(cmdStopAutoRead2));
		if (findPacket(&packetLen, pUart, rspStopAutoRead2CompleteHeader[1], rspStopAutoRead2CompleteHeader[2],
				500)!=RT_EOK)
		{
			continue;
		}
		mallocAfterFree(packetLen, &tempBuffer, &tempBufferFlag);
		rt_device_read(uart2, 0, tempBuffer, packetLen);
		//Look Condition
		uint8_t sameCard = 0;
		if (lastTypeCU2CRC[0] == autoReadPacket[autoReadPacketLen - 2]
				&& lastTypeCU2CRC[1] == autoReadPacket[autoReadPacketLen - 1] && rt_tick_get() - lastTypeCU2Tick < 2000)
		{
			sameCard = 1;
		}
		lastTypeCU2CRC[0] = autoReadPacket[autoReadPacketLen - 2];
		lastTypeCU2CRC[1] = autoReadPacket[autoReadPacketLen - 1];
		lastTypeCU2Tick = rt_tick_get();
		//Operation
		rt_enter_critical();
		enum CardOperation tOperation = operation;
		uint16_t tWriteOffset = writeOffset;
		uint16_t tWriteLength = writeLength;
		uint8_t * tWriteBuffer = writeBuffer;
		uint8_t tWriteBufferFlag;
		rt_exit_critical();
		mallocAfterFree(tWriteLength, &tWriteBuffer, &tWriteBufferFlag);
		memcpy(tWriteBuffer, writeBuffer, tWriteLength);
		switch (tOperation)
		{
		case NO:
		{
			break;
		}
		case AUTO_READ:
		{
			if (sameCard)
			{
				continue;
			}
			//Read Area
			struct CardDataHeader header;
			//Read Reserved Area
			packetLen = mallocReadAreaCmd(&tempBuffer, &tempBufferFlag, 0x00000000, autoReadPacket + 7,
					autoReadPacketLen - 10, 0x00, 0x0000, 0x0000);
			rt_device_write(uart2, 0, tempBuffer, packetLen);
			if (findPacket(&packetLen, pUart, rspReadArea[1], rspReadArea[2], 500) != RT_EOK)
			{
				continue;
			}
			mallocAfterFree(packetLen, &reservedArea, &reservedAreaFlag);
			rt_device_read(uart2, 0, reservedArea, packetLen);
			header.reserved = packetLen - 8;
			//Read EPC Area
			packetLen = mallocReadAreaCmd(&tempBuffer, &tempBufferFlag, 0x00000000, autoReadPacket + 7,
					autoReadPacketLen - 10, 0x01, 0x0000, 0x0000);
			rt_device_write(uart2, 0, tempBuffer, packetLen);
			if (findPacket(&packetLen, pUart, rspReadArea[1], rspReadArea[2], 500) != RT_EOK)
			{
				continue;
			}
			mallocAfterFree(packetLen, &epcArea, &epcAreaFlag);
			rt_device_read(uart2, 0, epcArea, packetLen);
			header.epc = packetLen - 8;
			//Read TID Area
			packetLen = mallocReadAreaCmd(&tempBuffer, &tempBufferFlag, 0x00000000, autoReadPacket + 7,
					autoReadPacketLen - 10, 0x02, 0x0000, 0x0000);
			rt_device_write(uart2, 0, tempBuffer, packetLen);
			if (findPacket(&packetLen, pUart, rspReadArea[1], rspReadArea[2], 500) != RT_EOK)
			{
				continue;
			}
			mallocAfterFree(packetLen, &tidArea, &tidAreaFlag);
			rt_device_read(uart2, 0, tidArea, packetLen);
			header.tid = packetLen - 8;
			//Read User Area
			packetLen = mallocReadAreaCmd(&tempBuffer, &tempBufferFlag, 0x00000000, autoReadPacket + 7,
					autoReadPacketLen - 10, 0x03, 0x0000, 0x0000);
			rt_device_write(uart2, 0, tempBuffer, packetLen);
			if (findPacket(&packetLen, pUart, rspReadArea[1], rspReadArea[2], 500) != RT_EOK)
			{
				continue;
			}
			mallocAfterFree(packetLen, &userArea, &userAreaFlag);
			rt_device_read(uart2, 0, userArea, packetLen);
			header.user = packetLen - 8;
			//Save
			rt_enter_critical();
			if (ringBufferEmptySize(&cardData)
					>= sizeof(header) + header.reserved + header.epc + header.tid + header.user)
			{
				Get_Time(RTC_GetCounter(), &header.time);
				ringBufferPut(&cardData, (uint8_t *) &header, sizeof(header));
				ringBufferPut(&cardData, reservedArea + 5, header.reserved);
				ringBufferPut(&cardData, epcArea + 5, header.epc);
				ringBufferPut(&cardData, tidArea + 5, header.tid);
				ringBufferPut(&cardData, userArea + 5, header.user);
				rt_sem_release(&cardDataSem);
			}
			rt_exit_critical();
			break;
		}
		case WRITE_RESERVED:
		{
			packetLen = mallocWriteAreaCmd(&tempBuffer, &tempBufferFlag, 0x00000000, autoReadPacket + 7,
					autoReadPacketLen - 10, 0x00, tWriteOffset, tWriteBuffer, tWriteLength);
			rt_device_write(uart2, 0, tempBuffer, packetLen);
			if (findPacket(&packetLen, pUart, rspWriteArea[1], rspWriteArea[2], 500) != RT_EOK)
			{
				continue;
			}
			mallocAfterFree(packetLen, &tempBuffer, &tempBufferFlag);
			rt_device_read(uart2, 0, tempBuffer, packetLen);
			operation = NO;
			break;
		}
		case WRITE_EPC:
		{
			packetLen = mallocWriteAreaCmd(&tempBuffer, &tempBufferFlag, 0x00000000, autoReadPacket + 7,
					autoReadPacketLen - 10, 0x01, tWriteOffset, tWriteBuffer, tWriteLength);
			rt_device_write(uart2, 0, tempBuffer, packetLen);
			if (findPacket(&packetLen, pUart, rspWriteArea[1], rspWriteArea[2], 500) != RT_EOK)
			{
				continue;
			}
			mallocAfterFree(packetLen, &tempBuffer, &tempBufferFlag);
			rt_device_read(uart2, 0, tempBuffer, packetLen);
			operation = NO;
			break;
		}
		case WRITE_TID:
		{
			packetLen = mallocWriteAreaCmd(&tempBuffer, &tempBufferFlag, 0x00000000, autoReadPacket + 7,
					autoReadPacketLen - 10, 0x02, tWriteOffset, tWriteBuffer, tWriteLength);
			rt_device_write(uart2, 0, tempBuffer, packetLen);
			if (findPacket(&packetLen, pUart, rspWriteArea[1], rspWriteArea[2], 500) != RT_EOK)
			{
				continue;
			}
			mallocAfterFree(packetLen, &tempBuffer, &tempBufferFlag);
			rt_device_read(uart2, 0, tempBuffer, packetLen);
			operation = NO;
			break;
		}
		case WRITE_USER:
		{
			packetLen = mallocWriteAreaCmd(&tempBuffer, &tempBufferFlag, 0x00000000, autoReadPacket + 7,
					autoReadPacketLen - 10, 0x03, tWriteOffset, tWriteBuffer, tWriteLength);
			rt_device_write(uart2, 0, tempBuffer, packetLen);
			if (findPacket(&packetLen, pUart, rspWriteArea[1], rspWriteArea[2], 500) != RT_EOK)
			{
				continue;
			}
			mallocAfterFree(packetLen, &tempBuffer, &tempBufferFlag);
			rt_device_read(uart2, 0, tempBuffer, packetLen);
			operation = NO;
			break;
		}
		}
	}
}

static void rspTimeOut(void * param)
{
	isRspTimeOut = 1;
}

void mallocAfterFree(rt_size_t size, uint8_t ** mem, uint8_t * memFlag)
{
	if (*memFlag)
	{
		*memFlag = 0;
		rt_free(*mem);
	}
	*mem = rt_malloc(size);
	if (*mem != RT_NULL)
	{
		*memFlag = 1;
	}
}

void * mlc(rt_size_t size, uint8_t * memFlag)
{
	uint8_t * m = rt_malloc(size);
	if (m != RT_NULL)
	{
		*memFlag = 1;
	}
	return (m);
}

rt_err_t fr(void * mem, uint8_t * memFlag)
{
	if (*memFlag)
	{
		*memFlag = 0;
		rt_free(mem);
	}
	return (RT_EOK);
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

rt_err_t waitRingBufferData(struct RingBuffer * pRingBuffer, struct UARTDevice * pUart, rt_size_t size,
		rt_tick_t delayPeriod)
{
	while (!isRspTimeOut)
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
			return (RT_EOK);
		}
	}
	return (RT_ETIMEOUT);
}

rt_err_t findPacket(rt_size_t * packetLen, struct UARTDevice * pUart, rt_uint8_t messageType, rt_uint8_t code,
		rt_tick_t timeOut)
{
	rt_uint8_t cmd3[3] =
	{ 0xBB, messageType, code };
	rt_uint8_t temp[3];
	struct RingBuffer tempRingBuffer;
	rt_timer_stop(&rspTimeOutTimer);
	rt_timer_control(&rspTimeOutTimer, RT_TIMER_CTRL_SET_TIME, &timeOut);
	isRspTimeOut = 0;
	rt_timer_start(&rspTimeOutTimer);
	while (1)
	{
		tempRingBuffer = pUart->pRxBuffer;
		//0xBB-MessageType-Code
		if (waitRingBufferData(&tempRingBuffer, pUart, 3, 5) != RT_EOK)
		{
			return (RT_ETIMEOUT);
		}
		if (matchArray(&pUart->pRxBuffer, cmd3, 3, temp) != RT_EOK)
		{
			rt_device_read(&pUart->parent, 0, temp, 1);
			continue;
		}
		ringBufferGet(&tempRingBuffer, temp, 3);
		//PayloadLength
		if (waitRingBufferData(&tempRingBuffer, pUart, 2, 5) != RT_EOK)
		{
			return (RT_ETIMEOUT);
		}
		ringBufferGet(&tempRingBuffer, temp, 2);
		rt_uint16_t payloadLen = (temp[0] << 8) + temp[1];
		//Payload-EndMask-CRC16
		if (waitRingBufferData(&tempRingBuffer, pUart, payloadLen + 3, 5) != RT_EOK)
		{
			return (RT_ETIMEOUT);
		}
		*packetLen = 3 + 2 + payloadLen + 3;
		break;
	}
	return (RT_EOK);
}

uint16_t mallocReadAreaCmd(uint8_t ** buffer, uint8_t * bufferFlag, uint32_t ap, uint8_t * epc, uint16_t epcLen,
		uint8_t mb, uint16_t sa, uint16_t dl)
{
	mallocAfterFree(19 + epcLen, buffer, bufferFlag);
	uint8_t * t = *buffer;
	uint16_t i = 0;
	t[i++] = 0xBB;
	t[i++] = 0x00;
	t[i++] = 0x29;
	uint16_t pl = 11 + epcLen;
	t[i++] = ((uint8_t *) &pl)[1];
	t[i++] = ((uint8_t *) &pl)[0];
	t[i++] = ((uint8_t *) &ap)[3];
	t[i++] = ((uint8_t *) &ap)[2];
	t[i++] = ((uint8_t *) &ap)[1];
	t[i++] = ((uint8_t *) &ap)[0];
	t[i++] = ((uint8_t *) &epcLen)[1];
	t[i++] = ((uint8_t *) &epcLen)[0];
	memcpy(t + i, epc, epcLen);
	i += epcLen;
	t[i++] = mb;
	t[i++] = ((uint8_t *) &sa)[1];
	t[i++] = ((uint8_t *) &sa)[0];
	uint16_t dwl = dl / 2;
	t[i++] = ((uint8_t *) &dwl)[1];
	t[i++] = ((uint8_t *) &dwl)[0];
	t[i++] = 0x7E;
	uint16_t crc = crc16(t + 1, 16 + epcLen);
	t[i++] = (crc >> 8) & 0xFF;
	t[i++] = crc & 0xFF;
	return (19 + epcLen);
}

uint16_t mallocWriteAreaCmd(uint8_t ** buffer, uint8_t * bufferFlag, uint32_t ap, uint8_t * epc, uint16_t epcLen,
		uint8_t mb, uint16_t sa, uint8_t * data, uint16_t dl)
{
	mallocAfterFree(19 + epcLen + dl, buffer, bufferFlag);
	uint8_t * t = *buffer;
	uint16_t i = 0;
	t[i++] = 0xBB;
	t[i++] = 0x00;
	t[i++] = 0x46;
	uint16_t pl = 11 + epcLen;
	t[i++] = ((uint8_t *) &pl)[1];
	t[i++] = ((uint8_t *) &pl)[0];
	t[i++] = ((uint8_t *) &ap)[3];
	t[i++] = ((uint8_t *) &ap)[2];
	t[i++] = ((uint8_t *) &ap)[1];
	t[i++] = ((uint8_t *) &ap)[0];
	t[i++] = ((uint8_t *) &epcLen)[1];
	t[i++] = ((uint8_t *) &epcLen)[0];
	memcpy(t + i, epc, epcLen);
	i += epcLen;
	t[i++] = mb;
	t[i++] = ((uint8_t *) &sa)[1];
	t[i++] = ((uint8_t *) &sa)[0];
	uint16_t dwl = dl / 2;
	t[i++] = ((uint8_t *) &dwl)[1];
	t[i++] = ((uint8_t *) &dwl)[0];
	memcpy(t + i, data, dl);
	i += dl;
	t[i++] = 0x7E;
	uint16_t crc = crc16(t + 1, 16 + epcLen + dl);
	t[i++] = (crc >> 8) & 0xFF;
	t[i++] = crc & 0xFF;
	return (19 + epcLen + dl);
}

void thread_autoLookAndDeleteCardData(void * param)
{
	while (1)
	{
		while (rt_sem_take(&cardDataSem, RT_WAITING_FOREVER) != RT_EOK)
		{
		}
		rt_enter_critical();
		rt_exit_critical();
	}
}

rt_err_t startAutoLookAndDeleteCardData()
{
	return (0);
}

rt_err_t writeUserAreaOfNextCard(uint16_t offset, uint16_t dataLength, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, dataLength);
	mallocAfterFree(dataLength, &writeBuffer, &writeBufferFlag);
	rt_enter_critical();
	int i;
	for (i = 0; i < dataLength; i++)
	{
	writeBuffer[i] = va_arg(arg_ptr,uint8_t);
}
operation = WRITE_USER;
writeOffset = offset;
writeLength = dataLength;
rt_exit_critical();
va_end(arg_ptr);
return (RT_EOK);
}

