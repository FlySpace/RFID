#include "board.h"
#include "usart.h"
#include "stm32f10x.h"
#include "rtthread.h"

void hw_board_init()
{
//	NVIC_SetVectorTable((uint32_t) __section_begin(".intvec"), 0);
	SysTick_Config(SystemCoreClock / SYS_TICK_PER_SECOND);

	rt_hw_usart_init();
	rt_device_t pUart1 = rt_device_find("uart1");
	struct UARTControlArgConfigure config;
	config.USART_BaudRate = 115200;
	config.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	config.USART_Parity = USART_Parity_No;
	config.USART_StopBits = USART_StopBits_2;
	config.USART_WordLength = USART_WordLength_8b;
	rt_device_control(pUart1, CONFIGURE, &config);

}

