/*
 * iic.h
 *
 *  Created on: 2013年9月17日
 *      Author: Administrator
 */

#ifndef I2C_H_
#define I2C_H_

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported types ------------------------------------------------------------*/
typedef enum _bool
{
	FALSE = 0, TRUE
} bool;

typedef enum i2c_result
{
	NO_ERR = 0,
	TIMEOUT = 1,
	BUS_BUSY = 2,
	SEND_START_ERR = 3,
	ADDR_MATCH_ERR = 4,
	ADDR_HEADER_MATCH_ERR = 5,
	DATA_TIMEOUT = 6,
	WAIT_COMM = 7,
	STOP_TIMEOUT = 8

} I2C_Result;

typedef enum i2c_state
{
	COMM_DONE = 0,  // done successfully
	COMM_PRE = 1,
	COMM_IN_PROCESS = 2,
	CHECK_IN_PROCESS = 3,
	COMM_EXIT = 4 // exit since failure

} I2C_STATE;

extern I2C_STATE i2c_comm_state;
extern vu8 MasterReceptionComplete;
extern vu8 MasterTransitionComplete;
extern vu8 WriteComplete;
extern vu8 SlaveReceptionComplete;
extern vu8 SlaveTransitionComplete;

void I2C_Comm_Init(I2C_TypeDef* I2Cx, u32 I2C_Speed, u16 I2C_Addr);
void I2C_Comm_MasterSend(I2C_TypeDef* I2Cx, u16 slave_addr, u8 offset, u8* pBuffer, u32 length);
void I2C_Comm_SlaveReceive(I2C_TypeDef* I2Cx, u8* pBuffer, u32 length);
void I2C_Comm_MasterReceive(I2C_TypeDef* I2Cx, u16 slave_addr, u8 offset, u8* pBuffer, u32 length);
void I2C_Comm_SlaveSend(I2C_TypeDef* I2Cx, u8* pBuffer, u32 length);

void i2c1_evt_isr(void);
void i2c1_err_isr(void);
void i2c2_evt_isr(void);
void i2c2_err_isr(void);

void i2c1_send_dma_isr(void);
void i2c1_receive_dma_isr(void);
void i2c2_receive_dma_isr(void);
void i2c2_send_dma_isr(void);

//void DMA1_Channel6_IRQHandler(void);
//void DMA1_Channel7_IRQHandler(void);
//void I2C1_EV_IRQHandler(void);
//void I2C1_ER_IRQHandler(void);

#endif /* IIC_H_ */
