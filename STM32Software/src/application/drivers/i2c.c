/*
 * iic.c
 *
 *  Created on: 2013年9月17日
 *      Author: Administrator
 */
#include "i2c.h"
#include "stm32f10x_rcc.h"

/******* specified by user ************/
//#define I2C_REMAP
//#define SLAVE_10BIT_ADDRESS
/******* specify by user ************/

#define I2C1_DR_Address        0x40005410
#define I2C2_DR_Address        0x40005810
#define Transmitter             0x00
#define Receiver                0x01
static __IO uint8_t MasterDirection = Transmitter;
static uint16_t SlaveADDR;
static uint8_t DeviceOffset = 0xff;
static bool check_begin = FALSE;
static bool OffsetDone = FALSE;

__IO uint8_t MasterReceptionComplete = 0;
__IO uint8_t MasterTransitionComplete = 0; // to indicate master's send process
__IO uint8_t SlaveReceptionComplete = 0;
__IO uint8_t SlaveTransitionComplete = 0;

__IO uint8_t WriteComplete = 0; // to indicate target's internal write process

/*P-V operation on I2C1 or I2C2*/
static __IO uint8_t PV_flag_1;
static __IO uint8_t PV_flag_2;

I2C_STATE i2c_comm_state;

void I2C_Comm_Init(I2C_TypeDef* I2Cx, uint32_t I2C_Speed, uint16_t I2C_Addr)
{
	/******* GPIO configuration and clock enable *********/
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	if (I2Cx == I2C1)
	{
#ifdef I2C_REMAP
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
#else
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
#endif
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
		I2C_DeInit(I2C1);
	}
	else if (I2Cx == I2C2)
	{
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
		I2C_DeInit(I2C2);
	}

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*********** I2C periphral configuration **********/
	I2C_DeInit(I2Cx);
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C; // fixed
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;  // fixed
	I2C_InitStructure.I2C_OwnAddress1 = I2C_Addr;  // user parameter
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable; // fixed
#ifdef SLAVE_10BIT_ADDRESS
			I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_10bit;  // user define
#else
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
#endif
	I2C_InitStructure.I2C_ClockSpeed = I2C_Speed; // user parameter
	I2C_Cmd(I2Cx, ENABLE);
	I2C_Init(I2Cx, &I2C_InitStructure);

	/************** I2C NVIC configuration *************************/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	if (I2Cx == I2C1)
	{
		NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
	}
	else
	{
		NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;
	}
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	if (I2Cx == I2C1)
	{
		NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
	}
	else
	{
		NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
	}
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}

/*******************************************************************************
 * Function Name  : I2C_Comm_MasterSend
 * Description    : write a block of data to slave devices.
 * Input          : - I2Cx : the I2C port from which mcu send data out.
 *                  - slave_addr : target slave
 *                  - offset : the internal memory of target slave to place data
 *                     if special value : no internal memory space of slave
 *                  - pBuffer : pointer to the data ready to send
 *                  - length : number of bytes to send.
 * Output         : None
 * Return         : None
 *******************************************************************************/
void I2C_Comm_MasterSend(I2C_TypeDef* I2Cx, uint16_t slave_addr, uint8_t offset, uint8_t* pBuffer, uint32_t length)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* PV operation */
	if (I2Cx == I2C1)
	{
		if (PV_flag_1 != 0)
			return;
		PV_flag_1 |= 1;
		if (PV_flag_1 != 1)
			return;
		PV_flag_1 = 3;
	}
	else
	{
		if (PV_flag_2 != 0)
			return;
		PV_flag_2 |= 1;
		if (PV_flag_2 != 1)
			return;
		PV_flag_2 = 3;
	}
	/* enter rountine safely */

	/* DMA initialization */
	if (I2Cx == I2C1)
	{
		DMA_DeInit(DMA1_Channel6);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) I2C1_DR_Address;
	}
	else
	{
		DMA_DeInit(DMA1_Channel4);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) I2C2_DR_Address;
	}
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) pBuffer; // from function input parameter
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // fixed for send function
	DMA_InitStructure.DMA_BufferSize = length; // from function input parameter
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // fixed
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // fixed
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte; //fixed
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //fixed
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // fixed
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;  // up to user
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; // fixed
	if (I2Cx == I2C1)
	{
		DMA_Init(DMA1_Channel6, &DMA_InitStructure);
		DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
	else
	{
		DMA_Init(DMA1_Channel4, &DMA_InitStructure);
		DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}

	/*initialize static parameter*/
	MasterDirection = Transmitter;
	MasterTransitionComplete = 0;

	/*initialize static parameter according to input parameter*/
	SlaveADDR = slave_addr; // this byte shoule be send by F/W (in loop or ISR way)
	DeviceOffset = offset; // this byte can be send by both F/W and DMA
	OffsetDone = FALSE;

	/* global state variable i2c_comm_state */
	i2c_comm_state = COMM_PRE;

	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	I2C_ITConfig(I2Cx, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);
	/* Send START condition */
	if (I2C1->CR1 & 0x200)
		I2C1->CR1 &= 0xFDFF;
	I2C_GenerateSTART(I2Cx, ENABLE);
}

/*******************************************************************************
 * Function Name  : I2C_Comm_MasterReceive
 * Description    : read a block of data from slave devices.
 * Input          : - I2Cx : the I2C port from which mcu send data out.
 *                  - slave_addr : target slave
 *                  - offset : the internal memory of target slave to place data
 *                     if special value : no internal memory space of slave
 *                  - pBuffer : pointer to the data ready to send
 *                  - length : number of bytes to send.
 * Output         : None
 * Return         : None
 *******************************************************************************/
void I2C_Comm_MasterReceive(I2C_TypeDef* I2Cx, uint16_t slave_addr, uint8_t offset, uint8_t* pBuffer, uint32_t length)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* PV operation */
	if (I2Cx == I2C1)
	{
		if (PV_flag_1 != 0)
			return;
		PV_flag_1 |= 1;
		if (PV_flag_1 != 1)
			return;
		PV_flag_1 = 3;
	}
	else
	{
		if (PV_flag_2 != 0)
			return;
		PV_flag_2 |= 1;
		if (PV_flag_2 != 1)
			return;
		PV_flag_2 = 3;
	}
	/* enter rountine safely */

	/* DMA initialization */
	if (I2Cx == I2C1)
	{
		DMA_DeInit(DMA1_Channel7);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) I2C1_DR_Address;
	}
	else
	{
		DMA_DeInit(DMA1_Channel5);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) I2C2_DR_Address;
	}
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) pBuffer; // from function input parameter
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; // fixed for receive function
	DMA_InitStructure.DMA_BufferSize = length; // from function input parameter
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // fixed
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // fixed
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte; //fixed
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //fixed
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // fixed
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;  // up to user
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; // fixed
	if (I2Cx == I2C1)
	{
		DMA_Init(DMA1_Channel7, &DMA_InitStructure);
		DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
	else
	{
		DMA_Init(DMA1_Channel5, &DMA_InitStructure);
		DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}

	/*initialize static parameter*/
	MasterDirection = Receiver;
	MasterReceptionComplete = 0;

	/*initialize static parameter according to input parameter*/
	SlaveADDR = slave_addr;
	DeviceOffset = offset;
	OffsetDone = FALSE;

	/* global state variable i2c_comm_state */
	i2c_comm_state = COMM_PRE;

	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	I2C_ITConfig(I2Cx, I2C_IT_EVT | I2C_IT_ERR, ENABLE); //only SB int allowed
	/* Send START condition */
	if (I2C1->CR1 & 0x200)
		I2C1->CR1 &= 0xFDFF;
	I2C_GenerateSTART(I2Cx, ENABLE);
}

/*******************************************************************************
 * Function Name  : I2C_Comm_SlaveReceive
 * Description    : receive a block of data from the master.
 * Input          : - I2Cx : the I2C port from which mcu read data in.
 *                  - pBuffer : ram's location to locate the received data
 *                  - length : number of bytes to receive.
 * Output         : None
 * Return         : None
 *******************************************************************************/
void I2C_Comm_SlaveReceive(I2C_TypeDef* I2Cx, uint8_t* pBuffer, uint32_t length)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* PV operation */
	if (I2Cx == I2C1)
	{
		if (PV_flag_1 != 0)
			return;
		PV_flag_1 |= 1;
		if (PV_flag_1 != 1)
			return;
		PV_flag_1 = 3;
	}
	else
	{
		if (PV_flag_2 != 0)
			return;
		PV_flag_2 |= 1;
		if (PV_flag_2 != 1)
			return;
		PV_flag_2 = 3;
	}
	/* enter rountine safely */

	/* DMA initialization */
	if (I2Cx == I2C1)
	{
		DMA_DeInit(DMA1_Channel7);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) I2C1_DR_Address;
	}
	else
	{
		DMA_DeInit(DMA1_Channel5);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) I2C2_DR_Address;
	}
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) pBuffer; // from function input parameter
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; // fixed for receive function
	DMA_InitStructure.DMA_BufferSize = length; // from function input parameter
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // fixed
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // fixed
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte; //fixed
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //fixed
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // fixed
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;  // up to user
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; // fixed
	if (I2Cx == I2C1)
	{
		DMA_Init(DMA1_Channel7, &DMA_InitStructure);
		DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
	else
	{
		DMA_Init(DMA1_Channel5, &DMA_InitStructure);
		DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}

	SlaveReceptionComplete = 0;

	/* global state variable i2c_comm_state */
	i2c_comm_state = COMM_PRE;

	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	I2C_ITConfig(I2Cx, I2C_IT_EVT | I2C_IT_ERR, ENABLE); // ADDR BTF only
}

/*******************************************************************************
 * Function Name  : I2C_Comm_SlaveSend
 * Description    : send a block of data out to the master.
 * Input          : - I2Cx : the I2C port from which mcu send data out.
 *                  - pBuffer : ram's location to locate the data to be send
 *                  - length : number of bytes to receive.
 * Output         : None
 * Return         : None
 *******************************************************************************/
void I2C_Comm_SlaveSend(I2C_TypeDef* I2Cx, uint8_t* pBuffer, uint32_t length)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* PV operation */
	if (I2Cx == I2C1)
	{
		if (PV_flag_1 != 0)
			return;
		PV_flag_1 |= 1;
		if (PV_flag_1 != 1)
			return;
		PV_flag_1 = 3;
	}
	else
	{
		if (PV_flag_2 != 0)
			return;
		PV_flag_2 |= 1;
		if (PV_flag_2 != 1)
			return;
		PV_flag_2 = 3;
	}
	/* enter rountine safely */

	/* DMA initialization */
	if (I2Cx == I2C1)
	{
		DMA_DeInit(DMA1_Channel6);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) I2C1_DR_Address;
	}
	else
	{
		DMA_DeInit(DMA1_Channel4);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) I2C2_DR_Address;
	}
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) pBuffer; // from function input parameter
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // fixed for send function
	DMA_InitStructure.DMA_BufferSize = length; // from function input parameter
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // fixed
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // fixed
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte; //fixed
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //fixed
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // fixed
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;  // up to user
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; // fixed
	if (I2Cx == I2C1)
	{
		DMA_Init(DMA1_Channel6, &DMA_InitStructure);
		DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
	else
	{
		DMA_Init(DMA1_Channel4, &DMA_InitStructure);
		DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}

	SlaveTransitionComplete = 0;

	/* global state variable i2c_comm_state */
	i2c_comm_state = COMM_PRE;

	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	I2C_ITConfig(I2Cx, I2C_IT_EVT | I2C_IT_ERR, ENABLE); // ADDR only
}

uint32_t test1[50];
uint16_t test_index = 0;
void i2c1_evt_isr()
{
	test1[test_index] = I2C_GetLastEvent(I2C1);
//  switch (I2C_GetLastEvent(I2C1))
	switch (test1[test_index])
	{
	/************************** Master Invoke**************************************/
	case I2C_EVENT_MASTER_MODE_SELECT: /* EV5 */
		// MSL SB BUSY 30001
		if (!check_begin)
			i2c_comm_state = COMM_IN_PROCESS;

		if (MasterDirection == Receiver)
		{
//                if (DeviceOffset != 0xffffffff)
			if (!OffsetDone)
				I2C_Send7bitAddress(I2C1, SlaveADDR, I2C_Direction_Transmitter);
			else
			{
				/* Send slave Address for read */
				I2C_Send7bitAddress(I2C1, SlaveADDR, I2C_Direction_Receiver);
				OffsetDone = FALSE;
			}
		}
		else
		{
			/* Send slave Address for write */
			I2C_Send7bitAddress(I2C1, SlaveADDR, I2C_Direction_Transmitter);
		}
		I2C_ITConfig(I2C1, I2C_IT_BUF, ENABLE); //also TxE int allowed
		break;

		/********************** Master Receiver events ********************************/
	case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED: /* EV6 */
		// MSL BUSY ADDR 0x30002
		I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF, DISABLE);

		//without it, no NAK signal on bus after last Data
		//I2C data line would be hold low ~~~
		I2C_DMALastTransferCmd(I2C1, ENABLE);

		I2C_DMACmd(I2C1, ENABLE);
		DMA_Cmd(DMA1_Channel7, ENABLE);
		break;

	case I2C_EVENT_MASTER_BYTE_RECEIVED: /* EV7 */
		// MSL BUSY RXNE 0x30040
		break;

		/************************* Master Transmitter events **************************/
	case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED: /* EV8 just after EV6 */
		//BUSY, MSL, ADDR, TXE and TRA 0x70082
		if (check_begin)
		{
			check_begin = FALSE;
			WriteComplete = 1;
			i2c_comm_state = COMM_DONE;
			I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
			I2C_GenerateSTOP(I2C1, ENABLE);
			PV_flag_1 = 0;
			break;
		}

		{
			I2C_SendData(I2C1, DeviceOffset);
			//DeviceOffset >>=8;
			OffsetDone = TRUE;
		}
		break;

	case I2C_EVENT_MASTER_BYTE_TRANSMITTING: /* EV8 */
		//TRA, BUSY, MSL, TXE 0x70080
//            if(!OffsetDone)
//              {
//                static uint8_t nb=0;
//                I2C_SendData(I2C1, DeviceOffset);
//                DeviceOffset >>=8;
//                nb++;
//
//                if (nb != 3)
//                  break;
//                if (nb ==3)
//                {
//                  nb = 0;
//                  OffsetDone = TRUE;
//                  break;
//                }
//              }

		if (MasterDirection == Receiver)
		{
			I2C_ITConfig(I2C1, I2C_IT_BUF, DISABLE);
			while ((I2C1->CR1 & 0x200) == 0x200)
				;
			I2C_GenerateSTART(I2C1, ENABLE);
			break;
		}
		else
		{
			I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF, DISABLE);
			I2C_DMACmd(I2C1, ENABLE);
			DMA_Cmd(DMA1_Channel6, ENABLE);
			break;
		}

	case I2C_EVENT_MASTER_BYTE_TRANSMITTED: /* EV8-2 */
		//TRA, BUSY, MSL, TXE and BTF 0x70084
		break;
	}
//test_index++;
}

void i2c1_err_isr()
{
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
	{
		if (check_begin)
			I2C_GenerateSTART(I2C1, ENABLE);
		else if (I2C1->SR2 & 0x01)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			i2c_comm_state = COMM_EXIT;
			PV_flag_1 = 0;
		}

		I2C_ClearFlag(I2C1, I2C_FLAG_AF);
	}

	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BERR))
	{
		if (I2C1->SR2 & 0x01)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			i2c_comm_state = COMM_EXIT;
			PV_flag_1 = 0;
		}

		I2C_ClearFlag(I2C1, I2C_FLAG_BERR);
	}
}


void i2c1_send_dma_isr()
{

	if (DMA_GetFlagStatus(DMA1_FLAG_TC6))
	{
		if (I2C1->SR2 & 0x01) // master send DMA finish, check process later
		{
			// DMA1-6 (I2C1 Tx DMA)transfer complete ISR
			I2C_DMACmd(I2C1, DISABLE);
			DMA_Cmd(DMA1_Channel6, DISABLE);
			// wait until BTF
			while (!(I2C1->SR1 & 0x04))
				;
			I2C_GenerateSTOP(I2C1, ENABLE);
			// wait until BUSY clear
			while (I2C1->SR2 & 0x02)
				;

			MasterTransitionComplete = 1;
			i2c_comm_state = COMM_IN_PROCESS;
			I2C_ITConfig(I2C1, I2C_IT_EVT, ENABLE); // use interrupt to handle check process
			check_begin = TRUE;
			if (I2C1->CR1 & 0x200)
				I2C1->CR1 &= 0xFDFF;
			I2C_GenerateSTART(I2C1, ENABLE);
		}
		else // slave send DMA finish
		{
//      I2C_DMACmd(I2C1, DISABLE);
//      i2c_comm_state = COMM_DONE;
//      SlaveTransitionComplete = 1;
//      PV_flag_1 = 0;
		}

		DMA_ClearFlag(DMA1_FLAG_TC6);
	}
	if (DMA_GetFlagStatus(DMA1_FLAG_GL6))
	{
		DMA_ClearFlag( DMA1_FLAG_GL6);
	}
	if (DMA_GetFlagStatus(DMA1_FLAG_HT6))
	{
		DMA_ClearFlag( DMA1_FLAG_HT6);
	}
}

void i2c1_receive_dma_isr()
{
	if (DMA_GetFlagStatus(DMA1_FLAG_TC7))
	{
		if (I2C1->SR2 & 0x01) // master receive DMA finish
		{
			I2C_DMACmd(I2C1, DISABLE);
			I2C_GenerateSTOP(I2C1, ENABLE);
			i2c_comm_state = COMM_DONE;
			MasterReceptionComplete = 1;
			PV_flag_1 = 0;
		}
		else // slave receive DMA finish
		{
//      i2c_comm_state = CHECK_IN_PROCESS;
//      (void)(I2C_GetITStatus(I2C1, I2C_IT_STOPF));
//      I2C_Cmd(I2C1, ENABLE);
//      SlaveReceptionComplete = 1;
//      I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF |I2C_IT_ERR, ENABLE); // use interrupt to handle check process
		}

		DMA_ClearFlag(DMA1_FLAG_TC7);
	}
	if (DMA_GetFlagStatus(DMA1_FLAG_GL7))
	{
		DMA_ClearFlag( DMA1_FLAG_GL7);
	}
	if (DMA_GetFlagStatus(DMA1_FLAG_HT7))
	{
		DMA_ClearFlag( DMA1_FLAG_HT7);
	}
}



/*******************************************************************************
* Function Name  : DMA1_Channel6_IRQHandler
* Description    : This function handles DMA1 Channel 6 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel6_IRQHandler(void)
{
  i2c1_send_dma_isr();
}

/*******************************************************************************
* Function Name  : DMA1_Channel7_IRQHandler
* Description    : This function handles DMA1 Channel 7 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel7_IRQHandler(void)
{
  i2c1_receive_dma_isr();
}

/*******************************************************************************
* Function Name  : I2C1_EV_IRQHandler
* Description    : This function handles I2C1 Event interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C1_EV_IRQHandler(void)
{
  i2c1_evt_isr();
}

/*******************************************************************************
* Function Name  : I2C1_ER_IRQHandler
* Description    : This function handles I2C1 Error interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C1_ER_IRQHandler(void)
{
  i2c1_err_isr();
}
