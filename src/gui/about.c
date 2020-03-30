/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
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
    wchar_t buffer[256];
    int text1_width, text1_height;
    int text2_width, text2_height;
    int text3_width, text3_height;
    int text_block_width;
    int text_block_height;
    int button1_width = MIN_BTN_WIDTH;
    int button1_height = MIN_BTN_HEIGHT;
    int button2_width = MIN_BTN_WIDTH;
    int button2_height = MIN_BTN_HEIGHT;
    int buttons_block_width;
    int buttons_block_height;
    int buttons_spacing = SMALL_SPACING;
    int client_width, client_height;
    int width, height;
    signed int delta;
    int ship_x, ship_y;
    int text_block_x;
    int text_block_y;
    int buttons_block_x;
    int buttons_block_y;
    BOOL result;
    RECT rc;
    
    /* get dimensions of labels */
    if(!GetWindowTextW(GetDlgItem(hwnd,IDC_TEXT1),buffer,sizeof(buffer)/sizeof(wchar_t))){
        WgxDbgPrint("ResizeAboutDialog: cannot get top label text");
        return;
    }
    buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
    result = WgxGetTextDimensions(buffer,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_TEXT1), &text1_width, &text1_height);
    if(result == FALSE) return;

    if(!GetWindowTextW(GetDlgItem(hwnd,IDC_TEXT2),buffer,sizeof(buffer)/sizeof(wchar_t))){
        WgxDbgPrint("ResizeAboutDialog: cannot get middle label text");
        return;
    }
    buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
    result = WgxGetTextDimensions(buffer,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_TEXT2), &text2_width, &text2_height);
    if(result == FALSE) return;

    if(!GetWindowTextW(GetDlgItem(hwnd,IDC_TEXT3),buffer,sizeof(buffer)/sizeof(wchar_t))){
        WgxDbgPrint("ResizeAboutDialog: cannot get bottom label text");
        return;
    }
    buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
    result = WgxGetTextDimensions(buffer,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_TEXT3), &text3_width, &text3_height);
    if(result == FALSE) return;

    /* calculate dimensions of the entire text block */
    text_block_width = max(text1_width,text2_width);
    text_block_width = max(text_block_width,text3_width);
    text1_width = text2_width = text3_width = text_block_width;
    text_block_height = text1_height + SMALL_SPACING + text2_height + \
        SMALL_SPACING + text3_height;

    /* calculate dimensions of buttons */
    if(!GetWindowTextW(GetDlgItem(hwnd,IDC_CREDITS),buffer,sizeof(buffer)/sizeof(wchar_t))){
        WgxDbgPrint("ResizeAboutDialog: cannot get Credits button text");
        return;
    }
    buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
    result = WgxGetTextDimensions(buffer,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_CREDITS), &width, &height);
    if(result == FALSE) return;
    if(width + 2 * BTN_H_SPACING > button1_width)
        button1_width = width + 2 * BTN_H_SPACING;
    if(height + 2 * BTN_V_SPACING > button1_height)
        button1_height = height + 2 * BTN_V_SPACING;

    if(!GetWindowTextW(GetDlgItem(hwnd,IDC_LICENSE),buffer,sizeof(buffer)/sizeof(wchar_t))){
        WgxDbgPrint("ResizeAboutDialog: cannot get License button text");
        return;
    }
    buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
    result = WgxGetTextDimensions(buffer,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_LICENSE), &width, &height);
    if(result == FALSE) return;
    if(width + 2 * BTN_H_SPACING > button1_width)
        button1_width = width + 2 * BTN_H_SPACING;
    if(height + 2 * BTN_V_SPACING > button1_height)
        button1_height = height + 2 * BTN_V_SPACING;

    if(!GetWindowTextW(GetDlgItem(hwnd,IDC_HOMEPAGE),buffer,sizeof(buffer)/sizeof(wchar_t))){
        WgxDbgPrint("ResizeAboutDialog: cannot get Homepage button text");
        return;
    }
    buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
    result = WgxGetTextDimensions(buffer,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_HOMEPAGE), &width, &height);
    if(result == FALSE) return;
    if(width + 2 * BTN_H_SPACING > button2_width)
        button2_width = width + 2 * BTN_H_SPACING;
    if(height + 2 * BTN_V_SPACING > button2_height)
        button2_height = height + 2 * BTN_V_SPACING;
    
    height = max(button1_height,button2_height);
    button1_height = button2_height = height;
    delta = button2_width - (button1_width * 2 + SMALL_SPACING);
    if(delta > 0){
        /* make top buttons wider */
        button1_width += delta / 2;
        if(delta % 2)
            buttons_spacing ++;
    } else {
        /* make bottom button wider */
        button2_width -= delta;
    }
    
    /* calculate dimensions of the entire block of buttons */
    buttons_block_width = button2_width;
    buttons_block_height = button1_height + SMALL_SPACING + button2_height;
    
    if(buttons_block_width > text_block_width){
        text_block_width = buttons_block_width;
        text1_width = text2_width = text3_width = text_block_width;
    } else {
        delta = text_block_width - buttons_block_width;
        /* make top buttons wider */
        button1_width += delta / 2;
        if(delta % 2)
            buttons_spacing ++;
        /* make bottom button wider */
        button2_width += delta;
        buttons_block_width = text_block_width;
    }
    
    /* calculate dimensions of the entire client area */
    client_width = SHIP_WIDTH + LARGE_SPACING + text_block_width + MARGIN * 2;
    client_height = max(SHIP_HEIGHT,text_block_height + LARGE_SPACING + buttons_block_height) + MARGIN * 2;
    
    rc.left = 0;
    rc.right = client_width;
    rc.top = 0;
    rc.bottom = client_height;
    if(!AdjustWindowRect(&rc,WS_CAPTION | WS_DLGFRAME,FALSE)){
        WgxDbgPrintLastError("ResizeAboutDialog: cannot calculate window dimensions");
        return;
    }
            
    /* resize main window */
    MoveWindow(hwnd,0,0,rc.right - rc.left,rc.bottom - rc.top,FALSE);
            
    /* reposition controls */
    ship_x = MARGIN;
    text_block_x = ship_x + SHIP_WIDTH + LARGE_SPACING;
    buttons_block_x = text_block_x;
    delta = SHIP_HEIGHT - (text_block_height + LARGE_SPACING + buttons_block_height);
    if(delta > 0){
        ship_y = MARGIN;
        text_block_y = MARGIN + delta / 2;
    } else {
        ship_y = MARGIN - delta / 2;
        text_block_y = MARGIN;
    }
    buttons_block_y = text_block_y + text_block_height + LARGE_SPACING;
    
    SetWindowPos(GetDlgItem(hwnd,IDC_SHIP),NULL,
        ship_x,
        ship_y,
        SHIP_WIDTH,
        SHIP_HEIGHT,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_TEXT1),NULL,
        text_block_x,
        text_block_y,
        text1_width,
        text1_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_TEXT2),NULL,
        text_block_x,
        text_block_y + text1_height + SMALL_SPACING,
        text2_width,
        text2_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_TEXT3),NULL,
        text_block_x,
        text_block_y + text1_height + text2_height + SMALL_SPACING * 2,
        text3_width,
        text3_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_CREDITS),NULL,
        buttons_block_x,
        buttons_block_y,
        button1_width,
        button1_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_LICENSE),NULL,
        buttons_block_x + button1_width + buttons_spacing,
        buttons_block_y,
        button1_width,
        button1_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_HOMEPAGE),NULL,
        buttons_block_x,
        buttons_block_y + button1_height + SMALL_SPACING,
        button2_width,
        button2_height,
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
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot create the About window!");
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
