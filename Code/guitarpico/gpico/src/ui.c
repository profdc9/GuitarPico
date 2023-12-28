/* ui.c

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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "guitarpico.h"
#include "buttons.h"
#include "ui.h"
#include "ssd1306_i2c.h"

#define setcursor(x,y,c) ssd1306_set_cursor(x,y,c)
#define cursoronoff(c) ssd1306_set_cursor_onoff(c)
#define writechar(c) ssd1306_printchar(c)
#define writestr(s) ssd1306_printstring(s)
#define readunbounced(c) button_readunbounced(c)
#define getbuttonpressed(c) button_getpressed(c)
#define displayrefresh() ssd1306_update()
#define idle_task() 
#define clearbuttons() buttons_clear()
#define getmillis() (time_us_32() / 1000)

void bar_graph(bargraph_dat *bgd)
{
    uint8_t ct = bgd->width_bars;
    int8_t width = bgd->bars;
    setcursor(bgd->col_bars,bgd->row_bars,1);
    while ((width > 0) && (ct > 0))
    {
      writechar(width+128);
      width -= 5;
      ct--;
    }
    while (ct > 0)
    {
      writechar(' ');
      ct--;
    }
	displayrefresh();
}

void write_num(uint32_t n, uint8_t digits, uint8_t decs)
{
  char s[20];
  char *p = number_str(s, n, digits, decs);
  writestr(p);
}

void write_str_with_spaces(uint8_t col, uint8_t row, const char *str, uint8_t len)
{
  setcursor(col,row,1);
  while (len > 0)
  {
    char c = *str++;
    if (c == 0) break;
    writechar(c);
    len--;
  }
  while (len > 0)
  {
    writechar(' ');
    len--;
  }
}

void do_show_menu_item(menu_str *menu)
{
  write_str_with_spaces(menu->col, menu->row, menu->items[menu->item], menu->width);
  displayrefresh();
}

uint8_t button_enter(void)
{
  return getbuttonpressed(0);
}

uint8_t button_left(void)
{
  return getbuttonpressed(2);
}

uint8_t button_right(void)
{
  return getbuttonpressed(3);
}

uint8_t button_up(void)
{
  return getbuttonpressed(1);
}

uint8_t button_down(void)
{
  return getbuttonpressed(4);
}

void select_item(menu_str *menu, uint8_t item)
{ 
    uint8_t i = 0;
    for (;;)
    { 
      if (menu->items[i] == NULL) break;
      if (i == item)
      {
        menu->item = item;
        do_show_menu_item(menu);
        break;
      }
      i++;
    }
}

uint8_t do_menu(menu_str *menu)
{
  idle_task();
  if (button_left())
  {
    clearbuttons();
    return 1;
  } else if (button_right())
  {
    clearbuttons();
    return 2;
  } else if (button_enter())
  {
    clearbuttons();
    return 3;
  } else if ((button_down()) && (menu->item > 0))
  {
    menu->item--;
    do_show_menu_item(menu);
  } else if ((button_up()) && (menu->items[menu->item + 1] != NULL))
  {
    menu->item++;
    do_show_menu_item(menu);
  } 
  return 0;
}

char *number_str(char *s, uint32_t n, uint8_t digits, uint8_t decs)
{
  char *d = s + digits + 2;
  *d = '\000';
  if (!decs) --decs;
  while (digits > 0)
  {
    uint32_t n10 = n / 10;
    uint8_t dig = n - n10 * 10;
    n = n10;
    if ((decs--) == 0) *(--d) = '.';
    *(--d) = dig + '0';
    digits--;
  }
  return d;
}

uint32_t pow10in(uint8_t n)
{
  uint32_t v = 1;
  while ((n--) > 0) v *= 10;
  return v;
}

uint8_t abort_button(void)
{
  return readunbounced(4);
}

void display_clear_row(uint8_t col, uint8_t row, uint8_t len)
{
  setcursor(col, row, 1);
  while (len > 0)
  {
    writechar(' ');
    len--;
  }
}

void scroll_number_redraw(scroll_number_dat *snd)
{
  setcursor(snd->col, snd->row, 1);
  write_num(snd->n, snd->digits, snd->decs);
  displayrefresh();

}

void scroll_number_key(scroll_number_dat *snd)
{
  uint8_t redraw = 1;

  setcursor(snd->col + snd->position + ((snd->decs != 0) && ((snd->position + snd->decs) >= snd->digits)), snd->row, 1);
  displayrefresh();
  idle_task();
  if (button_enter())
  {
    snd->entered = 1;
    return;
  } else if (button_left())
  {
    if (snd->position > 0)
      snd->position--;
    else
      snd->entered = 1;
  } else if (button_right())
  {
    if (snd->position < (snd->digits - 1))
      snd->position++;
    else
      snd->entered = 1;
  } else if (button_up())
  {
    uint32_t p10 = pow10in(snd->digits - snd->position - 1);
    if ((snd->n + p10) <= snd->maximum_number)
    {
      snd->n += p10;
      snd->changed = 1;
    }
  } else if (button_down())
  {
    if (snd->position >= snd->digits)
    {
      snd->entered = 1;
      return;
    }
    uint32_t p10 = (snd->digits - snd->position - 1);
    if (snd->n >= (snd->minimum_number + p10))
    {
      snd->n -= p10;
      snd->changed = 1;
    }
  } else redraw = 0;
  if (redraw)
    scroll_number_redraw(snd);
}

void scroll_number_start(scroll_number_dat *snd)
{
  snd->changed = 0;
  snd->entered = 0;
  scroll_number_redraw(snd);
  cursoronoff(1);
}

void scroll_number_stop(scroll_number_dat *snd)
{
  cursoronoff(0);
}

void scroll_alpha_redraw(scroll_alpha_dat *sad)
{
  uint8_t startpos, dp;

  dp = sad->displen >> 1;
  if (sad->position < dp)
    startpos = 0;
  else if (sad->position > (sad->numchars - dp))
    startpos = sad->numchars - sad->displen;
  else
    startpos = sad->position - dp;
  sad->cursorpos = sad->position - startpos;
  setcursor(sad->col, sad->row, 1);
  for (dp = 0; dp < sad->displen; dp++)
    writechar(sad->buffer[dp + startpos]);
  setcursor(sad->col + sad->cursorpos, sad->row, 1);
  displayrefresh();
}

void scroll_alpha_clear(scroll_alpha_dat *sad)
{
  display_clear_row(sad->col,sad->row,sad->displen);
  displayrefresh();

}

uint8_t scroll_alpha_find_key(scroll_alpha_dat *sad, uint8_t key)
{
  uint8_t cnt;

  for (cnt = 0; cnt < sad->num_validchars; cnt++)
  {
    uint8_t cmp_key = sad->validchars[cnt];
    if (key == cmp_key) return cnt;
  }
  return 0xFF;
}

void scroll_alpha_key(scroll_alpha_dat *sad)
{
  uint8_t redraw = 1;

  setcursor(sad->col + sad->cursorpos, sad->row, 1);
  idle_task();
  if (button_enter())
  { 
    sad->entered = 1;
    return;
  } else if (button_left())
  {
    if (sad->position > 0)
      sad->position--;
    else
      sad->exited = 1;
  } else if (button_right())
  {
    if (sad->position < (sad->numchars - 1))
      sad->position++;
    else
      sad->exited = 1;
  } else if (button_up())
  {
    uint8_t curkey = sad->buffer[sad->position];
    uint8_t val = scroll_alpha_find_key(sad, curkey);
    if (val != 0xFF)
    {
      if ((++val) >= sad->num_validchars) val = 0;
      sad->buffer[sad->position] = sad->validchars[val];
      sad->changed = 1;
    }
  } else if (button_down())
  {
    uint8_t curkey = sad->buffer[sad->position];
    uint8_t val = scroll_alpha_find_key(sad, curkey);
    if (val != 0xFF)
    {
      if (val == 0) val = sad->num_validchars - 1;
      else val--;
      sad->buffer[sad->position] = sad->validchars[val];
      sad->changed = 1;
    }
  } 
  if (redraw)
    scroll_alpha_redraw(sad);
}

void scroll_alpha_stop(scroll_alpha_dat *sad)
{
  cursoronoff(0);
}

void scroll_alpha_start(scroll_alpha_dat *sad)
{
  sad->changed = 0;
  sad->entered = 0;
  sad->exited = 0;
  if (sad->buffer[0] == 0)
    memset(sad->buffer, ' ', sad->numchars);
  scroll_alpha_redraw(sad);
  cursoronoff(1);
}

bool show_lr(uint8_t row, const char *message, const char *message_2)
{
  uint8_t cur_msg = 0;
  uint16_t last_tm = getmillis()-0x1000;
  clearbuttons();
  for (;;) {
    uint16_t cur_tm = getmillis();
    if ((cur_tm - last_tm) > 1000)
    {
      last_tm = cur_tm;
      write_str_with_spaces(0, row, cur_msg ? (message_2 != NULL ? message_2 : message) : message, 16);
      cur_msg = !cur_msg;
    }
    idle_task();
    if (button_left())
    {
      display_clear_row(0,row,16);
      clearbuttons();
      return false;
    }
    if (button_right())
    {
      display_clear_row(0,row,16);
      clearbuttons();
      return true;
    }
  }
}

void scroll_readout_initialize(scroll_readout_dat *srd)
{
  for (uint8_t i = 0; i < srd->numchars; i++)
    srd->buffer[i] = ' ';
  srd->position = srd->numchars - srd->displen;
  srd->exited = 0;
  srd->notchanged = 0;
}

void scroll_readout_add_character(scroll_readout_dat *srd, uint8_t ch)
{
  for (uint8_t i = 0; i < srd->numchars; i++)
    srd->buffer[i] = srd->buffer[i + 1];
  srd->buffer[srd->numchars - 1] = ch;
  srd->notchanged = 0;
}

void scroll_readout_back_character(scroll_readout_dat *srd, uint8_t ch)
{
  for (uint8_t i = srd->numchars; i > 0;)
  {
    --i;
    srd->buffer[i] = srd->buffer[i - 1];
  }
  srd->buffer[0] = ch;
  srd->notchanged = 0;
}

void scroll_readout_display(scroll_readout_dat *srd)
{
  if (!srd->notchanged)
  {
    setcursor(srd->col, srd->row, 1);
    for (uint8_t dp = 0; dp < srd->displen; dp++)
      writechar(srd->buffer[dp + srd->position]);
    srd->notchanged = 1;
  }
}

void scroll_readout_key(scroll_readout_dat *srd)
{
  setcursor(srd->col, srd->row, 1);
  displayrefresh();
  idle_task();
  if (button_left())
  {
    if (srd->position > 0)
    {
      srd->position--;
      srd->notchanged = 0;
      scroll_readout_display(srd);
    }
  } else if (button_right())
  {
    if (srd->position < (srd->numchars - srd->displen))
    {
      srd->position++;
      srd->notchanged = 0;
      scroll_readout_display(srd);
    }
  } else if (button_up() || button_down())
  {
    srd->position = srd->numchars - srd->displen;
    srd->notchanged = 0;
    scroll_readout_display(srd);
    srd->exited = 1;
  }
}
