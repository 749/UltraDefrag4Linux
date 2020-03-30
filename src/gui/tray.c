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
 * @file tray.c
 * @brief System tray icons.
 * @addtogroup SystemTray
 * @{
 */

#include "main.h"

#define SYSTRAY_ICON_ID 0x1

/*
* This structure is accepted by Windows
* NT 4.0 and more recent Windows editions.
*/
typedef struct _UD_NOTIFYICONDATAW {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    WCHAR  szTip[64];
} UD_NOTIFYICONDATAW, *PUD_NOTIFYICONDATAW;

/**
 * @brief Adds an icon to the taskbar
 * notification area (system tray) or
 * modifies them.
 * @param[in] dwMessage the first parameter
 * to be passed in to the Shell_NotifyIcon
 * routine.
 * @return Boolean value. TRUE indicates success.
 */
BOOL ShowSystemTrayIcon(DWORD dwMessage)
{
    UD_NOTIFYICONDATAW nid;
    int id;
    
    if(!minimize_to_system_tray && dwMessage != NIM_DELETE) return TRUE;
    
    memset(&nid,0,sizeof(UD_NOTIFYICONDATAW));
    nid.cbSize = sizeof(UD_NOTIFYICONDATAW);
    nid.hWnd = hWindow;
    nid.uID = SYSTRAY_ICON_ID;
    if(dwMessage != NIM_DELETE){
        nid.uFlags = NIF_ICON | NIF_TIP;
        if(dwMessage == NIM_ADD){
            nid.uFlags |= NIF_MESSAGE;
            nid.uCallbackMessage = WM_TRAYMESSAGE;
        }
        /* set icon */
        id = job_is_running ? IDI_TRAY_ICON_BUSY : IDI_TRAY_ICON;
        id = pause_flag ? IDI_TRAY_ICON_PAUSED : id;
        if(!WgxLoadIcon(hInstance,id,GetSystemMetrics(SM_CXSMICON),&nid.hIcon)) goto fail;
        /* set tooltip */
        wcscpy(nid.szTip,L"UltraDefrag");
    }
    if(Shell_NotifyIconW(dwMessage,(NOTIFYICONDATAW *)(void *)&nid)) return TRUE;
    letrace("Shell_NotifyIconW failed");
    
fail:
    if(dwMessage == NIM_ADD){
        /* show main window again */
        WgxShowWindow(hWindow);
        /* turn off minimize to tray option */
        minimize_to_system_tray = 0;
        itrace("minimize_to_system_tray option turned off");
    }
    return FALSE;
}

/**
 * @brief Sets the tray icon tooltip.
 */
void SetSystemTrayIconTooltip(wchar_t *text)
{
    UD_NOTIFYICONDATAW nid;
    
    if(!minimize_to_system_tray || text == NULL) return;
    
    memset(&nid,0,sizeof(UD_NOTIFYICONDATAW));
    nid.cbSize = sizeof(UD_NOTIFYICONDATAW);
    nid.hWnd = hWindow;
    nid.uID = SYSTRAY_ICON_ID;
    nid.uFlags = NIF_TIP;
    wcsncpy(nid.szTip,text,64);
    nid.szTip[63] = 0;
    if(!Shell_NotifyIconW(NIM_MODIFY,(NOTIFYICONDATAW *)(void *)&nid))
        letrace("Shell_NotifyIconW failed");
}

static BOOL InsertContextMenuSeparator(HMENU hMenu,UINT pos)
{
    MENUITEMINFOW mi;

    memset(&mi,0,MENUITEMINFOW_SIZE);
    mi.cbSize = MENUITEMINFOW_SIZE;
    mi.fMask = MIIM_TYPE;
    mi.fType = MFT_SEPARATOR;
    if(!InsertMenuItemW(hMenu,pos,TRUE,&mi)){
        letrace("separator insertion failed");
        return FALSE;
    }
    return TRUE;
}

static BOOL InsertContextMenuItem(HMENU hMenu,UINT pos,
    UINT id,UINT state,char *i18n_key,wchar_t *default_text)
{
    MENUITEMINFOW mi;
    wchar_t *text;
    BOOL result;

    memset(&mi,0,MENUITEMINFOW_SIZE);
    mi.cbSize = MENUITEMINFOW_SIZE;
    mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mi.fType = MFT_STRING;
    mi.wID = id;
    mi.fState = state;
    text = WgxGetResourceString(i18n_table,i18n_key);
    mi.dwTypeData = text ? text : default_text;
    result = InsertMenuItemW(hMenu,pos,TRUE,&mi);
    if(!result){
        letrace("InsertMenuItem failed");
    }
    free(text);
    return result;
}

/**
 * @brief Shows the tray icon context menu.
 */
void ShowSystemTrayIconContextMenu(void)
{
    HMENU hMenu = NULL;
    POINT pt;
    
    /* create popup menu */
    hMenu = CreatePopupMenu();
    if(hMenu == NULL){
        letrace("cannot create menu");
        goto done;
    }
    
    if(!InsertContextMenuItem(hMenu,0,IDM_SHOWHIDE,MFS_DEFAULT,
        IsWindowVisible(hWindow) ? "HIDE" : "SHOW",
        IsWindowVisible(hWindow) ? L"Hide" : L"Show")) goto done;
    if(!InsertContextMenuSeparator(hMenu,1)) goto done;
    if(!InsertContextMenuItem(hMenu,2,IDM_PAUSE,0,"PAUSE",L"Pause")) goto done;
    if(!InsertContextMenuSeparator(hMenu,3)) goto done;
    if(!InsertContextMenuItem(hMenu,4,IDM_EXIT,0,"EXIT",L"Exit")) goto done;
    if(pause_flag){
        CheckMenuItem(hMenu,IDM_PAUSE,MF_BYCOMMAND | MF_CHECKED);
    }

    /* show menu */
    if(!GetCursorPos(&pt)){
        letrace("cannot get cursor position");
        goto done;
    }
    (void)TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,hWindow,NULL);

done:
    if(hMenu) DestroyMenu(hMenu);
}

/**
 * @brief Removes an icon from the taskbar
 * notification area (system tray).
 * @return Boolean value. TRUE indicates success.
 */
BOOL HideSystemTrayIcon(void)
{
    return ShowSystemTrayIcon(NIM_DELETE);
}

/** @} */
