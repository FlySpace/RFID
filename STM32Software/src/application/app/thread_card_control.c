/*
 * thread_card_control.c
 *
 *  Created on: 2013-8-19
 *      Author: Fly
 */

#include "rtthread.h"
#include "threads.h"
#include "ringbuffer.h"
#include "finsh.h"
#include "stm32f10x.h"

#define POOL_SIZE_BIT_COUNT 8
struct RingBuffer cardData;
static unsigned char pool[1 << POOL_SIZE_BIT_COUNT];
FINSH_VAR_EXPORT(pool, finsh_type_charp, Card Pool)

void thread_card_control(void * param)
{
//	rt_device_t uart2 = rt_device_find("uart2");
//	struct UARTControlArgConfigure config;
//	config.USART_BaudRate = 115200;
//	config.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//	config.USART_Parity = USART_Parity_No;
//	config.USART_StopBits = USART_StopBits_1;
//	config.USART_WordLength = USART_WordLength_8b;
//	rt_device_control(uart2, CONFIGURE, &config);

	ringBufferInit(&cardData, pool, POOL_SIZE_BIT_COUNT);
	//TODO 初始化事件
	rt_thread_t card_thread = rt_thread_create(THREAD_CARD_NAME, thread_card, RT_NULL, 512, 5, 10);
//	while (1)
//	{
//		//TODO 等待事件
//		if (1) //TODO 启动读卡
//		{
	rt_thread_delay(2000);
	rt_thread_startup(card_thread);
//		}
//		else if (2) //TODO 关闭读卡
//		{
//			rt_thread_detach(card_thread);
//		}
//	}
}

rt_err_t lookMem(unsigned char * p, unsigned int len)
{
	for (int i = 0; i < len; i++)
	{
		rt_kprintf("%#x ", p[i]);
	}
	return (RT_EOK);
}
FINSH_FUNCTION_EXPORT(lookMem, lookMem)
