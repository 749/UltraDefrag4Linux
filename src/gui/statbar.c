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
 * @file statbar.c
 * @brief Status bar.
 * @addtogroup StatusBar
 * @{
 */

#include "main.h"

#define STATUS_BAR_PARTS  5

HWND hStatus;

static void SetIcon(int part,int id)
{
    HANDLE hImg;

    hImg = LoadImage(hInstance,(LPCSTR)(size_t)id,IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
    (void)SendMessage(hStatus,SB_SETICON,part,(LPARAM)hImg);
    (void)DestroyIcon(hImg);
}

/**
 * @brief Creates status bar.
 */
void CreateStatusBar(void)
{
    int array[STATUS_BAR_PARTS] = {10,20,30,40,50};
    
    /* create status bar and attach it to the main window */
    hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_BORDER, \
                                 "0 dirs", hWindow, IDM_STATUSBAR);
    if(hStatus == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot create status bar control!");
        return;
    }
    
    /* split status bar to parts */
    (void)SendMessage(hStatus, SB_SETPARTS, STATUS_BAR_PARTS, (LPARAM)array);
    
    /* set icons */
    (void)SetIcon(0,IDI_DIR);
    (void)SetIcon(1,IDI_UNFRAGM);
    (void)SetIcon(2,IDI_FRAGM);
    (void)SetIcon(3,IDI_CMP);
    (void)SetIcon(4,IDI_MFT);
}

/**
 * @brief Resizes status bar.
 * @note Accepts y coordinate of the bottom line
 * and width of the status bar, returns height
 * of the status bar.
 */
int ResizeStatusBar(int bottom, int width)
{
    RECT rc;
    int top;
    int x[STATUS_BAR_PARTS-1] = {110,210,345,465};
    int a[STATUS_BAR_PARTS];
    int i, height = 0;
    
    if(hStatus == NULL)
        return 0;

    if(GetClientRect(hStatus,&rc)){
        if(MapWindowPoints(hStatus,hWindow,(LPPOINT)(PRECT)(&rc),(sizeof(RECT)/sizeof(POINT))))
            height = rc.bottom - rc.top;
    }
    
    if(height == 0)
        return 0;

    top = bottom - height;
    (void)SetWindowPos(hStatus,NULL,0,top,width,height,0);

    /* adjust widths of parts */
    for(i = 0; i < STATUS_BAR_PARTS-1; i++)
        a[i] = width * x[i] / 560;
    a[STATUS_BAR_PARTS-1] = width;
    (void)SendMessage(hStatus,SB_SETPARTS,STATUS_BAR_PARTS,(LPARAM)a);

    return height;
}

/**
 * @brief Updates status bar.
 */
void UpdateStatusBar(udefrag_progress_info *pi)
{
    char s[32];
    #define BFSIZE 128
    short bf[BFSIZE];

    if(!hStatus) return;

    if(WaitForSingleObject(hLangPackEvent,INFINITE) != WAIT_OBJECT_0){
        WgxDbgPrintLastError("UpdateStatusBar: wait on hLangPackEvent failed");
        return;
    }
    
    (void)_snwprintf(bf,BFSIZE - 1,L"%lu %s",pi->directories,
            WgxGetResourceString(i18n_table,L"DIRS"));
    bf[BFSIZE - 1] = 0;
    (void)SendMessage(hStatus,SB_SETTEXTW,0,(LPARAM)bf);

    (void)_snwprintf(bf,BFSIZE - 1,L"%lu %s",pi->files,
            WgxGetResourceString(i18n_table,L"FILES"));
    bf[BFSIZE - 1] = 0;
    (void)SendMessage(hStatus,SB_SETTEXTW,1,(LPARAM)bf);

    (void)_snwprintf(bf,BFSIZE - 1,L"%lu %s",pi->fragmented,
            WgxGetResourceString(i18n_table,L"FRAGMENTED"));
    bf[BFSIZE - 1] = 0;
    (void)SendMessage(hStatus,SB_SETTEXTW,2,(LPARAM)bf);

    (void)_snwprintf(bf,BFSIZE - 1,L"%lu %s",pi->compressed,
            WgxGetResourceString(i18n_table,L"COMPRESSED"));
    bf[BFSIZE - 1] = 0;
    (void)SendMessage(hStatus,SB_SETTEXTW,3,(LPARAM)bf);

    (void)udefrag_bytes_to_hr(pi->mft_size,2,s,sizeof(s));
    (void)_snwprintf(bf,BFSIZE - 1,L"%S %s",s,
            WgxGetResourceString(i18n_table,L"MFT"));
    bf[BFSIZE - 1] = 0;
    (void)SendMessage(hStatus,SB_SETTEXTW,4,(LPARAM)bf);

    SetEvent(hLangPackEvent);
}

/** @} */
