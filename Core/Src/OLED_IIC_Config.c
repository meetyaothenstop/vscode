/**
 * ************************************************************************
 * 
 * @file OLED_IIC_Config.c
 * @brief IIC初始化配置和OLED屏幕的底层驱动（适配STM32G474CBT6）
 * 
 * ************************************************************************
 */

#include "OLED_IIC_Config.h"   // 对应的头文件

/* 屏幕缓冲区：8页 × 128列 */
unsigned char ScreenBuffer[SCREEN_PAGE_NUM][SCREEN_COLUMN];

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
