/*
 * rtc.c
 *
 *  Created on: 2013年9月9日
 *      Author: Administrator
 */


#include "rtc.h"
#include "finsh.h"

#define FEBRUARY		2
#define	STARTOFTIME		1970
#define SECDAY			86400
#define SECYR			(SECDAY * 365)
#define	leapyear(year)		((year) % 4 == 0)
#define	days_in_year(a) 	(leapyear(a) ? 366 : 365)
#define	days_in_month(a) 	(month_days[(a) - 1])

static int month_days[12] =
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

struct rtc_time TimeNow;

/* 秒中断标志，进入秒中断时置1，当时间被刷新之后清0 */
//__IO uint32_t TimeDisplay = 0;

/*
 * 函数名：NVIC_Configuration
 * 描述  ：配置RTC秒中断的主中断优先级为1，次优先级为0
 * 输入  ：无
 * 输出  ：无
 * 调用  ：外部调用
 */
static void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Configure one bit for preemption priority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	/* Enable the RTC Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/*
 * 函数名：RTC_CheckAndConfig
 * 描述  ：检查并配置RTC
 * 输入  ：用于读取RTC时间的结构体指针
 * 输出  ：无
 * 调用  ：外部调用
 */
static void RTC_CheckAndConfig(struct rtc_time *tm)
{
	/*在启动时检查备份寄存器BKP_DR1，如果内容不是0xA5A5,
	 则需重新配置时间并询问用户调整时间*/
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
		/* RTC Configuration */
		RTC_Configuration();

		/* Adjust time by users typed on the hyperterminal */
		//Time_Adjust(tm);

		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	}
	else
	{
		/*启动无需设置新时钟*/
		/*等待寄存器同步*/
		RTC_WaitForSynchro();

		/*允许RTC秒中断*/
//		RTC_ITConfig(RTC_IT_SEC, ENABLE);
		/*等待上次RTC寄存器写操作完成*/
//		RTC_WaitForLastTask();
	}
	/*定义了时钟输出宏，则配置校正时钟输出到PC13*/
#ifdef RTCClockOutput_Enable
	/* Enable PWR and BKP clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);

	/* Disable the Tamper Pin */
	BKP_TamperPinCmd(DISABLE); /* To output RTCCLK/64 on Tamper pin, the tamper
	 functionality must be disabled */

	/* Enable RTC Clock Output on Tamper Pin */
	BKP_RTCOutputConfig(BKP_RTCOutputSource_CalibClock);
#endif

	/* Clear reset flags */
	RCC_ClearFlag();

}

/*
 * 函数名：RTC_Configuration
 * 描述  ：配置RTC
 * 输入  ：无
 * 输出  ：无
 * 调用  ：外部调用
 */
static void RTC_Configuration(void)
{
	/* Enable PWR and BKP clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);

	/* Reset Backup Domain */
	BKP_DeInit();

	/* Enable LSE */
	RCC_LSEConfig(RCC_LSE_ON);
	/* Wait till LSE is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
	{
	}

	/* Select LSE as RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

	/* Enable RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC registers synchronization */
	RTC_WaitForSynchro();

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	/* Enable the RTC Second */
//	RTC_ITConfig(RTC_IT_SEC, ENABLE);
	/* Wait until last write operation on RTC registers has finished */
//	RTC_WaitForLastTask();
	/* Set RTC prescaler: set RTC period to 1sec */
	RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
}

/*
 * 函数名：Time_Regulate
 * 描述  ：返回用户在超级终端中输入的时间值，并将值储存在
 *         RTC 计数寄存器中。
 * 输入  ：用于读取RTC时间的结构体指针
 * 输出  ：用户在超级终端中输入的时间值，单位为 s
 * 调用  ：内部调用
 */
void Time_Regulate(struct rtc_time *tm, uint16_t YY, uint16_t MM, uint16_t DD, uint16_t hh, uint16_t mm, uint16_t ss)
{
	//输入时间
	tm->tm_year = YY;
	tm->tm_mon = MM;
	tm->tm_mday = DD;
	tm->tm_hour = hh;
	tm->tm_min = mm;
	tm->tm_sec = ss;
}


/*
 * 函数名：Time_Adjust
 * 描述  ：时间调节
 * 输入  ：用于读取RTC时间的结构体指针
 * 输出  ：无
 * 调用  ：外部调用
 */
void Time_Adjust(struct rtc_time *tm, uint16_t YY, uint16_t MM, uint16_t DD, uint16_t hh, uint16_t mm, uint16_t ss)
{
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	/* Get time entred by the user on the hyperterminal */
	Time_Regulate(tm,YY,MM,DD,hh,mm,ss);

	/* Get wday */
	GregorianDay(tm);

	/* 修改当前RTC计数寄存器内容 */
	RTC_SetCounter(mktimev(tm));

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();
}
FINSH_FUNCTION_EXPORT(Time_Adjust, Time_Adjust)

/*
 * This only works for the Gregorian calendar - i.e. after 1752 (in the UK)
 */
/*计算公历*/
void GregorianDay(struct rtc_time * tm)
{
	int leapsToDate;
	int lastYear;
	int day;
	int MonthOffset[] =
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

	lastYear = tm->tm_year - 1;

	/*计算从公元元年到计数的前一年之中一共经历了多少个闰年*/
	leapsToDate = lastYear / 4 - lastYear / 100 + lastYear / 400;

	/*如若计数的这一年为闰年，且计数的月份在2月之后，则日数加1，否则不加1*/
	if ((tm->tm_year % 4 == 0) && ((tm->tm_year % 100 != 0) || (tm->tm_year % 400 == 0)) && (tm->tm_mon > 2))
	{
		/*
		 * We are past Feb. 29 in a leap year
		 */
		day = 1;
	}
	else
	{
		day = 0;
	}

	day += lastYear * 365 + leapsToDate + MonthOffset[tm->tm_mon - 1] + tm->tm_mday; /*计算从公元元年元旦到计数日期一共有多少天*/

	tm->tm_wday = day % 7;
}

/* Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 * Assumes input in normal date format, i.e. 1980-12-31 23:59:59
 * => year=1980, mon=12, day=31, hour=23, min=59, sec=59.
 *
 * [For the Julian calendar (which was used in Russia before 1917,
 * Britain & colonies before 1752, anywhere else before 1582,
 * and is still in use by some communities) leave out the
 * -year/100+year/400 terms, and add 10.]
 *
 * This algorithm was first published by Gauss (I think).
 *
 * WARNING: this function will overflow on 2106-02-07 06:28:16 on
 * machines were long is 32-bit! (However, as time_t is signed, we
 * will already get problems at other places on 2038-01-19 03:14:08)

 *ADD by fire：本函数在工程中的输入参数为北京时间，
 所以在转换成时间戳时最后要从北京时间转换为标准时间的时间戳
 */
u32 mktimev(struct rtc_time *tm)
{
	if (0 >= (int) (tm->tm_mon -= 2))
	{ /* 1..12 -> 11,12,1..10 */
		tm->tm_mon += 12; /* Puts Feb last since it has leap day */
		tm->tm_year -= 1;
	}

	return ((((u32) (tm->tm_year / 4 - tm->tm_year / 100 + tm->tm_year / 400 + 367 * tm->tm_mon / 12 + tm->tm_mday)
			+ tm->tm_year * 365 - 719499) * 24 + tm->tm_hour /* now have hours */
	) * 60 + tm->tm_min /* now have minutes */
	) * 60 + tm->tm_sec - 8 * 60 * 60; /* finally seconds */
	/*Add by fire: -8*60*60 把输入的北京时间转换为标准时间，
	 再写入计时器中，确保计时器的数据为标准的UNIX时间戳*/

}

void Get_Time(u32 tim, struct rtc_time * tm)
{
	register u32 i;
	register long hms, day;

	tim += 8 * 60 * 60; //北京时间

	day = tim / SECDAY;
	hms = tim % SECDAY;

	/* Hours, minutes, seconds are easy */
	tm->tm_hour = hms / 3600;
	tm->tm_min = (hms % 3600) / 60;
	tm->tm_sec = (hms % 3600) % 60;

	/* Number of years in days *//*算出当前年份，起始的计数年份为1970年*/
	for (i = STARTOFTIME; day >= days_in_year(i); i++)
	{
		day -= days_in_year(i);
	}
	tm->tm_year = i;

	/* Number of months in days left */	/*计算当前的月份*/
	if (leapyear(tm->tm_year))
	{
		days_in_month(FEBRUARY) = 29;
	}
	for (i = 1; day >= days_in_month(i); i++)
	{
		day -= days_in_month(i);
	}
	days_in_month(FEBRUARY) = 28;
	tm->tm_mon = i;

	/* Days are what is left over (+1) from all that. *//*计算当前日期*/
	tm->tm_mday = day + 1;

	/*
	 * Determine the day of week
	 */
	GregorianDay(tm);

}
FINSH_FUNCTION_EXPORT(Get_Time, Get_time(u32 tim, struct rtc_time * tm))

void RTC_Init()
{
	NVIC_Configuration();
	RTC_Configuration();
	Time_Adjust(&TimeNow,2013,1,1,0,0,0);
//	RTC_CheckAndConfig(&TimeNow);
}
/************************END OF FILE***************************************/
