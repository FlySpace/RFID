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
	while (1)
	{
		rt_kprintf("thread_boot");
		rt_thread_delay(500);
	}
}
