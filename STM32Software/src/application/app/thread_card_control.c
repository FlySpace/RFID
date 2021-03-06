/*
 * thread_card_control.c
 *
 *  Created on: 2013-8-19
 *      Author: Fly
 */

#include "rtthread.h"
#include "threads.h"
#include "ringbuffer.h"
#include "stm32f10x.h"
#include "usart.h"
#include "rthw.h"

#define POOL_SIZE_BIT_COUNT 10
struct RingBuffer cardData;
static unsigned char pool[1 << POOL_SIZE_BIT_COUNT];

void thread_card_control(void * param)
{
	rt_device_t uart2 = rt_device_find("uart2");
	struct UARTControlArgConfigure config;
	config.USART_BaudRate = 115200;
	config.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	config.USART_Parity = USART_Parity_No;
	config.USART_StopBits = USART_StopBits_1;
	config.USART_WordLength = USART_WordLength_8b;
	rt_device_control(uart2, CONFIGURE, &config);
	rt_thread_delay(2000);
	ringBufferInit(&cardData, pool, POOL_SIZE_BIT_COUNT);
	rt_thread_t card_thread = rt_thread_create(THREAD_CARD_NAME, thread_card, RT_NULL, 512, 5, 10);
	rt_thread_startup(card_thread);
}

void printAndDeleteOneCardData()
{
}

rt_err_t startAutoReadCardData()
{
	return (0);
}

rt_err_t deleteCardData()
{
	uint8_t temp[20];
	rt_base_t l = rt_hw_interrupt_disable();
	while (rt_sem_trytake(&cardDataSem) == RT_EOK)
	{
	}
	while (ringBufferGet(&cardData, temp, 20) > 0)
	{
	}
	rt_hw_interrupt_enable(l);
	return (RT_EOK);
}
FINSH_FUNCTION_EXPORT( deleteCardData, rt_err_t deleteCardData())

rt_err_t lookCardData()
{
	struct CardDataHeader header;
	uint8_t * buffer;
	struct RingBuffer data = cardData;
	while (ringBufferDataSize(&data) > 0)
	{
		rt_kprintf("\n\n********************************");
		ringBufferGet(&data, (uint8_t *) &header, sizeof(header));
		rt_kprintf("\nTime: %.4d-%.2d-%.2d %.2d:%.2d:%.2d\n", header.time.tm_year, header.time.tm_mon,
				header.time.tm_mday, header.time.tm_hour, header.time.tm_min, header.time.tm_sec);
		//
		buffer = rt_malloc(header.reserved);
		ringBufferGet(&data, buffer, header.reserved);
		rt_kprintf("\nReserved:\n");
		for (int j = 0; j < header.reserved; j++)
		{
			rt_kprintf("%.2X ", buffer[j]);
		}
		rt_free(buffer);
		//
		buffer = rt_malloc(header.epc);
		ringBufferGet(&data, buffer, header.epc);
		rt_kprintf("\nEPC:\n");
		for (int j = 0; j < header.epc; j++)
		{
			rt_kprintf("%.2X ", buffer[j]);
		}
		rt_free(buffer);
		//
		buffer = rt_malloc(header.tid);
		ringBufferGet(&data, buffer, header.tid);
		rt_kprintf("\nTID:\n");
		for (int j = 0; j < header.tid; j++)
		{
			rt_kprintf("%.2X ", buffer[j]);
		}
		rt_free(buffer);
		//
		buffer = rt_malloc(header.user);
		ringBufferGet(&data, buffer, header.user);
		rt_kprintf("\nUser:\n");
		for (int j = 0; j < header.user; j++)
		{
			rt_kprintf("%.2X ", buffer[j]);
		}
		rt_free(buffer);
	}
	return (RT_EOK);
}
FINSH_FUNCTION_EXPORT( lookCardData, rt_err_t lookCardData())

rt_err_t look(unsigned char * p, unsigned int len)
{
	for (int i = 0; i < len; i++)
	{
		rt_kprintf("%.2X ", p[i]);
	}
	return (RT_EOK);
}
FINSH_FUNCTION_EXPORT(look, rt_err_t look(unsigned char * p, unsigned int len))
