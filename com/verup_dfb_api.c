/*
 * verup_dfb_api.c for G3NVR
 * Copyright(C) 2002-2017 MegaChips Co.,Ltd All right reserved.
 * author: "Zhou Naijian"<zhounj@hyron-js.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <directfb.h>
#include <directfb_strings.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "verup_dfb_api.h"

/** DirectFBにて、対象色のARGB8888の値 */
static DFBColor myColors_3[] = {
		/* A    R     G     B */
		{ 0xC0, 0x00, 0xFF, 0xFF }, // ALPHA_AQUA
		{ 0xC0, 0x80, 0x80, 0x80 }, // ALPHA_GRAY
		{ 0xC0, 0x00, 0x00, 0xFF }, // ALPHA_BLUE
		{ 0xC0, 0xF0, 0xF0, 0x00 }, // ALPHA_OLIVE
		{ 0xC0, 0xFF, 0xFF, 0x00 }, // ALPHA_YELLOW
		{ 0xC0, 0x80, 0x00, 0x80 }, // ALPHA_PURPLE
		{ 0xC0, 0x00, 0xFF, 0xFF }, // ALPHA_LIGHT_BLUE
		{ 0xC0, 0xFF, 0xFF, 0xFF }, // ALPHA_WHITE
		{ 0xFF, 0x00, 0x80, 0x00 }, // GREEN
		{ 0xFF, 0x00, 0x00, 0x80 }, // NAVY
		{ 0xFF, 0xC0, 0xC0, 0xC0 }, // SILVER
		{ 0xFF, 0x00, 0x00, 0x00 }, // BLACK
		{ 0xFF, 0x00, 0x00, 0xFF }, // BLUE
		{ 0xFF, 0x00, 0xFF, 0xFF }, // LIGHT_BLUE
		{ 0xFF, 0xC0, 0x80, 0x00 }, // OLIVE
		{ 0xFF, 0x80, 0x00, 0x80 }, // PURPLE
		{ 0xFF, 0xFF, 0x00, 0xFF }, // FUCHSIA
		{ 0xFF, 0xFF, 0x00, 0x00 }, // RED
		{ 0xFF, 0xFF, 0xFF, 0xFF }, // WHITE
		{ 0xFF, 0xFF, 0xFF, 0x00 }, // YELLOW
		{ 0xFF, 0x32, 0x32, 0x32 }, // GRAY
		{ 0x00, 0x00, 0x00, 0x00 }	// CLEAR
};

/** DirectFBにて、対象色のテーブル */
typedef enum {
		ALPHA_AQUA,
		ALPHA_GRAY,
		ALPHA_BLUE,
		ALPHA_OLIVE,
		ALPHA_YELLOW,
		ALPHA_PURPLE,
		ALPHA_LIGHT_BLUE,
		ALPHA_WHITE,
		GREEN,
		NAVY,
		SILVER,
		BLACK,
		BLUE,
		LIGHT_BLUE,
		OLIVE,
		PURPLE,
		FUCHSIA,
		RED,
		WHITE,
		YELLOW,
		GRAY,
		CLEAR,
} ColorSelection;

/** DirectFBにて、文字フォント */
#define D_DEFAULT_FONT					"/usr/mcc/lib/fonts/Tf1hrgw4.ttf"

/** DirectFBにて、画面の起点 */
#define D_DIRECT_FB_X_POS				(0)
#define D_DIRECT_FB_Y_POS				(0)

/** DirectFBにて、画面基色 */
#define D_DIRECT_FB_BACK_COLOR			(BLUE)				// 背景色
#define D_DIRECT_FB_FONT_COLOR			(WHITE)				// 文字色

/** DirectFBにて、文字サイズ */
#define	D_DIRECT_FB_FONTSIZE_FHD		(40)
#define	D_DIRECT_FB_FONTSIZE_XGA		(32)


static IDirectFB				*dfb			= NULL;		/** DirectFBリソース */
static IDirectFBSurface 		*surfaceOfMain	= NULL;		/** DirectFBのsurface */
static IDirectFBFont			*font			= NULL;		/** DirectFBのフォント */

#define MODNAME	 "[DFB_API]\t"

static int screenWidth;
static int screenHeight;

/******************************************************************************/
/*! @brief ウィンドウを初期化する

 @param			無
 @return		成功 0, 失敗 -1
 ******************************************************************************/
static int initSurface()
{
	int result;

	/** 1．surfaceをcreateする */
	DFBSurfaceDescription  sdsc;
	sdsc.flags = DSDESC_CAPS;
	sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE;
	result = dfb->CreateSurface( dfb, &sdsc, &surfaceOfMain );
	if( surfaceOfMain == NULL )
	{
		DirectFBError("CreateSurface() failed\n", result);
		return -1;
	}

	/** 2．文字フォントを設定する */
	if(0 != (result = surfaceOfMain->SetFont(surfaceOfMain, font)))
	{
		DirectFBError("SetFont() failed\n", result);
		return -1;
	}

	/** 3．surfaceをクリアする */
	surfaceOfMain->Clear(surfaceOfMain, 0, 0, 0, 0);

	return 0;
}

/******************************************************************************/
/*! @brief ウィンドウの解放

 @param			無
 @return		無
 ******************************************************************************/
static void releaseSurface()
{
	// surfaceを解放する
	if( surfaceOfMain != NULL )
	{
		surfaceOfMain->Release(surfaceOfMain);
		surfaceOfMain = NULL;
	}
}

/******************************************************************************/
/*! @brief リソースの解放

 @param			無
 @return		無
 ******************************************************************************/
static void releaseResource()
{
	// フォントリソースの解放
	if (font != NULL)
	{
		font->Release(font);
		font = NULL;
	}

	// DirectFbリソースの解放
	if( dfb != NULL )
	{
		dfb->Release(dfb);
		dfb = NULL;
	}
}

/******************************************************************************/
/*! @brief DirectFBを初期化する

 @param			無
 @return		成功 0, 失敗 -1
 ******************************************************************************/
static int init_dfb(void)
{
	DFBResult result;

	/** 1．DirectFBの初期化 */
	if (0 != (result = DirectFBInit(NULL, NULL)))
	{
		DirectFBError( "DirectFBInit() failed", result );
		return -1;
	}

	/** 2．DirectFBのCreate */
	if (0 != (result = DirectFBCreate(&dfb))) 
	{
		DirectFBError( "DirectFBCreate() failed", result );
		return -1;
	}

	/** 3．DirectFBの属性をsetする */
	if (0 != (result = dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN )))
	{
		DirectFBError( "SetCooperativeLevel() failed", result );
		return -1;
	}

	/** 4．DirectFBのスクリーンのwidth,heightをgetする */
	IDirectFBScreen* iDirectFBScreen;
	if (0 != (result = dfb->GetScreen(dfb, 0, &iDirectFBScreen)))
	{
		DirectFBError("GetScreen() failed", result);
		return -1;
	}
	else
	{
		iDirectFBScreen->GetSize(iDirectFBScreen, &screenWidth, &screenHeight);
		iDirectFBScreen->Release(iDirectFBScreen);
	}

	/** 5．DirectFBの文字フォントを設定する */
	DFBFontDescription fdesc;
	fdesc.flags = DFDESC_HEIGHT;
	if (screenWidth == 1920)
	{
		fdesc.height = D_DIRECT_FB_FONTSIZE_FHD;
	}
	else
	{
		fdesc.height = D_DIRECT_FB_FONTSIZE_XGA;
	}
	fdesc.flags |= DFDESC_ATTRIBUTES;
	fdesc.attributes = DFFA_NONE;
	if (0 != (result = dfb->CreateFont(dfb, D_DEFAULT_FONT, &fdesc, &font)))
	{
		DirectFBError("CreateFont() failed", result);
		return -1;
	}

	return 0;
}

/******************************************************************************/
/*! @brief 文字列の描画

 @param[in]		s		文字列
 @param[in]		x		文字列起点のxpos
 @param[in]		y		文字列起点のypos
 @return		無
 ******************************************************************************/
void hifb_dfb_draw_string(	const char *s, int x, int y)
{
	surfaceOfMain->SetColor(surfaceOfMain, myColors_3[D_DIRECT_FB_FONT_COLOR].r, myColors_3[D_DIRECT_FB_FONT_COLOR].g, myColors_3[D_DIRECT_FB_FONT_COLOR].b, myColors_3[D_DIRECT_FB_FONT_COLOR].a);
	surfaceOfMain->DrawString(surfaceOfMain, s, -1, x, y, DSTF_TOPLEFT);
	return;
}

/******************************************************************************/
/*! @brief 選択肢の描画

 @param[in]		s		選択肢
 @param[in]		x		選択肢起点のxpos
 @param[in]		y		選択肢起点のypos
 @param[in]		y		選択肢の長さ
 @return		無
 ******************************************************************************/
void hifb_dfb_draw_select_string(const char *s, int x, int y, int len)
{
	surfaceOfMain->SetColor(surfaceOfMain, myColors_3[D_DIRECT_FB_BACK_COLOR].r, myColors_3[D_DIRECT_FB_BACK_COLOR].g, myColors_3[D_DIRECT_FB_BACK_COLOR].b, myColors_3[D_DIRECT_FB_BACK_COLOR].a);
	if(screenWidth == 1920)
	{
		// 矩形塗り潰し表示
		surfaceOfMain->FillRectangle(surfaceOfMain, x, y, D_DIRECT_FB_FONTSIZE_FHD*len, D_DIRECT_FB_FONTSIZE_FHD);
	}
	else
	{
		// 矩形塗り潰し表示
		surfaceOfMain->FillRectangle(surfaceOfMain, x, y, D_DIRECT_FB_FONTSIZE_XGA*len, D_DIRECT_FB_FONTSIZE_XGA);
	}
	surfaceOfMain->SetColor(surfaceOfMain, myColors_3[D_DIRECT_FB_FONT_COLOR].r, myColors_3[D_DIRECT_FB_FONT_COLOR].g, myColors_3[D_DIRECT_FB_FONT_COLOR].b, myColors_3[D_DIRECT_FB_FONT_COLOR].a);
	surfaceOfMain->DrawString(surfaceOfMain, s, -1, x, y, DSTF_TOPLEFT);

	return;
}

/******************************************************************************/
/*! @brief 背景画面

 @param			無
 @return		無
 ******************************************************************************/
void hifb_dfb_background(void)
{
	surfaceOfMain->SetColor(surfaceOfMain, myColors_3[D_DIRECT_FB_BACK_COLOR].r, myColors_3[D_DIRECT_FB_BACK_COLOR].g, myColors_3[D_DIRECT_FB_BACK_COLOR].b, myColors_3[D_DIRECT_FB_BACK_COLOR].a);
	surfaceOfMain->FillRectangle( surfaceOfMain, D_DIRECT_FB_X_POS, D_DIRECT_FB_Y_POS, screenWidth, screenHeight );
	return;
}

/******************************************************************************/
/*! @brief 画面スクリーンの出力

 @param			無
 @return		無
 ******************************************************************************/
void hifb_dfb_output(void)
{
	surfaceOfMain->Flip(surfaceOfMain, NULL, DSFLIP_NONE);
	return;
}

/******************************************************************************/
/*! @brief 画面スクリーンのクリア

 @param			無
 @return		無
 ******************************************************************************/
void hifb_dfb_clear_screen(void)
{
	hifb_dfb_background();
	hifb_dfb_output();
	return;
}

/******************************************************************************/
/*! @brief 画面スクリーン描画を初期化する

 @param			無
 @return		無
 ******************************************************************************/
int hifb_dfb_init_screen(void)
{
	/** Ctrl+Cを無効にする */
	signal( SIGINT, SIG_IGN );

	/** DirectFBを初期化する */
	if (0 != init_dfb())
	{
		fprintf(stderr, MODNAME "failed to init_dfb\n");
		goto ERROR;
	}

	/** ウィンドウを初期化する */
	if (0 != initSurface())
	{
		fprintf(stderr, MODNAME "failed to initSurface\n");
		goto ERROR;
	}

	/** 画面のクリア */
	hifb_dfb_clear_screen();

	return 0;

ERROR:
	/** 解放 */
	releaseSurface();
	releaseResource();
	return -1;
}

/******************************************************************************/
/*! @brief 画面スクリーン描画を終了する

 @param			無
 @return		無
 ******************************************************************************/
int hifb_dfb_exit_screen(void)
{
	/** 画面のクリア */
	hifb_dfb_clear_screen();

	/** 終了処理 */
	surfaceOfMain->Clear(surfaceOfMain, 0, 0, 0, 0);
	releaseSurface();
	releaseResource();

	return 0;
}

