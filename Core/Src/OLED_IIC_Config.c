/**
 * ************************************************************************
 * 
 * @file OLED_IIC_Config.c
 * @brief IIC初始化配置和OLED屏幕的底层驱动（适配STM32G474CBT6）
 * 
 * ************************************************************************
 */

#include "OLED_IIC_Config.h"   // 对应的头文件
#include "power_build_config.h"
#include <string.h>

/* 屏幕缓冲区：8页 × 128列 */
unsigned char ScreenBuffer[SCREEN_PAGE_NUM][SCREEN_COLUMN];

#if POWER_OLED_DMA
/* All packets reside in ordinary SRAM, separate from the mutable drawing RAM.
 * Callbacks only publish progress; the foreground starts the next packet. */
typedef enum { OLED_IDLE, OLED_COMMAND, OLED_COMMAND_DONE,
               OLED_DATA_READY, OLED_DATA, OLED_DATA_DONE, OLED_ERROR } OledPhase;
static volatile OledPhase oled_phase = OLED_ERROR;
static DMA_HandleTypeDef oled_dma;
static uint8_t oled_commands[4], oled_data[SCREEN_COLUMN + 1];
static uint8_t oled_offline;
static uint32_t oled_packet_tick;
extern I2C_HandleTypeDef hi2c1;

static HAL_StatusTypeDef oled_fail(void)
{
    oled_phase = OLED_ERROR;
    if (!oled_offline) {
        oled_offline = 1;
        /* Do not abort synchronously or reuse a possibly active DMA buffer. */
        HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
        HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
        HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
    }
    return HAL_ERROR;
}

static void oled_dma_init(void)
{
    oled_phase = OLED_ERROR;
    oled_offline = 0;
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    oled_dma.Instance = DMA1_Channel1;
    oled_dma.Init.Request = DMA_REQUEST_I2C1_TX;
    oled_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    oled_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    oled_dma.Init.MemInc = DMA_MINC_ENABLE;
    oled_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    oled_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    oled_dma.Init.Mode = DMA_NORMAL;
    oled_dma.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&oled_dma) != HAL_OK) { (void)oled_fail(); return; }
    __HAL_LINKDMA(&hi2c1, hdmatx, oled_dma);
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 3, 0);
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 3, 0);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
    oled_phase = OLED_IDLE;
}

void DMA1_Channel1_IRQHandler(void) { HAL_DMA_IRQHandler(&oled_dma); }
void I2C1_EV_IRQHandler(void) { HAL_I2C_EV_IRQHandler(&hi2c1); }
void I2C1_ER_IRQHandler(void) { HAL_I2C_ER_IRQHandler(&hi2c1); }

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *i2c)
{
    if (i2c != &hi2c1 || HAL_I2C_GetState(i2c) != HAL_I2C_STATE_READY) return;
    if (oled_phase == OLED_COMMAND) oled_phase = OLED_COMMAND_DONE;
    else if (oled_phase == OLED_DATA) oled_phase = OLED_DATA_DONE;
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *i2c)
{
    if (i2c == &hi2c1 && oled_phase != OLED_IDLE) oled_phase = OLED_ERROR;
}

HAL_StatusTypeDef OLED_TransferPoll(uint8_t allow)
{
    OledPhase phase = oled_phase;
    if (phase == OLED_ERROR) return oled_fail();
    if (phase == OLED_COMMAND || phase == OLED_DATA) {
        if (HAL_GetTick() - oled_packet_tick >= 50U) return oled_fail();
        return HAL_BUSY;
    }
    if (phase == OLED_COMMAND_DONE || phase == OLED_DATA_READY) {
        if (!allow) { oled_phase = OLED_IDLE; return HAL_OK; }
        if (phase == OLED_COMMAND_DONE) oled_packet_tick = HAL_GetTick();
        else if (HAL_GetTick() - oled_packet_tick >= 50U) return oled_fail();
        oled_phase = OLED_DATA;
        /* Master API has no memory-address polling stage in the local HAL. */
        HAL_StatusTypeDef result = HAL_I2C_Master_Transmit_DMA(&hi2c1, OLED_ADDRESS,
                oled_data, sizeof(oled_data));
        if (result == HAL_BUSY) {
            /* No DMA submitted: preserve the snapshot and deadline for retry. */
            oled_phase = OLED_DATA_READY;
            return HAL_BUSY;
        }
        if (result != HAL_OK) return oled_fail();
        return HAL_BUSY;
    }
    if (phase == OLED_DATA_DONE) oled_phase = OLED_IDLE;
    return HAL_OK;
}

HAL_StatusTypeDef OLED_RefreshPage(uint8_t page)
{
    if (page >= SCREEN_PAGE_NUM) return HAL_ERROR;
    if (oled_phase == OLED_ERROR) return oled_fail();
    if (oled_phase != OLED_IDLE) return HAL_BUSY;
    oled_commands[0] = 0x00;
    oled_commands[1] = (uint8_t)(0xB0U + page);
    oled_commands[2] = 0x00;
    oled_commands[3] = 0x10;
    oled_data[0] = 0x40;
    memcpy(&oled_data[1], ScreenBuffer[page], SCREEN_COLUMN);
    oled_packet_tick = HAL_GetTick();
    oled_phase = OLED_COMMAND;
    HAL_StatusTypeDef result = HAL_I2C_Master_Transmit_DMA(&hi2c1, OLED_ADDRESS,
            oled_commands, sizeof(oled_commands));
    if (result == HAL_BUSY) {
        /* Not accepted: UI retains its page and may retry after handling keys. */
        oled_phase = OLED_IDLE;
        return HAL_BUSY;
    }
    if (result != HAL_OK) return oled_fail();
    return HAL_OK;
}
#else
HAL_StatusTypeDef OLED_TransferPoll(uint8_t allow) { (void)allow; return HAL_OK; }

HAL_StatusTypeDef OLED_RefreshPage(uint8_t page)
{
    extern I2C_HandleTypeDef hi2c1;
    if (page >= SCREEN_PAGE_NUM) return HAL_ERROR;
    uint8_t commands[3] = {(uint8_t)(0xB0U + page), 0x00, 0x10};
    HAL_StatusTypeDef result = HAL_I2C_Mem_Write(&hi2c1, OLED_ADDRESS, 0x00,
        I2C_MEMADD_SIZE_8BIT, commands, sizeof(commands), 10);
    if (result != HAL_OK) return result;
    return HAL_I2C_Mem_Write(&hi2c1, OLED_ADDRESS, 0x40,
        I2C_MEMADD_SIZE_8BIT, ScreenBuffer[page], SCREEN_COLUMN, 20);
}
#endif

/**
 * @brief  通过I2C向OLED寄存器写一个字节
 * @param  addr：寄存器地址（0x00=命令，0x40=数据）
 * @param  data：要写入的数据
 */
void I2C_WriteByte(uint8_t addr, uint8_t data)
{
    /* 引用 CubeMX 生成的 I2C1 句柄（在 main.c 或 i2c.c 中定义） */
    extern I2C_HandleTypeDef hi2c1;
    
    /* 发送：OLED地址(0x78) + 寄存器地址 + 数据 */
    /* HAL expects the 7-bit OLED address shifted left by one (0x3C << 1). */
    (void)HAL_I2C_Mem_Write(&hi2c1, OLED_ADDRESS, addr,
                            I2C_MEMADD_SIZE_8BIT, &data, 1, 10);
}

/**
 * @brief 写命令函数
 * @param cmd 写入的命令
 */
void WriteCmd(unsigned char cmd)
{
    I2C_WriteByte(0x00, cmd);   // 0x00 = 命令寄存器
}

/**
 * @brief 写数据函数
 * @param dat 写入的数据
 */
void WriteDat(unsigned char dat)
{
    I2C_WriteByte(0x40, dat);   // 0x40 = 数据寄存器
}

/**
 * @brief 开启OLED显示
 */
void OLED_ON(void)
{
    WriteCmd(0x8D);   // 设置电荷泵
    WriteCmd(0x14);   // 开启电荷泵
    WriteCmd(0xAF);   // OLED唤醒
}

/**
 * @brief 休眠OLED
 */
void OLED_OFF(void)
{
    WriteCmd(0x8D);   // 设置电荷泵
    WriteCmd(0x10);   // 关闭电荷泵
    WriteCmd(0xAE);   // OLED休眠
}

/**
 * @brief OLED清屏（直接操作硬件，不使用缓冲区）
 */
void OLED_CLS(void)
{
    for (unsigned char m = 0; m < 8; m++)
    {
        WriteCmd(0xB0 + m);   // 设置页地址（0~7）
        WriteCmd(0x00);       // 设置列低地址
        WriteCmd(0x10);       // 设置列高地址
        
        for (unsigned char n = 0; n < 128; n++)
        {
            WriteDat(0x00);   // 写入0x00，清空该列
        }
    }
}

/**
 * @brief OLED初始化函数
 * @note 适用于 SSD1306 驱动 IC 的 OLED 屏幕
 */
void OLED_Init(void)
{
    HAL_Delay(100);           // 等待OLED上电稳定
    
    WriteCmd(0xAE);           // 显示关闭
    WriteCmd(0x20);           // 设置内存寻址模式
    WriteCmd(0x02);           // SSD1306 20h + 02h: page addressing mode
    WriteCmd(0xB0);           // 设置页起始地址
    WriteCmd(0xC8);           // COM输出扫描方向（从下到上）
    WriteCmd(0x00);           // 设置列低地址
    WriteCmd(0x10);           // 设置列高地址
    WriteCmd(0x40);           // 设置起始行地址
    WriteCmd(0x81);           // 设置对比度
    WriteCmd(0xFF);           // 对比度最大值（最亮）
    WriteCmd(0xA1);           // 段重新映射（左右镜像）
    WriteCmd(0xA6);           // 正常显示（非反色）
    WriteCmd(0xA8);           // 设置复用比例
    WriteCmd(0x3F);           // 1/64 占空比
    WriteCmd(0xA4);           // 输出跟随RAM内容
    WriteCmd(0xD3);           // 设置显示偏移
    WriteCmd(0x00);           // 不偏移
    WriteCmd(0xD5);           // 设置时钟分频/振荡频率
    WriteCmd(0xF0);           // 分频比
    WriteCmd(0xD9);           // 设置预充电周期
    WriteCmd(0x22);           //
    WriteCmd(0xDA);           // 设置COM引脚硬件配置
    WriteCmd(0x12);           //
    WriteCmd(0xDB);           // 设置VCOMH电压
    WriteCmd(0x20);           // 0.77xVcc
    WriteCmd(0x8D);           // 使能DC-DC转换器
    WriteCmd(0x14);           //
    WriteCmd(0xAF);           // 打开OLED显示
    
    OLED_CLS();               // 初始化完成后清屏
#if POWER_OLED_DMA
    oled_dma_init();          // Boot only: called before Power_Init and PWM start.
#endif
}

/**
 * @brief 将缓冲区(ScreenBuffer)的内容刷新到OLED屏幕
 * @note 调用此函数后，屏幕才会显示你画好的内容
 */
void OLED_RefreshRAM(void)
{
    for (uint8_t page = 0; page < SCREEN_PAGE_NUM; ++page)
        if (OLED_RefreshPage(page) != HAL_OK) break;
}

/**
 * @brief 清除缓冲区（屏幕内容不会立即变化，需调用 OLED_RefreshRAM）
 */
void OLED_ClearRAM(void)
{
    for (unsigned short m = 0; m < SCREEN_ROW / 8; m++)
    {
        for (unsigned short n = 0; n < SCREEN_COLUMN; n++)
        {
            ScreenBuffer[m][n] = 0x00;   // 全部清零
        }
    }
}

/**
 * @brief 设置指定坐标的像素点
 * 
 * @param x          横坐标（0~127）
 * @param y          纵坐标（0~63）
 * @param set_pixel  1=点亮，0=熄灭
 */
void OLED_SetPixel(signed short int x, signed short int y, unsigned char set_pixel)
{
    /* 检查坐标是否在有效范围内 */
    if (x >= 0 && x < SCREEN_COLUMN && y >= 0 && y < SCREEN_ROW)
    {
        if (set_pixel)
        {
            /* 点亮：对应位置1 */
            ScreenBuffer[y / 8][x] |= (0x01 << (y % 8));
        }
        else
        {
            /* 熄灭：对应位清0 */
            ScreenBuffer[y / 8][x] &= ~(0x01 << (y % 8));
        }
    }
}

/**
 * @brief 设置显示模式（正常/反色/全亮）
 * 
 * @param mode  0xA6 = 正常模式
 *              0xA7 = 反色模式（像素点取反）
 */
void OLED_DisplayMode(unsigned char mode)
{
    WriteCmd(mode);
}

/**
 * @brief 设置屏幕亮度
 * 
 * @param intensity  亮度值（0~255），值越大越亮，默认0x7F
 */
void OLED_IntensityControl(unsigned char intensity)
{
    WriteCmd(0x81);           // 对比度设置命令
    WriteCmd(intensity);      // 写入亮度值
}

/**
 * @brief 全屏内容垂直偏移
 * 
 * @param shift_num  偏移距离（0~63）
 */
void OLED_Shift(unsigned char shift_num)
{
    for (unsigned char i = 0; i < shift_num; i++)
    {
        WriteCmd(0xD3);       // 设置显示偏移
        WriteCmd(i);          // 偏移量
        HAL_Delay(10);        // 每步延时10ms，产生滚动效果
    }
}

/**
 * @brief 屏幕内容水平滚动
 * 
 * @param start_page  起始页（0~7）
 * @param end_page    结束页（0~7）
 * @param direction   滚动方向：LEFT=0x27，RIGHT=0x26
 */
void OLED_HorizontalShift(unsigned char start_page, unsigned char end_page, unsigned char direction)
{
    WriteCmd(0x2E);           // 关闭滚动（先关再配）
    
    WriteCmd(direction);      // 设置滚动方向
    WriteCmd(0x00);           // 虚拟字节
    WriteCmd(start_page);     // 起始页
    WriteCmd(0x05);           // 滚动速度（帧频）
    WriteCmd(end_page);       // 结束页
    WriteCmd(0x00);           // 虚拟字节
    WriteCmd(0xFF);           // 虚拟字节
    
    WriteCmd(0x2F);           // 开启滚动
}
