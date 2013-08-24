/*
 * rt_boot_thread.c
 *
 *  Created on: 2013-8-13
 *      Author: Fly
 */

#include "usart.h"
#include "threads.h"
#include "rtthread.h"
#include "stdio.h"

void thread_boot(void * param)
{
	rt_device_t pUart1 = rt_device_find("uart1");
	struct UARTControlArgConfigure config;
	config.USART_BaudRate = 115200;
	config.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	config.USART_Parity = USART_Parity_No;
	config.USART_StopBits = USART_StopBits_2;
	config.USART_WordLength = USART_WordLength_8b;
	rt_device_control(pUart1, CONFIGURE, &config);
	rt_device_open(pUart1, RT_DEVICE_OFLAG_RDWR);
	rt_device_write(pUart1, 0, "abcdefg\nGF", 5);

	//TODO CPU占用率功能
	rt_thread_t card_control_thread = rt_thread_create(THREAD_CARD_CONTROL_NAME, thread_card_control, RT_NULL, 256, 2,
			10);
	rt_thread_t bt_control_thread = rt_thread_create(THREAD_CARD_CONTROL_NAME, thread_bluetooth_control, RT_NULL, 256,
			3, 10);

	rt_thread_startup(card_control_thread);
	rt_thread_startup(bt_control_thread);

}
