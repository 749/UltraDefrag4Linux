/*
 *  WGX - Windows GUI Extended Library.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * @file font.c
 * @brief Fonts.
 * @addtogroup Fonts
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>

#include "wgx.h"

/* Uses Lua */
#define lua_c
#include "../../lua5.1/lua.h"
#include "../../lua5.1/lauxlib.h"
#include "../../lua5.1/lualib.h"

/* returns 0 if variable is not defined */
static int getint(lua_State *L, char *variable)
{
    int ret;
    
    lua_getglobal(L, variable);
    ret = (int)lua_tointeger(L, lua_gettop(L));
    lua_pop(L, 1);
    return ret;
}

/**
 * @brief Creates a font.
 * @param[in] wgx_font_path the path to the font definition file.
 * @param[in,out] pFont pointer to structure receiving created font.
 * @return Boolean value. TRUE indicates success.
 * @note If font definition cannot be retrieved from the file,
 * this routine uses default font definition passed through pFont structure.
 * @par File contents example:
@verbatim
height = -12
width = 0
escapement = 0
orientation = 0
weight = 400
italic = 0
underline = 0
strikeout = 0
charset = 0
outprecision = 3
clipprecision = 2
quality = 1
pitchandfamily = 34
facename = "Arial Black"
@endverbatim
 */
BOOL WgxCreateFont(char *wgx_font_path,PWGX_FONT pFont)
{
    lua_State *L;
    int status;
    char *string;
    LOGFONT lf;
    
    if(pFont == NULL)
        return FALSE;
    
    /* don't reset lf, it may contain default application's font */
    pFont->hFont = NULL;
    
    if(wgx_font_path == NULL)
        goto use_default_font;

    L = lua_open();  /* create state */
    if(L == NULL){
        WgxDbgPrint("WgxCreateFont: cannot initialize Lua library\n");
        goto use_default_font;
    }
    
    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);  /* open libraries */
    lua_gc(L, LUA_GCRESTART, 0);

    status = luaL_dofile(L,wgx_font_path);
    if(!status){ /* successful */
        lf.lfHeight = getint(L,"height");
        lf.lfWidth = getint(L,"width");
        lf.lfEscapement = getint(L,"escapement");
        lf.lfOrientation = getint(L,"orientation");
        lf.lfWeight = getint(L,"weight");
        lf.lfItalic = (BYTE)getint(L,"italic");
        lf.lfUnderline = (BYTE)getint(L,"underline");
        lf.lfStrikeOut = (BYTE)getint(L,"strikeout");
        lf.lfCharSet = (BYTE)getint(L,"charset");
        lf.lfOutPrecision = (BYTE)getint(L,"outprecision");
        lf.lfClipPrecision = (BYTE)getint(L,"clipprecision");
        lf.lfQuality = (BYTE)getint(L,"quality");
        lf.lfPitchAndFamily = (BYTE)getint(L,"pitchandfamily");
        lua_getglobal(L, "facename");
        string = (char *)lua_tostring(L, lua_gettop(L));
        if(string){
            (void)strncpy(lf.lfFaceName,string,LF_FACESIZE);
            lf.lfFaceName[LF_FACESIZE - 1] = 0;
        } else {
            lf.lfFaceName[0] = 0;
        }
        lua_pop(L, 1);
        lua_close(L);
    } else {
        WgxDbgPrint("WgxCreateFont: cannot interprete %s\n",wgx_font_path);
        lua_close(L);
        goto use_default_font;
    }
    
    pFont->hFont = CreateFontIndirect(&lf);
    if(pFont->hFont == NULL){
        WgxDbgPrintLastError("WgxCreateFont: CreateFontIndirect for custom font failed");
use_default_font:
        /* try to use default font passed through pFont */
        pFont->hFont = CreateFontIndirect(&pFont->lf);
        if(pFont->hFont == NULL){
            WgxDbgPrintLastError("WgxCreateFont: CreateFontIndirect for default font failed");
            return FALSE;
        }
        return TRUE;
    }
    
    memcpy(&pFont->lf,&lf,sizeof(LOGFONT));
    return TRUE;
}

/**
 * @brief Sets a font for the window and all its children.
 * @param[in] hWnd handle to the window.
 * @param[in] pFont pointer to the font definition structure.
 */
void WgxSetFont(HWND hWnd, PWGX_FONT pFont)
{
    HWND hChild;
    
    if(pFont == NULL)
        return;
    
    if(pFont->hFont == NULL)
        return;
    
    (void)SendMessage(hWnd,WM_SETFONT,(WPARAM)pFont->hFont,MAKELPARAM(TRUE,0));
    hChild = GetWindow(hWnd,GW_CHILD);
    while(hChild){
        (void)SendMessage(hChild,WM_SETFONT,(WPARAM)pFont->hFont,MAKELPARAM(TRUE,0));
        hChild = GetWindow(hChild,GW_HWNDNEXT);
    }

    /* redraw the main window */
    (void)InvalidateRect(hWnd,NULL,TRUE);
    (void)UpdateWindow(hWnd);
}

/**
 * @brief Destroys font built by WgxCreateFont.
 */
void WgxDestroyFont(PWGX_FONT pFont)
{
    if(pFont){
        if(pFont->hFont){
            (void)DeleteObject(pFont->hFont);
            pFont->hFont = NULL;
        }
    }
}

/**
 * @brief Saves a font definition to the file.
 * @param[in] wgx_font_path the path to the font definition file.
 * @param[out] pFont pointer to structure to be saved.
 * @return Boolean value. TRUE indicates success.
 * @note This function shows a message box in case when
 * some error has been occured.
 */
BOOL WgxSaveFont(char *wgx_font_path,PWGX_FONT pFont)
{
    LOGFONT *lf;
    FILE *pf;
    int result;
    char err_msg[1024];
    
    if(wgx_font_path == NULL || pFont == NULL)
        return FALSE;
    
    lf = &pFont->lf;

    pf = fopen(wgx_font_path,"wt");
    if(!pf){
        (void)_snprintf(err_msg,sizeof(err_msg) - 1,
            "Cannot save font preferences to %s!\n%s",
            wgx_font_path,_strerror(NULL));
        err_msg[sizeof(err_msg) - 1] = 0;
        MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
        return FALSE;
    }

    result = fprintf(pf,
        "height = %li\n"
        "width = %li\n"
        "escapement = %li\n"
        "orientation = %li\n"
        "weight = %li\n"
        "italic = %i\n"
        "underline = %i\n"
        "strikeout = %i\n"
        "charset = %i\n"
        "outprecision = %i\n"
        "clipprecision = %i\n"
        "quality = %i\n"
        "pitchandfamily = %i\n"
        "facename = \"%s\"\n",
        lf->lfHeight,
        lf->lfWidth,
        lf->lfEscapement,
        lf->lfOrientation,
        lf->lfWeight,
        lf->lfItalic,
        lf->lfUnderline,
        lf->lfStrikeOut,
        lf->lfCharSet,
        lf->lfOutPrecision,
        lf->lfClipPrecision,
        lf->lfQuality,
        lf->lfPitchAndFamily,
        lf->lfFaceName
        );
    fclose(pf);
    if(result < 0){
        (void)_snprintf(err_msg,sizeof(err_msg) - 1,
            "Cannot write font preferences to %s!\n%s",
            wgx_font_path,_strerror(NULL));
        err_msg[sizeof(err_msg) - 1] = 0;
        MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
        return FALSE;
    }
    return TRUE;
}

/** @} */
