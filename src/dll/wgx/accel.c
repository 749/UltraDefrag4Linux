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
 * @file accel.c
 * @brief Accelerators.
 * @addtogroup Accelerators
 * @{
 */

/*
* An article of Mumtaz Zaheer from Pakistan helped us very much
* to make a valid subclassing:
* http://www.codeproject.com/KB/winsdk/safesubclassing.aspx
*/

#define WIN32_NO_STATUS
#include <windows.h>

#include "wgx.h"

/* 1024 child windows are more than enough for any serious application */
#define WIN_ARRAY_SIZE 1024

typedef struct _CHILD_WINDOW {
    HWND hWindow;
    WNDPROC OldWindowProcedure;
    HACCEL hAccelerator;
    HWND hMainWindow;
    BOOL isWindowUnicode;
} CHILD_WINDOW, *PCHILD_WINDOW;

CHILD_WINDOW win[WIN_ARRAY_SIZE] = {{0}};
int idx = 0;
BOOL first_call = TRUE;
BOOL error_flag = FALSE;

LRESULT CALLBACK NewWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    int i, found = 0;
    MSG message;

    /* search for our window in win array */
    for(i = 0; i <= idx; i++){
        if(win[i].hWindow == hWnd){
            found = 1; break;
        }
    }
    
    if(found){
        if(iMsg == WM_KEYDOWN){
            message.hwnd = hWnd;
            message.message = iMsg;
            message.wParam = wParam;
            message.lParam = lParam;
            message.pt.x = message.pt.y = 0;
            message.time = 0;
            /* if we'll receive a report about improper accelerators work
               we may use TranslateAcceleratorW to solve the problem;
               but currently everything seems to be working fine */
            (void)TranslateAccelerator(win[i].hMainWindow,win[i].hAccelerator,&message);
        }
        if(win[i].isWindowUnicode)
            return CallWindowProcW(win[i].OldWindowProcedure,hWnd,iMsg,wParam,lParam);
        else
            return CallWindowProc(win[i].OldWindowProcedure,hWnd,iMsg,wParam,lParam);
    }
    /* very extraordinary situation */
    if(!error_flag){ /* show message box once */
        error_flag = TRUE;
        MessageBox(NULL,"OldWindowProcedure is lost!","Error!",MB_OK | MB_ICONHAND);
    }
    return 0;
}

/**
 * @brief Loads an accelerator table and applies
 * them to a single window and all its children.
 * @param[in] hInstance handle to an instance
 * of the module whose executable file contains
 * the accelerator table to load.
 * @param[in] hWindow handle to the window.
 * @param[in] AccelId resource identifier
 * of the accelerator table.
 * @return Boolean value. TRUE indicates success.
 */
BOOL WgxAddAccelerators(HINSTANCE hInstance,HWND hWindow,UINT AccelId)
{
    HACCEL hAccel;
    HANDLE hChild;
    WNDPROC OldWndProc;
    BOOL isWindowUnicode;
    
    /* Load the accelerator table. */
    hAccel = LoadAccelerators(hInstance,MAKEINTRESOURCE(AccelId));
    if(!hAccel) return FALSE;

    if(first_call){
        memset(win,0,sizeof(win));
        idx = 0;
        first_call = FALSE;
    }
    
    /* No need to set accelerator for the main window. */
    /* Set accelerator for children. */
    hChild = GetWindow(hWindow,GW_CHILD);
    while(hChild){
        if(idx >= (WIN_ARRAY_SIZE - 1))
            return FALSE; /* too many child windows */

        if(IsWindowUnicode(hChild)) isWindowUnicode = TRUE;
        else isWindowUnicode = FALSE;

        if(isWindowUnicode)
            OldWndProc = (WNDPROC)SetWindowLongPtrW(hChild,GWLP_WNDPROC,(LONG_PTR)NewWndProc);
        else
            OldWndProc = (WNDPROC)SetWindowLongPtr(hChild,GWLP_WNDPROC,(LONG_PTR)NewWndProc);

        if(OldWndProc){
            win[idx].hWindow = hChild;
            win[idx].OldWindowProcedure = OldWndProc;
            win[idx].hAccelerator = hAccel;
            win[idx].hMainWindow = hWindow;
            win[idx].isWindowUnicode = isWindowUnicode;
            idx ++;
        }
        hChild = GetWindow(hChild,GW_HWNDNEXT);
    }
    
    return TRUE;
}

/** @} */
