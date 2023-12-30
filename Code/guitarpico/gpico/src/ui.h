/* ui.h

*/

/*
   Copyright (c) 2021 Daniel Marks

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _UI_H
#define _UI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _menu_str
{
  const char * const* items;
  uint8_t col;
  uint8_t row;
  uint8_t width;
  uint8_t item;
  uint8_t itemesc;
} menu_str;

typedef struct _bargraph_dat
{
  uint8_t width_bars;
  uint8_t col_bars;
  uint8_t row_bars;
  uint8_t bars;
} bargraph_dat;

typedef struct _scroll_number_dat
{
  uint8_t     col;
  uint8_t     row;
  uint8_t     digits;
  uint8_t     decs;
  uint32_t    minimum_number;
  uint32_t    maximum_number;

  uint8_t     position;
  uint32_t    n;
  uint8_t     entered;
  uint8_t     changed;
} scroll_number_dat;

typedef struct _scroll_alpha_dat
{
  uint8_t        col;
  uint8_t        row;
  uint8_t        displen;
  uint8_t        numchars;
  uint8_t       *buffer;
  const uint8_t *validchars;
  uint8_t        num_validchars;
  uint8_t        position;
  uint8_t        cursorpos;
  uint8_t        changed;
  uint8_t        entered;
  uint8_t        exited;
} scroll_alpha_dat;

void bar_graph(bargraph_dat *bgd);
uint8_t do_menu(menu_str *menu);
void do_show_menu_item(menu_str *menu);
bool show_lr(uint8_t row, const char *message, const char *message2);
void set_cursor(uint8_t x, uint8_t y);
void set_cursor_onoff(uint8_t c);
char *number_str(char *s, uint32_t n, uint8_t digits, uint8_t decs);
void write_str(uint8_t col, uint8_t row, const char *str);
void write_str_with_spaces(uint8_t col, uint8_t row, const char *str, uint8_t len);
void write_num(uint32_t n, uint8_t digits, uint8_t decs);
void clear_display(void);

void scroll_number_stop(scroll_number_dat *snd);
void scroll_number_start(scroll_number_dat *snd);
void scroll_number_key(scroll_number_dat *snd);
void scroll_number_redraw(scroll_number_dat *snd);

void scroll_alpha_stop(scroll_alpha_dat *sad);
void scroll_alpha_start(scroll_alpha_dat *sad);
void scroll_alpha_key(scroll_alpha_dat *sad);
void scroll_alpha_redraw(scroll_alpha_dat *sad);
void scroll_alpha_clear(scroll_alpha_dat *sad);

typedef struct _scroll_readout_dat
{
  uint8_t        col;
  uint8_t        row;
  uint8_t        displen;
  uint8_t        numchars;
  uint8_t       *buffer;
  uint8_t        position;
  uint8_t        exited;
  uint8_t        notchanged;
} scroll_readout_dat;

uint8_t button_enter(void);
uint8_t button_left(void);
uint8_t button_right(void);
uint8_t button_up(void);
uint8_t button_down(void);
void button_clear(void);
void display_refresh(void);

void scroll_readout_initialize(scroll_readout_dat *srd);
void scroll_readout_add_character(scroll_readout_dat *srd, uint8_t ch);
void scroll_readout_back_character(scroll_readout_dat *srd, uint8_t ch);
void scroll_readout_display(scroll_readout_dat *srd);
void scroll_readout_key(scroll_readout_dat *srd);

void display_clear_row(uint8_t col, uint8_t row, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif  /* _UI_H */
