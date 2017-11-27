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
static IDirectFBPalette			*paletteOfMain	= NULL;		/** DirectFBの調色板 */
static IDirectFBDisplayLayer	*layer			= NULL;		/** DirectFBの表示レイヤ */
static IDirectFBWindow			*winMain		= NULL;		/** DirectFBのウィンドウ */
static IDirectFBFont			*font			= NULL;		/** DirectFBのフォント */
static DFBDisplayLayerConfig	config;						/** DirectFBの表示レイヤの配置 */

#define MODNAME	 "[DFB Using window]\t"

static int screenWidth;
static int screenHeight;

/******************************************************************************/
/*! @brief ウィンドウをcreateする

 @param[in]		x		ウィンドウのxpos
 @param[in]		y		ウィンドウのypos
 @param[in]		width	ウィンドウの広さ
 @param[in]		height	ウィンドウの高さ
 @return		成功 ウィンドウのポインター, 失敗 0
 ******************************************************************************/
static IDirectFBWindow * createWindow(int x, int y, int width ,int height)
{
	int result;
	IDirectFBWindow *win;
	DFBWindowDescription wdesc;

	wdesc.flags 		= ( DWDESC_CAPS | 
							DWDESC_POSX | 
							DWDESC_POSY | 
							DWDESC_WIDTH | 
							DWDESC_HEIGHT );
	wdesc.caps 			= DWCAPS_ALPHACHANNEL;
	wdesc.posx 			= x;
	wdesc.posy 			= y;
	wdesc.width 		= width;
	wdesc.height 		= height;
	wdesc.pixelformat 	= DSPF_ARGB;

	// 表示レイヤ と ウィンドウをcreateする
	if( 0 != (result = layer->CreateWindow(layer, &wdesc, &win)) )
	{
		DirectFBError("createWindow() failed", result);
		return NULL;
	}

	return win;
}

/******************************************************************************/
/*! @brief surfaceをクリアする

 @param[in]		srf		surfaceのポインター
 @return		無
 ******************************************************************************/
static void clear_surface(IDirectFBSurface *srf)
{
	if( srf != 0 )
	{
		srf->Clear(srf, 0, 0, 0, 0);
	}
}

/******************************************************************************/
/*! @brief ウィンドウを初期化する

 @param			無
 @return		成功 0, 失敗 -1
 ******************************************************************************/
static int initWindow()
{
	int result;

	/** 1．ウィンドウをcreateする */
	printf("screenWidth:%d     screenHeight:%d\n", screenWidth, screenHeight);
	winMain = createWindow( 0, 0, screenWidth, screenHeight);
	if( winMain == NULL )
	{
		fprintf(stderr, MODNAME "createWindow failed\n");
		return -1;
	}

	/** 2．surfaceを取得する */
	winMain->GetSurface(winMain, &surfaceOfMain);
	if( surfaceOfMain == NULL )
	{
		fprintf(stderr, MODNAME "GetSurface failed\n");
		return -1;
	}

	/** 3．調色板を取得する */
	surfaceOfMain->GetPalette(surfaceOfMain, &paletteOfMain);
	if( paletteOfMain == NULL )
	{
		fprintf(stderr, MODNAME "GetPalette failed\n");
		return -1;
	}

	/** 4．調色板のリストを設定する */
	if( 0 != (result = paletteOfMain->SetEntries(paletteOfMain, myColors_3, sizeof(myColors_3)/sizeof(myColors_3[0]), 0)) )
	{
		fprintf(stderr, MODNAME "SetEntries failed\n");
		return -1;
	}

	/** 5．文字フォントを設定する */
	surfaceOfMain->SetFont(surfaceOfMain, font);

	/** 6．surfaceをクリアする */
	clear_surface(surfaceOfMain);

	/** 7．ウィンドウ初期表示時、全透明で非表示する */
	winMain->SetOpacity(winMain, 0x00);
	return 0;
}

/******************************************************************************/
/*! @brief ウィンドウの解放

 @param			無
 @return		無
 ******************************************************************************/
static void releaseWindow()
{
	// 調色板を解放する
	if( paletteOfMain != NULL )
	{
		paletteOfMain->Release(paletteOfMain);
		paletteOfMain = NULL;
	}

	// surfaceを解放する
	if( surfaceOfMain != NULL )
	{
		surfaceOfMain->Release(surfaceOfMain);
		surfaceOfMain = NULL;
	}

	// 画面を開放する
	if( winMain != NULL )
	{
		winMain->Release(winMain);
		winMain = NULL;
	}
}

/******************************************************************************/
/*! @brief surfaceの色を設定する

 @param[in]		srf		surfaceのポインター
 @param[in]		color	色
 @return		無
 ******************************************************************************/
static void SetColor(IDirectFBSurface* srf, ColorSelection color)
{
	if( !srf )
	{
		fprintf(stderr, "[%s] No surface!!!\n", __FUNCTION__);
		return;
	}

	if( (color < BLACK) || (color > CLEAR) )
	{
		fprintf(stderr, "[%s] Bad Color Index %d!!!\n", __FUNCTION__, color);
		return;
	}

	srf->SetColorIndex(srf, color);
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
		goto ERROR;
	}

	/** 2．DirectFBのCreate */
	if (0 != (result = DirectFBCreate(&dfb))) 
	{
		DirectFBError( "DirectFBCreate() failed", result );
		goto ERROR;
	}

	/** 3．DirectFBの表示レイヤのCreate */
	if (0 != (result = dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &layer)))
	{
		if (result == DFB_IDNOTFOUND)
		{
			fprintf(stderr, MODNAME "Unknown layer id, check 'dfbinfo' for valid values.\n");
		}
		else
		{
			DirectFBError( "IDirectFB::GetDisplayLayer() failed", result );
		}
		goto ERROR_DFB;
	}

	/** 4．DirectFBの表示レイヤの協同組合レベルを設定する */
	if (0 != (result = layer->SetCooperativeLevel(layer, DLSCL_EXCLUSIVE))) 
	{
		DirectFBError("IDirectFBDisplayLayer::SetCooperativeLevel() failed", result);
		goto ERROR_LAYER;
	}

	/** 5．DirectFBの表示レイヤの属性をgetする */
	if (0 != (result = layer->GetConfiguration(layer, &config)))
	{
		DirectFBError( "IDirectFBDisplayLayer::GetConfiguration() failed", result );
		goto ERROR_LAYER;
	}

	/** 6．DirectFBの表示レイヤの属性をsetする */
	config.flags = DLCONF_BUFFERMODE | DLCONF_PIXELFORMAT;
	config.buffermode = DLBM_FRONTONLY;
	config.pixelformat = DSPF_ARGB;
	if (0 != (result = layer->SetConfiguration(layer, &config))) 
	{
		DirectFBError( "IDirectFBDisplayLayer::SetConfiguration() failed", result );
		goto ERROR_LAYER;
	}

	/** 7．DirectFBのスクリーンの属性をgetする */
	IDirectFBScreen* iDirectFBScreen;
	if (0 != (result = dfb->GetScreen(dfb, 0, &iDirectFBScreen)))
	{
		DirectFBError("Couldn't get screen data .. \n", result);
		goto ERROR_LAYER;
	}
	else
	{
		iDirectFBScreen->GetSize(iDirectFBScreen, &screenWidth, &screenHeight);
		iDirectFBScreen->Release(iDirectFBScreen);
		printf( "GetScreen with ( width=%d, height=%d )\n", screenWidth, screenHeight);
	}

	/** 8．DirectFBの文字フォントを設定する */
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
	result = dfb->CreateFont(dfb, D_DEFAULT_FONT, &fdesc, &font);
	if (0 != result)
	{
		DirectFBError("Couldn't create font", result);
		goto ERROR_LAYER;
	}

	/** 9．DirectFBのマウスカーソルが無効に設定する */
	layer->EnableCursor(layer, 0);
	return 0;

ERROR_LAYER:
	layer->Release(layer);
ERROR_DFB:
	dfb->Release(dfb);
ERROR:
	return -1;
}

/******************************************************************************/
/*! @brief 文字列の描画

 @param[in]		s		文字列
 @param[in]		x		文字列起点のxpos
 @param[in]		y		文字列起点のypos
 @return		無
 ******************************************************************************/
void hub_verup_draw_string(	const char *s, int x, int y) 
{
	SetColor(surfaceOfMain, D_DIRECT_FB_FONT_COLOR);
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
void hub_verup_draw_select_string(const char *s, int x, int y, int len) 
{
	surfaceOfMain->SetColorIndex(surfaceOfMain, D_DIRECT_FB_BACK_COLOR);
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
	surfaceOfMain->SetColorIndex(surfaceOfMain, D_DIRECT_FB_FONT_COLOR);
	surfaceOfMain->DrawString(surfaceOfMain, s, -1, x, y, DSTF_TOPLEFT);

	return;
}

/******************************************************************************/
/*! @brief 画面スクリーンのクリア

 @param			無
 @return		無
 ******************************************************************************/
void hub_verup_clear_screen(void)
{
	surfaceOfMain->SetColorIndex( surfaceOfMain, D_DIRECT_FB_BACK_COLOR );
	surfaceOfMain->FillRectangle( surfaceOfMain, D_DIRECT_FB_X_POS, D_DIRECT_FB_Y_POS, screenWidth, screenHeight );

	// 非透明設定で非表示⇒表示
	winMain->SetOpacity(winMain, 0xFF);
	surfaceOfMain->Flip(surfaceOfMain, NULL, DSFLIP_BLIT);

	return;
}

/******************************************************************************/
/*! @brief 画面スクリーンの出力

 @param			無
 @return		無
 ******************************************************************************/
void hub_verup_output_screen(void)
{
	surfaceOfMain->Flip(surfaceOfMain, NULL, DSFLIP_BLIT);

	return;
}

/******************************************************************************/
/*! @brief 画面スクリーン描画を初期化する

 @param			無
 @return		無
 ******************************************************************************/
int hub_verup_init_screen(void)
{
	/** Ctrl+Cを無効にする */
	signal( SIGINT, SIG_IGN );

	/** DirectFBを初期化する */
	if (0 != init_dfb())
	{
		fprintf(stderr, MODNAME "failed to build DirectFB\n");
		return -1;
	}

	/** ウィンドウを初期化する */
	if (0 != initWindow())
	{
		fprintf(stderr, MODNAME "failed to initWindow\n");
		return -1;
	}

	/** 画面のクリア */
	hub_verup_clear_screen();

	return 0;
}

/******************************************************************************/
/*! @brief 画面スクリーン描画を終了する

 @param			無
 @return		無
 ******************************************************************************/
int hub_verup_exit_screen(void)
{
	/** 画面のクリア */
	hub_verup_clear_screen();
	clear_surface(surfaceOfMain);

	/** リソース開放を忘れると、OSD画面にカスが残る */
	releaseWindow();

	// フォントリソースの解放
	if (font != NULL)
	{
		font->Release(font);
		font = NULL;
	}

	// Layerリソースの解放
	if( layer != NULL )
	{
		layer->Release(layer);
		layer = NULL;
	}

	// DirectFbリソースの解放
	if( dfb != NULL )
	{
		dfb->Release(dfb);
		dfb = NULL;
	}

	return 0;
}

