#include"board.h"
#include"intrinsics.h"
#include "rtthread_custom.h"
#include "stm32f10x.h"


int main()
{
	__disable_interrupt();
	hw_board_init();
	rtthread_startup();
	return (0);
}

