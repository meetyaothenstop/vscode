/**
 * @file    key.c
 * @brief   按键驱动实现（状态机消抖）
 */

#include "key.h"

/* ==================== 按键状态枚举 ==================== */
typedef enum {
    KEY_STATE_IDLE = 0,         /* 空闲：等待按下 */
    KEY_STATE_DEBOUNCE,         /* 消抖：检测到低电平，等待消抖时间 */
    KEY_STATE_PRESSED,          /* 确认按下：产生事件 */
    KEY_STATE_WAIT_RELEASE      /* 等待释放：防止重复触发 */
} KeyState_t;

/* ==================== 单个按键控制结构体 ==================== */
typedef struct {
    GPIO_TypeDef *port;         /* GPIO端口 */
    uint16_t      pin;          /* GPIO引脚 */
    KeyState_t    state;        /* 当前状态 */
    uint32_t      tick;         /* 状态切换时间戳 */
    uint8_t       event;        /* 按下事件标志（1=有事件待处理） */
} KeyCtrl_t;

/* ==================== 按键实例数组 ==================== */
static KeyCtrl_t g_keys[KEY_COUNT];

/* ==================== 查找表：编号 → 端口/引脚 ==================== */
static void Key_GetPortPin(uint8_t key_id, GPIO_TypeDef **port, uint16_t *pin)
{
    switch (key_id) {
        case KEY_ID_1: *port = KEY1_GPIO_PORT; *pin = KEY1_GPIO_PIN; break;
        case KEY_ID_2: *port = KEY2_GPIO_PORT; *pin = KEY2_GPIO_PIN; break;
        case KEY_ID_3: *port = KEY3_GPIO_PORT; *pin = KEY3_GPIO_PIN; break;
        case KEY_ID_4: *port = KEY4_GPIO_PORT; *pin = KEY4_GPIO_PIN; break;
        default:       *port = NULL;          *pin = 0;            break;
    }
}

/* ==================== 按键初始化 ==================== */
void Key_Init(void)
{
    /* ---- 初始化状态机 ---- */
    for (uint8_t i = 0; i < KEY_COUNT; i++) {
        Key_GetPortPin(i + 1, &g_keys[i].port, &g_keys[i].pin);
        g_keys[i].state = KEY_STATE_IDLE;
        g_keys[i].tick  = 0;
        g_keys[i].event = 0;
    }
}

/* ==================== 按键扫描（状态机消抖） ==================== */
void Key_Scan(void)
{
    static uint32_t last_scan_tick = 0;
    uint32_t now = HAL_GetTick();

    /* 限频：确保两次扫描至少间隔 KEY_SCAN_INTERVAL_MS */
    if (now - last_scan_tick < KEY_SCAN_INTERVAL_MS) {
        return;
    }
    last_scan_tick = now;

    /* 逐一扫描每个按键 */
    for (uint8_t i = 0; i < KEY_COUNT; i++) {
        uint8_t level = (HAL_GPIO_ReadPin(g_keys[i].port, g_keys[i].pin) == GPIO_PIN_RESET) ? 1 : 0;
        /* level=1 表示低电平（按下），level=0 表示高电平（松开） */

        switch (g_keys[i].state) {

            case KEY_STATE_IDLE:
                if (level == 1) {                   /* 检测到按下 */
                    g_keys[i].state = KEY_STATE_DEBOUNCE;
                    g_keys[i].tick  = now;
                }
                break;

            case KEY_STATE_DEBOUNCE:
                if (level == 1) {
                    if (now - g_keys[i].tick >= KEY_DEBOUNCE_MS) {
                        /* 消抖时间到，确认按下 */
                        g_keys[i].state = KEY_STATE_PRESSED;
                        g_keys[i].event = 1;        /* 置位事件标志 */
                    }
                } else {
                    /* 消抖期间电平恢复，视为抖动，回到空闲 */
                    g_keys[i].state = KEY_STATE_IDLE;
                }
                break;

            case KEY_STATE_PRESSED:
                /* 立即转入等待释放，防止在同一状态反复置位事件 */
                g_keys[i].state = KEY_STATE_WAIT_RELEASE;
                break;

            case KEY_STATE_WAIT_RELEASE:
                if (level == 0) {                   /* 检测到松开 */
                    g_keys[i].state = KEY_STATE_IDLE;
                }
                break;

            default:
                g_keys[i].state = KEY_STATE_IDLE;
                break;
        }
    }
}

/* ==================== 查询按下事件（边缘触发，读后清除） ==================== */
uint8_t Key_IsPressed(uint8_t key_id)
{
    if (key_id < 1 || key_id > KEY_COUNT) return 0;

    uint8_t idx = key_id - 1;
    if (g_keys[idx].event) {
        g_keys[idx].event = 0;      /* 清除事件标志 */
        return 1;
    }
    return 0;
}

/* ==================== 查询实时电平状态 ==================== */
uint8_t Key_IsDown(uint8_t key_id)
{
    if (key_id < 1 || key_id > KEY_COUNT) return 0;

    uint8_t idx = key_id - 1;
    /* 返回1表示正在按住（低电平） */
    return (HAL_GPIO_ReadPin(g_keys[idx].port, g_keys[idx].pin) == GPIO_PIN_RESET) ? 1 : 0;
}
