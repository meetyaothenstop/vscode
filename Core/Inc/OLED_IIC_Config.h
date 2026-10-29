#ifndef OLED_IIC_CONFIG_H
#define OLED_IIC_CONFIG_H

#include "main.h"

#define OLED_ADDRESS    0x78

// OLED 命令宏（不变）
#define LEFT    0x27
#define RIGHT   0x26
#define UP      0x29
#define DOWM    0x2A
#define ON      0xA7
#define OFF     0xA6

// 屏幕参数宏（不变）
#define SCREEN_PAGE_NUM     (8)
#define SCREEN_PAGEDATA_NUM (128)
#define SCREEN_COLUMN       (128)
#define SCREEN_ROW          (64)

// 函数声明（不变）
void WriteCmd(unsigned char cmd);
void WriteDat(unsigned char dat);
void OLED_ON(void);
void OLED_OFF(void);
void OLED_CLS(void);
void OLED_Init(void);
/* Legacy full-refresh helpers are for the blocking backend. FAST live PowerUI
 * uses DrawStr into RAM followed by TransferPoll/RefreshPage only. */
void OLED_RefreshRAM(void);
HAL_StatusTypeDef OLED_RefreshPage(uint8_t page);
/* Live backend: RefreshPage OK means accepted, BUSY means no submission.
 * Poll after keys: allow=0 drains a packet but never starts the data packet.
 * ERROR latches display offline until reset; it does not stop power control. */
HAL_StatusTypeDef OLED_TransferPoll(uint8_t allow);
void OLED_ClearRAM(void);
void OLED_SetPixel(signed short int x, signed short int y, unsigned char set_pixel);
void OLED_DisplayMode(unsigned char mode);
void OLED_IntensityControl(unsigned char intensity);
void OLED_Shift(unsigned char shift_num);
void OLED_HorizontalShift(unsigned char start_page, unsigned char end_page, unsigned char direction);

#endif
