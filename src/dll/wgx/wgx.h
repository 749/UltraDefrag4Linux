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

/*
* WGX - Windows GUI Extended Library.
* Contains functions that extend Windows Graphical API 
* to simplify GUI applications development.
*/

#ifndef _WGX_H_
#define _WGX_H_

#include <wchar.h>

/* definitions missing on several development systems */
#ifndef USE_WINDDK

#ifndef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLong
#endif
#ifndef SetWindowLongPtrA
#define SetWindowLongPtrA SetWindowLongA
#endif
#ifndef SetWindowLongPtrW
#define SetWindowLongPtrW SetWindowLongW
#endif

#define LONG_PTR LONG

#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC GWL_WNDPROC
#endif

#endif /* !USE_WINDDK */

#ifdef USE_MSVC
#define DWORD_PTR DWORD
typedef int intptr_t;
typedef unsigned uintptr_t;
#endif

#ifndef LR_VGACOLOR
/* this constant is not defined in winuser.h on mingw */
#define LR_VGACOLOR         0x0080
#endif

/* wgx structures */
typedef struct _WGX_I18N_RESOURCE_ENTRY {
    int ControlID;
    wchar_t *Key;
    wchar_t *DefaultString;
    wchar_t *LoadedString;
} WGX_I18N_RESOURCE_ENTRY, *PWGX_I18N_RESOURCE_ENTRY;

typedef struct _WGX_FONT {
    LOGFONT lf;
    HFONT hFont;
} WGX_FONT, *PWGX_FONT;

enum {
    WGX_CFG_EMPTY,
    WGX_CFG_COMMENT,
    WGX_CFG_INT,
    WGX_CFG_STRING
};

typedef struct _WGX_OPTION {
    int type;             /* one of WGX_CFG_xxx constants */
    int value_length;     /* length of the value buffer, in bytes (including terminal zero) */
    char *name;           /* option name, NULL indicates end of options table */
    void *value;          /* value buffer */
    void *default_value;  /* default value */
} WGX_OPTION, *PWGX_OPTION;

typedef struct _WGX_MENU {
    UINT flags;                 /* combination of MF_xxx flags (see MSDN for details) */
    UINT id;                    /* menu item identifier */
    struct _WGX_MENU *submenu;  /* pointer to submenu table in case of MF_POPUP */
    wchar_t *text;              /* menu item text in case of MF_STRING */
    int toolbar_image_id;       /* position of the image on the toolbar ( -1 if not used, ignored for separators) */
} WGX_MENU, *PWGX_MENU;

/* wgx routines prototypes */
BOOL WgxAddAccelerators(HINSTANCE hInstance,HWND hWindow,UINT AccelId);

HMENU WgxBuildMenu(WGX_MENU *menu_table,HBITMAP toolbar_bmp);
HMENU WgxBuildPopupMenu(WGX_MENU *menu_table,HBITMAP toolbar_bmp);

HBITMAP WgxCreateMenuBitmapMasked(HBITMAP hBMSrc, COLORREF crTransparent);

/* lines in language files are limited by 8191 characters, which is more than enough */
BOOL WgxBuildResourceTable(PWGX_I18N_RESOURCE_ENTRY table,wchar_t *lng_file_path);
void WgxApplyResourceTable(PWGX_I18N_RESOURCE_ENTRY table,HWND hWindow);
void WgxSetText(HWND hWnd, PWGX_I18N_RESOURCE_ENTRY table, wchar_t *key);
wchar_t *WgxGetResourceString(PWGX_I18N_RESOURCE_ENTRY table,wchar_t *key);
void WgxDestroyResourceTable(PWGX_I18N_RESOURCE_ENTRY table);

void WgxEnableWindows(HANDLE hMainWindow, ...);
void WgxDisableWindows(HANDLE hMainWindow, ...);
void WgxSetIcon(HINSTANCE hInstance,HWND hWindow,UINT IconID);
void WgxCheckWindowCoordinates(LPRECT lprc,int min_width,int min_height);
void WgxCenterWindow(HWND hwnd);
WNDPROC WgxSafeSubclassWindow(HWND hwnd,WNDPROC NewProc);
LRESULT WgxSafeCallWndProc(WNDPROC OldProc,HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

BOOL WgxShellExecuteW(HWND hwnd,LPCWSTR lpOperation,LPCWSTR lpFile,
    LPCWSTR lpParameters,LPCWSTR lpDirectory,INT nShowCmd);

BOOL WgxCreateFont(char *wgx_font_path,PWGX_FONT pFont);
void WgxSetFont(HWND hWnd, PWGX_FONT pFont);
void WgxDestroyFont(PWGX_FONT pFont);
BOOL WgxSaveFont(char *wgx_font_path,PWGX_FONT pFont);

BOOL IncreaseGoogleAnalyticsCounter(char *hostname,char *path,char *account);
/* NOTE: this routine is not safe, avoid its use */
void IncreaseGoogleAnalyticsCounterAsynch(char *hostname,char *path,char *account);

void WgxDbgPrint(char *format, ...);
void WgxDbgPrintLastError(char *format, ...);
int WgxDisplayLastError(HWND hParent,UINT msgbox_flags, char *format, ...);

typedef void (*WGX_SAVE_OPTIONS_CALLBACK)(char *error);

BOOL WgxGetOptions(char *config_file_path,WGX_OPTION *opts_table);
BOOL WgxSaveOptions(char *config_file_path,WGX_OPTION *opts_table,WGX_SAVE_OPTIONS_CALLBACK cb);

#endif /* _WGX_H_ */
