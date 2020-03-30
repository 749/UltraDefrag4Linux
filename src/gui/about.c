/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
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

/**
 * @file about.c
 * @brief About box.
 * @addtogroup AboutBox
 * @{
 */

#include "main.h"

BOOL CALLBACK AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);

/**
 * @brief Adjusts position of controls inside
 * the about box to make it nice looking 
 * independent from text lengths and font size.
 */
static void ResizeAboutDialog(HWND hwnd)
{
    int text1_width, text1_height;
    int text2_width, text2_height;
    int text3_width, text3_height;
    int btn1_width, btn1_height;
    int btn2_width, btn2_height;
    int btn3_width, btn3_height;
    int buttons_spacing = SMALL_SPACING;
    int width, height;
    int block_height, delta;
    int client_width, client_height;
    int ship_x, ship_y;
    int block_x, block_y;
    HFONT hFont;
    RECT rc;
    
    /* get dimensions of controls */
    hFont = use_custom_font_in_dialogs ? wgxFont.hFont : NULL;
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_TEXT1),
        hFont,&text1_width, &text1_height)) return;
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_TEXT2),
        hFont,&text2_width, &text2_height)) return;
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_TEXT3),
        hFont,&text3_width, &text3_height)) return;
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_CREDITS),
        hFont,&btn1_width, &btn1_height)) return;
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_LICENSE),
        hFont,&btn2_width, &btn2_height)) return;
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_HOMEPAGE),
        hFont,&btn3_width, &btn3_height)) return;
    
    /* adjust dimensions of buttons */
    width = max(btn1_width,btn2_width);
    btn1_width = btn2_width = max(width,MIN_BTN_WIDTH);
    height = max(btn1_height,btn2_height);
    height = max(height,btn3_height);
    height = max(height,MIN_BTN_HEIGHT);
    btn1_height = btn2_height = btn3_height = height;
    
    /* adjust controls */
    width = max(text1_width,text2_width);
    width = max(width,text3_width);
    width = max(width,btn1_width * 2 + buttons_spacing);
    width = max(width,btn3_width);
    text1_width = text2_width = width;
    text3_width = btn3_width = width;
    btn1_width = btn2_width = (width - SMALL_SPACING) / 2;
    if((width - SMALL_SPACING) % 2) buttons_spacing ++;
    
    /* calculate dimensions of the entire client area */
    height = text1_height + text2_height + text3_height + 2 * SMALL_SPACING;
    block_height = height + LARGE_SPACING + btn1_height * 2 + SMALL_SPACING;
    client_width = SHIP_WIDTH + LARGE_SPACING + text1_width + MARGIN * 2;
    client_height = max(SHIP_HEIGHT,block_height) + MARGIN * 2;

    /* resize main window */
    rc.left = 0, rc.right = client_width;
    rc.top = 0, rc.bottom = client_height;
    if(!AdjustWindowRect(&rc,WS_CAPTION | WS_DLGFRAME,FALSE)){
        letrace("cannot calculate window dimensions");
        return;
    }
    MoveWindow(hwnd,0,0,rc.right - rc.left,rc.bottom - rc.top,FALSE);

    /* reposition controls */
    ship_x = MARGIN;
    block_x = ship_x + SHIP_WIDTH + LARGE_SPACING;
    delta = SHIP_HEIGHT - block_height;
    if(delta > 0){
        ship_y = MARGIN;
        block_y = MARGIN + delta / 2;
    } else {
        ship_y = MARGIN - delta / 2;
        block_y = MARGIN;
    }
    
    SetWindowPos(GetDlgItem(hwnd,IDC_SHIP),NULL,
        ship_x,ship_y,SHIP_WIDTH,SHIP_HEIGHT,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_TEXT1),NULL,
        block_x,block_y,text1_width,text1_height,
        SWP_NOZORDER);
    block_y += text1_height + SMALL_SPACING;
    SetWindowPos(GetDlgItem(hwnd,IDC_TEXT2),NULL,
        block_x,block_y,text2_width,text2_height,
        SWP_NOZORDER);
    block_y += text2_height + SMALL_SPACING;
    SetWindowPos(GetDlgItem(hwnd,IDC_TEXT3),NULL,
        block_x,block_y,text3_width,text3_height,
        SWP_NOZORDER);
    block_y += text3_height + LARGE_SPACING;
    SetWindowPos(GetDlgItem(hwnd,IDC_CREDITS),NULL,
        block_x,block_y,btn1_width,btn1_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_LICENSE),NULL,
        block_x + btn1_width + buttons_spacing,
        block_y,btn2_width,btn2_height,
        SWP_NOZORDER);
    block_y += btn1_height + SMALL_SPACING;
    SetWindowPos(GetDlgItem(hwnd,IDC_HOMEPAGE),NULL,
        block_x,block_y,btn3_width,btn3_height,
        SWP_NOZORDER);
    
    /* center window over its parent */
    WgxCenterWindow(hwnd);
}

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
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,L"Cannot create the About window!");
}

BOOL CALLBACK AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    switch(msg){
        case WM_INITDIALOG:
            /* Window Initialization */
            if(use_custom_font_in_dialogs)
                WgxSetFont(hWnd,&wgxFont);
            WgxSetText(hWnd,i18n_table,"ABOUT_WIN_TITLE");
            WgxSetText(GetDlgItem(hWnd,IDC_CREDITS),i18n_table,"CREDITS");
            WgxSetText(GetDlgItem(hWnd,IDC_LICENSE),i18n_table,"LICENSE");
            (void)WgxAddAccelerators(hInstance,hWnd,IDR_ACCELERATOR2);
            /* adjust positions of controls to make the window nice looking */
            ResizeAboutDialog(hWnd);
            (void)SetFocus(GetDlgItem(hWnd,IDC_HOMEPAGE));
            return FALSE;
        case WM_COMMAND:
            switch(LOWORD(wParam)){
                case IDC_CREDITS:
                    OpenWebPage("Credits.html", NULL);
                    break;
                case IDC_LICENSE:
                    (void)WgxShellExecute(hWindow,L"open",L".\\LICENSE.TXT",
                        NULL,NULL,SW_SHOW,WSH_ALLOW_DEFAULT_ACTION);
                    break;
                case IDC_HOMEPAGE:
                    (void)SetFocus(GetDlgItem(hWnd,IDC_CREDITS));
                    (void)WgxShellExecute(hWindow,L"open",
                        L"http://ultradefrag.sourceforge.net",
                        NULL,NULL,SW_SHOW,WSH_ALLOW_DEFAULT_ACTION);
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
