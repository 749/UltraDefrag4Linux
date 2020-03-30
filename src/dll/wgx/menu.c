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
 * @file menu.c
 * @brief Menu.
 * @addtogroup Menu
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>

#include "wgx.h"

/**
 * @brief Masks bitmaps.
 * @param[in] hBMSrc handle to the bitmap.
 * @param[in] crTransparent transparent color. If set to
 * (COLORREF)-1 the color of pixel 0/0 of the image is used.
 * @return Handle of the created bitmap,
 * NULL indicates failure.
 * @note Based on an example at
 * http://forum.pellesc.de/index.php?topic=3265.0
 */
HBITMAP WgxCreateMenuBitmapMasked(HBITMAP hBMSrc, COLORREF crTransparent)
{
    HDC hDCMem = NULL, hDCMem2 = NULL, hDCDst1 = NULL;
    BITMAP bm = {0};
    HBITMAP hBMDst1 = NULL;
    HBITMAP hBMDst2 = NULL;
    HBITMAP hBMMask = NULL;
    RECT rc = {0};

    GetObject(hBMSrc, sizeof(bm), &bm);

    hDCMem = CreateCompatibleDC(NULL);
    if(hDCMem == NULL) return NULL;

    hDCMem2 = CreateCompatibleDC(NULL);
    if(hDCMem2 == NULL){
        DeleteDC(hDCMem);
        return NULL;
    }

    hBMDst1 = CreateBitmap(bm.bmWidth, bm.bmHeight, bm.bmPlanes, bm.bmBitsPixel, NULL);    // new bitmap
    hBMDst2 = CreateBitmap(bm.bmWidth, bm.bmHeight, bm.bmPlanes, bm.bmBitsPixel, NULL);    // copy bitmap
    hBMMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL); // mask bitmap
    // make copy of source bitmap
    SelectObject(hDCMem, hBMSrc);      // source
    SelectObject(hDCMem2, hBMDst2);    // target
    BitBlt(hDCMem2, 0, 0, bm.bmWidth, bm.bmHeight, hDCMem, 0, 0, SRCCOPY);    //
    DeleteDC(hDCMem);    // free source
    DeleteDC(hDCMem2);
    // make masks

    hDCMem = CreateCompatibleDC(NULL);
    if(hDCMem == NULL){
        DeleteObject(hBMDst1);
        DeleteObject(hBMDst2);
        DeleteObject(hBMMask);
        return NULL;
    }

    hDCMem2 = CreateCompatibleDC(NULL);
    if(hDCMem2 == NULL){
        DeleteDC(hDCMem);
        DeleteObject(hBMDst1);
        DeleteObject(hBMDst2);
        DeleteObject(hBMMask);
        return NULL;
    }

    SelectObject(hDCMem, hBMDst2);
    SelectObject(hDCMem2, hBMMask);
    /* get color from pixel 0/0, if transparent color is not set */
    if(crTransparent == (COLORREF)-1)
        SetBkColor(hDCMem, GetPixel(hDCMem, 0, 0));
    else
        SetBkColor(hDCMem, crTransparent);

    BitBlt(hDCMem2, 0, 0, bm.bmWidth, bm.bmHeight, hDCMem, 0, 0, SRCCOPY);
    BitBlt(hDCMem, 0, 0, bm.bmWidth, bm.bmHeight, hDCMem2, 0, 0, SRCINVERT);
    DeleteDC(hDCMem);
    DeleteDC(hDCMem2);

    rc.top = rc.left = 0;
    rc.right = bm.bmWidth;
    rc.bottom = bm.bmHeight;

    hDCMem = CreateCompatibleDC(NULL);
    if(hDCMem == NULL){
        DeleteObject(hBMDst1);
        DeleteObject(hBMDst2);
        DeleteObject(hBMMask);
        return NULL;
    }

    hDCDst1 = CreateCompatibleDC(NULL);
    if(hDCDst1 == NULL){
        DeleteDC(hDCMem);
        DeleteObject(hBMDst1);
        DeleteObject(hBMDst2);
        DeleteObject(hBMMask);
        return NULL;
    }

    SelectObject(hDCDst1, hBMDst1);
    FillRect(hDCDst1, &rc, GetSysColorBrush(COLOR_MENU));

    SelectObject(hDCMem, hBMMask);
    BitBlt(hDCDst1, 0, 0, bm.bmWidth, bm.bmHeight, hDCMem, 0, 0, SRCAND);
    SelectObject(hDCMem, hBMDst2);
    BitBlt(hDCDst1, 0, 0, bm.bmWidth, bm.bmHeight, hDCMem, 0, 0, SRCPAINT);
    DeleteDC(hDCMem);
    DeleteDC(hDCDst1);

    DeleteObject(hBMDst2);
    DeleteObject(hBMMask);
    return hBMDst1;
}

/**
 * @internal
 * @brief Extracts menu bitmaps from toolbar bitmaps.
 * @param[in] hBMSrc handle to the toolbar bitmap.
 * @param[in] nPos position of menu bitmap.
 * @return Handle of the created bitmap,
 * NULL indicates failure.
 * @note Based on an example at
 * http://forum.pellesc.de/index.php?topic=3265.0
 */
HBITMAP WgxGetToolbarBitmapForMenu(HBITMAP hBMSrc, int nPos)
{
    HDC hDCSrc = NULL, hDCDst = NULL;
    BITMAP bm = {0};
    HBITMAP hBMDst = NULL, hOldBmp = NULL;
    int cx, cy;

    cx = GetSystemMetrics(SM_CXMENUCHECK);
    cy = GetSystemMetrics(SM_CYMENUCHECK);
    
    if ((hDCSrc = CreateCompatibleDC(NULL)) != NULL) {
        if ((hDCDst = CreateCompatibleDC(NULL)) != NULL) {
            SelectObject(hDCSrc, hBMSrc);
            GetObject(hBMSrc, sizeof(bm), &bm);
            hBMDst = CreateBitmap(cx, cy, bm.bmPlanes, bm.bmBitsPixel, NULL);
            if (hBMDst) {
                hOldBmp = SelectObject(hDCDst, hBMDst);
                StretchBlt(hDCDst, 0, 0, cx, cy, hDCSrc, nPos*bm.bmHeight, 0, bm.bmHeight, bm.bmHeight, SRCCOPY);
                SelectObject(hDCDst, hOldBmp);
                GetObject(hBMDst, sizeof(bm), &bm);
            }
            DeleteDC(hDCDst);
        }
        DeleteDC(hDCSrc);
    }
    return hBMDst;
}

/**
 * @internal
 * @brief WgxBuildMenu helper.
 */
static HMENU BuildMenu(HMENU hMenu,WGX_MENU *menu_table,HBITMAP toolbar_bmp)
{
    MENUITEMINFO mi;
    HMENU hPopup;
    HBITMAP hBMitem;
    int i;

    for(i = 0; menu_table[i].flags || menu_table[i].id || menu_table[i].text; i++){
        if(menu_table[i].flags & MF_SEPARATOR){
            if(!AppendMenuW(hMenu,MF_SEPARATOR,0,NULL))
                goto append_menu_fail;
            continue;
        }
        if(menu_table[i].flags & MF_POPUP){
            hPopup = WgxBuildPopupMenu(menu_table[i].submenu,toolbar_bmp);
            if(hPopup == NULL){
                WgxDbgPrintLastError("WgxBuildMenu: cannot build popup menu");
                return NULL;
            }
            if(!AppendMenuW(hMenu,menu_table[i].flags,(UINT_PTR)hPopup,menu_table[i].text))
                goto append_menu_fail;
            /* set id anyway */
            memset(&mi,0,sizeof(MENUITEMINFO));
            mi.cbSize = sizeof(MENUITEMINFO);
            mi.fMask = MIIM_ID;
            mi.fType = MFT_STRING;
            mi.wID = menu_table[i].id;
            if(!SetMenuItemInfo(hMenu,i,TRUE,&mi))
                goto set_menu_info_fail;
            continue;
        }
        if(!AppendMenuW(hMenu,menu_table[i].flags,menu_table[i].id,menu_table[i].text))
            goto append_menu_fail;

        if(toolbar_bmp != NULL){
            hBMitem = NULL;
            
            if(menu_table[i].toolbar_image_id > -1){
                hBMitem = WgxGetToolbarBitmapForMenu(toolbar_bmp,menu_table[i].toolbar_image_id);
                
                if(hBMitem != NULL)
                    SetMenuItemBitmaps(hMenu,menu_table[i].id,MF_BYCOMMAND,hBMitem,hBMitem);
            }
        }
    }
    
    /* success */
    return hMenu;

append_menu_fail:
    WgxDbgPrintLastError("WgxBuildMenu: cannot append menu");
    return NULL;

set_menu_info_fail:
    WgxDbgPrintLastError("WgxBuildMenu: cannot set menu item id");
    return NULL;
}

/**
 * @brief Builds menu from user defined tables.
 * @param[in] menu_table pointer to array of
 * WGX_MENU structures. All fields of the structure
 * equal to zero indicate the end of the table.
 * @param[in] toolbar_bmp handle to the toolbar bitmap.
 * @return Handle of the created menu,
 * NULL indicates failure.
 * @note The following flags are supported:
 * - MF_STRING - data field must point to
 * zero terminated Unicode string.
 * - MF_SEPARATOR - all fields are ignored.
 * - MF_POPUP - id field must point to
 * another menu table describing a submenu.
 */
HMENU WgxBuildMenu(WGX_MENU *menu_table,HBITMAP toolbar_bmp)
{
    HMENU hMenu;
    
    if(menu_table == NULL)
        return NULL;

    hMenu = CreateMenu();
    if(hMenu == NULL){
        WgxDbgPrintLastError("WgxBuildMenu: cannot create menu");
        return NULL;
    }
    
    if(BuildMenu(hMenu,menu_table,toolbar_bmp) == NULL){
        DestroyMenu(hMenu);
        return NULL;
    }
    
    return hMenu;
}

/**
 * @brief WgxBuildMenu analog, 
 * but works for popup menus.
 */
HMENU WgxBuildPopupMenu(WGX_MENU *menu_table,HBITMAP toolbar_bmp)
{
    HMENU hMenu;
    
    if(menu_table == NULL)
        return NULL;

    hMenu = CreatePopupMenu();
    if(hMenu == NULL){
        WgxDbgPrintLastError("WgxBuildPopupMenu: cannot create menu");
        return NULL;
    }
    
    if(BuildMenu(hMenu,menu_table,toolbar_bmp) == NULL){
        DestroyMenu(hMenu);
        return NULL;
    }
    
    return hMenu;
}

/** @} */
