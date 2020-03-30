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
    {MF_STRING | MF_ENABLED,IDM_DEFRAG,                              NULL, L"&Defragment\tF6",                1 },
    {MF_STRING | MF_ENABLED,IDM_QUICK_OPTIMIZE,                      NULL, L"&Quick optimization\tF7",        2 },
    {MF_STRING | MF_ENABLED,IDM_FULL_OPTIMIZE,                       NULL, L"&Full optimization\tCtrl+F7",    3 },
    {MF_STRING | MF_ENABLED,IDM_OPTIMIZE_MFT,                        NULL, L"&Optimize MFT\tShift+F7",        4 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_PAUSE,                NULL, L"&Pause\tSpace",                 -1 },
    {MF_STRING | MF_ENABLED,IDM_STOP,                                NULL, L"&Stop\tCtrl+C",                  5 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_REPEAT_ACTION,        NULL, L"Re&peat action\tShift+R",       -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_CHECKED,IDM_IGNORE_REMOVABLE_MEDIA, NULL, L"Skip removable &media\tCtrl+M", -1 },
    {MF_STRING | MF_ENABLED,IDM_RESCAN,                              NULL, L"&Rescan drives\tCtrl+D",        -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED,IDM_REPAIR,                              NULL, L"Repair dri&ves",                -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_POPUP,IDM_WHEN_DONE,                when_done_menu,L"&When done",           -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED,IDM_EXIT,                                NULL, L"E&xit\tAlt+F4",                 -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU report_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_SHOW_REPORT, NULL, L"&Show report\tF8", 6 },
    {0,0,NULL,NULL,0}
};

WGX_MENU language_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_CHANGE_LOG,     NULL, L"&View change log",            -1 },
    {MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_REPORT,         NULL, L"View translation &report",    -1 },
    {MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_FOLDER,         NULL, L"&Translations folder",        -1 },
    {MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_SUBMIT,         NULL, L"&Submit current translation", -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_CHECKED,IDM_LANGUAGE + 0x1, NULL, L"English (US)", -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU gui_config_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_CFG_GUI_FONT,     NULL, L"&Font\tF9"    , -1 },
    {MF_STRING | MF_ENABLED,IDM_CFG_GUI_SETTINGS, NULL, L"&Options\tF10",  7 },
    {0,0,NULL,NULL,0}
};

WGX_MENU boot_config_menu[] = {
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_BOOT_ENABLE, NULL, L"&Enable\tF11", -1 },
    {MF_STRING | MF_ENABLED,               IDM_CFG_BOOT_SCRIPT, NULL, L"&Script\tF12",  8 },
    {0,0,NULL,NULL,0}
};

WGX_MENU sorting_menu[] = {
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_SORTING_SORT_BY_PATH,              NULL, L"Sort by &path",                   -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_SORTING_SORT_BY_SIZE,              NULL, L"Sort by &size",                   -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_SORTING_SORT_BY_CREATION_TIME,     NULL, L"Sort by &creation time",          -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_SORTING_SORT_BY_MODIFICATION_TIME, NULL, L"Sort by last &modification time", -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_SORTING_SORT_BY_ACCESS_TIME,       NULL, L"Sort by &last access time",       -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_SORTING_SORT_ASCENDING,            NULL, L"Sort in &ascending order",        -1 },
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_SORTING_SORT_DESCENDING,           NULL, L"Sort in &descending order",       -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU settings_menu[] = {
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_LANGUAGE,     language_menu,     L"&Language",            -1 },
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_CFG_GUI,      gui_config_menu,   L"&Graphical interface", -1 },
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_CFG_BOOT,     boot_config_menu,  L"&Boot time scan",      -1 },
    {MF_STRING | MF_ENABLED,            IDM_CFG_REPORTS,  NULL,              L"&Reports\tCtrl+R",     -1 },
    {MF_STRING | MF_ENABLED | MF_POPUP, IDM_CFG_SORTING,  sorting_menu,      L"&Sorting",             -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU debug_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_OPEN_LOG,                           NULL, L"Open &log\tAlt+L", -1 },
    {MF_STRING | MF_ENABLED,IDM_REPORT_BUG,                         NULL, L"Send bug &report", -1 },
    {0,0,NULL,NULL,0}
};

WGX_MENU help_menu[] = {
    {MF_STRING | MF_ENABLED,IDM_CONTENTS,      NULL, L"&Contents\tF1",        9 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED,IDM_BEST_PRACTICE, NULL, L"Best &practice\tF2",  10 },
    {MF_STRING | MF_ENABLED,IDM_FAQ,           NULL, L"&FAQ\tF3",            -1 },
    {MF_STRING | MF_ENABLED,IDM_CM_LEGEND,     NULL, L"Cluster map &legend", -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED | MF_POPUP,IDM_DEBUG, debug_menu, L"&Debug",     -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED,IDM_CHECK_UPDATE,  NULL, L"Check for &update",   -1 },
    {MF_SEPARATOR,0,NULL,NULL,0},
    {MF_STRING | MF_ENABLED,IDM_ABOUT,         NULL, L"&About\tF4",          11 },
    {0,0,NULL,NULL,0}
};

WGX_MENU preview_menu[] = {
    {MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_PREVIEW_DUMMY, NULL, L"Dummy",  -1 },
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
    HBITMAP hBitmap = NULL;
    HBITMAP hBitmapMasked = NULL;
    OSVERSIONINFO osvi;
    int cx, id;

    trace(I"Menu row height ........ %d",GetSystemMetrics(SM_CYMENU));
    trace(I"Menu button size ....... %d x %d",GetSystemMetrics(SM_CXMENUSIZE),GetSystemMetrics(SM_CYMENUSIZE));
    trace(I"Menu check-mark size ... %d x %d",GetSystemMetrics(SM_CXMENUCHECK),GetSystemMetrics(SM_CYMENUCHECK));

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    /* menu icons look nicely on Vista and above */
    if(osvi.dwMajorVersion > 5 && show_menu_icons){
        cx = GetSystemMetrics(SM_CXMENUCHECK);
        if(cx < 16){
            /* 100% DPI */
            id = IDB_MENU_ICONS_15;
        } else if(cx < 20){
            /* 125% DPI */
            id = IDB_MENU_ICONS_19;
        } else if(cx < 26){
            /* 150% DPI */
            id = IDB_MENU_ICONS_25;
        } else {
            /* 200% DPI */
            id = IDB_MENU_ICONS_31;
        }
        hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(id));
        hBitmapMasked = WgxCreateMenuBitmapMasked(hBitmap, (COLORREF)-1);
        hMainMenu = WgxBuildMenu(main_menu,hBitmapMasked);
        if(hBitmap != NULL)
            DeleteObject(hBitmap);
        if(hBitmapMasked != NULL)
            DeleteObject(hBitmapMasked);
    } else {
        /* on systems below Vista avoid icons use */
        hMainMenu = WgxBuildMenu(main_menu,NULL);
    }
    
    /* attach menu to the window */
    if(!SetMenu(hWindow,hMainMenu)){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            L"Cannot set main menu!");
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
    
    if(sorting_flags & SORT_BY_PATH){
        CheckMenuItem(hMainMenu,IDM_CFG_SORTING_SORT_BY_PATH,MF_BYCOMMAND | MF_CHECKED);
        (void)SetEnvironmentVariable("UD_SORTING","path");
    } else if(sorting_flags & SORT_BY_SIZE){
        CheckMenuItem(hMainMenu,IDM_CFG_SORTING_SORT_BY_SIZE,MF_BYCOMMAND | MF_CHECKED);
        (void)SetEnvironmentVariable("UD_SORTING","size");
    } else if(sorting_flags & SORT_BY_CREATION_TIME){
        CheckMenuItem(hMainMenu,IDM_CFG_SORTING_SORT_BY_CREATION_TIME,MF_BYCOMMAND | MF_CHECKED);
        (void)SetEnvironmentVariable("UD_SORTING","c_time");
    } else if(sorting_flags & SORT_BY_MODIFICATION_TIME){
        CheckMenuItem(hMainMenu,IDM_CFG_SORTING_SORT_BY_MODIFICATION_TIME,MF_BYCOMMAND | MF_CHECKED);
        (void)SetEnvironmentVariable("UD_SORTING","m_time");
    } else if(sorting_flags & SORT_BY_ACCESS_TIME){
        CheckMenuItem(hMainMenu,IDM_CFG_SORTING_SORT_BY_ACCESS_TIME,MF_BYCOMMAND | MF_CHECKED);
        (void)SetEnvironmentVariable("UD_SORTING","a_time");
    }
    if(sorting_flags & SORT_ASCENDING){
        CheckMenuItem(hMainMenu,IDM_CFG_SORTING_SORT_ASCENDING,MF_BYCOMMAND | MF_CHECKED);
        (void)SetEnvironmentVariable("UD_SORTING_ORDER","asc");
    } else {
        CheckMenuItem(hMainMenu,IDM_CFG_SORTING_SORT_DESCENDING,MF_BYCOMMAND | MF_CHECKED);
        (void)SetEnvironmentVariable("UD_SORTING_ORDER","desc");
    }

    if(!DrawMenuBar(hWindow))
        letrace("cannot redraw main menu");

    return 0;
}

/** @} */
