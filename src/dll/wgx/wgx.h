/*
 *  WGX - Windows GUI Extended Library.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
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

#ifndef LR_VGACOLOR
/* this constant is not defined in winuser.h on mingw */
#define LR_VGACOLOR         0x0080
#endif

/* wgx structures */
typedef struct _WGX_I18N_RESOURCE_ENTRY {
    int ControlID;
    char *Key;
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

HMENU WgxBuildMenu(WGX_MENU *menu_table,HBITMAP bitmap);
HMENU WgxBuildPopupMenu(WGX_MENU *menu_table,HBITMAP bitmap);

HBITMAP WgxCreateMenuBitmapMasked(HBITMAP hSrc,COLORREF crTransparent);

BOOL WgxBuildResourceTable(PWGX_I18N_RESOURCE_ENTRY table,char *path);
void WgxApplyResourceTable(PWGX_I18N_RESOURCE_ENTRY table,HWND hWindow);
void WgxSetText(HWND hWnd, PWGX_I18N_RESOURCE_ENTRY table, char *key);
wchar_t *WgxGetResourceString(PWGX_I18N_RESOURCE_ENTRY table,char *key);
void WgxDestroyResourceTable(PWGX_I18N_RESOURCE_ENTRY table);

void WgxEnableWindows(HANDLE hMainWindow, ...);
void WgxDisableWindows(HANDLE hMainWindow, ...);
BOOL WgxLoadIcon(HINSTANCE hInstance,UINT IconID,UINT size,HICON *phIcon);
void WgxSetIcon(HINSTANCE hInstance,HWND hWindow,UINT IconID);
void WgxCheckWindowCoordinates(LPRECT lprc,int min_width,int min_height);
void WgxCenterWindow(HWND hwnd);
BOOL WgxGetTextDimensions(wchar_t *text,HFONT hFont,HWND hWnd,int *pWidth,int *pHeight);
WNDPROC WgxSafeSubclassWindow(HWND hwnd,WNDPROC NewProc);
LRESULT WgxSafeCallWndProc(WNDPROC OldProc,HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

BOOL WgxShellExecuteW(HWND hwnd,LPCWSTR lpOperation,LPCWSTR lpFile,
    LPCWSTR lpParameters,LPCWSTR lpDirectory,INT nShowCmd);

BOOL WgxCreateFont(char *wgx_font_path,PWGX_FONT pFont);
void WgxSetFont(HWND hWnd, PWGX_FONT pFont);
void WgxDestroyFont(PWGX_FONT pFont);
BOOL WgxSaveFont(char *wgx_font_path,PWGX_FONT pFont);

BOOL IncreaseGoogleAnalyticsCounter(char *hostname,char *path,char *account);

typedef void (*WGX_DBG_PRINT_HANDLER)(char *format, ...);
void WgxSetDbgPrintHandler(WGX_DBG_PRINT_HANDLER h);
void WgxDbgPrint(char *format, ...);
void WgxDbgPrintLastError(char *format, ...);
int WgxDisplayLastError(HWND hParent,UINT msgbox_flags, char *format, ...);

typedef void (*WGX_SAVE_OPTIONS_CALLBACK)(char *error);

BOOL WgxGetOptions(char *path,WGX_OPTION *table);
BOOL WgxSaveOptions(char *path,WGX_OPTION *table,WGX_SAVE_OPTIONS_CALLBACK cb);

BOOL WgxSetTaskbarIconOverlay(HWND hWindow,HINSTANCE hInstance,int resource_id, wchar_t *description);
BOOL WgxRemoveTaskbarIconOverlay(HWND hWindow);

BOOL WgxCreateProcess(char *cmd,char *args);
BOOL WgxCreateThread(LPTHREAD_START_ROUTINE routine,LPVOID param);
BOOL WgxCheckAdminRights(void);

void WgxPrintUnicodeString(wchar_t *string,FILE *f);

#endif /* _WGX_H_ */
