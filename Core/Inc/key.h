/**
 * @file    key.h
 * @brief   按键驱动头文件
 * @note    PB4=KEY1, PB5=KEY2, PB6=KEY3, PC13=KEY4
 *          引脚配置：上拉输入(GPIO_MODE_INPUT + GPIO_PULLUP)
 *          硬件连接：按键一端接GPIO引脚，另一端接GND
 *          按下为低电平(GPIO_PIN_RESET)，松开为高电平(GPIO_PIN_SET)
 */

#ifndef __KEY_H__
#define __KEY_H__

#include "main.h"

/* ==================== 按键引脚宏定义 ==================== */
#define KEY1_GPIO_PORT      key1_GPIO_Port
#define KEY1_GPIO_PIN       key1_Pin

#define KEY2_GPIO_PORT      key2_GPIO_Port
#define KEY2_GPIO_PIN       key2_Pin

#define KEY3_GPIO_PORT      key3_GPIO_Port
#define KEY3_GPIO_PIN       key3_Pin

#define KEY4_GPIO_PORT      key4_GPIO_Port
#define KEY4_GPIO_PIN       key4_Pin

/* ==================== 按键编号 ==================== */
#define KEY_ID_1            1
#define KEY_ID_2            2
#define KEY_ID_3            3
#define KEY_ID_4            4
#define KEY_COUNT           4       /* 按键总数 */

/* ==================== 消抖参数 ==================== */
#define KEY_SCAN_INTERVAL_MS    10   /* 扫描间隔(ms) */
#define KEY_DEBOUNCE_MS         25   /* 消抖时间(ms) */

/* ==================== 函数声明 ==================== */

/**
 * @brief  按键初始化
 * @note   请先调用 CubeMX 生成的 MX_GPIO_Init()，本函数只初始化内部状态机。
 */
void Key_Init(void);

/**
 * @brief  按键扫描（需在主循环中周期性调用）
 * @note   内部以 KEY_SCAN_INTERVAL_MS 为周期自动限频，
 *         用户无需关心调用频率，放 while(1) 中即可
 */
void Key_Scan(void);

/**
 * @brief  查询指定按键是否有"按下事件"（边缘触发）
 * @param  key_id  按键编号：KEY_ID_1 ~ KEY_ID_4
 * @retval 1: 检测到按下事件（调用后自动清除）
 *         0: 无按下事件
 */
uint8_t Key_IsPressed(uint8_t key_id);

/**
 * @brief  获取指定按键当前实时电平状态
 * @param  key_id  按键编号：KEY_ID_1 ~ KEY_ID_4
 * @retval 1: 按键正在被按住（低电平）
 *         0: 按键已松开（高电平）
 */
uint8_t Key_IsDown(uint8_t key_id);

#endif /* __KEY_H__ */
