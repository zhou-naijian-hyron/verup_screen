/*
 * verup_dfb_api.h for G3NVR
 * Copyright(C) 2002-2017 MegaChips Co.,Ltd All right reserved.
 * author: "Zhou Naijian"<zhounj@hyron-js.com>
 */
#ifndef _VERUP_DFB_API_H_
#define _VERUP_DFB_API_H_

void hub_verup_draw_string(	const char *s, int x, int y);
void hub_verup_draw_select_string(const char *s, int x, int y, int len);
void hub_verup_clear_screen(void);
void hub_verup_output_screen(void);
int hub_verup_init_screen(void);
int hub_verup_exit_screen(void);

#endif	//_VERUP_DFB_API_H_

