#include "rtthread_custom.h"
#include "rtthread.h"
#include "rthw.h"
#include "threads.h"
#include "stm32f10x.h"
#include "usart.h"
#include "shell.h"


#pragma section="RT_HEAP"

static void rt_thread_idle_hook();
static void rt_application_init();
static void rt_console_init();

/*rtt启动总入口*/
void rtthread_startup()
{
	rt_system_heap_init(__section_begin("RT_HEAP"), __section_end("RT_HEAP"));

	rt_device_init_all();

	rt_system_scheduler_init();

	rt_system_timer_thread_init();

	rt_thread_idle_init();
	rt_thread_idle_sethook(rt_thread_idle_hook);

	rt_application_init();

	rt_console_init();

	finsh_system_init();
	finsh_set_device("uart3");

	rt_system_scheduler_start();
}

static void rt_console_init()
{
	rt_device_t pUart = rt_device_find("uart3");
	struct UARTControlArgConfigure config;
	config.USART_BaudRate = 115200;
	config.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	config.USART_Parity = USART_Parity_No;
	config.USART_StopBits = USART_StopBits_1;
	config.USART_WordLength = USART_WordLength_8b;
	rt_device_control(pUart, CONFIGURE, &config);

	rt_console_set_device("uart3");
}

/*用户线程初始化*/
static void rt_application_init()
{
	rt_thread_startup(rt_thread_create(THREAD_BOOT_NAME, thread_boot, RT_NULL, 512, 0, 10));
}

int idle_count = 0;
/*rtt空闲线程钩子*/
static void rt_thread_idle_hook()
{
	rt_base_t l = rt_hw_interrupt_disable();
	idle_count++;
	rt_hw_interrupt_enable(l);
}

/*SysTick中断用作rtt的systick*/
void SysTick_Handler()
{
	rt_interrupt_enter();
	rt_tick_increase();
	rt_interrupt_leave();
}
