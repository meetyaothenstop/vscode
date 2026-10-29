#ifndef TEST_STM32G4XX_HAL_H
#define TEST_STM32G4XX_HAL_H

#include <stdint.h>

typedef struct { int unused; } ADC_HandleTypeDef;
typedef struct { int unused; } HRTIM_HandleTypeDef;
typedef struct {
    uint32_t Request, Direction, PeriphInc, MemInc, PeriphDataAlignment;
    uint32_t MemDataAlignment, Mode, Priority;
} DMA_InitTypeDef;
typedef struct { void *Instance; DMA_InitTypeDef Init; void *Parent; } DMA_HandleTypeDef;
typedef struct { void *Instance; DMA_HandleTypeDef *hdmatx; uint32_t State; } I2C_HandleTypeDef;
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
#define I2C_MEMADD_SIZE_8BIT 1U
#define I2C1 ((void *)1)
#define DMA1_Channel1 ((void *)2)
#define DMA_REQUEST_I2C1_TX 17U
#define DMA_MEMORY_TO_PERIPH 1U
#define DMA_PINC_DISABLE 0U
#define DMA_MINC_ENABLE 1U
#define DMA_PDATAALIGN_BYTE 0U
#define DMA_MDATAALIGN_BYTE 0U
#define DMA_NORMAL 0U
#define DMA_PRIORITY_LOW 0U
#define HAL_I2C_STATE_READY 1U
#define HAL_I2C_STATE_BUSY_TX 2U
#define DMA1_Channel1_IRQn 11
#define I2C1_EV_IRQn 12
#define I2C1_ER_IRQn 13
#define __HAL_RCC_DMA1_CLK_ENABLE() test_oled_clock(1U)
#define __HAL_RCC_DMAMUX1_CLK_ENABLE() test_oled_clock(2U)
#define __HAL_LINKDMA(h, field, dma) do { (h)->field = &(dma); (dma).Parent = (h); } while (0)
void test_oled_clock(unsigned clock);
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *dma);
void HAL_DMA_IRQHandler(DMA_HandleTypeDef *dma);
void HAL_I2C_EV_IRQHandler(I2C_HandleTypeDef *i2c);
void HAL_I2C_ER_IRQHandler(I2C_HandleTypeDef *i2c);
uint32_t HAL_I2C_GetState(const I2C_HandleTypeDef *i2c);
void HAL_NVIC_SetPriority(int irq, uint32_t preempt, uint32_t sub);
void HAL_NVIC_EnableIRQ(int irq);
void HAL_NVIC_DisableIRQ(int irq);
HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef *i2c, uint16_t address,
    uint8_t *bytes, uint16_t size);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t delay);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *i2c, uint16_t address,
    uint16_t memory, uint16_t address_size, uint8_t *bytes, uint16_t size, uint32_t timeout);

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc);

#endif
