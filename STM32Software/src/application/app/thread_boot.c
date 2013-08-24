/*
 * rt_boot_thread.c
 *
 *  Created on: 2013-8-13
 *      Author: Fly
 */

#include "threads.h"
#include "rtthread.h"
#include "stdio.h"
#include "stm32f10x.h"

extern uint8_t a[];
void USART1_BT_Tx(uint8_t *BTTxArray);
void rt_hw_usart_init(void);

void thread_boot(void * param)
{
	//TODO CPU占用率功能
	rt_thread_t card_control_thread =rt_thread_create(THREAD_CARD_CONTROL_NAME, thread_card_control, RT_NULL,
			256, 2, 10);
	rt_thread_t bt_control_thread = rt_thread_create(THREAD_CARD_CONTROL_NAME,
			thread_bluetooth_control, RT_NULL, 256, 3, 10);

	rt_thread_startup(card_control_thread);
	rt_thread_startup(bt_control_thread);
	rt_hw_usart_init();
	USART1_BT_Tx(a);
}
