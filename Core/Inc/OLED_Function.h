/**
 * ************************************************************************
 * 
 * @file OLED_Function.h
 * @author zxr
 * @brief OLED功能函数驱动头文件
 * 
 * ************************************************************************
 * @copyright Copyright (c) 2024 zxr 
 * ************************************************************************
 */
#ifndef _OLED_FUNCTION_H_
#define _OLED_FUNCTION_H_

#include "OLED_IIC_Config.h"

//字符串显示函数
void OLED_ShowStr(signed short int x, signed short int y, unsigned char ch[], unsigned char TextSize);
/* Draw clipped 6x8 ASCII into RAM; no I2C transfer. */
void OLED_DrawStr(int16_t x, int16_t y, const char *text);
//中文汉字显示函数
void OLED_ShowChinese(signed short int x, signed short int y, unsigned char* ch);
//BMP图片显示函数
void OLED_ShowBMP(signed short int x0,signed short int y0,signed short int L,signed short int H,const unsigned char BMP[]);

#endif /* _OLED_FUNCTION_H_ */

