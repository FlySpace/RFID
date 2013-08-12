#define __IIC_C
#include "stm32f10x.h"

//#include "mcu.h"
#include "iic.h"


//#include"stm32f10x_gpio.h"
//#include"tos.h"
//#include"stm32stdio.h"
//#include"cfg.h"

#define IIC_DELAYTIME     10
#define  ACKCHECKTIME     200

#define INPUT 0x00
#define OUTPUT 0x01


#define  IIC_PAGE 16

void IIC_Delay(u16 times)
		{
		u16 loop,loop1;
		for(loop=0;loop<times;loop++)
		for(loop1=times;loop1>0;loop1--);
		}
void  IIC_SetSclPin(void)
		{
		GPIO_WriteBit(GPIOB,GPIO_Pin_6,Bit_SET);
		}
void    IIC_ClrSclPin(void)
		{
		GPIO_WriteBit(GPIOB,GPIO_Pin_6,Bit_RESET);
		}
void    IIC_SetSdaPin(void)
		{
		GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET);
		}
void    IIC_ClrSdaPin(void)
		{
		GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_RESET);
		}
bool    IIC_GetSdaStus(void)
		{
		bool tmp;
		
		if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)&0x01) tmp=TRUE;
		else tmp=FALSE;
		return tmp;
		
		}
bool   IIC_CheckACK(void)
		{
		return IIC_GetSdaStus();
		}
void    IIC_SetScl(bool HLow)
		{
		HLow?IIC_SetSclPin():IIC_ClrSclPin();
		}
void    IIC_SetSda(bool HLow)
		{
		HLow?IIC_SetSdaPin():IIC_ClrSdaPin();
		}
void   IIC_Start(void)
		{

		//    SCL(Low);
		IIC_SetScl(LOW);
		//  Delay(DelayTime);
		IIC_Delay(IIC_DELAYTIME);
		//  SDA(Hight);
		IIC_SetSda(HIGHT);

		//  Delay(DelayTime);
		IIC_Delay(IIC_DELAYTIME);
		// SCL(Hight);
		IIC_SetScl(HIGHT);
		//  Delay(DelayTime);
		IIC_Delay(IIC_DELAYTIME);
		//   SDA(Low);
		IIC_SetSda(LOW);
		//  Delay(DelayTime);
		IIC_Delay(IIC_DELAYTIME);
		//  SCL(Low);
		IIC_SetScl(LOW);

		}
void   IIC_Stop(void)
		{

		IIC_SetSda(LOW);

		IIC_Delay(IIC_DELAYTIME);
		IIC_SetScl(HIGHT);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetSda(HIGHT);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetScl(LOW);
		}
bool   IIC_Send(u8 ucVal)
		{
		bool Ack;
		u16 AckLoop;
		u8 i;
		for(i=0;i<8;i++)
		{
		IIC_SetScl(LOW);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetSda((ucVal & 0x80)?TRUE:FALSE);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetScl(HIGHT);
		IIC_Delay(IIC_DELAYTIME);
		ucVal<<= 1;
		}
		IIC_SetScl(LOW);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetSda(HIGHT);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetScl(HIGHT);
		Ack = IIC_NOACK;
		for(AckLoop=0;AckLoop<ACKCHECKTIME;AckLoop++) //260us
		{
		if(!IIC_GetSdaStus())
		{
		Ack = IIC_ACK;
		break;
		}
		IIC_Delay(IIC_DELAYTIME);
		}
		IIC_SetScl(LOW);
		return Ack;			//return success=0 / failure=1
		}
u8 IIC_Read(bool bACK)
		{
		u8 Data;
		u8 i;
		Data = 0;
		for(i=0;i<8;i++)
		{
		Data <<= 1;
		IIC_SetScl(LOW);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetSda(HIGHT);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetScl(HIGHT);
		IIC_Delay(IIC_DELAYTIME);
		if(IIC_GetSdaStus())
		Data |= 1;
		else
		Data &= 0xfe;
		}
		IIC_SetScl(LOW);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetSda(bACK);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetScl(HIGHT);
		IIC_Delay(IIC_DELAYTIME);
		IIC_SetScl(LOW);
		return Data;

		}
bool IIC_ByteWrite(u8 DeviceAdd,u8 address,u8 ucVal)
		{

	
		 IIC_Start();
		if(IIC_Send(DeviceAdd|((address>>7)&0x0e)))//NACK
			{
	         return IIC_NOACK;
			}
		IIC_Send((u8)(address&0xff));
		IIC_Send(ucVal);
		IIC_Stop();

		IIC_Delay(IIC_DELAYTIME);
		IIC_Delay(IIC_DELAYTIME);
		IIC_Delay(20*IIC_DELAYTIME);
		
		return IIC_ACK;
		}
u8 IIC_ByteRead(u8 DeviceAdd,u8 address)
		{
		u8 value;
		
		IIC_Start();
		IIC_Send(DeviceAdd|((address>>7)&0x0e));
		IIC_Send((u8)(address&0xff));
		IIC_Stop();
		IIC_Start();
		IIC_Send(DeviceAdd|0x01);
		value=IIC_Read(IIC_NOACK);
		IIC_Stop();
		IIC_Delay(IIC_DELAYTIME);
		IIC_Delay(IIC_DELAYTIME);
		IIC_Delay(IIC_DELAYTIME);
		
		return value;
		}
bool IIC_BytesWrite(u8 DeviceAdd,u8 address,u8 *buf,u16 nByte)
			{
			int lp=0;
			for(lp=0;lp<nByte;lp++)
				{
				IIC_ByteWrite(DeviceAdd,lp+address,*buf++);
				}
                         return TRUE;
			}
bool IIC_BytesRead(u8 DeviceAdd,u8 address,u8 *buf,u16 nByte)
			{
			int lp=0;
			for(lp=0;lp<nByte;lp++)
				{
				*buf++=IIC_ByteRead(DeviceAdd,lp+address);
				}
                        return TRUE;
			}



