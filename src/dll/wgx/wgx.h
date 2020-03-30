/*
 *  WGX - Windows GUI Extended Library.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
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

#include "../../include/dbg.h"

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

/* wgx routines prototypes */
/* accel.c */
BOOL WgxAddAccelerators(HINSTANCE hInstance,HWND hWindow,UINT AccelId);

/* config.c */
enum {
    WGX_CFG_EMPTY,
    WGX_CFG_COMMENT,
    WGX_CFG_INT,
    WGX_CFG_STRING
};

typedef struct _WGX_OPTION {
    char *name;           /* the option name, NULL indicates the end of the table */
    int type;             /* one of the WGX_CFG_xxx constants */
    union {
        int *number;      /* the buffer for the WGX_CFG_INT value */
        char *string;     /* the buffer for the WGX_CFG_STRING value */
    };
    int string_length;    /* length of the string buffer (including terminal zero) */
    union {
        int default_number;   /* the default numeric value */
        char *default_string; /* the default string value */
    };
} WGX_OPTION, *PWGX_OPTION;

typedef void (*WGX_SAVE_OPTIONS_CALLBACK)(char *error);
BOOL WgxGetOptions(char *path,WGX_OPTION *table);
BOOL WgxSaveOptions(char *path,WGX_OPTION *table,WGX_SAVE_OPTIONS_CALLBACK cb);

/* console.c */
void WgxPrintUnicodeString(wchar_t *string,FILE *f);

/* dbg.c */
/* debugging macros */

/* set it, otherwise warnings will be shown for the tracing macros */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Waddress"
#else
#pragma warning(disable: 4550)
#endif

/* prints whatever specified */
#define trace(format,...)   { if(WgxTraceHandler) WgxTraceHandler(0,format,## __VA_ARGS__); }

/* prints {prefix}{function name}: {specified string} */
#define etrace(format,...)  { if(WgxTraceHandler) WgxTraceHandler(0,E "%s: " format,__FUNCTION__,## __VA_ARGS__); }
#define itrace(format,...)  { if(WgxTraceHandler) WgxTraceHandler(0,I "%s: " format,__FUNCTION__,## __VA_ARGS__); }
#define dtrace(format,...)  { if(WgxTraceHandler) WgxTraceHandler(0,D "%s: " format,__FUNCTION__,## __VA_ARGS__); }

/* prints {error prefix}{function name}: {specified string}: {last error and its description} */
#define letrace(format,...) { if(WgxTraceHandler) WgxTraceHandler(LAST_ERROR_FLAG,E "%s: " format,__FUNCTION__,## __VA_ARGS__); }

/* prints {error prefix}{function name}: not enough memory */
#define mtrace() etrace("not enough memory")

/* flags are defined in ../../include/dbg.h file */
typedef void (*WGX_TRACE_HANDLER)(int flags,char *format, ...);
void WgxSetInternalTraceHandler(WGX_TRACE_HANDLER h);

int WgxDisplayLastError(HWND hParent,UINT msgbox_flags, wchar_t *format, ...);

/* exec.c */
BOOL WgxCreateProcess(char *cmd,char *args);
BOOL WgxCreateThread(LPTHREAD_START_ROUTINE routine,LPVOID param);
BOOL WgxSetProcessPriority(DWORD priority_class);
BOOL WgxCheckAdminRights(void);

/* font.c */
typedef struct _WGX_FONT {
    LOGFONT lf;
    HFONT hFont;
} WGX_FONT, *PWGX_FONT;

BOOL WgxCreateFont(char *wgx_font_path,PWGX_FONT pFont);
void WgxSetFont(HWND hWnd, PWGX_FONT pFont);
void WgxDestroyFont(PWGX_FONT pFont);
BOOL WgxSaveFont(char *wgx_font_path,PWGX_FONT pFont);

/* i18n.c */
typedef struct _WGX_I18N_RESOURCE_ENTRY {
    int ControlID;
    char *Key;
    wchar_t *DefaultString;
    wchar_t *LoadedString;
} WGX_I18N_RESOURCE_ENTRY, *PWGX_I18N_RESOURCE_ENTRY;

BOOL WgxBuildResourceTable(PWGX_I18N_RESOURCE_ENTRY table,char *path);
void WgxApplyResourceTable(PWGX_I18N_RESOURCE_ENTRY table,HWND hWindow);
void WgxSetText(HWND hWnd, PWGX_I18N_RESOURCE_ENTRY table, char *key);
wchar_t *WgxGetResourceString(PWGX_I18N_RESOURCE_ENTRY table,char *key);
void WgxDestroyResourceTable(PWGX_I18N_RESOURCE_ENTRY table);

/* menu.c */
typedef struct _WGX_MENU {
    UINT flags;                 /* combination of MF_xxx flags (see MSDN for details) */
    UINT id;                    /* menu item identifier */
    struct _WGX_MENU *submenu;  /* pointer to submenu table in case of MF_POPUP */
    wchar_t *text;              /* menu item text in case of MF_STRING */
    int toolbar_image_id;       /* position of the image on the toolbar ( -1 if not used, ignored for separators) */
} WGX_MENU, *PWGX_MENU;

HMENU WgxBuildMenu(WGX_MENU *menu_table,HBITMAP bitmap);
HMENU WgxBuildPopupMenu(WGX_MENU *menu_table,HBITMAP bitmap);
HBITMAP WgxCreateMenuBitmapMasked(HBITMAP hSrc,COLORREF crTransparent);

/* misc.c */
void WgxEnableWindows(HANDLE hMainWindow, ...);
void WgxDisableWindows(HANDLE hMainWindow, ...);
BOOL WgxLoadIcon(HINSTANCE hInstance,UINT IconID,UINT size,HICON *phIcon);
void WgxSetIcon(HINSTANCE hInstance,HWND hWindow,UINT IconID);
void WgxCheckWindowCoordinates(LPRECT lprc,int min_width,int min_height);
void WgxCenterWindow(HWND hwnd);
BOOL WgxGetTextDimensions(wchar_t *text,HFONT hFont,HWND hWnd,int *pWidth,int *pHeight);
BOOL WgxGetControlDimensions(HWND hControl,HFONT hFont,int *pWidth,int *pHeight);
WNDPROC WgxSafeSubclassWindow(HWND hwnd,WNDPROC NewProc);
LRESULT WgxSafeCallWndProc(WNDPROC OldProc,HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

/* shell.c */
/* flags for WgxShellExecute */
#define WSH_SILENT               0x1 /* don't show message box in case of errors */
#define WSH_NOASYNC              0x2 /* wait till the request completes; useful when the calling thread terminates quickly */
#define WSH_ALLOW_DEFAULT_ACTION 0x4 /* try the default action in case of errors */

BOOL WgxShellExecute(HWND hwnd,LPCWSTR lpOperation,LPCWSTR lpFile,
    LPCWSTR lpParameters,LPCWSTR lpDirectory,INT nShowCmd,INT nFlags);

/* string.c */
char *wgx_vsprintf(const char *format,va_list arg);
char *wgx_sprintf(const char *format, ...);
wchar_t *wgx_vswprintf(const wchar_t *format,va_list arg);
wchar_t *wgx_swprintf(const wchar_t *format, ...);

/* taskbar.c */
typedef enum {
    TBPF_NOPROGRESS	= 0,
    TBPF_INDETERMINATE	= 0x1,
    TBPF_NORMAL	= 0x2,
    TBPF_ERROR	= 0x4,
    TBPF_PAUSED	= 0x8
} TBPFLAG;

BOOL WgxSetTaskbarIconOverlay(HWND hWindow,
    HINSTANCE hInstance,int resource_id,
    wchar_t *description);
BOOL WgxRemoveTaskbarIconOverlay(HWND hWindow);
BOOL WgxSetTaskbarProgressState(HWND hWindow,TBPFLAG flag);
BOOL WgxSetTaskbarProgressValue(HWND hWindow,ULONGLONG completed,ULONGLONG total);

/* web-analytics.c */
BOOL IncreaseGoogleAnalyticsCounter(char *hostname,char *path,char *account);

/* wgx macro definitions */

/*
* The following two definitions are intended
* for use instead of ShowWindow freakishly
* depending on STARTUPINFO structure.
*/
#define WgxShowWindow(hWindow) SetWindowPos((hWindow), \
    NULL,0,0,0,0,SWP_SHOWWINDOW | SWP_NOACTIVATE | \
    SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER)
#define WgxHideWindow(hWindow) SetWindowPos((hWindow), \
    NULL,0,0,0,0,SWP_HIDEWINDOW | SWP_NOACTIVATE | \
    SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER)

#endif /* _WGX_H_ */
