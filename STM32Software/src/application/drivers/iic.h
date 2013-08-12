#ifndef __IIC_H
#define __IIC_H

#ifdef _IIC_C_
#define  _IIC_EXTERN  
#else
#define _IIC_EXTERN extern  
#endif

#include "stm32f10x.h"
#define IIC_NOACK         TRUE
#define IIC_ACK           FALSE
#define HIGHT             TRUE
#define LOW               FALSE

typedef enum _bool{
FALSE = 0,
TRUE
}bool;

	_IIC_EXTERN	void    IIC_SetSclPin(void);
	_IIC_EXTERN	void    IIC_ClrSclPin(void);
	_IIC_EXTERN	void    IIC_SetSdaPin(void);
	_IIC_EXTERN	void    IIC_ClrSdaPin(void);
	_IIC_EXTERN	bool    IIC_GetSdaStus(void);
	_IIC_EXTERN	bool    IIC_CheckACK(void);
	_IIC_EXTERN	void    IIC_SetScl(bool HLow);
	_IIC_EXTERN	void    IIC_SetSda(bool HLow);
	_IIC_EXTERN	void    IIC_Start(void);
	_IIC_EXTERN	void    IIC_Stop(void);
	_IIC_EXTERN	bool    IIC_Send(u8 ucVal);
	_IIC_EXTERN	u8      IIC_Read(bool bACK);
	_IIC_EXTERN	bool    IIC_ByteWrite(u8 DeviceAdd,u8 address,u8 ucVal);
	_IIC_EXTERN	u8      IIC_ByteRead(u8 DeviceAdd,u8 address);
	_IIC_EXTERN	bool    IIC_BytesWrite(u8 DeviceAdd,u8 address,u8 *buf,u16 nByte);
	_IIC_EXTERN	bool    IIC_BytesRead(u8 DeviceAdd,u8 address,u8 *buf,u16 nByte);

#endif	   
