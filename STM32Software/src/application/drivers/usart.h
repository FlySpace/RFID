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
#include "ringbuffer.h"

#define UART1_TX_BUFFER_SIZE_BIT_COUNT 9
#define UART1_RX_BUFFER_SIZE_BIT_COUNT 7
#define UART2_TX_BUFFER_SIZE_BIT_COUNT 7
#define UART2_RX_BUFFER_SIZE_BIT_COUNT 7

#define UART1_TX_BUFFER_SIZE ((unsigned int)1 << UART1_TX_BUFFER_SIZE_BIT_COUNT)
#define UART1_RX_BUFFER_SIZE ((unsigned int)1 << UART1_RX_BUFFER_SIZE_BIT_COUNT)
#define UART2_TX_BUFFER_SIZE ((unsigned int)1 << UART2_TX_BUFFER_SIZE_BIT_COUNT)
#define UART2_RX_BUFFER_SIZE ((unsigned int)1 << UART2_RX_BUFFER_SIZE_BIT_COUNT)

struct UARTControlArgConfigure
{
	uint32_t USART_BaudRate;
	uint16_t USART_WordLength;
	uint16_t USART_StopBits;
	uint16_t USART_Parity;
	uint16_t USART_HardwareFlowControl;
};

struct UARTDevice
{
	struct rt_device parent;
	USART_TypeDef * USARTx;
	struct RingBuffer pTxBuffer;
	struct RingBuffer pRxBuffer;
	struct UARTControlArgConfigure config;
	struct rt_mutex writeLock;
};

enum UARTControlCmd
{
	CONFIGURE, SET_ON_TX_BUFFER_CHANGE, SET_ON_RX_BUFFER_CHANGE
};

unsigned int crc16(unsigned char *ptr, unsigned char count);
void rt_hw_usart_init();
rt_uint32_t uartCalcDelayTicks(rt_uint32_t charCount, rt_uint32_t baudRate, rt_uint32_t bitPerChar);

#endif
