#ifndef TEST_STM32G4XX_HAL_H
#define TEST_STM32G4XX_HAL_H

#include <stdint.h>

typedef struct { int unused; } ADC_HandleTypeDef;
typedef struct { int unused; } HRTIM_HandleTypeDef;
typedef struct { int unused; } I2C_HandleTypeDef;
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
#define I2C_MEMADD_SIZE_8BIT 1U
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t delay);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *i2c, uint16_t address,
    uint16_t memory, uint16_t address_size, uint8_t *bytes, uint16_t size, uint32_t timeout);

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc);

#endif
