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

#include "stm32f10x.h"
#include "rtthread.h"
#include "rtdevice.h"

#define UART1_TX_BUFFER_SIZE 512
#define UART1_RX_BUFFER_SIZE 128
#define UART2_TX_BUFFER_SIZE 128
#define UART2_RX_BUFFER_SIZE 128

struct UARTDevice
{
	struct rt_device parent;
	USART_TypeDef * USARTx;
	struct rt_ringbuffer pTxBuffer;
	struct rt_ringbuffer pRxBuffer;
	void (*onTxBufferChange)(struct UARTDevice * pUART);
	void (*onRxBufferChange)(struct UARTDevice * pUART);
};

enum UARTControlCmd
{
	CONFIGURE, SET_ON_TX_BUFFER_CHANGE, SET_ON_RX_BUFFER_CHANGE
};

struct UARTControlArgConfigure
{
	uint32_t USART_BaudRate;
	uint16_t USART_WordLength;
	uint16_t USART_StopBits;
	uint16_t USART_Parity;
	uint16_t USART_HardwareFlowControl;
};

unsigned int crc16(unsigned char *ptr, unsigned char count);
void rt_hw_usart_init();

#endif
