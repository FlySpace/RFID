#include"board.h"
#include"intrinsics.h"
#include "rtthread_custom.h"

void rtthread_startup();

int main()
{
	__disable_interrupt();
	hw_board_init();
	rtthread_startup();
	return (0);
}

