#define main original_dma_main
#include "oled_dma_test.c"
#undef main
__attribute__((weak)) uint8_t OLED_TakePageComplete(void) {return 0;}
int main(void)
{
    tick=100;boot();
    for(unsigned n=0;n<128;n++)ScreenBuffer[3][n]=(uint8_t)n;
    assert(OLED_RefreshPage(3)==HAL_OK);complete();
    unsigned sent=txs;
    tick+=120000U;
    assert(OLED_TransferPoll(0)==HAL_BUSY && txs==sent && !disabled);
    memset(ScreenBuffer[3],0xEE,128);
    for(unsigned n=0;n<10;n++){tick+=10000;assert(OLED_TransferPoll(0)==HAL_BUSY && txs==sent);}
    assert(!OLED_TakePageComplete());
    assert(OLED_TransferPoll(1)==HAL_BUSY && txs==sent+1 && packet_size==129);
    for(unsigned n=0;n<128;n++)assert(packet[n+1]==(uint8_t)n);
    complete();assert(!OLED_TakePageComplete());
    assert(OLED_TransferPoll(0)==HAL_OK);
    assert(OLED_TakePageComplete()==1 && !OLED_TakePageComplete());
    errors_and_timeouts();
    fail_init=fail_tx=busy_tx=0;tick=100;boot();
    assert(OLED_RefreshPage(2)==HAL_OK);complete();busy_tx=1;
    assert(OLED_TransferPoll(1)==HAL_BUSY);tick+=51;
    assert(OLED_TransferPoll(1)==HAL_ERROR);
    puts("MAIN pause retains immutable page; one-shot completion; active/retry timeouts: PASS");
    return 0;
}
