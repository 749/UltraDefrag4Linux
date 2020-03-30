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
 * @file menu.c
 * @brief Menu.
 * @addtogroup Menu
 * @{
 */

/*
* Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
* and Dmitri Arkhangelski <dmitriar@gmail.com>.
*/

#include "main.h"

HMENU hMainMenu = NULL;

WGX_MENU when_done_menu[] = {
    {MF_STRING | MF_ENABLED | MF_CHECKED,IDM_WHEN_DONE_NONE,        NULL, L"&None",       -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_WHEN_DONE_EXIT,      NULL, L"E&xit",       -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_WHEN_DONE_STANDBY,   NULL, L"Stan&dby",    -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_WHEN_DONE_HIBERNATE, NULL, L"&Hibernate",  -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_WHEN_DONE_LOGOFF,    NULL, L"&Logoff",     -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_WHEN_DONE_REBOOT,    NULL, L"&Reboot",     -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_WHEN_DONE_SHUTDOWN,  NULL, L"&Shutdown",   -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU action_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_ANALYZE,                             NULL, L"&Analyze\tF5",                   0 },
    {MF_STRING | MF_ENABLED,IDM_DEFRAG,                              NULL, L"&Defragment\tF6",                3 },
    {MF_STRING | MF_ENABLED,IDM_QUICK_OPTIMIZE,                      NULL, L"&Quick optimization\tF7",        4 },
    {MF_STRING | MF_ENABLED,IDM_FULL_OPTIMIZE,                       NULL, L"&Full optimization\tCtrl+F7",    5 },
    {MF_STRING | MF_ENABLED,IDM_OPTIMIZE_MFT,                        NULL, L"&Optimize MFT\tShift+F7",        6 },
    {MF_STRING | MF_ENABLED,IDM_STOP,                                NULL, L"&Stop\tCtrl+C",                  7 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_REPEAT_ACTION,        NULL, L"Re&peat action\tShift+R",        1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_CHECKED,IDM_IGNORE_REMOVABLE_MEDIA, NULL, L"Skip removable &media\tCtrl+M", -1 },
    {MF_STRING | MF_ENABLED,IDM_RESCAN,                              NULL, L"&Rescan drives\tCtrl+D",        -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_POPUP,IDM_WHEN_DONE,                when_done_menu,L"&When done",           -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED,IDM_EXIT,                                NULL, L"E&xit\tAlt+F4",                 -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU report_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_SHOW_REPORT, NULL, L"&Show report\tF8", 8 },
    {0,0,NULL,NULL,0}
};

WGX_MENU language_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_FOLDER,         NULL, L"&Translations folder", -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_CHECKED,IDM_LANGUAGE + 0x1, NULL, L"English (US)",         -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU gui_config_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_CFG_GUI_FONT,     NULL, L"&Font\tF9"    , -1 },
    {MF_STRING | MF_ENABLED,IDM_CFG_GUI_SETTINGS, NULL, L"&Options\tF10",  9 },
    {0,0,NULL,NULL,0}
};

WGX_MENU boot_config_menu[] = {
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_BOOT_ENABLE, NULL, L"&Enable\tF11", 10 },
    {MF_STRING | MF_ENABLED,IDM_CFG_BOOT_SCRIPT,                NULL, L"&Script\tF12", 11 },
    {0,0,NULL,NULL,0}
};

WGX_MENU settings_menu[] = {
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_LANGUAGE,    language_menu,    L"&Language",            -1 },
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_CFG_GUI,     gui_config_menu,  L"&Graphical interface", -1 },
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_CFG_BOOT,    boot_config_menu, L"&Boot time scan",      -1 },
    {MF_STRING | MF_ENABLED,            IDM_CFG_REPORTS, NULL,             L"&Reports\tCtrl+R",     -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU help_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_CONTENTS,      NULL, L"&Contents\tF1",      12 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED,IDM_BEST_PRACTICE, NULL, L"Best &practice\tF2", -1 },
    {MF_STRING | MF_ENABLED,IDM_FAQ,           NULL, L"&FAQ\tF3",           -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED,IDM_ABOUT,         NULL, L"&About\tF4",         -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU preview_menu[] = {
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_PREVIEW_LARGEST,  NULL, L"Find largest free space",  -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_PREVIEW_MATCHING, NULL, L"Find matching free space", -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU main_menu[] = {
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_ACTION,   action_menu,   L"&Action",   -1 },
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_REPORT,   report_menu,   L"&Report",   -1 },
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_SETTINGS, settings_menu, L"&Settings", -1 },
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_HELP,     help_menu,     L"&Help",     -1 },
#ifndef _UD_HIDE_PREVIEW_
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_PREVIEW,  preview_menu,  L"Preview",   -1 },
#endif /* _UD_HIDE_PREVIEW_ */
    {0,0,NULL,NULL,0}
};

/**
 * @brief Creates main menu.
 * @return Zero for success,
 * negative value otherwise.
 */
int CreateMainMenu(void)
{
    HBITMAP hBMtoolbar = NULL, hBMtoolbarMasked = NULL;
    int id;
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    if(osvi.dwMajorVersion > 5)
        id = IDB_TOOLBAR;
    else
        id = IDB_MENU;

    WgxDbgPrint("Menu row height ........ %d",GetSystemMetrics(SM_CYMENU));
    WgxDbgPrint("Menu button size ....... %d x %d",GetSystemMetrics(SM_CXMENUSIZE),GetSystemMetrics(SM_CYMENUSIZE));
    WgxDbgPrint("Menu check-mark size ... %d x %d",GetSystemMetrics(SM_CXMENUCHECK),GetSystemMetrics(SM_CYMENUCHECK));

    hBMtoolbar = LoadBitmap(hInstance, MAKEINTRESOURCE(id));

    if(osvi.dwMajorVersion > 5)
        hBMtoolbarMasked = WgxCreateMenuBitmapMasked(hBMtoolbar, (COLORREF)-1);
        
    /* create menu */
    /* if(hBMtoolbarMasked == NULL)
        hMainMenu = WgxBuildMenu(main_menu,hBMtoolbar);
    else
        hMainMenu = WgxBuildMenu(main_menu,hBMtoolbarMasked); */
        hMainMenu = WgxBuildMenu(main_menu,NULL);
    
    if(hBMtoolbar != NULL)
        DeleteObject(hBMtoolbar);
    if(hBMtoolbarMasked != NULL)
        DeleteObject(hBMtoolbarMasked);
    
    /* attach menu to the window */
    if(!SetMenu(hWindow,hMainMenu)){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot set main menu!");
        DestroyMenu(hMainMenu);
        return (-1);
    }
    
    if(skip_removable == 0){
        CheckMenuItem(hMainMenu,
            IDM_IGNORE_REMOVABLE_MEDIA,
            MF_BYCOMMAND | MF_UNCHECKED);
    }
    
    if(repeat_action){
        CheckMenuItem(hMainMenu,
            IDM_REPEAT_ACTION,
            MF_BYCOMMAND | MF_CHECKED);
    }

    if(btd_installed){
        if(IsBootTimeDefragEnabled()){
            boot_time_defrag_enabled = 1;
            CheckMenuItem(hMainMenu,
                IDM_CFG_BOOT_ENABLE,
                MF_BYCOMMAND | MF_CHECKED);
        }
    } else {
        EnableMenuItem(hMainMenu,IDM_CFG_BOOT_ENABLE,MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMainMenu,IDM_CFG_BOOT_SCRIPT,MF_BYCOMMAND | MF_GRAYED);
    }
    
    if(job_flags & UD_PREVIEW_MATCHING){
        CheckMenuItem(hMainMenu,
            IDM_PREVIEW_MATCHING,
            MF_BYCOMMAND | MF_CHECKED);
    } else {
        CheckMenuItem(hMainMenu,
            IDM_PREVIEW_LARGEST,
            MF_BYCOMMAND | MF_CHECKED);
    }

    if(!DrawMenuBar(hWindow))
        WgxDbgPrintLastError("Cannot redraw main menu");

    return 0;
}

/** @} */
