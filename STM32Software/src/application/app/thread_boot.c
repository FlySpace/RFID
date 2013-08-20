/*
 * rt_boot_thread.c
 *
 *  Created on: 2013-8-13
 *      Author: Fly
 */

#include "threads.h"
#include "rtthread.h"
#include "stdio.h"

void thread_boot(void * param)
{
	//TODO CPU占用率功能
	rt_thread_t card_control_thread = rt_thread_create(THREAD_CARD_CONTROL_NAME,
			thread_card_control, RT_NULL, 256, 2, 10);
	rt_thread_t bt_control_thread = rt_thread_create(THREAD_CARD_CONTROL_NAME,
			thread_bluetooth_control, RT_NULL, 256, 3, 10);

	rt_thread_startup(card_control_thread);
	rt_thread_startup(bt_control_thread);

}
