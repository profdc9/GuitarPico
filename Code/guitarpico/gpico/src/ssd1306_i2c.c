/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Modified by Daniel L. Marks, all changes also under BSD-3-Clause */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

/* Example code to talk to an SSD1306-based OLED display

   The SSD1306 is an OLED/PLED driver chip, capable of driving displays up to
   128x64 pixels.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefore I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (on Pico this is GP4 (pin 6)) -> SDA on display
   board
   GPIO PICO_DEFAULT_I2C_SCL_PIN (on Pico this is GP5 (pin 7)) -> SCL on
   display board
   3.3v (pin 36) -> VCC on display board
   GND (pin 38)  -> GND on display board
*/

#include "ssd1306_i2c.h"
#include "ssd1306_font.h"

static uint8_t ssd1306_modified = 0;
static uint8_t ssd1306_cursor_x = 0;
static uint8_t ssd1306_cursor_y = 0;
static uint8_t ssd1306_cursor_on = 0;
static uint8_t ssd1306_cursor_painted = 0;
uint8_t ssd1306_buf[SSD1306_BUF_LEN];

struct render_area ssd1306_frame_area = {
    start_col: 0,
    end_col : SSD1306_WIDTH - 1,
    start_page : 0,
    end_page : SSD1306_NUM_PAGES - 1
};


void calc_render_area_buflen(struct render_area *area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void SSD1306_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(ssd1306_i2c_device, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(uint8_t buf[], int buflen) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one gooooooo!

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning

    uint8_t *temp_buf = malloc(buflen + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buflen);

    i2c_write_blocking(ssd1306_i2c_device, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);

    free(temp_buf);
}

void SSD1306_init() {
    // Some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like
    // Some configuration values are recommended by the board manufacturer

    uint8_t cmds[] = {
        SSD1306_SET_DISP,               // set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE,           // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
        0x00,                           // horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE,    // set display start line to 0
        SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map, column address 127 is mapped to SEG0
        SSD1306_SET_MUX_RATIO,          // set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
        SSD1306_SET_DISP_OFFSET,        // set display offset
        0x00,                           // no offset
        SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number. 
                                        // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
        0x02,                           
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV,       // set display clock divide ratio
        0x80,                           // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE,          // set pre-charge period
        0xF1,                           // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL,         // set VCOMH deselect level
        0x30,                           // 0.83xVcc
        /* display */
        SSD1306_SET_CONTRAST,           // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON,          // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP,           // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP,        // set charge pump
        0x14,                           // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00,      // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
        SSD1306_SET_DISP | 0x01, // turn display on
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_scroll(bool on) {
    // configure horizontal scrolling
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00, // dummy byte
        0x00, // start page 0
        0x00, // time interval
        0x03, // end page 3 SSD1306_NUM_PAGES ??
        0x00, // dummy byte
        0xFF, // dummy byte
        SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void render(uint8_t *buf, struct render_area *area) {
    // update a portion of the display with a render area
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };
    
    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, area->buflen);
}

static void SetPixel(uint8_t *buf, int x,int y, bool on) {
    assert(x >= 0 && x < SSD1306_WIDTH && y >=0 && y < SSD1306_HEIGHT);

    // The calculation to determine the correct bit to set depends on which address
    // mode we are in. This code assumes horizontal

    // The video ram on the SSD1306 is split up in to 8 rows, one bit per pixel.
    // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->7,
    // byte 1 is x = 1, y=0->7 etc

    // This code could be optimised, but is like this for clarity. The compiler
    // should do a half decent job optimising it anyway.

    const int BytesPerRow = SSD1306_WIDTH ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |=  1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}
// Basic Bresenhams.
static void DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) {

    int dx =  abs(x1-x0);
    int sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0);
    int sy = y0<y1 ? 1 : -1;
    int err = dx+dy;
    int e2;

    while (true) {
        SetPixel(buf, x0, y0, on);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2*err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static inline int GetFontIndex(uint8_t ch) {
    return (ch <= '~') ? ch : 0;
}

static void InvertBlock(uint8_t *buf, int16_t x, int16_t y)
{
    if (x > (SSD1306_WIDTH/8 - 1) || y > (SSD1306_HEIGHT/8 - 1))
        return;
    int fb_idx = y * 128 + x * 8;
    for (int i=0;i<8;i++) {
        buf[fb_idx++] ^= 0xFF;
    }
}

static void InvertCursor(void)
{
    if (ssd1306_cursor_on) 
    {
        InvertBlock(ssd1306_buf, ssd1306_cursor_x, ssd1306_cursor_y);
        ssd1306_cursor_painted = 1;
    }
}

static void ssd1306_remove_cursor(void)
{
    if (ssd1306_cursor_painted)
    {
        InvertCursor();
        ssd1306_cursor_painted = 0;
    }
}

static void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    if (x > (SSD1306_WIDTH/8 - 1) || y > (SSD1306_HEIGHT/8 - 1))
        return;

    int idx = GetFontIndex(ch) * 8;
    int fb_idx = y * 128 + x * 8;

    for (int i=0;i<8;i++) {
        buf[fb_idx++] = font68[idx + i];
    }
}

static void WriteCharAtCursor(uint8_t *buf, uint8_t ch)
{
    ssd1306_remove_cursor();
    WriteChar(buf, ssd1306_cursor_x, ssd1306_cursor_y, ch);
    if (ssd1306_cursor_x >= (SSD1306_WIDTH/8 - 1)) 
    {
       ssd1306_cursor_x = 0;
       if (ssd1306_cursor_y < (SSD1306_HEIGHT/8 - 1))
           ssd1306_cursor_y++;
    } else ssd1306_cursor_x++;
}

static void WriteStringAtCursor(uint8_t *buf, const char *str)
{
    while (*str)
       WriteCharAtCursor(buf, *str++);
}

static void WriteString(uint8_t *buf, int16_t x, int16_t y, const char *str) {
    // Cull out any string off the screen
    if (x > (SSD1306_WIDTH/8 - 1) || y > (SSD1306_HEIGHT/8 - 1))
        return;

    while (*str) {
        WriteChar(buf, x, y, *str++);
        if (x < (SSD1306_WIDTH/8 - 1)) x++;
    }
}

void ssd1306_WriteString(int16_t x, int16_t y, const char *str)
{
    ssd1306_remove_cursor();
    WriteString(ssd1306_buf, x, y, str);
    ssd1306_modified = 1;
}

void ssd1306_DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on)
{
    ssd1306_remove_cursor();
    DrawLine(ssd1306_buf, x0, y0, x1, y1, on);
}

void ssd1306_set_cursor(uint8_t x, uint8_t y)
{
    ssd1306_remove_cursor();
    if ((ssd1306_cursor_x != x) || (ssd1306_cursor_y != y))
    {
        ssd1306_cursor_x = x;
        ssd1306_cursor_y = y;
        ssd1306_modified = 1;
    }
}

void ssd1306_set_cursor_onoff(uint8_t on)
{
    ssd1306_remove_cursor();
    if (ssd1306_cursor_on != on)
    {
        ssd1306_cursor_on = on;
        ssd1306_modified = 1;
    }
}

void ssd1306_printchar(uint8_t ch)
{
    WriteCharAtCursor(ssd1306_buf, ch);
    ssd1306_modified = 1;
}

void ssd1306_printstring(const char *str)
{
    WriteStringAtCursor(ssd1306_buf, str);
    ssd1306_modified = 1;
}

void ssd1306_render(void)
{
    InvertCursor();
    render(ssd1306_buf, &ssd1306_frame_area);
    ssd1306_modified = 0;
}

void ssd1306_update(void)
{
    if (ssd1306_modified) ssd1306_render();
}


void ssd1306_Clear_Buffer(void)
{
    memset(ssd1306_buf, 0, SSD1306_BUF_LEN);
    ssd1306_modified = 1;
    ssd1306_cursor_painted = 0;
}

void ssd1306_Initialize(void)
{
    i2c_init(ssd1306_i2c_device, SSD1306_I2C_CLK * 1000);
    gpio_set_function(PICO_SSD1306_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_SSD1306_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_SSD1306_I2C_SDA_PIN);
    gpio_pull_up(PICO_SSD1306_I2C_SCL_PIN);
    SSD1306_init();

    calc_render_area_buflen(&ssd1306_frame_area);
    ssd1306_Clear_Buffer();
	ssd1306_render();
}

void ssd1306_Set_All_On(void)
{
    SSD1306_send_cmd(SSD1306_SET_ALL_ON); 
}

void ssd1306_Set_Entire_On(void)
{
    SSD1306_send_cmd(SSD1306_SET_ENTIRE_ON);
}

void ssd1306_Set_Inv_Disp(void)
{
    SSD1306_send_cmd(SSD1306_SET_INV_DISP);
}

void ssd1306_Set_Norm_Disp(void)
{
   SSD1306_send_cmd(SSD1306_SET_NORM_DISP);
}

