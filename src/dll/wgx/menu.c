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

/**
 * @file menu.c
 * @brief Menu.
 * @addtogroup Menu
 * @{
 */

#include "wgx-internals.h"

/**
 * @internal
 * @brief Copies one bitmap to another.
 * @param[in] hBmpDst the destination bitmap.
 * @param[in] hBmpSrc the source bitmap.
 * @param[in] flags the flags passed to
 * the BitBlt routine.
 */
static void WgxCopyBitmap(HBITMAP hBmpDest,HBITMAP hBmpSrc,DWORD flags)
{
    HDC hDC = NULL, hDC2 = NULL;
    HBITMAP hOldBmp, hOldBmp2;
    BITMAP bm = {0};
    
    /* create device contexts */
    hDC = CreateCompatibleDC(NULL);
    hDC2 = CreateCompatibleDC(NULL);
    
    /* copy bitmap */
    if(hDC && hDC2){
        GetObject(hBmpSrc,sizeof(bm),&bm);
        hOldBmp = SelectObject(hDC,hBmpSrc);
        hOldBmp2 = SelectObject(hDC2,hBmpDest);
        BitBlt(hDC2,0,0,bm.bmWidth,bm.bmHeight,hDC,0,0,flags);
        SelectObject(hDC,hOldBmp);
        SelectObject(hDC2,hOldBmp2);
    }
    
    /* cleanup */
    if(hDC) DeleteDC(hDC);
    if(hDC2) DeleteDC(hDC2);
}

/**
 * @internal
 * @brief Fills bitmap by specified color.
 */
static void WgxFillBitmap(HBITMAP hBitmap,HBRUSH hBrush)
{
    BITMAP bm = {0};
    HBITMAP hOldBmp;
    HDC hDC;
    RECT rc;
    
    hDC = CreateCompatibleDC(NULL);
    if(hDC){
        GetObject(hBitmap,sizeof(bm),&bm);
        rc.top = rc.left = 0;
        rc.right = bm.bmWidth;
        rc.bottom = bm.bmHeight;
        hOldBmp = SelectObject(hDC,hBitmap);
        FillRect(hDC,&rc,hBrush);
        SelectObject(hDC,hOldBmp);
        DeleteDC(hDC);
    }
}

/**
 * @brief Prepares bitmap for use in menu.
 * @param[in] hBitmap handle to the bitmap.
 * @param[in] crTransparent transparent color. If set to
 * (COLORREF)-1 the color of pixel 0/0 of the image is used.
 * @return Handle of the created bitmap, NULL indicates failure.
 * @note Based on an example at
 * http://forum.pellesc.de/index.php?topic=3265.0
 */
HBITMAP WgxCreateMenuBitmapMasked(HBITMAP hBitmap,COLORREF crTransparent)
{
    HDC hDC = NULL, hDC2 = NULL;
    HBITMAP hOldBmp, hOldBmp2;
    HBITMAP hNewBitmap = NULL;
    HBITMAP hMask1 = NULL;
    HBITMAP hMask2 = NULL;
    BITMAP bm = {0};

    /* create device contexts */
    hDC = CreateCompatibleDC(NULL);
    hDC2 = CreateCompatibleDC(NULL);
    if(hDC == NULL || hDC2 == NULL)
        goto fail;
    
    /* create bitmaps */
    GetObject(hBitmap,sizeof(bm),&bm);
    hNewBitmap = CreateBitmap(bm.bmWidth,bm.bmHeight,bm.bmPlanes,bm.bmBitsPixel,NULL);
    hMask1 = CreateBitmap(bm.bmWidth,bm.bmHeight,bm.bmPlanes,bm.bmBitsPixel,NULL);
    hMask2 = CreateBitmap(bm.bmWidth,bm.bmHeight,1,1,NULL);
    
    /* make copy of source bitmap */
    WgxCopyBitmap(hMask1,hBitmap,SRCCOPY);

    /* make masks */
    hOldBmp = SelectObject(hDC,hMask1);
    hOldBmp2 = SelectObject(hDC2,hMask2);
    
    /* get color from pixel 0/0, if transparent color is not set */
    if(crTransparent == (COLORREF)-1)
        SetBkColor(hDC,GetPixel(hDC,0,0));
    else
        SetBkColor(hDC,crTransparent);

    BitBlt(hDC2,0,0,bm.bmWidth,bm.bmHeight,hDC,0,0,SRCCOPY);
    BitBlt(hDC,0,0,bm.bmWidth,bm.bmHeight,hDC2,0,0,SRCINVERT);
    
    SelectObject(hDC,hOldBmp);
    SelectObject(hDC2,hOldBmp2);

    /* combine them all together */
    WgxFillBitmap(hNewBitmap,GetSysColorBrush(COLOR_MENU));
    WgxCopyBitmap(hNewBitmap,hMask2,SRCAND);
    WgxCopyBitmap(hNewBitmap,hMask1,SRCPAINT);

    DeleteObject(hMask1);
    DeleteObject(hMask2);
    return hNewBitmap;
    
fail:
    if(hDC) DeleteDC(hDC);
    if(hDC2) DeleteDC(hDC2);
    if(hMask1) DeleteObject(hMask1);
    if(hMask2) DeleteObject(hMask2);
    if(hNewBitmap) DeleteObject(hNewBitmap);
    return NULL;
}

/**
 * @internal
 * @brief Creates menu item bitmap
 * by cutting off a part of the
 * specified bitmap.
 * @details The bitmap passed in
 * must contain series of pictures
 * placed sequentially one after
 * another. The second parameter
 * defines index of picture in series.
 * @return Handle of the created bitmap,
 * NULL indicates failure.
 * @note Based on an example at
 * http://forum.pellesc.de/index.php?topic=3265.0
 */
static HBITMAP WgxGetBitmapForMenuItem(HBITMAP hBitmap,int i)
{
    HDC hDC = NULL, hDC2 = NULL;
    HBITMAP hOldBmp, hOldBmp2;
    HBITMAP hNewBitmap = NULL;
    BITMAP bm = {0};
    int cx, cy;

    /* get dimensions of menu check mark */
    cx = GetSystemMetrics(SM_CXMENUCHECK);
    cy = GetSystemMetrics(SM_CYMENUCHECK);
    
    /* create device contexts */
    hDC = CreateCompatibleDC(NULL);
    hDC2 = CreateCompatibleDC(NULL);
    
    /* fill desired bitmap */
    if(hDC && hDC2){
        GetObject(hBitmap,sizeof(bm),&bm);
        hNewBitmap = CreateBitmap(cx,cy,bm.bmPlanes,bm.bmBitsPixel,NULL);
        if(hNewBitmap){
            hOldBmp = SelectObject(hDC,hBitmap);
            hOldBmp2 = SelectObject(hDC2,hNewBitmap);
            if(SetStretchBltMode(hDC2,HALFTONE))
                SetBrushOrgEx(hDC2,0,0,NULL);
            StretchBlt(hDC2,0,0,cx,cy,hDC,i * bm.bmHeight,
                0,bm.bmHeight,bm.bmHeight,SRCCOPY);
            SelectObject(hDC,hOldBmp);
            SelectObject(hDC2,hOldBmp2);
        }
    }
    
    /* cleanup */
    if(hDC) DeleteDC(hDC);
    if(hDC2) DeleteDC(hDC2);
    return hNewBitmap;
}

/**
 * @internal
 * @brief WgxBuildMenu helper.
 */
static HMENU BuildMenu(HMENU hMenu,WGX_MENU *menu_table,HBITMAP bitmap)
{
    MENUITEMINFO mi;
    HMENU hPopup;
    HBITMAP hBitmapItem;
    int i;

    for(i = 0; menu_table[i].flags || menu_table[i].id || menu_table[i].text; i++){
        if(menu_table[i].flags & MF_SEPARATOR){
            if(!AppendMenuW(hMenu,MF_SEPARATOR,0,NULL))
                goto append_menu_fail;
            continue;
        }
        if(menu_table[i].flags & MF_POPUP){
            hPopup = WgxBuildPopupMenu(menu_table[i].submenu,bitmap);
            if(hPopup == NULL){
                letrace("cannot build popup menu");
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

        if((menu_table[i].toolbar_image_id > -1) && bitmap){
            hBitmapItem = WgxGetBitmapForMenuItem(bitmap,menu_table[i].toolbar_image_id);
            if(hBitmapItem != NULL)
                SetMenuItemBitmaps(hMenu,menu_table[i].id,MF_BYCOMMAND,hBitmapItem,hBitmapItem);
        }
    }
    
    /* success */
    return hMenu;

append_menu_fail:
    letrace("cannot append menu");
    return NULL;

set_menu_info_fail:
    letrace("cannot set menu item id");
    return NULL;
}

/**
 * @brief Builds menu from user defined tables.
 * @param[in] menu_table pointer to array of
 * WGX_MENU structures. All fields of the structure
 * equal to zero indicate the end of the table.
 * @param[in] bitmap handle to bitmap
 * intended to extract menu icons from.
 * @return Handle of the created menu,
 * NULL indicates failure.
 * @note The following flags are supported:
 * - MF_STRING - data field must point to
 * zero terminated Unicode string.
 * - MF_SEPARATOR - all fields are ignored.
 * - MF_POPUP - id field must point to
 * another menu table describing a submenu.
 */
HMENU WgxBuildMenu(WGX_MENU *menu_table,HBITMAP bitmap)
{
    HMENU hMenu;
    
    if(menu_table == NULL)
        return NULL;

    hMenu = CreateMenu();
    if(hMenu == NULL){
        letrace("cannot create menu");
        return NULL;
    }
    
    if(BuildMenu(hMenu,menu_table,bitmap) == NULL){
        DestroyMenu(hMenu);
        return NULL;
    }
    
    return hMenu;
}

/**
 * @brief WgxBuildMenu analog, 
 * but works for popup menus.
 */
HMENU WgxBuildPopupMenu(WGX_MENU *menu_table,HBITMAP bitmap)
{
    HMENU hMenu;
    
    if(menu_table == NULL)
        return NULL;

    hMenu = CreatePopupMenu();
    if(hMenu == NULL){
        letrace("cannot create menu");
        return NULL;
    }
    
    if(BuildMenu(hMenu,menu_table,bitmap) == NULL){
        DestroyMenu(hMenu);
        return NULL;
    }
    
    return hMenu;
}

/** @} */
