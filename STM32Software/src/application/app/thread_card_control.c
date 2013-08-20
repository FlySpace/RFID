/*
 * thread_card_control.c
 *
 *  Created on: 2013-8-19
 *      Author: Fly
 */

#include "rtthread.h"
#include "threads.h"

void thread_card_control(void * param)
{
	//TODO 初始化事件
	rt_thread_t card_thread = rt_thread_create(THREAD_CARD_NAME, thread_card,
			RT_NULL, 128, 5, 10);
	while (1)
	{
		//TODO 等待事件
		if (1) //TODO 启动读卡
		{
			rt_thread_startup(card_thread);
		}
		else if (2) //TODO 关闭读卡
		{
			rt_thread_detach(card_thread);
		}
	}
}
