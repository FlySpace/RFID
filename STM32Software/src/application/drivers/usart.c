/*
 * File      : usart.c
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
 * 2010-03-29     Bernard      remove interrupt Tx and DMA Rx mode
 */

#include "usart.h"
#include "rthw.h"
#include "stm32f10x.h"
#include "ringbuffer.h"

static struct UARTDevice uart1;
static rt_uint8_t UART1RxBuffer[UART1_RX_BUFFER_SIZE];
static rt_uint8_t UART1TxBuffer[UART1_TX_BUFFER_SIZE];
//static struct UARTDevice uart2;
//static rt_uint8_t UART2RxBuffer[UART2_RX_BUFFER_SIZE];
//static rt_uint8_t UART2TxBuffer[UART2_TX_BUFFER_SIZE];

/* USART1_REMAP = 0 */
#define UART1_GPIO_TX		GPIO_Pin_9
#define UART1_GPIO_RX		GPIO_Pin_10
#define UART1_GPIO			GPIOA
#define RCC_APBPeriph_UART1	RCC_APB2Periph_USART1
#define UART1_TX_DMA		DMA1_Channel4
#define UART1_RX_DMA		DMA1_Channel5

/* USART2_REMAP = 0 */
#define UART2_GPIO_TX		GPIO_Pin_2
#define UART2_GPIO_RX		GPIO_Pin_3
#define UART2_GPIO			GPIOA
#define RCC_APBPeriph_UART2	RCC_APB1Periph_USART2
#define UART2_TX_DMA		DMA1_Channel7
#define UART2_RX_DMA		DMA1_Channel6

static void RCC_Configuration(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	/* Enable USART1 and GPIOA clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	/* Enable USART2 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
}

static void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Configure USART1 Rx (PA.10) as input floating */
	GPIO_InitStructure.GPIO_Pin = UART1_GPIO_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(UART1_GPIO, &GPIO_InitStructure);

	/* Configure USART1 Tx (PA.09) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = UART1_GPIO_TX;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(UART1_GPIO, &GPIO_InitStructure);

	/* Configure USART2 Rx as input floating */
	GPIO_InitStructure.GPIO_Pin = UART2_GPIO_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(UART2_GPIO, &GPIO_InitStructure);

	/* Configure USART2 Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = UART2_GPIO_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(UART2_GPIO, &GPIO_InitStructure);
}

static void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable the USART1 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the USART2 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void USART_Configuration(void)
{
	USART_ClockInitTypeDef USART_ClockInitStructure;

	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;

	USART_ClockInit(USART1, &USART_ClockInitStructure);
	USART_Cmd(USART1, ENABLE);
	USART_ClearFlag(USART1, USART_FLAG_TC );

	//uart2
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;

	USART_ClockInit(USART2, &USART_ClockInitStructure);
	USART_Cmd(USART2, ENABLE);
	USART_ClearFlag(USART2, USART_FLAG_TC );
}

unsigned int crc16(unsigned char *ptr, unsigned char count)
{
	unsigned int crc = 0xFFFF;
	unsigned char i;
	while (count-- > 0)
	{
		crc = (crc ^ (((unsigned int) *ptr) << 8));
		for (i = 0; i < 8; i++)
		{
			if (crc & 0x8000)
				crc = ((crc << 1) ^ 0x1021);
			else
				crc <<= 1;
		}
		ptr++;
	}
	return (crc);
}

static rt_err_t uart1Init(rt_device_t dev)
{
	rt_base_t l = rt_hw_interrupt_disable();

	uart1.USARTx = USART1;
	rt_mutex_init(&uart1.writeLock, "", RT_IPC_FLAG_PRIO);
	ringBufferInit(&uart1.pRxBuffer, UART1RxBuffer, UART1_RX_BUFFER_SIZE_BIT_COUNT);
	ringBufferInit(&uart1.pTxBuffer, UART1TxBuffer, UART1_TX_BUFFER_SIZE_BIT_COUNT);
	struct UARTControlArgConfigure config;
	config.USART_BaudRate = 9600;
	config.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	config.USART_Parity = USART_Parity_No;
	config.USART_StopBits = USART_StopBits_1;
	config.USART_WordLength = USART_WordLength_8b;
	rt_device_control(&uart1.parent, CONFIGURE, &config);

	dev->flag |= RT_DEVICE_FLAG_ACTIVATED;

	rt_hw_interrupt_enable(l);
	return (RT_EOK);
}

static rt_err_t uartOpen(struct rt_device *dev, rt_uint16_t oflag)
{
	rt_base_t l = rt_hw_interrupt_disable();

	struct UARTDevice * pUart = (struct UARTDevice *) dev;
	USART_ITConfig(pUart->USARTx, USART_IT_RXNE, ENABLE);
	USART_ITConfig(pUart->USARTx, USART_IT_TXE, DISABLE);
	USART_ClearITPendingBit(pUart->USARTx, USART_IT_RXNE );
	USART_ClearITPendingBit(pUart->USARTx, USART_IT_TXE );

	rt_hw_interrupt_enable(l);
	return (RT_EOK);
}

static rt_err_t uartClose(struct rt_device *dev)
{
	rt_base_t l = rt_hw_interrupt_disable();

	struct UARTDevice * pUart = (struct UARTDevice *) dev;
	USART_ITConfig(pUart->USARTx, USART_IT_RXNE, DISABLE);
	USART_ITConfig(pUart->USARTx, USART_IT_TXE, DISABLE);

	rt_hw_interrupt_enable(l);
	return (RT_EOK);
}

static rt_size_t uartRead(struct rt_device *dev, rt_off_t pos, void *buffer, rt_size_t size)
{
	struct UARTDevice * pUart = (struct UARTDevice *) dev;
	rt_size_t length = ringBufferGet(&pUart->pRxBuffer, buffer, size);
	return (length);
}

static rt_size_t uartWrite(struct rt_device *dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
	struct UARTDevice * pUart = (struct UARTDevice *) dev;

	rt_size_t length = 0;

	if (rt_interrupt_get_nest() > 0 || rt_thread_self() == RT_NULL)
	{
		length += ringBufferPut(&pUart->pTxBuffer, (unsigned char *) buffer, size);
	}
	else
	{
		while (rt_mutex_take(&pUart->writeLock, RT_TICK_MAX) != RT_EOK)
		{
		}
		length += ringBufferPut(&pUart->pTxBuffer, (unsigned char *) buffer + length, size - length);
		if (length < size)
		{
			rt_uint32_t charCount =
					(size - length) < (pUart->pTxBuffer._indexMSBMask / 2) ?
							(size - length) : (pUart->pTxBuffer._indexMSBMask / 2);
			rt_uint32_t bitPerChar = (pUart->config.USART_Parity == USART_Parity_No ? 0 : 1) + 2
					+ (pUart->config.USART_WordLength == USART_WordLength_8b ? 8 : 9);
			rt_uint32_t delayTicks = uartCalcDelayTicks(charCount, pUart->config.USART_BaudRate, bitPerChar);
			while (length < size)
			{
				rt_thread_delay(delayTicks);
				length += ringBufferPut(&pUart->pTxBuffer, (unsigned char *) buffer + length, size - length);
			}
		}
		rt_mutex_release(&pUart->writeLock);
	}

	USART_ITConfig(pUart->USARTx, USART_IT_TXE, ENABLE);
	return (length);
}

static rt_err_t uartControl(struct rt_device *dev, rt_uint8_t cmd, void *args)
{
	rt_base_t l = rt_hw_interrupt_disable();

	struct UARTDevice * pUart = (struct UARTDevice *) dev;
	switch (cmd)
	{
	case CONFIGURE:
	{
		struct UARTControlArgConfigure * pArg = args;
		struct UARTControlArgConfigure * pConfig = &pUart->config;
		USART_InitTypeDef USART_InitStructure;
		USART_InitStructure.USART_BaudRate = pConfig->USART_BaudRate = pArg->USART_BaudRate;
		USART_InitStructure.USART_WordLength = pConfig->USART_WordLength = pArg->USART_WordLength;
		USART_InitStructure.USART_StopBits = pConfig->USART_StopBits = pArg->USART_StopBits;
		USART_InitStructure.USART_Parity = pConfig->USART_Parity = pArg->USART_Parity;
		USART_InitStructure.USART_HardwareFlowControl = pConfig->USART_HardwareFlowControl =
				pArg->USART_HardwareFlowControl;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		USART_Init(pUart->USARTx, &USART_InitStructure);
		break;
	}
	}

	rt_hw_interrupt_enable(l);
	return (RT_EOK);
}

static rt_err_t uartRegister(struct UARTDevice * pUART, const char * name, rt_uint16_t flag, void * userData,
		rt_err_t (*init)(rt_device_t dev))
{
	rt_device_t dev = &(pUART->parent);
	dev->type = RT_Device_Class_Char;
	dev->rx_indicate = RT_NULL;
	dev->tx_complete = RT_NULL;
	dev->init = init;
	dev->open = uartOpen;
	dev->close = uartClose;
	dev->read = uartRead;
	dev->write = uartWrite;
	dev->control = uartControl;
	dev->user_data = userData;
	return (rt_device_register(dev, name, flag));
}

static void uartISR(struct UARTDevice * pUart)
{
	rt_base_t level = rt_hw_interrupt_disable();

	if (USART_GetFlagStatus(pUart->USARTx, USART_FLAG_RXNE ) == SET)
	{
		unsigned char ch = USART_ReceiveData(pUart->USARTx) & 0xff;
		rt_size_t length = ringBufferPut(&pUart->pRxBuffer, &ch, 1);
		if (pUart->parent.rx_indicate != RT_NULL && length)
		{
			pUart->parent.rx_indicate(&pUart->parent, length);
		}
	}

	if (USART_GetITStatus(pUart->USARTx, USART_IT_TXE ) == SET)
	{
		rt_uint8_t ch;
		rt_size_t length = ringBufferGet(&pUart->pTxBuffer, &ch, 1);
		if (pUart->parent.tx_complete != RT_NULL && length)
		{
			pUart->parent.tx_complete(&pUart->parent, &ch);
		}
		if (length)
		{
			USART_SendData(pUart->USARTx, ch);
		}
		else
		{
			USART_ITConfig(pUart->USARTx, USART_IT_TXE, DISABLE);
		}
	}

	rt_hw_interrupt_enable(level);
}

/*
 * Init all related hardware in here
 * rt_hw_serial_init() will register all supported USART device
 */
void rt_hw_usart_init()
{
	RCC_Configuration();

	GPIO_Configuration();

	NVIC_Configuration();

	USART_Configuration();

	uartRegister(&uart1, "uart1",
			RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_STREAM, RT_NULL,
			uart1Init);

	/* register uart2 */
}

rt_uint32_t uartCalcDelayTicks(rt_uint32_t charCount, rt_uint32_t baudRate, rt_uint32_t bitPerChar)
{
	return ((rt_uint32_t) (charCount * bitPerChar * RT_TICK_PER_SECOND / baudRate) + 1);
}

void USART1_IRQHandler()
{
	rt_interrupt_enter();
	uartISR(&uart1);
	rt_interrupt_leave();
}
