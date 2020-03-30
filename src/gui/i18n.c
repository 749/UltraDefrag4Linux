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
 * @file i18n.c
 * @brief Internationalization.
 * @addtogroup Internationalization
 * @{
 */

/*
* Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
* and Dmitri Arkhangelski <dmitriar@gmail.com>.
*/

#include "main.h"

/* list of languages for which we need to use custom font for dialog boxes */
char *special_font_languages[] = {
    "Burmese (Padauk)",
    NULL /* end of the table */
};

int use_custom_font_in_dialogs = 0;

WGX_I18N_RESOURCE_ENTRY i18n_table[] = {
    /* action menu */
    {0, "ACTION",                   L"&Action",                  NULL},
    {0, "ANALYSE",                  L"&Analyze",                 NULL},
    {0, "DEFRAGMENT",               L"&Defragment",              NULL},
    {0, "QUICK_OPTIMIZE",           L"&Quick optimization",      NULL},
    {0, "FULL_OPTIMIZE",            L"&Full optimization",       NULL},
    {0, "OPTIMIZE_MFT",             L"&Optimize MFT",            NULL},
    {0, "PAUSE",                    L"&Pause",                   NULL},
    {0, "STOP",                     L"&Stop",                    NULL},
    {0, "REPEAT_ACTION",            L"Re&peat action",           NULL},
    {0, "SKIP_REMOVABLE_MEDIA",     L"Skip removable &media",    NULL},
    {0, "RESCAN_DRIVES",            L"&Rescan drives",           NULL},
    {0, "REPAIR_DRIVES",            L"Repair dri&ves",           NULL},
    {0, "WHEN_DONE",                L"&When done",               NULL},
    {0, "WHEN_DONE_NONE",           L"&None",                    NULL},
    {0, "WHEN_DONE_EXIT",           L"E&xit",                    NULL},
    {0, "WHEN_DONE_STANDBY",        L"Stan&dby",                 NULL},
    {0, "WHEN_DONE_HIBERNATE",      L"&Hibernate",               NULL},
    {0, "WHEN_DONE_LOGOFF",         L"&Logoff",                  NULL},
    {0, "WHEN_DONE_REBOOT",         L"&Reboot",                  NULL},
    {0, "WHEN_DONE_SHUTDOWN",       L"&Shutdown",                NULL},
    {0, "EXIT",                     L"E&xit",                    NULL},

    /* report menu */
    {0, "REPORT",                   L"&Report",                  NULL},
    {0, "SHOW_REPORT",              L"&Show report",             NULL},

    /* settings menu */
    {0, "SETTINGS",                 L"&Settings",                NULL},
    {0, "LANGUAGE",                 L"&Language",                NULL},
    {0, "TRANSLATIONS_CHANGE_LOG",  L"&View change log",         NULL},
    {0, "TRANSLATIONS_REPORT",      L"View translation &report", NULL},
    {0, "TRANSLATIONS_FOLDER",      L"&Translations folder",     NULL},
    {0, "TRANSLATIONS_SUBMIT",      L"&Submit current translation", NULL},
    {0, "GRAPHICAL_INTERFACE",      L"&Graphical interface",     NULL},
    {0, "FONT",                     L"&Font",                    NULL},
    {0, "OPTIONS",                  L"&Options",                 NULL},
    {0, "BOOT_TIME_SCAN",           L"&Boot time scan",          NULL},
    {0, "ENABLE",                   L"&Enable",                  NULL},
    {0, "SCRIPT",                   L"&Script",                  NULL},
    {0, "REPORTS",                  L"&Reports",                 NULL},
    {0, "SORTING",                  L"&Sorting",                 NULL},
    {0, "SORT_BY_PATH",             L"Sort by &path",            NULL},
    {0, "SORT_BY_SIZE",             L"Sort by &size",            NULL},
    {0, "SORT_BY_C_TIME",           L"Sort by &creation time",   NULL},
    {0, "SORT_BY_M_TIME",           L"Sort by last &modification time", NULL},
    {0, "SORT_BY_A_TIME",           L"Sort by &last access time", NULL},
    {0, "SORT_ASCENDING",           L"Sort in &ascending order", NULL},
    {0, "SORT_DESCENDING",          L"Sort in &descending order", NULL},

    /* help menu */
    {0, "HELP",                     L"&Help",                    NULL},
    {0, "CONTENTS",                 L"&Contents",                NULL},
    {0, "BEST_PRACTICE",            L"Best &practice",           NULL},
    {0, "FAQ",                      L"&FAQ",                     NULL},
    {0, "CM_LEGEND",                L"Cluster map &legend",      NULL},
    {0, "DEBUG",                    L"De&bug",                   NULL},
    {0, "OPEN_LOG",                 L"Open &log",                NULL},
    {0, "REPORT_BUG",               L"Send bug &report",         NULL},
    {0, "CHECK_UPDATE",             L"Check for &update",        NULL},
    {0, "ABOUT",                    L"&About",                   NULL},

    /* toolbar tooltips */
    {0, "BOOT_TIME_SCRIPT",         L"Boot time script",         NULL},

    /* volume characteristics */
    {0, "VOLUME",                   L"Disk",                     NULL},
    {0, "STATUS",                   L"Status",                   NULL},
    {0, "FRAGMENTATION",            L"Fragmentation",            NULL},
    {0, "TOTAL",                    L"Total Space",              NULL},
    {0, "FREE",                     L"Free Space",               NULL},
    {0, "PERCENT",                  L"% free",                   NULL},

    /* volume processing status */
    {0, "STATUS_RUNNING",           L"Executing",                NULL},
    {0, "STATUS_ANALYSED",          L"Analyzed",                 NULL},
    {0, "STATUS_DEFRAGMENTED",      L"Defragmented",             NULL},
    {0, "STATUS_OPTIMIZED",         L"Optimized",                NULL},
    {0, "STATUS_DIRTY",             L"Disk needs to be repaired",NULL},

    /* status bar */
    {0, "DIRS",                     L"folders",                  NULL},
    {0, "FILES",                    L"files",                    NULL},
    {0, "FRAGMENTED",               L"fragmented",               NULL},
    {0, "COMPRESSED",               L"compressed",               NULL},

    /* about box */
    {0, "ABOUT_WIN_TITLE",          L"About Ultra Defragmenter", NULL},
    {0, "CREDITS",                  L"&Credits",                 NULL},
    {0, "LICENSE",                  L"&License",                 NULL},

    /* shutdown confirmation */
    {0,                 "PLEASE_CONFIRM",             L"Please Confirm",             NULL},
    {0,                 "REALLY_SHUTDOWN_WHEN_DONE",  L"Do you really want to shutdown when done?",  NULL},
    {0,                 "REALLY_HIBERNATE_WHEN_DONE", L"Do you really want to hibernate when done?", NULL},
    {0,                 "REALLY_LOGOFF_WHEN_DONE",    L"Do you really want to log off when done?",   NULL},
    {0,                 "REALLY_REBOOT_WHEN_DONE",    L"Do you really want to reboot when done?",    NULL},
    {0,                 "SECONDS_TILL_SHUTDOWN",      L"seconds until shutdown",     NULL},
    {0,                 "SECONDS_TILL_HIBERNATION",   L"seconds until hibernation",  NULL},
    {0,                 "SECONDS_TILL_LOGOFF",        L"seconds until logoff",       NULL},
    {0,                 "SECONDS_TILL_REBOOT",        L"seconds until reboot",       NULL},
    {IDC_YES_BUTTON,    "YES",                        L"&Yes",                       NULL},
    {IDC_NO_BUTTON,     "NO",                         L"&No",                        NULL},

    
    /* tray icon context menu */
    {0,                 "SHOW",                       L"Show",                       NULL},
    {0,                 "HIDE",                       L"Hide",                       NULL},
    
    /* upgrade dialog */
    {0,                 "UPGRADE_CAPTION",            L"You can upgrade me ^-^",     NULL},
    {0,                 "UPGRADE_MESSAGE",            L"release is available for download!", NULL},

    /* taskbar icon overlay message */
    {0,                 "JOB_IS_RUNNING",             L"A job is running",           NULL},
    {0,                 "JOB_IS_PAUSED",              L"A job is paused",            NULL},
    
    /* end of the table */
    {0,                 NULL,                         NULL,                          NULL}
};

struct menu_item {
    int id;         /* menu item identifier */
    char *key;      /* i18n table entry key */
    char *hotkeys;  /* hotkeys assigned to the item */
};

/**
 * @internal
 * @brief Simplifies build of localized menu.
 */
struct menu_item menu_items[] = {
    {IDM_ANALYZE,                 "ANALYSE",                  "F5"    },
    {IDM_DEFRAG,                  "DEFRAGMENT",               "F6"    },
    {IDM_QUICK_OPTIMIZE,          "QUICK_OPTIMIZE",           "F7"    },
    {IDM_FULL_OPTIMIZE,           "FULL_OPTIMIZE",            "Ctrl+F7"},
    {IDM_OPTIMIZE_MFT,            "OPTIMIZE_MFT",             "Shift+F7"},
    {IDM_PAUSE,                   "PAUSE",                    "Space"},
    {IDM_STOP,                    "STOP",                     "Ctrl+C"},
    {IDM_REPEAT_ACTION,           "REPEAT_ACTION",            "Shift+R"},
    {IDM_IGNORE_REMOVABLE_MEDIA,  "SKIP_REMOVABLE_MEDIA",     "Ctrl+M"},
    {IDM_RESCAN,                  "RESCAN_DRIVES",            "Ctrl+D"},
    {IDM_REPAIR,                  "REPAIR_DRIVES",            NULL},
    {IDM_OPEN_LOG,                "OPEN_LOG",                 "Alt+L"},
    {IDM_REPORT_BUG,              "REPORT_BUG",               NULL},
    {IDM_WHEN_DONE_NONE,          "WHEN_DONE_NONE",           NULL},
    {IDM_WHEN_DONE_EXIT,          "WHEN_DONE_EXIT",           NULL},
    {IDM_WHEN_DONE_STANDBY,       "WHEN_DONE_STANDBY",        NULL},
    {IDM_WHEN_DONE_HIBERNATE,     "WHEN_DONE_HIBERNATE",      NULL},
    {IDM_WHEN_DONE_LOGOFF,        "WHEN_DONE_LOGOFF",         NULL},
    {IDM_WHEN_DONE_REBOOT,        "WHEN_DONE_REBOOT",         NULL},
    {IDM_WHEN_DONE_SHUTDOWN,      "WHEN_DONE_SHUTDOWN",       NULL},
    {IDM_EXIT,                    "EXIT",                     "Alt+F4"},
    {IDM_SHOW_REPORT,             "SHOW_REPORT",              "F8"    },
    {IDM_TRANSLATIONS_CHANGE_LOG, "TRANSLATIONS_CHANGE_LOG",  NULL    },
    {IDM_TRANSLATIONS_REPORT,     "TRANSLATIONS_REPORT",      NULL    },
    {IDM_TRANSLATIONS_FOLDER,     "TRANSLATIONS_FOLDER",      NULL    },
    {IDM_TRANSLATIONS_SUBMIT,     "TRANSLATIONS_SUBMIT",      NULL    },
    {IDM_CFG_GUI_FONT,            "FONT",                     "F9"    },
    {IDM_CFG_GUI_SETTINGS,        "OPTIONS",                  "F10"   },
    {IDM_CFG_BOOT_ENABLE,         "ENABLE",                   "F11"   },
    {IDM_CFG_BOOT_SCRIPT,         "SCRIPT",                   "F12"   },
    {IDM_CFG_REPORTS,             "REPORTS",                  "Ctrl+R"},
    {IDM_CFG_SORTING,             "SORTING",                  NULL    },
    {IDM_CFG_SORTING_SORT_BY_PATH,              "SORT_BY_PATH",    NULL },
    {IDM_CFG_SORTING_SORT_BY_SIZE,              "SORT_BY_SIZE",    NULL },
    {IDM_CFG_SORTING_SORT_BY_CREATION_TIME,     "SORT_BY_C_TIME",  NULL },
    {IDM_CFG_SORTING_SORT_BY_MODIFICATION_TIME, "SORT_BY_M_TIME",  NULL },
    {IDM_CFG_SORTING_SORT_BY_ACCESS_TIME,       "SORT_BY_A_TIME",  NULL },
    {IDM_CFG_SORTING_SORT_ASCENDING,            "SORT_ASCENDING",  NULL },
    {IDM_CFG_SORTING_SORT_DESCENDING,           "SORT_DESCENDING", NULL },
    {IDM_CONTENTS,                "CONTENTS",                 "F1"    },
    {IDM_BEST_PRACTICE,           "BEST_PRACTICE",            "F2"    },
    {IDM_FAQ,                     "FAQ",                      "F3"    },
    {IDM_CM_LEGEND,               "CM_LEGEND",                NULL    },
    {IDM_CHECK_UPDATE,            "CHECK_UPDATE",             NULL    },
    {IDM_ABOUT,                   "ABOUT",                    "F4"    },
    /* submenus */
    {IDM_DEBUG,                   "DEBUG",                    NULL},
    {IDM_WHEN_DONE,               "WHEN_DONE",                NULL},
    {IDM_LANGUAGE,                "LANGUAGE",                 NULL},
    {IDM_CFG_GUI,                 "GRAPHICAL_INTERFACE",      NULL},
    {IDM_CFG_BOOT,                "BOOT_TIME_SCAN",           NULL},
    {IDM_ACTION,                  "ACTION",                   NULL},
    {IDM_REPORT,                  "REPORT",                   NULL},
    {IDM_SETTINGS,                "SETTINGS",                 NULL},
    {IDM_HELP,                    "HELP",                     NULL},
    {0, NULL, NULL}
};

/**
 * @internal
 * @brief Synchronization event.
 */
HANDLE hLangMenuEvent = NULL;

int lang_ini_tracking_stopped = 0;
int stop_track_lang_ini = 0;
int i18n_folder_tracking_stopped = 0;
int stop_track_i18n_folder = 0;

/**
 * @brief Applies selected language pack to all GUI controls.
 */
void ApplyLanguagePack(void)
{
    char lang_name[MAX_PATH];
    char path[MAX_PATH];
    udefrag_progress_info pi;
    MENUITEMINFOW mi;
    wchar_t *text;
    wchar_t *s = L"";
    wchar_t buffer[256];
    int i;
    LVCOLUMNW lvc;
    
    /* read lang.ini file */
    GetPrivateProfileString("Language","Selected","",lang_name,MAX_PATH,".\\lang.ini");
    if(lang_name[0] == 0){
        etrace("selected language name not found in lang.ini file");
        /* assign default strings to the toolbar tooltips */
        UpdateToolbarTooltips();
        return;
    }
    _snprintf(path,MAX_PATH,".\\\\i18n\\\\%s.lng",lang_name);
    path[MAX_PATH - 1] = 0;
    
    /* destroy resource table */
    WgxDestroyResourceTable(i18n_table);
    
    /* build new resource table */
    WgxBuildResourceTable(i18n_table,path);
    
    /* apply new strings to the list of volumes */
    lvc.mask = LVCF_TEXT;
    lvc.pszText = text = WgxGetResourceString(i18n_table,"VOLUME");
    if(text){
        SendMessage(hList,LVM_SETCOLUMNW,0,(LPARAM)&lvc);
        free(text);
    }
    lvc.pszText =  text = WgxGetResourceString(i18n_table,"STATUS");
    if(text){
        SendMessage(hList,LVM_SETCOLUMNW,1,(LPARAM)&lvc);
        free(text);
    }
    lvc.pszText =  text = WgxGetResourceString(i18n_table,"FRAGMENTATION");
    if(text){
        SendMessage(hList,LVM_SETCOLUMNW,2,(LPARAM)&lvc);
        free(text);
    }
    lvc.pszText =  text = WgxGetResourceString(i18n_table,"TOTAL");
    if(text){
        SendMessage(hList,LVM_SETCOLUMNW,3,(LPARAM)&lvc);
        free(text);
    }
    lvc.pszText =  text = WgxGetResourceString(i18n_table,"FREE");
    if(text){
        SendMessage(hList,LVM_SETCOLUMNW,4,(LPARAM)&lvc);
        free(text);
    }
    lvc.pszText =  text = WgxGetResourceString(i18n_table,"PERCENT");
    if(text){
        SendMessage(hList,LVM_SETCOLUMNW,5,(LPARAM)&lvc);
        free(text);
    }
    
    /* apply new strings to the main menu */
    for(i = 0; menu_items[i].id; i++){
        s = WgxGetResourceString(i18n_table,menu_items[i].key);
        if(s){
            if(menu_items[i].hotkeys)
                _snwprintf(buffer,256,L"%ws\t%hs",s,menu_items[i].hotkeys);
            else
                _snwprintf(buffer,256,L"%ws",s);
            buffer[255] = 0;
            memset(&mi,0,MENUITEMINFOW_SIZE);
            mi.cbSize = MENUITEMINFOW_SIZE;
            mi.fMask = MIIM_TYPE;
            mi.fType = MFT_STRING;
            mi.dwTypeData = buffer;
            SetMenuItemInfoW(hMainMenu,menu_items[i].id,FALSE,&mi);
            free(s);
        }
    }
    
    /* apply new strings to the toolbar */
    UpdateToolbarTooltips();
    
    /* redraw main menu */
    if(!DrawMenuBar(hWindow))
        letrace("cannot redraw main menu");
    
    /* refresh volume status fields */
    update_status_of_all_jobs();
    
    /* refresh status bar */
    if(current_job){
        UpdateStatusBar(&current_job->pi);
    } else {
        memset(&pi,0,sizeof(udefrag_progress_info));
        UpdateStatusBar(&pi);
    }
    
    /* update taskbar icon overlay */
    if(show_taskbar_icon_overlay){
        if(WaitForSingleObject(hTaskbarIconEvent,INFINITE) != WAIT_OBJECT_0){
            letrace("wait on hTaskbarIconEvent failed");
        } else {
            RemoveTaskbarIconOverlay();
            if(job_is_running){
                if(pause_flag)
                    SetTaskbarIconOverlay(IDI_PAUSED,"JOB_IS_PAUSED");
                else
                    SetTaskbarIconOverlay(IDI_BUSY,"JOB_IS_RUNNING");
            }
            SetEvent(hTaskbarIconEvent);
        }
    }
    
    /* define whether to use custom font for dialog boxes or not */
    use_custom_font_in_dialogs = 0;
    for(i = 0; special_font_languages[i]; i++){
        if(strcmp(lang_name,special_font_languages[i]) == 0){
            use_custom_font_in_dialogs = 1;
            break;
        }
    }
}

/**
 * @brief Auxiliary routine used to sort file names in binary tree.
 */
static int names_compare(const void *prb_a, const void *prb_b, void *prb_param)
{
    return _wcsicmp((wchar_t *)prb_a,(wchar_t *)prb_b);
}

/**
 * @brief Auxiliary routine used to free memory allocated for tree items.
 */
static void free_item (void *prb_item, void *prb_param)
{
    wchar_t *item = (wchar_t *)prb_item;
    free(item);
}

/**
 * @brief Builds Language menu.
 */
void BuildLanguageMenu(void)
{
    MENUITEMINFO mi;
    wchar_t *text;
    HMENU hLangMenu;
    intptr_t h;
    struct _wfinddata_t lng_file;
    wchar_t filename[MAX_PATH];
    int i, length;
    wchar_t selected_lang_name[MAX_PATH];
    UINT flags;
    
    struct prb_table *pt;
    struct prb_traverser t;
    wchar_t *f;

    /* synchronize with other threads */
    if(WaitForSingleObject(hLangMenuEvent,INFINITE) != WAIT_OBJECT_0){
        letrace("wait on hLangMenuEvent failed");
        return;
    }
    
    /* get selected language name */
    GetPrivateProfileStringW(L"Language",L"Selected",L"",selected_lang_name,MAX_PATH,L".\\lang.ini");
    if(selected_lang_name[0] == 0)
        wcscpy(selected_lang_name,L"English (US)");

    /* get submenu handle */
    memset(&mi,0,MENUITEMINFO_SIZE);
    mi.cbSize = MENUITEMINFO_SIZE;
    mi.fMask = MIIM_SUBMENU;
    if(!GetMenuItemInfo(hMainMenu,IDM_LANGUAGE,FALSE,&mi)){
        letrace("cannot get submenu handle");
        SetEvent(hLangMenuEvent);
        return;
    }
    hLangMenu = mi.hSubMenu;
    
    /* detach submenu */
    memset(&mi,0,MENUITEMINFO_SIZE);
    mi.cbSize = MENUITEMINFO_SIZE;
    mi.fMask = MIIM_SUBMENU;
    mi.hSubMenu = NULL;
    if(!SetMenuItemInfo(hMainMenu,IDM_LANGUAGE,FALSE,&mi)){
        letrace("cannot detach submenu");
        SetEvent(hLangMenuEvent);
        return;
    }
    
    /* destroy submenu */
    DestroyMenu(hLangMenu);
    
    /* build new menu from the list of installed files */
    hLangMenu = CreatePopupMenu();
    if(hLangMenu == NULL){
        letrace("cannot create submenu");
        SetEvent(hLangMenuEvent);
        return;
    }
    
    /* add translation menu items and a separator */
    text = WgxGetResourceString(i18n_table,"TRANSLATIONS_CHANGE_LOG");
    if(text){
        if(!AppendMenuW(hLangMenu,MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_CHANGE_LOG,text))
            letrace("cannot append change log");
        free(text);
    } else {
        if(!AppendMenuW(hLangMenu,MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_CHANGE_LOG,L"&View change log"))
            letrace("cannot append change log");
    }
    text = WgxGetResourceString(i18n_table,"TRANSLATIONS_REPORT");
    if(text){
        if(!AppendMenuW(hLangMenu,MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_REPORT,text))
            letrace("cannot append report");
        free(text);
    } else {
        if(!AppendMenuW(hLangMenu,MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_REPORT,L"View translation &report"))
            letrace("cannot append report");
    }
    text = WgxGetResourceString(i18n_table,"TRANSLATIONS_FOLDER");
    if(text){
        if(!AppendMenuW(hLangMenu,MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_FOLDER,text))
            letrace("cannot append folder");
        free(text);
    } else {
        if(!AppendMenuW(hLangMenu,MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_FOLDER,L"&Translations folder"))
            letrace("cannot append folder");
    }
    text = WgxGetResourceString(i18n_table,"TRANSLATIONS_SUBMIT");
    if(text){
        if(!AppendMenuW(hLangMenu,MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_SUBMIT,text))
            letrace("cannot append submit");
        free(text);
    } else {
        if(!AppendMenuW(hLangMenu,MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_SUBMIT,L"&Submit current translation"))
            letrace("cannot append submit");
    }
    AppendMenu(hLangMenu,MF_SEPARATOR,0,NULL);
    
    h = _wfindfirst(L".\\i18n\\*.lng",&lng_file);
    if(h == -1){
        etrace("no language packs found");
no_files_found:
        /* add default US English */
        if(!AppendMenu(hLangMenu,MF_STRING | MF_ENABLED | MF_CHECKED,IDM_LANGUAGE + 0x1,"English (US)")){
            letrace("cannot append menu item");
            DestroyMenu(hLangMenu);
            SetEvent(hLangMenuEvent);
            return;
        }
    } else {
        /* use binary tree to sort items */
        pt = prb_create(names_compare,NULL,NULL);
        if(pt == NULL){
            /* this case is extraordinary */
            letrace("prb_create failed");
            _findclose(h);
            goto no_files_found;
        }
        wcsncpy(filename,lng_file.name,MAX_PATH - 1);
        filename[MAX_PATH - 1] = 0;
        length = wcslen(filename);
        if(length > (int)wcslen(L".lng"))
            filename[length - 4] = 0;
        f = _wcsdup(filename);
        if(f == NULL){
            mtrace();
        } else {
            if(prb_probe(pt,(void *)f) == NULL){
                etrace("prb_probe failed for %ws",f);
                free(f);
            }
        }
        while(_wfindnext(h,&lng_file) == 0){
            wcsncpy(filename,lng_file.name,MAX_PATH - 1);
            filename[MAX_PATH - 1] = 0;
            length = wcslen(filename);
            if(length > (int)wcslen(L".lng"))
                filename[length - 4] = 0;
            f = _wcsdup(filename);
            if(f == NULL){
                mtrace();
            } else {
                if(prb_probe(pt,(void *)f) == NULL){
                    etrace("prb_probe failed for %ws",f);
                    free(f);
                }
            }
        }
        _findclose(h);
        
        /* build the menu */
        i = 0x1;
        prb_t_init(&t,pt);
        f = (wchar_t *)prb_t_first(&t,pt);
        while(f != NULL){
            flags = MF_STRING | MF_ENABLED;
            if(wcscmp(selected_lang_name,f) == 0)
                flags |= MF_CHECKED;
            if(!AppendMenuW(hLangMenu,flags,IDM_LANGUAGE + i,f)){
                letrace("cannot append menu item");
            } else {
                i++;
            }
            f = (wchar_t *)prb_t_next(&t);
        }
        
        /* destroy binary tree */
        prb_destroy(pt,free_item);
    }
    
    /* attach submenu to the Language menu */
    memset(&mi,0,MENUITEMINFO_SIZE);
    mi.cbSize = MENUITEMINFO_SIZE;
    mi.fMask = MIIM_SUBMENU;
    mi.hSubMenu = hLangMenu;
    if(!SetMenuItemInfo(hMainMenu,IDM_LANGUAGE,FALSE,&mi)){
        letrace("cannot attach submenu");
        DestroyMenu(hLangMenu);
        SetEvent(hLangMenuEvent);
        return;
    }
    
    /* end of synchronization */
    SetEvent(hLangMenuEvent);
}

/**
 * @internal
 * @brief StartLangIniChangesTracking thread routine.
 */
DWORD WINAPI LangIniChangesTrackingProc(LPVOID lpParameter)
{
    HANDLE h;
    DWORD status;
    wchar_t selected_lang_name[MAX_PATH];
    wchar_t text[MAX_PATH];
    MENUITEMINFOW mi;
    int i;
    
    h = FindFirstChangeNotification(".",
            FALSE,FILE_NOTIFY_CHANGE_LAST_WRITE);
    if(h == INVALID_HANDLE_VALUE){
        letrace("FindFirstChangeNotification failed");
        lang_ini_tracking_stopped = 1;
        return 0;
    }
    
    while(!stop_track_lang_ini){
        status = WaitForSingleObject(h,100);
        if(status == WAIT_OBJECT_0){
            ApplyLanguagePack();
            /* update language menu */
            GetPrivateProfileStringW(L"Language",L"Selected",L"",selected_lang_name,MAX_PATH,L".\\lang.ini");
            if(selected_lang_name[0] == 0)
                wcscpy(selected_lang_name,L"English (US)");
            for(i = IDM_LANGUAGE + 1; i < IDM_CFG_GUI; i++){
                if(CheckMenuItem(hMainMenu,i,MF_BYCOMMAND | MF_UNCHECKED) == -1)
                    break;
                /* check the selected language */
                memset(&mi,0,MENUITEMINFOW_SIZE);
                mi.cbSize = MENUITEMINFOW_SIZE;
                mi.fMask = MIIM_TYPE;
                mi.fType = MFT_STRING;
                mi.dwTypeData = text;
                mi.cch = MAX_PATH;
                if(!GetMenuItemInfoW(hMainMenu,i,FALSE,&mi)){
                    letrace("cannot get menu item info");
                } else {
                    if(wcscmp(selected_lang_name,text) == 0)
                        CheckMenuItem(hMainMenu,i,MF_BYCOMMAND | MF_CHECKED);
                }
            }
            /* wait for the next notification */
            if(!FindNextChangeNotification(h)){
                letrace("FindNextChangeNotification failed");
                break;
            }
        }
    }
    
    /* cleanup */
    FindCloseChangeNotification(h);
    lang_ini_tracking_stopped = 1;
    return 0;
}

/**
 * @brief Starts tracking of lang.ini changes.
 */
void StartLangIniChangesTracking()
{
    if(!WgxCreateThread(LangIniChangesTrackingProc,NULL)){
        letrace("cannot create thread for lang.ini changes tracking");
        lang_ini_tracking_stopped = 1;
    }
}

/**
 * @brief Stops tracking of lang.ini changes.
 */
void StopLangIniChangesTracking()
{
    stop_track_lang_ini = 1;
    while(!lang_ini_tracking_stopped)
        Sleep(100);
}

/**
 * @internal
 * @brief StartI18nFolderChangesTracking thread routine.
 */
DWORD WINAPI I18nFolderChangesTrackingProc(LPVOID lpParameter)
{
    HANDLE h;
    DWORD status;
    ULONGLONG counter = 0;
    
    h = FindFirstChangeNotification(".\\i18n",
            FALSE,FILE_NOTIFY_CHANGE_LAST_WRITE \
            | FILE_NOTIFY_CHANGE_FILE_NAME \
            | FILE_NOTIFY_CHANGE_DIR_NAME \
            | FILE_NOTIFY_CHANGE_SIZE);
    if(h == INVALID_HANDLE_VALUE){
        letrace("FindFirstChangeNotification failed");
        i18n_folder_tracking_stopped = 1;
        return 0;
    }
    
    while(!stop_track_i18n_folder){
        status = WaitForSingleObject(h,100);
        if(status == WAIT_OBJECT_0){
            if(counter % 2 == 0 && !is_nt4){
                /*
                * Do nothing; see comment in 
                * settings.c::PrefsChangesTrackingProc
                */
            } else {
                ApplyLanguagePack();
                /* update language menu anyway */
                BuildLanguageMenu();
            }
            counter ++;
            /* wait for the next notification */
            if(!FindNextChangeNotification(h)){
                letrace("FindNextChangeNotification failed");
                break;
            }
        }
    }
    
    /* cleanup */
    FindCloseChangeNotification(h);
    i18n_folder_tracking_stopped = 1;
    return 0;
}

/**
 * @brief Starts tracking of i18n folder changes.
 */
void StartI18nFolderChangesTracking()
{
    if(!WgxCreateThread(I18nFolderChangesTrackingProc,NULL)){
        letrace("cannot create thread for i18n folder changes tracking");
        i18n_folder_tracking_stopped = 1;
    }
}

/**
 * @brief Stops tracking of i18n folder changes.
 */
void StopI18nFolderChangesTracking()
{
    stop_track_i18n_folder = 1;
    while(!i18n_folder_tracking_stopped)
        Sleep(100);
}

/** @} */
