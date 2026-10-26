#include "OLED_Function.h"
#include "power_menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern unsigned char ScreenBuffer[8][128];
I2C_HandleTypeDef hi2c1;
static unsigned writes, data_writes, last_page;
static int parameter_for = -1, memory_mode = -1, fail_command;

uint32_t HAL_GetTick(void) { return 0; }
void HAL_Delay(uint32_t delay) { (void)delay; }
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *i2c, uint16_t address,
    uint16_t memory, uint16_t address_size, uint8_t *bytes, uint16_t size, uint32_t timeout)
{
    (void)i2c; (void)address_size; (void)timeout;
    assert(address == 0x78); ++writes;
    if (memory == 0) {
        if (fail_command) return HAL_TIMEOUT;
        for (unsigned i = 0; i < size; ++i) {
            if (parameter_for >= 0) {
                if (parameter_for == 0x20) memory_mode = bytes[i] & 3;
                parameter_for = -1;
            } else {
                switch (bytes[i]) {
                case 0x20: case 0x81: case 0xA8: case 0xD3: case 0xD5:
                case 0xD9: case 0xDA: case 0xDB: case 0x8D:
                    parameter_for = bytes[i]; break;
                default:
                    if ((bytes[i] & 0xF8) == 0xB0) last_page = bytes[i] & 7;
                    break;
                }
            }
        }
    } else { assert(memory == 0x40); ++data_writes; }
    return HAL_OK;
}
static void preview(const char *name, const PowerMenu *menu, const MenuTelemetry *data)
{
    MenuFrame frame;
    Menu_Render(menu, data, &frame);
    OLED_ClearRAM();
    for (unsigned row = 0; row < 8; ++row) OLED_DrawStr(0, (int16_t)(row * 8), frame.rows[row]);
    char path[120];
    (void)snprintf(path, sizeof(path), "tests/%s.pbm", name);
    FILE *file = fopen(path, "wb");
    assert(file);
    fputs("P1\n128 64\n", file);
    for (unsigned y = 0; y < 64; ++y) {
        for (unsigned x = 0; x < 128; ++x)
            fprintf(file, "%u ", (ScreenBuffer[y / 8][x] >> (y % 8)) & 1);
        fputc('\n', file);
    }
    fclose(file);
}
int main(void)
{
    OLED_Init();
    assert(memory_mode == 2); /* Page transfers require actual page addressing. */
    writes = data_writes = 0;
    OLED_ClearRAM();
    OLED_DrawStr(0, 0, "A");
    assert(writes == 0);
    assert(ScreenBuffer[0][0] == 0x00 && ScreenBuffer[0][1] == 0x7C && ScreenBuffer[0][2] == 0x12);
    OLED_ClearRAM();
    OLED_DrawStr(0, 0, "~"); /* The existing font ends before '~'; use '?'. */
    assert(ScreenBuffer[0][1] == 0x02 && ScreenBuffer[0][2] == 0x01);
    OLED_ClearRAM();
    OLED_DrawStr(120, 56, "AB");
    assert(ScreenBuffer[7][121] == 0x7C);
    for (unsigned page = 0; page < 7; ++page)
        for (unsigned x = 0; x < 128; ++x) assert(ScreenBuffer[page][x] == 0);
    assert(OLED_RefreshPage(3) == HAL_OK);
    assert(last_page == 3 && writes == 2 && data_writes == 1);
    unsigned before = writes;
    assert(OLED_RefreshPage(8) == HAL_ERROR && writes == before);
    fail_command = 1;
    assert(OLED_RefreshPage(0) == HAL_TIMEOUT && data_writes == 1);
    fail_command = 0;
    PowerMenu menu;
    Menu_Init(&menu);
    MenuTelemetry data = {.valid = 1, .v_left = 12.25f, .v_right = 35.75f, .il = -1.25f};
    preview("oled_home", &menu, &data);
    (void)Menu_HandleKey(&menu, MENU_KEY_OK, &data);
    preview("oled_forward", &menu, &data);
    (void)Menu_HandleKey(&menu, MENU_KEY_OK, &data);
    (void)Menu_HandleKey(&menu, MENU_KEY_UP, &data);
    preview("oled_edit", &menu, &data);
    (void)Menu_HandleKey(&menu, MENU_KEY_BACK, &data);
    (void)Menu_HandleKey(&menu, MENU_KEY_BACK, &data);
    (void)Menu_HandleKey(&menu, MENU_KEY_DOWN, &data);
    (void)Menu_HandleKey(&menu, MENU_KEY_OK, &data);
    preview("oled_reverse", &menu, &data);
    puts("OLED RAM drawing, clipping, page commands and font previews: PASS");
    return 0;
}
