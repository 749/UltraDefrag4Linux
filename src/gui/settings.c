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
 * @file settings.c
 * @brief Preferences.
 * @addtogroup Preferences
 * @{
 */

#include "main.h"

/*
* These options have the same effect as environment 
* variables accepted by the command line interface.
*/
char in_filter[32767];
char ex_filter[32767];
char file_size_threshold[128];
char timelimit[256];
int fragments_threshold;
int refresh_interval;
int disable_reports;
char dbgprint_level[32] = {0};
char log_file_path[32767] = {0};
int dry_run;

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
int user_defined_column_widths[] = {0,0,0,0,0};
int list_height = 0;
int repeat_action = FALSE;
int show_menu_icons = 1;
int show_taskbar_icon_overlay = 1;

int rx = UNDEFINED_COORD;
int ry = UNDEFINED_COORD;
int rwidth = 0;
int rheight = 0;

extern RECT r_rc;
extern int map_block_size;
extern int grid_line_width;
extern int grid_color_r;
extern int grid_color_g;
extern int grid_color_b;
extern double pix_per_dialog_unit;
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

/* options read from guiopts.lua */
WGX_OPTION read_only_options[] = {
    /* type, value buffer size, name, value, default value */
    {WGX_CFG_STRING,  sizeof(in_filter), "in_filter", in_filter, ""},
    {WGX_CFG_STRING,  sizeof(ex_filter), "ex_filter", ex_filter, ""},
    {WGX_CFG_STRING,  sizeof(file_size_threshold), "file_size_threshold", file_size_threshold, ""},
    {WGX_CFG_INT,     0, "fragments_threshold", &fragments_threshold, 0},
    {WGX_CFG_STRING,  sizeof(timelimit), "time_limit", timelimit, ""},
    {WGX_CFG_INT,     0, "refresh_interval", &refresh_interval, (void *)DEFAULT_REFRESH_INTERVAL},
    {WGX_CFG_INT,     0, "disable_reports", &disable_reports, 0},
    {WGX_CFG_STRING,  sizeof(dbgprint_level), "dbgprint_level", dbgprint_level, ""},
    {WGX_CFG_STRING,  sizeof(log_file_path), "log_file_path", log_file_path, ""},
    {WGX_CFG_INT,     0, "dry_run", &dry_run, 0},
    {WGX_CFG_INT,     0, "seconds_for_shutdown_rejection", &seconds_for_shutdown_rejection, (void *)60},
    {WGX_CFG_INT,     0, "map_block_size", &map_block_size, (void *)DEFAULT_MAP_BLOCK_SIZE},
    {WGX_CFG_INT,     0, "grid_line_width", &grid_line_width, (void *)DEFAULT_GRID_LINE_WIDTH},
    {WGX_CFG_INT,     0, "grid_color_r", &grid_color_r, (void *)0},
    {WGX_CFG_INT,     0, "grid_color_g", &grid_color_g, (void *)0},
    {WGX_CFG_INT,     0, "grid_color_b", &grid_color_b, (void *)0},
    {WGX_CFG_INT,     0, "disable_latest_version_check", &disable_latest_version_check, 0},
    {WGX_CFG_INT,     0, "scale_by_dpi", &scale_by_dpi, (void *)1},
    {WGX_CFG_INT,     0, "restore_default_window_size", &restore_default_window_size, 0},
    {WGX_CFG_INT,     0, "show_menu_icons", &show_menu_icons, 0},
    {WGX_CFG_INT,     0, "show_taskbar_icon_overlay", &show_taskbar_icon_overlay, 0},
    
    {0,               0, NULL, NULL, NULL}
};

/* options stored in guiopts-internals.lua */
WGX_OPTION internal_options[] = {
    /* type, value buffer size, name, value, default value */
    {WGX_CFG_COMMENT, 0, "the settings below are not changeable by the user,", NULL, ""},
    {WGX_CFG_COMMENT, 0, "they are always overwritten when the program ends", NULL, ""},
    
    {WGX_CFG_INT,     0, "rx", &rx, (void *)UNDEFINED_COORD},
    {WGX_CFG_INT,     0, "ry", &ry, (void *)UNDEFINED_COORD},
    {WGX_CFG_INT,     0, "rwidth",  &rwidth,  (void *)DEFAULT_WIDTH},
    {WGX_CFG_INT,     0, "rheight", &rheight, (void *)DEFAULT_HEIGHT}, 
    {WGX_CFG_INT,     0, "maximized", &maximized_window, 0},
    {WGX_CFG_EMPTY,   0, "", NULL, ""},

    {WGX_CFG_INT,     0, "skip_removable", &skip_removable, (void *)1},
    {WGX_CFG_INT,     0, "repeat_action", &repeat_action, (void *)0},
    {WGX_CFG_EMPTY,   0, "", NULL, ""},

    {WGX_CFG_INT,     0, "column1_width", &user_defined_column_widths[0], 0},
    {WGX_CFG_INT,     0, "column2_width", &user_defined_column_widths[1], 0},
    {WGX_CFG_INT,     0, "column3_width", &user_defined_column_widths[2], 0},
    {WGX_CFG_INT,     0, "column4_width", &user_defined_column_widths[3], 0},
    {WGX_CFG_INT,     0, "column5_width", &user_defined_column_widths[4], 0},
    {WGX_CFG_INT,     0, "list_height", &list_height, (void *)0},
    {WGX_CFG_EMPTY,   0, "", NULL, ""},

    {WGX_CFG_INT,     0, "job_flags", &job_flags, (void *)UD_PREVIEW_MATCHING},
    {WGX_CFG_EMPTY,   0, "", NULL, ""},

    {0,               0, NULL, NULL, NULL}
};

void DeleteEnvironmentVariables(void)
{
    (void)SetEnvironmentVariable("UD_IN_FILTER",NULL);
    (void)SetEnvironmentVariable("UD_EX_FILTER",NULL);
    (void)SetEnvironmentVariable("UD_FILE_SIZE_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_FRAGMENTS_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_REFRESH_INTERVAL",NULL);
    (void)SetEnvironmentVariable("UD_DISABLE_REPORTS",NULL);
    (void)SetEnvironmentVariable("UD_DBGPRINT_LEVEL",NULL);
    (void)SetEnvironmentVariable("UD_LOG_FILE_PATH",NULL);
    (void)SetEnvironmentVariable("UD_TIME_LIMIT",NULL);
    (void)SetEnvironmentVariable("UD_DRY_RUN",NULL);
}

void SetEnvironmentVariables(void)
{
    char buffer[128];
    
    if(in_filter[0])
        (void)SetEnvironmentVariable("UD_IN_FILTER",in_filter);
    if(ex_filter[0])
        (void)SetEnvironmentVariable("UD_EX_FILTER",ex_filter);
    if(file_size_threshold[0])
        (void)SetEnvironmentVariable("UD_FILE_SIZE_THRESHOLD",file_size_threshold);
    if(timelimit[0])
        (void)SetEnvironmentVariable("UD_TIME_LIMIT",timelimit);
    if(fragments_threshold){
        (void)sprintf(buffer,"%i",fragments_threshold);
        (void)SetEnvironmentVariable("UD_FRAGMENTS_THRESHOLD",buffer);
    }
    if(refresh_interval){
        (void)sprintf(buffer,"%i",refresh_interval);
        (void)SetEnvironmentVariable("UD_REFRESH_INTERVAL",buffer);
    }
    (void)sprintf(buffer,"%i",disable_reports);
    (void)SetEnvironmentVariable("UD_DISABLE_REPORTS",buffer);
    if(dbgprint_level[0])
        (void)SetEnvironmentVariable("UD_DBGPRINT_LEVEL",dbgprint_level);
    if(log_file_path[0]){
        (void)SetEnvironmentVariable("UD_LOG_FILE_PATH",log_file_path);
        (void)udefrag_set_log_file_path();
    }
    if(dry_run)
        (void)SetEnvironmentVariable("UD_DRY_RUN","1");
}

void GetPrefs(void)
{
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

    DeleteEnvironmentVariables();
    SetEnvironmentVariables();
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
            WgxDbgPrint("Cannot create .\\options directory: errno = %u\n",errno);
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
    
    h = FindFirstChangeNotification(".\\options",
            FALSE,FILE_NOTIFY_CHANGE_LAST_WRITE);
    if(h == INVALID_HANDLE_VALUE){
        WgxDbgPrintLastError("PrefsChangesTrackingProc: FindFirstChangeNotification failed");
        changes_tracking_stopped = 1;
        return 0;
    }
    
    while(!stop_track_changes){
        status = WaitForSingleObject(h,100);
        if(status == WAIT_OBJECT_0){
            /* synchronize preferences reload with map redraw */
            if(WaitForSingleObject(hMapEvent,INFINITE) != WAIT_OBJECT_0){
                WgxDbgPrintLastError("PrefsChangesTrackingProc: wait on hMapEvent failed");
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
                    ResizeMap(last_x,last_y,last_width,last_height);
                    InvalidateRect(hMap,NULL,TRUE);
                    UpdateWindow(hMap);
                } else {
                    /* redraw map if grid color changed */
                    RedrawMap(current_job,0);
                }
            }
            /* wait for the next notification */
            if(!FindNextChangeNotification(h)){
                WgxDbgPrintLastError("PrefsChangesTrackingProc: FindNextChangeNotification failed");
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
    HANDLE h;
    DWORD id;
    
    h = create_thread(PrefsChangesTrackingProc,NULL,&id);
    if(h == NULL){
        WgxDbgPrintLastError("Cannot create thread for guiopts.lua changes tracking");
        changes_tracking_stopped = 1;
    } else {
        CloseHandle(h);
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
    HKEY hKey;
    DWORD type, size;
    char *data, *curr_pos;
    DWORD i, length, curr_len;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Session Manager",
            0,
            KEY_QUERY_VALUE,
            &hKey) != ERROR_SUCCESS){
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot open SMSS key!");
        return FALSE;
    }

    type = REG_MULTI_SZ;
    (void)RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
    data = malloc(size + 10);
    if(!data){
        (void)RegCloseKey(hKey);
        MessageBox(0,"Not enough memory for IsBootTimeDefragEnabled()!",
            "Error",MB_OK | MB_ICONHAND);
        return FALSE;
    }

    type = REG_MULTI_SZ;
    if(RegQueryValueEx(hKey,"BootExecute",NULL,&type,
            data,&size) != ERROR_SUCCESS){
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot query BootExecute value!");
        (void)RegCloseKey(hKey);
        free(data);
        return FALSE;
    }

    length = size - 1;
    for(i = 0; i < length;){
        curr_pos = data + i;
        curr_len = strlen(curr_pos) + 1;
        /* if the command is yet registered then exit */
        if(!strcmp(curr_pos,"defrag_native")){
            (void)RegCloseKey(hKey);
            free(data);
            return TRUE;
        }
        i += curr_len;
    }

    (void)RegCloseKey(hKey);
    free(data);
    return FALSE;
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
        WgxDbgPrintLastError("BootExecTrackingProc: cannot open SMSS key");
        boot_exec_tracking_stopped = 1;
        return 0;
    }
            
    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if(hEvent == NULL){
        WgxDbgPrintLastError("BootExecTrackingProc: CreateEvent failed");
        goto done;
    }

track_again:    
    error = RegNotifyChangeKeyValue(hKey,FALSE,
        REG_NOTIFY_CHANGE_LAST_SET,hEvent,TRUE);
    if(error != ERROR_SUCCESS){
        WgxDbgPrint("BootExecTrackingProc: RegNotifyChangeKeyValue failed with code 0x%x",(UINT)error);
        CloseHandle(hEvent);
        goto done;
    }
    
    while(!stop_track_boot_exec){
        if(WaitForSingleObject(hEvent,100) == WAIT_OBJECT_0){
            if(IsBootTimeDefragEnabled()){
                WgxDbgPrint("Boot time defragmenter enabled (externally)\n");
                boot_time_defrag_enabled = 1;
                CheckMenuItem(hMainMenu,
                    IDM_CFG_BOOT_ENABLE,
                    MF_BYCOMMAND | MF_CHECKED);
                SendMessage(hToolbar,TB_CHECKBUTTON,IDM_CFG_BOOT_ENABLE,MAKELONG(TRUE,0));
            } else {
                WgxDbgPrint("Boot time defragmenter disabled (externally)\n");
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
    HANDLE h;
    DWORD id;

    if(btd_installed){
        h = create_thread(BootExecTrackingProc,NULL,&id);
        if(h == NULL){
            WgxDbgPrintLastError("Cannot create thread for BootExecute registry value changes tracking");
            boot_exec_tracking_stopped = 1;
        } else {
            CloseHandle(h);
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
