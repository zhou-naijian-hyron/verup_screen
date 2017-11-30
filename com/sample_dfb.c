/*
 * sample_dfb.c for G3NVR
 * Copyright(C) 2002-2017 MegaChips Co.,Ltd All right reserved.
 * author: "Zhou Naijian"<zhounj@hyron-js.com>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "verup_dfb_api.h"

// 進捗表示レイアウト構造体
typedef struct {
	unsigned int					Width;				// 幅
	unsigned int					Height;				// 高さ
	unsigned int					FontSize;			// フォントサイズ
} ST_VS_MAIN_LAYOUT_TBL;

// 画面表示文字列長
#define D_DISP_STRING_LEN			(128)

// モニター種別毎の行幅
const ST_VS_MAIN_LAYOUT_TBL g_LayoutPix[] =
{
	{1024,	768,	32},	// SXGA
	{1920,	1080,	40}		// Full-HD
};

static int layoutPatton = 1;

int main(int argc, char* argv[])
{
	char title[D_DISP_STRING_LEN];
	int ret;
	int xpos;

	// 1. 画面初期化
	ret = hifb_dfb_init_screen();
	if( ret != 0 )
	{
		fprintf(stderr, "[VerUpProgress] hifb_dfb_init_screen() failed.\n");
		return -1;
	}
	printf("[VerUpProgress] initialization complete.wait 10s.\n");
	sleep(10);

	// 2. 文字表示
	// 背景色
	hifb_dfb_background();
	// 画面サイズと書き込み文字数より描画中央位置の算出
	snprintf(title,sizeof(title),"ファームウェア更新 (1/1)");
	xpos = ((g_LayoutPix[ layoutPatton ].Width / g_LayoutPix[ layoutPatton ].FontSize) - (strlen(title) / 2)) / 2;
	hifb_dfb_draw_string(title, (int)(xpos * g_LayoutPix[ layoutPatton ].FontSize), (int)(1 * g_LayoutPix[ layoutPatton ].FontSize));
	// 出力
	hifb_dfb_output();
	printf("[VerUpProgress] string display. wait 20s.\n");
	sleep(20);

	// 3. 画面クローズ
	hifb_dfb_exit_screen();
	printf("[VerUpProgress] exit screen.\n");

	return 0;
}
