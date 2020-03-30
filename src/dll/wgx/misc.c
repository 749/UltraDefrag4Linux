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
 * @file misc.c
 * @brief Miscellaneous.
 * @addtogroup Misc
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "wgx.h"

/**
 * @brief Enables a child windows.
 * @param[in] hMainWindow handle to the main window.
 * @param[in] ... the list of resource identifiers
 *                of child windows.
 * @note The list of identifiers must be terminated by zero.
 */
void WgxEnableWindows(HANDLE hMainWindow, ...)
{
    va_list marker;
    int id;
    
    va_start(marker,hMainWindow);
    do {
        id = va_arg(marker,int);
        if(id) (void)EnableWindow(GetDlgItem(hMainWindow,id),TRUE);
    } while(id);
    va_end(marker);
}

/**
 * @brief Disables a child windows.
 * @param[in] hMainWindow handle to the main window.
 * @param[in] ... the list of resource identifiers
 *                of child windows.
 * @note The list of identifiers must be terminated by zero.
 */
void WgxDisableWindows(HANDLE hMainWindow, ...)
{
    va_list marker;
    int id;
    
    va_start(marker,hMainWindow);
    do {
        id = va_arg(marker,int);
        if(id) (void)EnableWindow(GetDlgItem(hMainWindow,id),FALSE);
    } while(id);
    va_end(marker);
}

/**
 * @brief Sets a window icon.
 * @param[in] hInstance handle to an instance
 * of the module whose executable file contains
 * the icon to load.
 * @param[in] hWindow handle to the window.
 * @param[in] IconID the resource identifier of the icon.
 */
void WgxSetIcon(HINSTANCE hInstance,HWND hWindow,UINT IconID)
{
    HICON hIcon;

    hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IconID));
    (void)SendMessage(hWindow,WM_SETICON,1,(LRESULT)hIcon);
    if(hIcon) (void)DeleteObject(hIcon);
}

/**
 * @brief Prevents a window to be outside the screen.
 * @param[in,out] lprc pointer to the structure 
 * containing windows coordinates.
 * @param[in] min_width width of the minimal
 * visible part of the window.
 * @param[in] min_height height of the minimal
 * visible part of the window.
 */
void WgxCheckWindowCoordinates(LPRECT lprc,int min_width,int min_height)
{
    int cx,cy;

    cx = GetSystemMetrics(SM_CXSCREEN);
    cy = GetSystemMetrics(SM_CYSCREEN);
    if(lprc->left < 0) lprc->left = 0; if(lprc->top < 0) lprc->top = 0;
    if(lprc->left >= (cx - min_width)) lprc->left = cx - min_width;
    if(lprc->top >= (cy - min_height)) lprc->top = cy - min_height;
}

/**
 * @brief Centers window over its parent.
 * @note Based on the public domain code:
 * http://www.catch22.net/tuts/tips#CenterWindow
 */
void WgxCenterWindow(HWND hwnd)
{
    HWND hwndParent;
    RECT rect, rectP;
    int width, height;      
    int screenwidth, screenheight;
    int x, y;

    //make the window relative to its parent
    hwndParent = GetParent(hwnd);
    if(hwndParent == NULL)
        hwndParent = GetDesktopWindow();

    GetWindowRect(hwnd, &rect);
    GetWindowRect(hwndParent, &rectP);

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    x = ((rectP.right-rectP.left) - width) / 2 + rectP.left;
    y = ((rectP.bottom-rectP.top) - height) / 2 + rectP.top;

    screenwidth = GetSystemMetrics(SM_CXSCREEN);
    screenheight = GetSystemMetrics(SM_CYSCREEN);

    //make sure that the dialog box never moves outside of
    //the screen
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x + width > screenwidth) x = screenwidth - width;
    if(y + height > screenheight) y = screenheight - height;

    MoveWindow(hwnd, x, y, width, height, FALSE);
}

/**
 * @brief Safe equivalent of SetWindowLongPtr(GWLP_WNDPROC).
 * @details Works safe regardless of whether the window
 * is unicode or not.
 */
WNDPROC WgxSafeSubclassWindow(HWND hwnd,WNDPROC NewProc)
{
    if(IsWindowUnicode(hwnd))
        return (WNDPROC)SetWindowLongPtrW(hwnd,GWLP_WNDPROC,(LONG_PTR)NewProc);
    else
        return (WNDPROC)SetWindowLongPtrA(hwnd,GWLP_WNDPROC,(LONG_PTR)NewProc);
}

/**
 * @brief Safe equivalent of CallWindowProc.
 * @details Works safe regardless of whether
 * the window is unicode or not.
 */
LRESULT WgxSafeCallWndProc(WNDPROC OldProc,HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    if(IsWindowUnicode(hwnd))
        return CallWindowProcW(OldProc,hwnd,msg,wParam,lParam);
    else
        return CallWindowProcA(OldProc,hwnd,msg,wParam,lParam);
}

/** @} */
