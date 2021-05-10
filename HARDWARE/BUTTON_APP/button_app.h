#ifndef __BUTTON_APP_H
#define __BUTTON_APP_H

#include "sys.h"

void lcd_draw_bline(u16 x1, u16 y1, u16 x2, u16 y2,u8 size,u16 color);

void ctp_test(void);
void TFT_LCD_Init(void);
void bsp_GUI_Init(void);
void scan_touch_key(void);
void Button_App(void);

#endif
