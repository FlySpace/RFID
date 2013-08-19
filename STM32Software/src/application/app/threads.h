/*
 * thread_boot.h
 *
 *  Created on: 2013-8-13
 *      Author: Fly
 */

#ifndef THREADS_H_
#define THREADS_H_

#define THREAD_BOOT_NAME "boot"

void thread_boot(void * param);
void thread_button(void * param);
void thread_card(void * param);

#endif /* THREAD_BOOT_H_ */
