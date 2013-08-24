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
//#include <serial.h>
#include <stm32f10x_dma.h>


uint8_t a[]={0xBB,0x00,0x27,0x00,0x03,0x22,0x00,0x64,0x7E};
__IO uint16_t BTTxCnt1;
uint8_t *BTTXS;
__IO uint16_t BTTxLen;


/*
 * Use UART1 as console output and finsh input
 * interrupt Rx and poll Tx (stream mode)
 *
 * Use UART2 with interrupt Rx and poll Tx
 * Use UART3 with DMA Tx and interrupt Rx -- DMA channel 2
 *
 * USART DMA setting on STM32
 * USART1 Tx --> DMA Channel 4
 * USART1 Rx --> DMA Channel 5
 * USART2 Tx --> DMA Channel 7
 * USART2 Rx --> DMA Channel 6
 * USART3 Tx --> DMA Channel 2
 * USART3 Rx --> DMA Channel 3
 */

//#ifdef RT_USING_UART1                         //RTT相关结构体,先注释掉
//struct stm32_serial_int_rx uart1_int_rx;
//struct stm32_serial_device uart1 =
//{
//	USART1,
//	&uart1_int_rx,
//	RT_NULL
//};
//struct rt_device uart1_device;
//#endif
//
//#ifdef RT_USING_UART2
//struct stm32_serial_int_rx uart2_int_rx;
//struct stm32_serial_device uart2 =
//{
//	USART2,
//	&uart2_int_rx,
//	RT_NULL
//};
//struct rt_device uart2_device;
//#endif
//
//#ifdef RT_USING_UART3
//struct stm32_serial_int_rx uart3_int_rx;
//struct stm32_serial_dma_tx uart3_dma_tx;
//struct stm32_serial_device uart3 =
//{
//	USART3,
//	&uart3_int_rx,
//	&uart3_dma_tx
//};
//struct rt_device uart3_device;
//#endif

#define USART1_DR_Base  0x40013804
#define USART2_DR_Base  0x40004404
#define USART3_DR_Base  0x40004804

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

//static void DMA_Configuration(void)
//{
//	DMA_InitTypeDef DMA_InitStructure;
//
//#if defined (RT_USING_UART1)
//	/* fill init structure */
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
//	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//
//	/* DMA1 Channel5 (triggered by USART3 Tx event) Config */
//	DMA_DeInit(UART1_TX_DMA);
//	DMA_InitStructure.DMA_PeripheralBaseAddr = USART1_DR_Base;
//	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
//	/* As we will set them before DMA actually enabled, the DMA_MemoryBaseAddr
//	 * and DMA_BufferSize are meaningless. So just set them to proper values
//	 * which could make DMA_Init happy.
//	 */
//	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
//	DMA_InitStructure.DMA_BufferSize = 1;
//	DMA_Init(UART1_TX_DMA, &DMA_InitStructure);
//	DMA_ITConfig(UART1_TX_DMA, DMA_IT_TC | DMA_IT_TE, ENABLE);
//	DMA_ClearFlag(DMA1_FLAG_TC2);
//#endif
//
//#if defined (RT_USING_UART2)
//	/* fill init structure */
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
//	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//
//	/* DMA1 Channel5 (triggered by USART3 Tx event) Config */
//	DMA_DeInit(UART2_TX_DMA);
//	DMA_InitStructure.DMA_PeripheralBaseAddr = USART2_DR_Base;
//	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
//	/* As we will set them before DMA actually enabled, the DMA_MemoryBaseAddr
//	 * and DMA_BufferSize are meaningless. So just set them to proper values
//	 * which could make DMA_Init happy.
//	 */
//	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
//	DMA_InitStructure.DMA_BufferSize = 1;
//	DMA_Init(UART2_TX_DMA, &DMA_InitStructure);
//	DMA_ITConfig(UART2_TX_DMA, DMA_IT_TC | DMA_IT_TE, ENABLE);
//	DMA_ClearFlag(DMA1_FLAG_TC2);
//#endif
//
//#if defined (RT_USING_UART3)
////	DMA_InitTypeDef DMA_InitStructure;
//
//	/* fill init structure */
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
//	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//
//	/* DMA1 Channel5 (triggered by USART3 Tx event) Config */
//	DMA_DeInit(UART3_TX_DMA);
//	DMA_InitStructure.DMA_PeripheralBaseAddr = USART3_DR_Base;
//	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
//	/* As we will set them before DMA actually enabled, the DMA_MemoryBaseAddr
//	 * and DMA_BufferSize are meaningless. So just set them to proper values
//	 * which could make DMA_Init happy.
//	 */
//	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
//	DMA_InitStructure.DMA_BufferSize = 1;
//	DMA_Init(UART3_TX_DMA, &DMA_InitStructure);
//	DMA_ITConfig(UART3_TX_DMA, DMA_IT_TC | DMA_IT_TE, ENABLE);
//	DMA_ClearFlag(DMA1_FLAG_TC2);
//#endif
//}

static void USART_Configuration(void)
{
	USART_InitTypeDef USART_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;

	USART_Init(USART1,&USART_InitStructure);
	USART_ClockInit(USART1,&USART_ClockInitStructure);
	USART_Cmd(USART1, ENABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_ClearFlag(USART1,USART_FLAG_TC);

	//uart2
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;

	USART_Init(USART2,&USART_InitStructure);
	USART_ClockInit(USART2,&USART_ClockInitStructure);
	USART_Cmd(USART2, ENABLE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_ClearFlag(USART2,USART_FLAG_TC);
}

unsigned int crc16(unsigned char *ptr,unsigned char count)

{
	unsigned int crc =0xFFFF;
	unsigned char i;
	while(count-- >0)
	{
	crc = ( crc^(((unsigned int)*ptr)<<8));
	for(i=0;i<8;i++)
	{
	  if(crc&0x8000) crc= ((crc<<1)^0x1021);
	  else crc <<= 1;
	}
	ptr++;
	}
	return crc;
}

//unsigned short crc16(unsigned char d[], int len)    //另一种crc16计算方法
//{
//  unsigned char b = 0;
//  unsigned short crc = 0xFFFF;
//  int i, j;
//
//  for(i=0; i<len; i++)
//  {
//    for(j=0; j<8; j++)
//    {
//      b = ((d[i]<<j)&0x80) ^ ((crc&0x8000)>>8);
//      crc<<=1;
//      if(b!=0) crc^=0x1021;
//    }
//  }
//  return crc;
//}

/*
 * Init all related hardware in here
 * rt_hw_serial_init() will register all supported USART device
 */
void rt_hw_usart_init(void)
{
	RCC_Configuration();

	GPIO_Configuration();

	NVIC_Configuration();

	USART_Configuration();

	/* register uart1 */
//	rt_hw_serial_register(&uart1_device, "uart1",
//		RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
//		&uart1);

	/* register uart2 */
//	rt_hw_serial_register(&uart2_device, "uart2",
//		RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
//		&uart2);

}

void USART2_RFID_RxDataProcess(void)
{

}

void USART1_BT_Rx()
{

}

void USART1_BT_Tx(uint8_t *BTTxArray)
{
	BTTxCnt1=0;
	BTTXS=BTTxArray;
	BTTxLen=(uint16_t)0x0100*BTTxArray[3]+(uint16_t)BTTxArray[4]+0x0006;
	USART_ITConfig(USART1, USART_IT_TXE,ENABLE);
}



