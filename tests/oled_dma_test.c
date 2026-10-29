#include "OLED_IIC_Config.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern unsigned char ScreenBuffer[8][128];
I2C_HandleTypeDef hi2c1 = {.Instance = I2C1};
static uint32_t tick;
static unsigned clocks, inits, enabled, disabled, txs, blocking, dma_irqs, ev_irqs;
static int fail_init, fail_tx, busy_tx;
static uint8_t *packet;
static uint16_t packet_size;
static DMA_HandleTypeDef *dma_handle;

/* Original blocking backend can link, then fails the DMA contract assertion. */
__attribute__((weak)) HAL_StatusTypeDef OLED_TransferPoll(uint8_t allow)
{ (void)allow; return HAL_OK; }
__attribute__((weak)) void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *h) { (void)h; }
__attribute__((weak)) void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *h) { (void)h; }
__attribute__((weak)) void DMA1_Channel1_IRQHandler(void) { }
__attribute__((weak)) void I2C1_EV_IRQHandler(void) { }
__attribute__((weak)) void I2C1_ER_IRQHandler(void) { }

uint32_t HAL_GetTick(void) { return tick; }
void HAL_Delay(uint32_t delay) { (void)delay; }
void test_oled_clock(unsigned clock) { clocks |= clock; }
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *dma)
{
    ++inits; dma_handle = dma;
    assert(clocks == 3U && dma->Instance == DMA1_Channel1);
    assert(dma->Init.Request == DMA_REQUEST_I2C1_TX);
    assert(dma->Init.Direction == DMA_MEMORY_TO_PERIPH);
    assert(dma->Init.PeriphInc == DMA_PINC_DISABLE && dma->Init.MemInc == DMA_MINC_ENABLE);
    assert(dma->Init.PeriphDataAlignment == DMA_PDATAALIGN_BYTE);
    assert(dma->Init.MemDataAlignment == DMA_MDATAALIGN_BYTE);
    assert(dma->Init.Mode == DMA_NORMAL && dma->Init.Priority == DMA_PRIORITY_LOW);
    return fail_init ? HAL_ERROR : HAL_OK;
}
void HAL_NVIC_SetPriority(int irq, uint32_t preempt, uint32_t sub)
{ assert(irq >= 11 && irq <= 13 && preempt == 3U && sub == 0U); }
void HAL_NVIC_EnableIRQ(int irq) { assert(irq >= 11 && irq <= 13); enabled |= 1U << (irq - 11); }
void HAL_NVIC_DisableIRQ(int irq) { assert(irq >= 11 && irq <= 13); disabled |= 1U << (irq - 11); }
void HAL_DMA_IRQHandler(DMA_HandleTypeDef *h) { assert(h == dma_handle); ++dma_irqs; }
void HAL_I2C_EV_IRQHandler(I2C_HandleTypeDef *h)
{ assert(h == &hi2c1); ++ev_irqs; h->State = HAL_I2C_STATE_READY; HAL_I2C_MasterTxCpltCallback(h); }
void HAL_I2C_ER_IRQHandler(I2C_HandleTypeDef *h)
{ assert(h == &hi2c1); h->State = HAL_I2C_STATE_READY; HAL_I2C_ErrorCallback(h); }
uint32_t HAL_I2C_GetState(const I2C_HandleTypeDef *h) { return h->State; }
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t address,
    uint16_t memory, uint16_t address_size, uint8_t *bytes, uint16_t size, uint32_t timeout)
{
    (void)h; (void)address; (void)memory; (void)address_size; (void)bytes; (void)size; (void)timeout;
    ++blocking; return HAL_OK; /* Allowed only during boot before Power_Init. */
}
HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef *h, uint16_t address,
    uint8_t *bytes, uint16_t size)
{
    assert(h == &hi2c1 && address == 0x78 && (size == 4U || size == 129U));
    assert(hi2c1.hdmatx == dma_handle && dma_handle->Parent == &hi2c1);
    assert(h->State == HAL_I2C_STATE_READY);
    ++txs; packet = bytes; packet_size = size;
    if (busy_tx) return HAL_BUSY; /* Local HAL submits nothing on BUSY. */
    if (fail_tx) return HAL_ERROR;
    h->State = HAL_I2C_STATE_BUSY_TX;
    return HAL_OK;
}
static void boot(void)
{
    clocks = inits = enabled = disabled = txs = blocking = dma_irqs = ev_irqs = 0;
    hi2c1.State = HAL_I2C_STATE_READY;
    OLED_Init();
    assert(inits == 1U);
    if (!fail_init) assert(enabled == 7U);
    blocking = 0;
}
static void complete(void) { DMA1_Channel1_IRQHandler(); I2C1_EV_IRQHandler(); }
static void page_contract(void)
{
    tick = 100;
    boot();
    for (unsigned n = 0; n < 128; ++n) ScreenBuffer[3][n] = (uint8_t)n;
    assert(OLED_TransferPoll(1) == HAL_OK);
    assert(OLED_RefreshPage(8) == HAL_ERROR && txs == 0U);
    assert(OLED_RefreshPage(3) == HAL_OK && txs == 1U && packet_size == 4U);
    const uint8_t cmd[] = {0, 0xB3, 0, 0x10};
    assert(memcmp(packet, cmd, 4) == 0);
    memset(ScreenBuffer[3], 0xEE, 128); /* Own immutable snapshot, not screen RAM. */
    assert(OLED_RefreshPage(4) == HAL_BUSY && txs == 1U);
    I2C_HandleTypeDef other = {.State = HAL_I2C_STATE_READY};
    HAL_I2C_MasterTxCpltCallback(&other);
    HAL_I2C_ErrorCallback(&other);
    assert(OLED_TransferPoll(1) == HAL_BUSY && txs == 1U);
    complete();
    HAL_I2C_MasterTxCpltCallback(&hi2c1); /* Duplicate while command already done. */
    assert(OLED_TransferPoll(1) == HAL_BUSY && txs == 2U && packet_size == 129U);
    assert(packet[0] == 0x40);
    for (unsigned n = 0; n < 128; ++n) assert(packet[n + 1] == (uint8_t)n);
    HAL_I2C_MasterTxCpltCallback(&hi2c1); /* No STOP: handle still BUSY. */
    assert(OLED_TransferPoll(1) == HAL_BUSY && txs == 2U);
    complete();
    assert(OLED_TransferPoll(1) == HAL_OK);
    assert(dma_irqs == 2U && ev_irqs == 2U && blocking == 0U);
    assert(OLED_RefreshPage(4) == HAL_OK && txs == 3U);
    complete();
    assert(OLED_TransferPoll(0) == HAL_OK && txs == 3U); /* Startup drops unsent data. */
    assert(OLED_RefreshPage(5) == HAL_OK);
    complete(); assert(OLED_TransferPoll(1) == HAL_BUSY);
    complete(); assert(OLED_TransferPoll(0) == HAL_OK); /* Already active data drains. */
}
static void errors_and_timeouts(void)
{
    for (unsigned data_phase = 0; data_phase < 2; ++data_phase) {
        boot();
        assert(OLED_RefreshPage(0) == HAL_OK);
        if (data_phase) { complete(); assert(OLED_TransferPoll(1) == HAL_BUSY); }
        I2C1_ER_IRQHandler();
        assert(OLED_TransferPoll(1) == HAL_ERROR && disabled == 7U);
        unsigned before = txs;
        complete(); /* Late callback must not restart anything. */
        assert(OLED_RefreshPage(0) == HAL_ERROR && txs == before);
    }
    for (unsigned data_phase = 0; data_phase < 2; ++data_phase) {
        tick = UINT32_MAX - 20U; boot();
        assert(OLED_RefreshPage(0) == HAL_OK);
        if (data_phase) { complete(); assert(OLED_TransferPoll(1) == HAL_BUSY); }
        tick += 49;
        assert(OLED_TransferPoll(1) == HAL_BUSY);
        tick += 2;
        assert(OLED_TransferPoll(1) == HAL_ERROR && disabled == 7U);
        complete();
        assert(OLED_TransferPoll(1) == HAL_ERROR);
    }
    for (unsigned data_phase = 0; data_phase < 2; ++data_phase) {
        fail_tx = 0; boot();
        if (data_phase) { assert(OLED_RefreshPage(0) == HAL_OK); complete(); }
        fail_tx = 1;
        assert((data_phase ? OLED_TransferPoll(1) : OLED_RefreshPage(0)) == HAL_ERROR);
        assert(OLED_TransferPoll(1) == HAL_ERROR);
    }
    fail_tx = 0; fail_init = 1; boot();
    assert(OLED_RefreshPage(0) == HAL_ERROR && txs == 0U);
}
static void transient_busy(void)
{
    tick = 100; boot();
    busy_tx = 1;
    assert(OLED_RefreshPage(2) == HAL_BUSY && disabled == 0U);
    assert(OLED_TransferPoll(1) == HAL_OK);
    assert(OLED_RefreshPage(2) == HAL_BUSY && disabled == 0U);
    busy_tx = 0;
    assert(OLED_RefreshPage(2) == HAL_OK);
    complete();
    busy_tx = 1;
    assert(OLED_TransferPoll(1) == HAL_BUSY && disabled == 0U);
    uint8_t saved[129]; memcpy(saved, packet, sizeof(saved));
    memset(ScreenBuffer[2], 0xF0, 128);
    tick += 20;
    assert(OLED_TransferPoll(1) == HAL_BUSY && disabled == 0U);
    assert(memcmp(saved, packet, sizeof(saved)) == 0);
    busy_tx = 0;
    assert(OLED_TransferPoll(1) == HAL_BUSY && packet_size == 129U);
    assert(memcmp(saved, packet, sizeof(saved)) == 0);
    complete(); assert(OLED_TransferPoll(1) == HAL_OK);

    assert(OLED_RefreshPage(2) == HAL_OK); complete();
    busy_tx = 1;
    assert(OLED_TransferPoll(1) == HAL_BUSY);
    unsigned before = txs;
    assert(OLED_TransferPoll(0) == HAL_OK && txs == before); /* Startup cancels unsubmitted data. */
    busy_tx = 0;
    assert(OLED_RefreshPage(0) == HAL_OK); complete();
    busy_tx = 1; assert(OLED_TransferPoll(1) == HAL_BUSY);
    tick += 51;
    assert(OLED_TransferPoll(1) == HAL_ERROR && disabled == 7U); /* Bounded data retry. */
    busy_tx = 0;
}
int main(void)
{
    page_contract(); transient_busy(); errors_and_timeouts();
    puts("OLED DMA packets/snapshot, deferred completion, muted drain, errors/timeouts/rollover: PASS");
    return 0;
}
