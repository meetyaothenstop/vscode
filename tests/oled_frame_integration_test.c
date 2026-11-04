/* Actual PowerUI + DMA driver; only hardware callbacks and text drawing mocked. */
#define main original_dma_main
#include "oled_dma_test.c"
#undef main
#include "power_ui.h"
#include "power_control.h"
#include "power_event_diag.h"
#include "key.h"
volatile PowerStatus power_status;
static AdcMeasurements measured;
static unsigned completed_frames,pages_seen;
static uint8_t current_page,panel[8][128];
void Key_Scan(void) {}
uint8_t Key_IsPressed(uint8_t id) {(void)id;return 0;}
int Power_IsStartPending(void) {return 0;}
int Power_SamplesAreValid(void) {return 1;}
int Power_RequestStart(unsigned mode,float target){(void)mode;(void)target;return 0;}
void Power_Stop(void){power_status.state=POWER_STOPPED;}
void PowerEvent_UIMark(uint32_t stage,uint32_t page,uint32_t status)
{(void)page;(void)status;if(stage==POWER_EVENT_UI_FRAME_DONE)++completed_frames;}
void OLED_DrawStr(int16_t x,int16_t y,const char *text)
{(void)x;memcpy(ScreenBuffer[y/8],text,strlen(text));}
static void poll_and_complete(PowerState next)
{
    power_status.state=next;
    unsigned before=txs;
    PowerUI_Service(&measured);
    if(next==POWER_BOOTSTRAP)assert(txs==before);
    if(hi2c1.State==HAL_I2C_STATE_BUSY_TX) {
        if(packet_size==4)current_page=packet[1]-0xB0;
        else {
            assert(packet_size==129 && current_page<8);
            memcpy(panel[current_page],packet+1,128);pages_seen|=1U<<current_page;
        }
        complete();
    }
    ++tick;
}
int main(void)
{
    boot();PowerUI_Init();measured.v_high=12;measured.v_low=5.9;
    for(unsigned n=0;n<800;n++)poll_and_complete(n%2?POWER_RUNNING:POWER_BOOTSTRAP);
    assert(pages_seen==255 && completed_frames>0 && !disabled);
    /* The paused command must survive a long mute without starting data. */
    for(unsigned n=0;n<1000;n++)poll_and_complete(POWER_BOOTSTRAP);
    for(unsigned n=0;n<80;n++)poll_and_complete(POWER_RUNNING);
    assert(!disabled);
    power_status.fault=16;
    for(unsigned n=0;n<80;n++)poll_and_complete(POWER_FAULT);
    assert(strstr((const char*)panel[7],"FAULT:16"));
    power_status.fault=0;
    for(unsigned n=0;n<80;n++)poll_and_complete(POWER_STOPPED);
    assert(strstr((const char*)panel[7],"STOPPED"));
    puts("Real UI/DMA: all8 pages across startup churn, mute, complete-frame and fault/stop: PASS");
    return 0;
}
