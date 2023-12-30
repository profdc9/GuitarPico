
/* Modified by Daniel L. Marks, all changes also under BSD-3-Clause */

#ifndef __SSD1306_I2C_H
#define __SSD1306_I2C_H

#ifdef __cplusplus
extern "C"
{
#endif

// Define the size of the display we have attached. This can vary, make sure you
// have the right size defined or the output will look rather odd!
// Code has been tested on 128x32 and 128x64 OLED displays
#define SSD1306_HEIGHT              64
#define SSD1306_WIDTH               128

#define SSD1306_I2C_ADDR            _u(0x3C)

// 400 is usual, but often these can be overclocked to improve display response.
// Tested at 1000 on both 32 and 84 pixel height devices and it worked.
#define SSD1306_I2C_CLK             400
//#define SSD1306_I2C_CLK             1000

#define ssd1306_i2c_device &i2c1_inst

// commands (see datasheet)
#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

#define PICO_SSD1306_I2C_SDA_PIN 10
#define PICO_SSD1306_I2C_SCL_PIN 11

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;

    int buflen;
};


void ssd1306_Initialize(void);
void ssd1306_WriteString(int16_t x, int16_t y, const char *str);
void ssd1306_DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on);
void ssd1306_render(void);
void ssd1306_Clear_Buffer(void);
void ssd1306_Set_All_On(void);
void ssd1306_Set_Entire_On(void);
void ssd1306_Set_Inv_Disp(void);
void ssd1306_Set_Norm_Disp(void);

void ssd1306_set_cursor(uint8_t x, uint8_t y);
void ssd1306_set_cursor_onoff(uint8_t on);
void ssd1306_printchar(uint8_t ch);
void ssd1306_printstring(const char *str);
void ssd1306_update(void);

#ifdef __cplusplus
}
#endif

#endif /* _SSD1306_I2C_H */
