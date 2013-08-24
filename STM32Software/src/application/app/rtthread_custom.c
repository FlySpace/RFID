#include "rtthread_custom.h"
#include "rtthread.h"
#include "rthw.h"
#include "stdio.h"
#include "threads.h"


#pragma section="RT_HEAP"

static void rt_thread_idle_hook();
static void rt_application_init();

/*配置rt_kprintf的输出到IAR运行库的标准输出*/
void rt_hw_console_output(const char *str)
{
	puts(str);
}

/*rtt启动总入口*/
void rtthread_startup()
{
	rt_console_set_device(RT_NULL);

	rt_show_version();

	rt_system_heap_init(__section_begin("RT_HEAP"), __section_end("RT_HEAP"));

	rt_device_init_all();

	rt_system_scheduler_init();

	rt_system_timer_thread_init();

	rt_thread_idle_init();
	rt_thread_idle_sethook(rt_thread_idle_hook);

	rt_application_init();

	rt_system_scheduler_start();
}

/*用户线程初始化*/
static void rt_application_init()
{
	rt_thread_startup(
			rt_thread_create(THREAD_BOOT_NAME, thread_boot, RT_NULL, 256, 0,
					10));
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

/*定时器中断*/
