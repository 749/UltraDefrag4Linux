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

/**
 * @file misc.c
 * @brief Miscellaneous.
 * @addtogroup Misc
 * @{
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "wgx.h"

enum {
   LIM_SMALL, // corresponds to SM_CXSMICON/SM_CYSMICON
   LIM_LARGE, // corresponds to SM_CXICON/SM_CYICON
};
typedef HRESULT (WINAPI *LOAD_ICON_METRIC_PROC)(HINSTANCE hinst,PCWSTR pszName,int lims,HICON *phico);

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
 * @brief Loads an icon of the specified
 * size from the specified resource.
 * @param[in] hInstance handle to an instance
 * of the module whose executable file contains
 * the icon to load.
 * @param[in] IconID the resource identifier of the icon.
 * @param[in] size size of the icon to be loaded.
 * @param[out] phIcon pointer to the variable receiving
 * the handle of the loaded icon.
 * @return Boolean value. TRUE indicates success.
 */
BOOL WgxLoadIcon(HINSTANCE hInstance,UINT IconID,UINT size,HICON *phIcon)
{
    LOAD_ICON_METRIC_PROC pLoadIconMetric;
    HMODULE hLib;
    HRESULT hr;
    int is_standard_icon_size = 0;
    int lims = 0;
    HICON ScaledIcon;

    /* validate parameters */
    if(size == 0 || phIcon == NULL) return FALSE;
    *phIcon = NULL;
   
    /* do we need to load a standard small/large icon? */
    if(size == GetSystemMetrics(SM_CXSMICON)){
        is_standard_icon_size = 1, lims = LIM_SMALL;
    } else if(size == GetSystemMetrics(SM_CXICON)){
        is_standard_icon_size = 1, lims = LIM_LARGE;
    }
    
    /*
    * On Windows Vista and more recent Windows
    * editions an excellent LoadIconMetric routine
    * exists.
    */
    if(is_standard_icon_size){
        hLib = LoadLibrary("comctl32.dll");
        if(hLib == NULL){
            WgxDbgPrintLastError("WgxLoadIcon: cannot load comctl32.dll library");
        } else {
            pLoadIconMetric = (LOAD_ICON_METRIC_PROC)GetProcAddress(hLib,"LoadIconMetric");
            if(pLoadIconMetric == NULL){
                WgxDbgPrintLastError("WgxLoadIcon: LoadIconMetric procedure not found in comctl32.dll library");
            } else {
                hr = pLoadIconMetric(hInstance,MAKEINTRESOURCEW(IconID),lims,phIcon);
                if(hr != S_OK || *phIcon == NULL){
                    WgxDbgPrint("WgxLoadIcon: LoadIconMetric failed with code 0x%x",(UINT)hr);
                } else {
                    return TRUE;
                }
            }
        }
    }
    
    /*
    * LoadIconMetric routine emulation starts here.
    * Try to load icon of exact size first.
    */
    *phIcon = (HICON)LoadImage(hInstance,
        MAKEINTRESOURCE(IconID),
        IMAGE_ICON,size,size,
        LR_DEFAULTCOLOR);
    if(*phIcon == NULL){
        WgxDbgPrintLastError("WgxLoadIcon: LoadImage failed (case 1)");
    } else {
        return TRUE;
    }
    
    /*
    * Icon of the specified size
    * not found, so try to scale
    * a standard large icon.
    */
    *phIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IconID));
    if(*phIcon == NULL){
        WgxDbgPrintLastError("WgxLoadIcon: LoadIcon failed");
    } else {
        ScaledIcon = (HICON)CopyImage((HANDLE)*phIcon,
            IMAGE_ICON,size,size,0);
        if(ScaledIcon == NULL){
            WgxDbgPrintLastError("WgxLoadIcon: CopyImage failed (case 1)");
            DestroyIcon(*phIcon);
            *phIcon = NULL;
        } else {
            DestroyIcon(*phIcon);
            *phIcon = ScaledIcon;
            return TRUE;
        }
    }

    /*
    * A standard large icon does not exist,
    * so try to scale at least something.
    */
    *phIcon = (HICON)LoadImage(hInstance,
        MAKEINTRESOURCE(IconID),
        IMAGE_ICON,0,0,
        LR_DEFAULTCOLOR);
    if(*phIcon == NULL){
        WgxDbgPrintLastError("WgxLoadIcon: LoadImage failed (case 2)");
    } else {
        ScaledIcon = (HICON)CopyImage((HANDLE)*phIcon,
            IMAGE_ICON,size,size,0);
        if(ScaledIcon == NULL){
            WgxDbgPrintLastError("WgxLoadIcon: CopyImage failed (case 2)");
            DestroyIcon(*phIcon);
            *phIcon = NULL;
            return FALSE;
        } else {
            DestroyIcon(*phIcon);
            *phIcon = ScaledIcon;
        }
    }
    return TRUE;
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
 * @brief Retrieves dimensions of text.
 * @param[in] text the text.
 * @param[in] hFont handle to the font
 * to be used to draw the text, NULL
 * forces to use default font.
 * @param[in] hWnd handle to the window
 * to be used to draw the text on.
 * @param[out] pWidth pointer to variable
 * receiving width of the text.
 * @param[out] pHeight pointer to variable
 * receiving height of the text.
 * @return Boolean value. TRUE indicates
 * success.
 */
BOOL WgxGetTextDimensions(wchar_t *text,HFONT hFont,HWND hWnd,int *pWidth,int *pHeight)
{
    HDC hdc;
    HFONT hOldFont;
    SIZE size;
    BOOL result;
    
    /* validate parameters */
    if(pWidth == NULL || pHeight == NULL) return FALSE;
    *pWidth = *pHeight = 0;
    
    if(text == NULL)
        return TRUE;
    
    /* get default font if requested */
    if(hFont == NULL){
        hFont = (HFONT)SendMessage(hWnd,WM_GETFONT,0,0);
        if(hFont == NULL){
            WgxDbgPrintLastError("WgxGetTextDimensions: cannot get default font");
            return FALSE;
        }
    }
    
    /* get dimensions of text */
    hdc = GetDC(hWnd);
    if(hdc == NULL){
        WgxDbgPrintLastError("WgxGetTextDimensions: cannot get device context of the window");
        return FALSE;
    }
    hOldFont = SelectObject(hdc,hFont);
    result = GetTextExtentPoint32W(hdc,text,wcslen(text),&size);
    if(result == FALSE){
        WgxDbgPrintLastError("WgxGetTextDimensions: cannot get text dimensions");
    } else {
        *pWidth = size.cx;
        *pHeight = size.cy;
    }
    SelectObject(hdc,hOldFont);
    ReleaseDC(hWnd,hdc);
    return result;
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
