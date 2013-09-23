/*
 * buzzer.c
 *
 *  Created on: 2013年9月16日
 *      Author: Administrator
 */
#include "stm32f10x.h"
#include "finsh.h"

void Buzzer_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	//配置时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	//配置PB8
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;                 //设置为复用推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//配置定时器
	TIM_DeInit(TIM4);
	TIM_InternalClockConfig(TIM4);
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;
	TIM_TimeBaseStructure.TIM_Period = 1500;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	//配置PWM
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;                //设置为pwm1输出模式
	TIM_OCInitStructure.TIM_Pulse = 749;                             //设置占空比时间
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;         //设置输出极性
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;    //使能该通道输出
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM4, ENABLE);
//	TIM_Cmd(TIM4, ENABLE);
}

void buzzerEnable()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_8);
	TIM_Cmd(TIM4, ENABLE);
}
FINSH_FUNCTION_EXPORT(buzzerEnable, buzzerEnable)

void buzzerDisable()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_8);
	TIM_Cmd(TIM4, DISABLE);
}
FINSH_FUNCTION_EXPORT(buzzerDisable, buzzerDisable)
