/*
 * OLED.c
 *
 *  Created on: 2013年9月23日
 *      Author: Administrator
 */

#include "i2c.h"
#include "finsh.h"
#include "stm32f10x.h"

static uint8_t cmdOledPowerOn[] =
{ 		0xAE,			//Display Off
		0xD5, 0x80,		//Set Display Clock Divide Ratio/Oscillator Frequency (0000b 1000b)
		0x8D, 0x14,		//Set Charge Pump (Internal DC/DC Circuit)
		0xD9, 0xF1,		//Set Pre-charge Period (Internal DC/DC Circuit)
		0x81, 0xCF,		//Set Contrast Control (Internal DC/DC Circuit)
		0xDB, 0x30,		//Set VCOMH Deselect Level (~ 0.83 x VCC)
		0x20, 0x00,		//Set Memory Addressing Mode (Horizontal)
		0x21, 0x00, 0x7F, //Set Column Address (0-127h)
		0x22, 0x00, 0x07, //Set Page Address (0-7h)
		0xA0,			//Set Segment Re-map (column address 0 is mapped to	SEG0)
		0xA6,			//Set Normal/Inverse Display (Normal)
		0xA8, 0x3F,		//Set Multiplex Ratio (64MUX)
		0xC0,			//Set COM Output Scan Direction (normal mode Scan from COM0 to COM[N –1], Where N is the Multiplex ratio.)
		0xDA, 0x12,		//Set COM Pins Hardware	Configuration (Disable COM Left/Right remap / Alternative COM pin	configuration)
		0xD3, 0x00,		//Set Display Offset (00h)
		0x40,			//Set Display Start Line (0)
		0xA4,			//Entire Display ON (Resume to RAM content display)
		0xAF			//Display On
};
static const uint8_t displayRam2[] =
{
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xDF,0xDF,0x1F,0x5F,0x4F,0x5F,0x1F,0xDF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0x9F,0x7F,0x1F,
		      0x7F,0x8F,0x3F,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0x7F,0x7F,0x0F,0x7F,0x7F,0x7F,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0x3F,0xCF,0xDF,0x5F,
		      0x5F,0x5F,0xDF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0x0F,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xBF,0x0F,0xBF,0xDF,
		      0xBF,0x0F,0x9F,0x7F,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0x9F,0x7F,0x7F,0x8F,0x3F,0xBF,0x9F,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0x03,0xFA,0xC1,0xC9,0x41,0x7A,0x83,
		      0xFF,0xFF,0xFF,0xFF,0xD1,0xC0,0xEF,0x02,
		      0x82,0x80,0x7A,0x82,0xFF,0xFF,0xFF,0xFF,
		      0xBF,0xDF,0xEF,0xF0,0xF1,0xEF,0x9F,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFE,0xFE,
		      0xFE,0xE0,0x8F,0xB7,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xBF,0xBF,0x80,0xBE,0xBE,0xBF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xF7,0x00,0xFE,0xBE,
		      0xBE,0xB7,0xB6,0x80,0xFF,0xFF,0xFF,0xFF,
		      0xF7,0xC1,0xBE,0xBF,0xCF,0xE0,0xDF,0xBF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0x7F,0xBF,0x7F,0x7F,0x7F,0x7F,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,
		      0xFF,0x3F,0xFF,0x7F,0xFF,0xFF,0xFF,0xFF,
		      0xBF,0x3F,0x3F,0xBF,0xFF,0xFF,0x3F,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,
		      0xFF,0x3F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0x3F,0xBF,0xBF,0xBF,0xBF,0x3F,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,
		      0x3F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xBF,0xBF,0xBF,0x3F,0xBF,0xBF,0xBF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xCD,0xF2,0x00,0xF7,0xF7,0x00,0xF7,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xEE,0x00,0xF6,0xE0,
		      0xEA,0x00,0x8A,0x60,0xFF,0xFF,0xFF,0xFF,
		      0x00,0xA0,0xA0,0x00,0xF4,0x06,0x18,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0x7B,0x80,0x39,0xF7,
		      0x6A,0xAD,0x02,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0x04,0xF5,0xF5,0x01,0xF5,0x04,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0x79,0x80,0x99,0x78,
		      0xFE,0x00,0xB6,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFB,0x7B,0x8B,0xE0,0x9B,0x7B,0x7F,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFE,0xFF,0xFE,0xFE,0xFE,0xFE,0xFE,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFD,0xFE,
		      0xFE,0xFF,0xFE,0xFC,0xFF,0xFF,0xFF,0xFF,
		      0xFE,0xFE,0xFE,0xFE,0xFF,0xFC,0xFE,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFE,
		      0xFE,0xFE,0xFF,0xFE,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFE,0xFE,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFF,
		      0xFF,0xFC,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xEF,0x00,
		      0xEF,0x13,0xF8,0x03,0x1B,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xF7,0x00,0xFD,0x05,0x51,0x54,0x05,0xFD,
		      0xFF,0xFF,0xFF,0xFF,0xB3,0x04,0xFA,0x45,
		      0x4A,0x4A,0x0E,0x40,0xFF,0xFF,0xFF,0xFF,
		      0xBF,0x00,0xD7,0x00,0xB5,0x01,0xB5,0x00,
		      0xFF,0xFF,0xFF,0xFF,0x9D,0xED,0x01,0xAC,
		      0xAD,0xAD,0x05,0xFD,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF8,
		      0xFC,0xF7,0xF7,0xF8,0xFF,0xFC,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xF0,0xF7,0xF8,0xF5,0xF5,0xF8,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xF8,0xFF,0xFF,
		      0xFC,0xFF,0xF8,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xF7,0xF8,0xFB,0xFC,0xFF,0xF0,0xF7,0xF8,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF8,0xFE,
		      0xFE,0xF6,0xF8,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};
static const uint8_t displayRam1[] =
{
		0x29,0x16,0x2B,0x2D,0x1B,0x15,0x2F,0x15,
		      0x2F,0x15,0x3F,0x55,0x3F,0x55,0x7F,0x55,
		      0xFF,0xAD,0x7B,0xDF,0x75,0xDF,0x7B,0xDF,
		      0xF5,0xBF,0xF7,0xDB,0xFF,0xDB,0xFF,0xDB,
		      0xFF,0x7F,0xDB,0xFF,0xFF,0xEF,0xFF,0xFF,
		      0xFF,0xEF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xDF,0xBF,0xFF,0x5F,0xB7,0x5F,0xB7,0x5F,
		      0x8B,0x57,0xA5,0x4B,0xA5,0x52,0xA1,0xD0,
		      0xA2,0xE1,0xE2,0xF1,0xFA,0xF4,0xFA,0xF9,
		      0xFE,0xFF,0xFF,0xFE,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0x60,0xB8,0xD0,0x78,0xD0,0xA8,0xF0,0x50,
		      0xA0,0xF0,0xA0,0xE0,0x40,0xE0,0x41,0xC0,
		      0x40,0x82,0x41,0x80,0x43,0x81,0x05,0x83,
		      0x0A,0x03,0x0A,0x07,0x9A,0x27,0x5E,0xB7,
		      0xAF,0x7B,0xDF,0x7D,0xEF,0xFF,0xFF,0xFE,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFD,
		      0xFF,0xFD,0xF6,0xFB,0xFD,0xF6,0xFD,0xFA,
		      0xFF,0x7D,0xFE,0xEF,0xBD,0xFF,0xD7,0x7D,
		      0x5F,0x37,0x5B,0x1E,0x17,0x0B,0x1F,0x07,
		      0x0B,0x07,0x0F,0x07,0x07,0x03,0x07,0x0F,
		      0x07,0x0F,0x07,0x0F,0x1F,0x0F,0x3F,0x3F,
		      0x3F,0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xAB,0x6A,0xDF,0xB5,0x6F,0x12,0x6F,0x15,
		      0x0A,0x17,0x0A,0x17,0x0D,0x17,0x0D,0x07,
		      0x0D,0x17,0x1D,0x07,0x1D,0x17,0x1D,0x2A,
		      0x1F,0x35,0x7D,0x56,0x7C,0xD5,0xFA,0x54,
		      0xEA,0xB5,0x55,0xAF,0x55,0xFE,0x57,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0x77,0xFB,0xDE,0x77,0x15,0x06,0x00,0x01,
		      0xE0,0xA0,0xF0,0xD8,0x7C,0xFE,0xD4,0x80,
		      0x00,0x00,0x00,0x00,0x00,0x10,0x08,0x3C,
		      0x14,0x18,0x88,0x1E,0xCC,0x54,0xA0,0xB0,
		      0xA8,0xF9,0xF1,0xD2,0xF7,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0x4A,0xD5,0xA2,0x54,0x81,0x01,0x00,0x00,
		      0x00,0x00,0x00,0x00,0x30,0x78,0x14,0xEA,
		      0xBE,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,
		      0x08,0x00,0x18,0x00,0x00,0x00,0x20,0x33,
		      0x01,0x06,0x2D,0xAA,0xF5,0xAA,0x55,0x6F,
		      0xBA,0xEF,0xBF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xDD,0xF7,0x40,0xE0,0xB0,0xE0,0xB1,
		      0xEA,0xB1,0xEA,0xF1,0xDB,0xF1,0xB9,0xF5,
		      0xE9,0xF8,0xF8,0xFC,0xF8,0xFC,0xFC,0xFE,
		      0xFE,0xFF,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xAB,0xFA,0xAE,0xFB,0xD6,0xBD,0xF5,0xAD,
		      0xFA,0x54,0xEA,0x7C,0xA8,0xF4,0xD8,0xB4,
		      0xE8,0xB8,0xE9,0xB8,0xF4,0x58,0xFC,0xB4,
		      0xFA,0x5C,0xFB,0xDE,0x7D,0xF6,0x5F,0xF6,
		      0xFB,0xAE,0xFA,0x56,0xBA,0xEF,0x59,0xF7,
		      0x5D,0xEA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xE2,0xDB,0xEE,0xFB,0xDE,0xEB,0xFE,0xF7,
		      0xED,0xFF,0xF7,0xDD,0xFF,0xEA,0xFF,0xFA,
		      0xEF,0xFA,0xFF,0xEE,0xFB,0xFF,0xED,0xFF,
		      0xFD,0xEF,0xFE,0xFB,0xEF,0xFD,0xF7,0xFE,
		      0xEF,0xFA,0xDF,0xED,0xFA,0xEF,0xF5,0xDF,
		      0xF5,0xEF,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

void oledPowerOn(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_5);

	I2C_Comm_Init(I2C1, 400000, 0x0);
	I2C_Comm_MasterSend(I2C1, 0x78, 0x00, cmdOledPowerOn, sizeof(cmdOledPowerOn));
	while (i2c_comm_state != COMM_DONE)
		;
}
FINSH_FUNCTION_EXPORT(oledPowerOn, oledPowerOn)

void display(uint8_t num)
{
	if(num == 1)
		I2C_Comm_MasterSend(I2C1, 0x78, 0x40, (uint8_t *)displayRam1, sizeof(displayRam1));
	else if (num == 2)
		I2C_Comm_MasterSend(I2C1, 0x78, 0x40, (uint8_t *)displayRam2, sizeof(displayRam2));
	while (i2c_comm_state != COMM_DONE)
	;
}
FINSH_FUNCTION_EXPORT(display,display)