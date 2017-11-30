/*
 * verup_dfb_api.h for G3NVR
 * Copyright(C) 2002-2017 MegaChips Co.,Ltd All right reserved.
 * author: "Zhou Naijian"<zhounj@hyron-js.com>
 */
#ifndef _VERUP_DFB_API_H_
#define _VERUP_DFB_API_H_

void hifb_dfb_draw_string(	const char *s, int x, int y);
void hifb_dfb_draw_select_string(const char *s, int x, int y, int len);
void hifb_dfb_background();
void hifb_dfb_output(void);
void hifb_dfb_clear_screen(void);

int hifb_dfb_init_screen(void);
int hifb_dfb_exit_screen(void);

#endif	//_VERUP_DFB_API_H_

