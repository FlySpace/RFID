#include"rtthread_custom.h"
#include"rtthread.h"
#include"rthw.h"
#include"stdio.h"

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

	//rt_system_heap_init

	rt_system_scheduler_init();

	rt_device_init_all();

	rt_application_init();

	rt_system_timer_thread_init();

	rt_thread_idle_init();
	rt_thread_idle_sethook(rt_thread_idle_hook);

	rt_system_scheduler_start();
}

/*rtt空闲线程钩子*/
int i = 0;
static void rt_thread_idle_hook()
{
	if (!(i = (i + 1) % 100000))
	{
		rt_kprintf("systick=%d\n",rt_tick_get());
		rt_show_version();
	}
}

static void rt_application_init()
{

}

