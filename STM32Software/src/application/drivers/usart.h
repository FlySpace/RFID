/*
 * File      : usart.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

#ifndef __USART_H__
#define __USART_H__

#define RT_USING_UART1
#define RT_USING_UART2

//#include <rthw.h>
//#include <rtthread.h>
#include "stm32f10x.h"


unsigned int crc16(unsigned char *ptr,unsigned char count);
void rt_hw_usart_init(void);

#endif
