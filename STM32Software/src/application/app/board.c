#include "board.h"
#include "usart.h"
#include "stm32f10x.h"
#include "rtthread.h"

void hw_board_init()
{
//	NVIC_SetVectorTable((uint32_t) __section_begin(".intvec"), 0);
	SysTick_Config(SystemCoreClock / SYS_TICK_PER_SECOND);
	rt_hw_usart_init();
}

