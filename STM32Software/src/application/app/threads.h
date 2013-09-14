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
#include "rtc.h"

#define THREAD_BOOT_NAME "boot"
#define THREAD_BUTTON_NAME "button"
#define THREAD_CARD_NAME "card"
#define THREAD_CARD_CONTROL_NAME "cc"
#define THREAD_BLUETOOTH_NAME "bt"
#define THREAD_BLUETOOTH_CONTROL_NAME "btc"
#define THREAD_AUTO_LOOK_AND_DELETE_CARD_DATA_STACK_SIZE 128

struct CardDataHeader
{
	uint16_t reserved;
	uint16_t epc;
	uint16_t tid;
	uint16_t user;
	struct rtc_time time;
};

extern struct RingBuffer cardData;

enum CardOperation
{
	NO, AUTO_READ, WRITE_RESERVED, WRITE_EPC, WRITE_TID, WRITE_USER
};

void thread_boot(void * param);
void thread_button(void * param);
void thread_card(void * param);
void thread_card_control(void * param);
void thread_bluetooth(void * param);
void thread_bluetooth_control(void * param);
void mallocAfterFree(rt_size_t size, uint8_t ** mem, uint8_t * memFlag);

extern enum CardOperation operation;
extern uint16_t writeOffset;
extern uint16_t writeLength;
extern uint8_t * writeBuffer;
extern uint8_t writeBufferFlag;

#endif /* THREAD_BOOT_H_ */
