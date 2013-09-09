/*
 * rtc.h
 *
 *  Created on: 2013年9月9日
 *      Author: Administrator
 */

#ifndef RTC_H_
#define RTC_H_

#include "stm32f10x.h"

/* 如果定义了下面这个宏的话,PC13就会输出频率为RTC Clock/64的时钟 */
//#define RTCClockOutput_Enable  /* RTC Clock/64 is output on tamper pin(PC.13) */

struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
//	int tm_wday;
};
extern struct rtc_time TimeNow;

//void GregorianDay(struct rtc_time * tm);
uint32_t mktimev(struct rtc_time *tm);
void Get_Time(uint32_t tim, struct rtc_time * tm);
void Set_TimeNow(uint16_t YY, uint16_t MM, uint16_t DD, uint16_t hh, uint16_t mm, uint16_t ss);
void Get_TimeNow();

static void NVIC_Configuration(void);
static void RTC_Configuration(void);
static void RTC_CheckAndConfig(struct rtc_time *tm);
void Time_Regulate(struct rtc_time *tm, uint16_t YY, uint16_t MM, uint16_t DD, uint16_t hh, uint16_t mm, uint16_t ss);
void Time_Adjust(struct rtc_time *tm, uint16_t YY, uint16_t MM, uint16_t DD, uint16_t hh, uint16_t mm, uint16_t ss);
void RTC_Init();

#endif /* RTC_H_ */
