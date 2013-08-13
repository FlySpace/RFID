#include "board.h"
#include "stm32f10x.h"
#include"rtthread.h"

void hw_board_init()
{
	SysTick_Config(SystemCoreClock / 10);
}

void SysTick_Handler()
{
	rt_interrupt_enter();
	rt_kprintf("SysTick_Handler");
	rt_interrupt_leave();
}
