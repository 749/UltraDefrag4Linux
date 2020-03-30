/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
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
 * @file about.c
 * @brief About box.
 * @addtogroup AboutBox
 * @{
 */

#include "main.h"

BOOL CALLBACK AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);

/**
 * @brief Displays about box on the screen.
 */
void AboutBox(void)
{
    HDC hdc;
    int bpp = 32;
    int id;
    
    hdc = GetDC(hWindow);
    if(hdc){
        bpp = GetDeviceCaps(hdc,BITSPIXEL);
        ReleaseDC(hWindow,hdc);
    }
    if(bpp <= 8)
        id = IDD_ABOUT_8_BIT;
    else
        id = IDD_ABOUT;

    if(DialogBoxW(hInstance,MAKEINTRESOURCEW(id),hWindow,(DLGPROC)AboutDlgProc) == (-1))
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot create the About window!");
}

BOOL CALLBACK AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    switch(msg){
        case WM_INITDIALOG:
            /* Window Initialization */
            WgxCenterWindow(hWnd);
            if(use_custom_font_in_dialogs)
                WgxSetFont(hWnd,&wgxFont);
            if(WaitForSingleObject(hLangPackEvent,INFINITE) != WAIT_OBJECT_0){
                WgxDbgPrintLastError("AboutDlgProc: wait on hLangPackEvent failed");
            } else {
                WgxSetText(hWnd,i18n_table,L"ABOUT_WIN_TITLE");
                WgxSetText(GetDlgItem(hWnd,IDC_CREDITS),i18n_table,L"CREDITS");
                WgxSetText(GetDlgItem(hWnd,IDC_LICENSE),i18n_table,L"LICENSE");
                SetEvent(hLangPackEvent);
            }
            (void)WgxAddAccelerators(hInstance,hWnd,IDR_ACCELERATOR2);
            (void)SetFocus(GetDlgItem(hWnd,IDC_HOMEPAGE));
            return FALSE;
        case WM_COMMAND:
            switch(LOWORD(wParam)){
                case IDC_CREDITS:
                    OpenWebPage("Credits.html");
                    break;
                case IDC_LICENSE:
                    (void)WgxShellExecuteW(hWindow,L"open",L".\\LICENSE.TXT",NULL,NULL,SW_SHOW);
                    break;
                case IDC_HOMEPAGE:
                    (void)SetFocus(GetDlgItem(hWnd,IDC_CREDITS));
                    (void)WgxShellExecuteW(hWindow,L"open",L"http://ultradefrag.sourceforge.net",NULL,NULL,SW_SHOW);
            }
            if(LOWORD(wParam) == IDCANCEL){
                (void)EndDialog(hWnd,1);
                return TRUE;
            }
            if(LOWORD(wParam) != IDOK)
                return FALSE;
        case WM_CLOSE:
            (void)EndDialog(hWnd,1);
            return TRUE;
    }
    return FALSE;
}

/** @} */
