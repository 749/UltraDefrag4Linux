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
 * @file settings.c
 * @brief Preferences.
 * @addtogroup Preferences
 * @{
 */

#include "main.h"

/*
* GUI specific options
*/
int seconds_for_shutdown_rejection = 60;
int scale_by_dpi = 1;
int restore_default_window_size = 0;
int maximized_window = 0;
int init_maximized_window = 0;
int skip_removable = TRUE;
int disable_latest_version_check = 0;
int user_defined_column_widths[] = {0,0,0,0,0,0};
int list_height = 0;
int repeat_action = FALSE;
int show_menu_icons = 1;
int show_taskbar_icon_overlay = 1;
int show_progress_in_taskbar = 1;
int minimize_to_system_tray = 0;

int rx = UNDEFINED_COORD;
int ry = UNDEFINED_COORD;
int rwidth = 0;
int rheight = 0;

int dry_run;

extern RECT r_rc;
extern int map_block_size;
extern int grid_line_width;
extern int grid_color_r;
extern int grid_color_g;
extern int grid_color_b;
extern int free_color_r;
extern int free_color_g;
extern int free_color_b;
extern int last_block_size;
extern int last_grid_width;
extern int last_x;
extern int last_y;
extern int last_width;
extern int last_height;

int stop_track_changes = 0;
int changes_tracking_stopped = 0;
int stop_track_boot_exec = 0;
int boot_exec_tracking_stopped = 0;
extern int boot_time_defrag_enabled;

RECT map_rc = {0,0,0,0};

/* options read from guiopts.lua */
WGX_OPTION read_only_options[] = {
    /* name, type, value buffer, buffer length, default value */
    {"disable_latest_version_check", WGX_CFG_INT, &disable_latest_version_check, 0, 0},
    {"dry_run", WGX_CFG_INT, &dry_run, 0, 0},
    {"free_color_r", WGX_CFG_INT, &free_color_r, 0, 255},
    {"free_color_g", WGX_CFG_INT, &free_color_g, 0, 255},
    {"free_color_b", WGX_CFG_INT, &free_color_b, 0, 255},
    {"grid_color_r", WGX_CFG_INT, &grid_color_r, 0, 0},
    {"grid_color_g", WGX_CFG_INT, &grid_color_g, 0, 0},
    {"grid_color_b", WGX_CFG_INT, &grid_color_b, 0, 0},
    {"grid_line_width", WGX_CFG_INT, &grid_line_width, 0, DEFAULT_GRID_LINE_WIDTH},
    {"map_block_size", WGX_CFG_INT, &map_block_size, 0, DEFAULT_MAP_BLOCK_SIZE},
    {"minimize_to_system_tray", WGX_CFG_INT, &minimize_to_system_tray, 0, 0},
    {"restore_default_window_size", WGX_CFG_INT, &restore_default_window_size, 0, 0},
    {"scale_by_dpi", WGX_CFG_INT, &scale_by_dpi, 0, 1},
    {"seconds_for_shutdown_rejection", WGX_CFG_INT, &seconds_for_shutdown_rejection, 0, 60},
    {"show_menu_icons", WGX_CFG_INT, &show_menu_icons, 0, 1},
    {"show_progress_in_taskbar", WGX_CFG_INT, &show_progress_in_taskbar, 0, 1},
    {"show_taskbar_icon_overlay", WGX_CFG_INT, &show_taskbar_icon_overlay, 0, 1},
    {NULL, 0, NULL, 0, 0}
};

/* options stored in guiopts-internals.lua */
WGX_OPTION internal_options[] = {
    /* name, type, value buffer, buffer length, default value */
    {"the settings below are not changeable by the user,", WGX_CFG_COMMENT, NULL, 0, 0},
    {"they are always overwritten when the program ends", WGX_CFG_COMMENT, NULL, 0, 0},
    
    {"rx", WGX_CFG_INT, &rx, 0, UNDEFINED_COORD},
    {"ry", WGX_CFG_INT, &ry, 0, UNDEFINED_COORD},
    {"rwidth", WGX_CFG_INT, &rwidth, 0, DEFAULT_WIDTH},
    {"rheight", WGX_CFG_INT, &rheight, 0, DEFAULT_HEIGHT}, 
    {"maximized", WGX_CFG_INT, &maximized_window, 0, 0},
    {"", WGX_CFG_EMPTY, NULL, 0, 0},

    {"skip_removable", WGX_CFG_INT, &skip_removable, 0, 1},
    {"repeat_action", WGX_CFG_INT, &repeat_action, 0, 0},
    {"", WGX_CFG_EMPTY, NULL, 0, 0},

    {"column1_width", WGX_CFG_INT, &user_defined_column_widths[0], 0, C1_DEFAULT_WIDTH},
    {"column2_width", WGX_CFG_INT, &user_defined_column_widths[1], 0, C2_DEFAULT_WIDTH},
    {"column2b_width", WGX_CFG_INT, &user_defined_column_widths[2], 0, C3_DEFAULT_WIDTH},
    {"column3_width", WGX_CFG_INT, &user_defined_column_widths[3], 0, C4_DEFAULT_WIDTH},
    {"column4_width", WGX_CFG_INT, &user_defined_column_widths[4], 0, C5_DEFAULT_WIDTH},
    {"column5_width", WGX_CFG_INT, &user_defined_column_widths[5], 0, C6_DEFAULT_WIDTH},
    {"list_height", WGX_CFG_INT, &list_height, 0, 0},
    {"", WGX_CFG_EMPTY, NULL, 0, 0},

    {"job_flags", WGX_CFG_INT, &job_flags, 0, 0},
    {"sorting_flags", WGX_CFG_INT, &sorting_flags, 0, SORT_BY_PATH | SORT_ASCENDING},
    {"", WGX_CFG_EMPTY, NULL, 0, 0},
    {NULL, 0, NULL, 0, 0}
};

/**
 * @brief Cleans up the environment
 * by removing all the variables
 * controlling the program behavior.
 */
static void CleanupEnvironment(void)
{
    (void)SetEnvironmentVariable("UD_IN_FILTER",NULL);
    (void)SetEnvironmentVariable("UD_EX_FILTER",NULL);
    (void)SetEnvironmentVariable("UD_FRAGMENT_SIZE_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_FILE_SIZE_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_OPTIMIZER_FILE_SIZE_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_FRAGMENTS_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_FRAGMENTATION_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_REFRESH_INTERVAL",NULL);
    (void)SetEnvironmentVariable("UD_DISABLE_REPORTS",NULL);
    (void)SetEnvironmentVariable("UD_DBGPRINT_LEVEL",NULL);
    (void)SetEnvironmentVariable("UD_LOG_FILE_PATH",NULL);
    (void)SetEnvironmentVariable("UD_TIME_LIMIT",NULL);
    (void)SetEnvironmentVariable("UD_DRY_RUN",NULL);
    (void)SetEnvironmentVariable("UD_SORTING",NULL);
    (void)SetEnvironmentVariable("UD_SORTING_ORDER",NULL);
}

/**
 * @brief Validates GUI options
 * by passing them through Lua
 * interpreter.
 */
static void ValidateGUIOptions(void)
{
    FILE *f;
    
    /* check file existence */
    f = fopen(".\\options\\guiopts.lua","r");
    if(f) fclose(f);
    else return;

    /* run Lua interpreter */
    (void)WgxCreateProcess(".\\lua5.1a_gui.exe", ".\\options\\guiopts.lua");
}

void GetPrefs(void)
{
    /*
    * The program should be configurable
    * through guiopts.lua file only.
    */
    CleanupEnvironment();
    
    ValidateGUIOptions();
    WgxGetOptions(".\\options\\guiopts.lua",read_only_options);
    WgxGetOptions(".\\options\\guiopts-internals.lua",internal_options);
    
    /* get restored main window coordinates */
    r_rc.left = rx;
    r_rc.top = ry;
    r_rc.right = rx + rwidth;
    r_rc.bottom = ry + rheight;
    
    init_maximized_window = maximized_window;
    if(map_block_size < 0)
        map_block_size = DEFAULT_MAP_BLOCK_SIZE;
    if(grid_line_width < 0)
        grid_line_width = DEFAULT_GRID_LINE_WIDTH;

    (void)udefrag_set_log_file_path();
}

void SavePrefsCallback(char *error)
{
    MessageBox(NULL,error,"Warning!",MB_OK | MB_ICONWARNING);
}

void SavePrefs(void)
{
    rx = (int)r_rc.left;
    ry = (int)r_rc.top;
    rwidth = (int)(r_rc.right - r_rc.left);
    rheight = (int)(r_rc.bottom - r_rc.top);

    if(_mkdir(".\\options") < 0){
        if(errno != EEXIST)
            etrace("cannot create .\\options directory: errno = %u",errno);
    }
    WgxSaveOptions(".\\options\\guiopts-internals.lua",internal_options,SavePrefsCallback);
}

/**
 * @brief Initializes default font
 * and replaces it by a custom one
 * if an appropriate configuration
 * file exists.
 */
void InitFont(void)
{
    memset(&wgxFont.lf,0,sizeof(LOGFONT));
    /* default font should be Courier New 9pt */
    (void)strcpy(wgxFont.lf.lfFaceName,"Courier New");
    wgxFont.lf.lfHeight = DPI(-12);
    WgxCreateFont(".\\options\\font.lua",&wgxFont);
}

/**
 * @internal
 * @brief StartPrefsChangesTracking thread routine.
 */
DWORD WINAPI PrefsChangesTrackingProc(LPVOID lpParameter)
{
    HANDLE h;
    DWORD status;
    RECT rc;
    int s_maximized, s_init_maximized;
    int s_skip_removable;
    int s_repeat_action;
    int s_job_flags;
    int cw[sizeof(user_defined_column_widths) / sizeof(int)];
    int s_list_height;
    int s_show_taskbar_icon_overlay;
    int s_minimize_to_system_tray;
    ULONGLONG counter = 0;
    
    h = FindFirstChangeNotification(".\\options",
            FALSE,FILE_NOTIFY_CHANGE_LAST_WRITE);
    if(h == INVALID_HANDLE_VALUE){
        letrace("FindFirstChangeNotification failed");
        changes_tracking_stopped = 1;
        return 0;
    }
    
    while(!stop_track_changes){
        status = WaitForSingleObject(h,100);
        if(status == WAIT_OBJECT_0){
            if(counter % 2 == 0 && !is_nt4){
                /*
                * Do nothing, since
                * "If a change occurs after a call 
                * to FindFirstChangeNotification but
                * before a call to FindNextChangeNotification,
                * the operating system records the change.
                * When FindNextChangeNotification is executed,
                * the recorded change immediately satisfies 
                * a wait for the change notification." (MSDN)
                * And so on... it happens between
                * FindNextChangeNotification calls too.
                *
                * However, everything mentioned above is not true
                * when the program modifies the file itself.
                */
            } else {
                /* synchronize preferences reload with map redraw */
                if(WaitForSingleObject(hMapEvent,INFINITE) != WAIT_OBJECT_0){
                    letrace("wait on hMapEvent failed");
                } else {
                    /* save state */
                    memcpy(&rc,&r_rc,sizeof(RECT));
                    s_maximized = maximized_window;
                    s_init_maximized = init_maximized_window;
                    s_skip_removable = skip_removable;
                    s_repeat_action = repeat_action;
                    memcpy(&cw,&user_defined_column_widths,sizeof(user_defined_column_widths));
                    s_list_height = list_height;
                    s_job_flags = job_flags;
                    s_show_taskbar_icon_overlay = show_taskbar_icon_overlay;
                    s_minimize_to_system_tray = minimize_to_system_tray;
                    
                    /* reload preferences */
                    GetPrefs();
                    
                    /* restore state */
                    memcpy(&r_rc,&rc,sizeof(RECT));
                    maximized_window = s_maximized;
                    init_maximized_window = s_init_maximized;
                    skip_removable = s_skip_removable;
                    repeat_action = s_repeat_action;
                    memcpy(&user_defined_column_widths,&cw,sizeof(user_defined_column_widths));
                    list_height = s_list_height;
                    job_flags = s_job_flags;
                    
                    SetEvent(hMapEvent);
    
                    if(dry_run == 0){
                        if(portable_mode) SetWindowText(hWindow,VERSIONINTITLE_PORTABLE);
                        else SetWindowText(hWindow,VERSIONINTITLE);
                    } else {
                        if(portable_mode) SetWindowText(hWindow,VERSIONINTITLE_PORTABLE " (dry run)");
                        else SetWindowText(hWindow,VERSIONINTITLE " (dry run)");
                    }
    
                    /* if block size or grid line width changed since last redraw, resize map */
                    if(map_block_size != last_block_size  || grid_line_width != last_grid_width){
                        /* ResizeMap is not safe to be called from threads */
                        map_rc.left = last_x;
                        map_rc.top = last_y;
                        map_rc.right = last_width;
                        map_rc.bottom = last_height;
                        PostMessage(hMap,WM_RESIZE_MAP,0,(LPARAM)&map_rc);
                        InvalidateRect(hMap,NULL,TRUE);
                        UpdateWindow(hMap);
                    } else {
                        /* redraw map if grid color changed */
                        RedrawMap(current_job,0);
                    }
                    
                    /* handle show_taskbar_icon_overlay and minimize_to_system_tray options adjustment */
                    if(WaitForSingleObject(hTaskbarIconEvent,INFINITE) != WAIT_OBJECT_0){
                        letrace("wait on hTaskbarIconEvent failed");
                    } else {
                        if(show_taskbar_icon_overlay != s_show_taskbar_icon_overlay){
                            if(show_taskbar_icon_overlay && job_is_running){
                                if(pause_flag)
                                    SetTaskbarIconOverlay(IDI_PAUSED,"JOB_IS_PAUSED");
                                else
                                    SetTaskbarIconOverlay(IDI_BUSY,"JOB_IS_RUNNING");
                            } else {
                                RemoveTaskbarIconOverlay();
                            }
                        }
                        if(minimize_to_system_tray != s_minimize_to_system_tray){
                            if(IsIconic(hWindow)){
                                if(minimize_to_system_tray)
                                    WgxHideWindow(hWindow);
                                else
                                    WgxShowWindow(hWindow);
                            }
                            /* set/remove notification area icon */
                            if(minimize_to_system_tray)
                                ShowSystemTrayIcon(NIM_ADD);
                            else
                                HideSystemTrayIcon();
                        }
                        SetEvent(hTaskbarIconEvent);
                    }
                }
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
    changes_tracking_stopped = 1;
    return 0;
}

/**
 * @brief Starts tracking of guiopts.lua changes.
 */
void StartPrefsChangesTracking()
{
    if(!WgxCreateThread(PrefsChangesTrackingProc,NULL)){
        letrace("cannot create thread for guiopts.lua changes tracking");
        changes_tracking_stopped = 1;
    }
}

/**
 * @brief Stops tracking of guiopts.lua changes.
 */
void StopPrefsChangesTracking()
{
    stop_track_changes = 1;
    while(!changes_tracking_stopped)
        Sleep(100);
}

/**
 * @brief Defines whether the boot time
 * defragmenter is enabled or not.
 * @return Nonzero value indicates that
 * it is enabled.
 */
int IsBootTimeDefragEnabled(void)
{
    return udefrag_bootex_check(L"defrag_native") > 0 ? 1 : 0;
}

/**
 * @internal
 * @brief StartBootExecTracking thread routine.
 */
DWORD WINAPI BootExecTrackingProc(LPVOID lpParameter)
{
    HKEY hKey;
    HANDLE hEvent;
    LONG error;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Session Manager",
            0,
            KEY_NOTIFY,
            &hKey) != ERROR_SUCCESS){
        letrace("cannot open SMSS key");
        boot_exec_tracking_stopped = 1;
        return 0;
    }
            
    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if(hEvent == NULL){
        letrace("CreateEvent failed");
        goto done;
    }

track_again:    
    error = RegNotifyChangeKeyValue(hKey,FALSE,
        REG_NOTIFY_CHANGE_LAST_SET,hEvent,TRUE);
    if(error != ERROR_SUCCESS){
        etrace("RegNotifyChangeKeyValue failed with code 0x%x",(UINT)error);
        CloseHandle(hEvent);
        goto done;
    }
    
    while(!stop_track_boot_exec){
        if(WaitForSingleObject(hEvent,100) == WAIT_OBJECT_0){
            if(IsBootTimeDefragEnabled()){
                itrace("boot time defragmenter enabled (externally)");
                boot_time_defrag_enabled = 1;
                CheckMenuItem(hMainMenu,
                    IDM_CFG_BOOT_ENABLE,
                    MF_BYCOMMAND | MF_CHECKED);
                SendMessage(hToolbar,TB_CHECKBUTTON,IDM_CFG_BOOT_ENABLE,MAKELONG(TRUE,0));
            } else {
                itrace("boot time defragmenter disabled (externally)");
                boot_time_defrag_enabled = 0;
                CheckMenuItem(hMainMenu,
                    IDM_CFG_BOOT_ENABLE,
                    MF_BYCOMMAND | MF_UNCHECKED);
                SendMessage(hToolbar,TB_CHECKBUTTON,IDM_CFG_BOOT_ENABLE,MAKELONG(FALSE,0));
            }
            goto track_again;
        }
    }
    CloseHandle(hEvent);

done:
    RegCloseKey(hKey);
    boot_exec_tracking_stopped = 1;
    return 0;
}

/**
 * @brief Starts tracking of BootExecute registry value changes.
 */
void StartBootExecChangesTracking()
{
    if(btd_installed){
        if(!WgxCreateThread(BootExecTrackingProc,NULL)){
            letrace("cannot create thread for BootExecute registry value changes tracking");
            boot_exec_tracking_stopped = 1;
        }
    } else {
        boot_exec_tracking_stopped = 1;
    }
}

/**
 * @brief Stops tracking of guiopts.lua changes.
 */
void StopBootExecChangesTracking()
{
    stop_track_boot_exec = 1;
    while(!boot_exec_tracking_stopped)
        Sleep(100);
}

/** @} */
