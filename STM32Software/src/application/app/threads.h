/*
 * thread_boot.h
 *
 *  Created on: 2013-8-13
 *      Author: Fly
 */

#ifndef THREADS_H_
#define THREADS_H_

#include "ringbuffer.h"
#include "stm32f10x.h"

#define THREAD_BOOT_NAME "boot"
#define THREAD_BUTTON_NAME "button"
#define THREAD_CARD_NAME "card"
#define THREAD_CARD_CONTROL_NAME "cc"
#define THREAD_BLUETOOTH_NAME "bt"
#define THREAD_BLUETOOTH_CONTROL_NAME "btc"

struct CardDataHeader
{
	uint16_t reserved;
	uint16_t epc;
	uint16_t tid;
	uint16_t user;
};

extern struct RingBuffer cardData;

void thread_boot(void * param);
void thread_button(void * param);
void thread_card(void * param);
void thread_card_control(void * param);
void thread_bluetooth(void * param);
void thread_bluetooth_control(void * param);

#endif /* THREAD_BOOT_H_ */
