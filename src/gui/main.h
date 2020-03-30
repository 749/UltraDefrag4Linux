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

#ifndef _UDEFRAG_GUI_MAIN_H_
#define _UDEFRAG_GUI_MAIN_H_

/*
* To include ICC_STANDARD_CLASSES definition on mingw
* the _WIN32_WINNT constant must be set at least to 0x501.
*/
#if defined(__GNUC__)
#define _WIN32_WINNT 0x501
#endif
#include <windows.h>

#ifndef _WIN64
/* force menu handling to work on nt4 */
#define MENUITEMINFO_SIZE  (is_nt4 ? 0x2C : sizeof(MENUITEMINFO))
#define MENUITEMINFOW_SIZE (is_nt4 ? 0x2C : sizeof(MENUITEMINFOW))
#else
#define MENUITEMINFO_SIZE  (sizeof(MENUITEMINFO))
#define MENUITEMINFOW_SIZE (sizeof(MENUITEMINFOW))
#endif

/*
* Next definition is very important for mingw:
* _WIN32_IE must be no less than 0x0400
* to include some important constant definitions.
*/
#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>

#ifndef SHTDN_REASON_MAJOR_OTHER
#define SHTDN_REASON_MAJOR_OTHER   0x00000000
#endif
#ifndef SHTDN_REASON_MINOR_OTHER
#define SHTDN_REASON_MINOR_OTHER   0x00000000
#endif
#ifndef SHTDN_REASON_FLAG_PLANNED
#define SHTDN_REASON_FLAG_PLANNED  0x80000000
#endif
#ifndef EWX_FORCEIFHUNG
#define EWX_FORCEIFHUNG     0x00000010
#endif

#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY    (0x0200)
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>
#include <wchar.h>
#include <process.h>
#include <io.h>
#include <direct.h>
#include <errno.h>

#include "prb.h"

#define lua_c
#include "../lua5.1/lua.h"
#include "../lua5.1/lauxlib.h"
#include "../lua5.1/lualib.h"

#define WgxTraceHandler udefrag_dbg_print
#include "../dll/wgx/wgx.h"

#include "../dll/udefrag/udefrag.h"
#include "../include/ultradfgver.h"

#include "resource.h"

/* default GUI preferences */
#define DEFAULT_MAP_BLOCK_SIZE  4
#define DEFAULT_GRID_LINE_WIDTH 1

/* Main window layout constants (in pixels for 96 DPI). */
#define DEFAULT_WIDTH         658 /* default window width */
#define DEFAULT_HEIGHT        513 /* default window height */
#define MIN_WIDTH             500 /* minimal window width */
#define MIN_HEIGHT            400 /* minimal window height */
#define SPACING               7   /* spacing between controls */
#define VLIST_HEIGHT          130 /* volume list height */
#define MIN_LIST_HEIGHT       40  /* minimal list height */

/*
* An article of Mumtaz Zaheer from Pakistan helped us very much
* to make a valid subclassing:
* http://www.codeproject.com/KB/winsdk/safesubclassing.aspx
*/

/*
* The 'Flicker Free Drawing' article of James Brown
* helped us to reduce GUI flicker on window resize:
* http://www.catch22.net/tuts/flicker
*/

/*
* Due to some reason GetClientRect returns
* improper coordinates, therefore we're
* calculating width as right - left instead of
* right - left + 1, which would be more correct.
*/

typedef struct _udefrag_map {
    HBITMAP hbitmap;      /* bitmap used to draw map on */
    HDC hdc;              /* device context of the bitmap */
    int width;            /* width of the map, in pixels */
    int height;           /* height of the map, in pixels */
    char *buffer;         /* internal map representation */
    int size;             /* size of internal representation, in bytes */
    char *scaled_buffer;  /* scaled map representation */
    int scaled_size;      /* size of scaled representation, in bytes */
} udefrag_map;

typedef struct _volume_processing_job {
    char volume_letter;
    int dirty_volume;
    udefrag_job_type job_type;
    int termination_flag;
    udefrag_progress_info pi;
    udefrag_map map;
} volume_processing_job;

/* a type of the job being never executed */
#define NEVER_EXECUTED_JOB 0x100

/* prototypes */
void init_jobs(void);
volume_processing_job *get_job(char volume_letter);
int get_job_index(volume_processing_job *job);
void update_status_of_all_jobs(void);
void start_selected_jobs(udefrag_job_type job_type);
void stop_all_jobs(void);
void release_jobs(void);

void SetPause(void);
void ReleasePause(void);

void RepairSelectedVolumes(void);

int CreateMainMenu(void);
int CreateToolbar(void);
void UpdateToolbarTooltips(void);

void ApplyLanguagePack(void);
void BuildLanguageMenu(void);

/*void InitProgress(void);
void ShowProgress(void);
void HideProgress(void);
void SetProgress(wchar_t *message, int percentage);
*/
void AboutBox(void);
void ShowReports(void);

void InitFont(void);

extern HANDLE hListEvent;
void InitVolList(void);
void VolListNotifyHandler(LPARAM lParam);
void VolListGetColumnWidths(void);
void UpdateVolList(void);
void VolListUpdateStatusField(volume_processing_job *job);
void VolListUpdateFragmentationField(volume_processing_job *job);
void VolListRefreshItem(volume_processing_job *job);
void ReleaseVolList(void);
void SelectAllDrives(void);
void SetVolumeDirtyStatus(int index,volume_info *v);

void InitMap(void);
void RedrawMap(volume_processing_job *job, int map_refill_required);
void ReleaseMap(void);

void CreateStatusBar(void);
void UpdateStatusBar(udefrag_progress_info *pi);

void ResizeMap(int x, int y, int width, int height);
int  ResizeVolList(int x, int y, int width, int height, int expand);
int  GetMinVolListHeight(void);
int  GetMaxVolListHeight(void);
int  ResizeStatusBar(int bottom, int width);

void GetPrefs(void);
void SavePrefs(void);
int  IsBootTimeDefragEnabled(void);

void CheckForTheNewVersion(void);

void StartPrefsChangesTracking();
void StopPrefsChangesTracking();
void StartBootExecChangesTracking();
void StopBootExecChangesTracking();
void StartLangIniChangesTracking();
void StopLangIniChangesTracking();
void StartI18nFolderChangesTracking();
void StopI18nFolderChangesTracking();

int ShutdownOrHibernate(void);

void OpenWebPage(char *page, char *anchor);

extern HANDLE hTaskbarIconEvent;
extern int job_is_running;
void SetTaskbarIconOverlay(int resource_id, char *description_key);
void RemoveTaskbarIconOverlay(void);

#define WM_TRAYMESSAGE           (WM_APP+1)
#define WM_MAXIMIZE_MAIN_WINDOW  (WM_USER + 1)
#define WM_RESIZE_MAP            (WM_USER + 1)

BOOL ShowSystemTrayIcon(DWORD dwMessage);
BOOL HideSystemTrayIcon(void);
void ShowSystemTrayIconContextMenu(void);
void SetSystemTrayIconTooltip(wchar_t *text);

void StartCrashInfoCheck(void);
void StopCrashInfoCheck(void);

/* common global variables */
extern int is_nt4;
extern HINSTANCE hInstance;
extern HWND hWindow;
extern HWND hList;
extern HWND hMap;
extern HWND hStatus;
extern HMENU hMainMenu;
extern HWND hToolbar;
extern double fScale;
extern WGX_FONT wgxFont;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];
extern volume_processing_job *current_job;
extern HANDLE hMapEvent;
extern int busy_flag;
extern int when_done_action;
extern int shutdown_requested;
extern int exit_pressed;
extern int boot_time_defrag_enabled;
extern HANDLE hLangMenuEvent;
extern int use_custom_font_in_dialogs;
extern int portable_mode;
extern int btd_installed;

extern int pause_flag;
extern int stop_pressed;

/* common preferences */
extern int seconds_for_shutdown_rejection;
extern int scale_by_dpi;
extern int restore_default_window_size;
extern int maximized_window;
extern int init_maximized_window;
extern int skip_removable;
extern int disable_latest_version_check;
extern int user_defined_column_widths[];
extern int list_height;
extern int dry_run;
extern int job_flags;
extern int sorting_flags;
extern int repeat_action;
extern int show_menu_icons;
extern int show_taskbar_icon_overlay;
extern int show_progress_in_taskbar;
extern int minimize_to_system_tray;

/*
* NOTE: the following code causes a deadlock
* because the sending thread is blocked until
* the receiving thread processes the message.
*
* HWND hWindow;
* int stop;
* int done;
*
* DWORD WINAPI ThreadProc(LPVOID lpParameter)
* {
*     done = 0;
*     while(!stop){
*         Sleep(100);
*     }
*     SendMessage(hWindow, ...); // waits for WM_DESTROY
*                                // handling completion
*     done = 1;
*     return 0;
* }
*
* window_proc(HWND hWnd, UINT uMsg, ...)
* {
*     if(uMsg == WM_CREATE){
*         hWindow = hWnd;
*         stop = 0;
*         create_thread(ThreadProc,NULL,NULL);
*     } else if(uMsg == WM_DESTROY){ // waits for done flag
*         stop = 1;
*         while(!done){
*             Sleep(100);
*         }
*     }
* }
*/

#define UNDEFINED_COORD (-10000)

/* this macro converts pixels from 96 DPI to the current one */
#define DPI(x) ((int)((double)x * fScale))

/* window layout constants, used in shutdown confirmation and about dialogs */
/* based on layout guidelines: http://msdn.microsoft.com/en-us/library/aa511279.aspx */
#define ICON_SIZE      DPI(32) /* size of the shutdown icon */
#define SHIP_WIDTH     109
#define SHIP_HEIGHT    147
#define SMALL_SPACING  DPI(7)  /* spacing between related controls */
#define LARGE_SPACING  DPI(11) /* spacing between unrelated controls */
#define MARGIN         DPI(11) /* dialog box margins */
#define MIN_BTN_WIDTH  DPI(75) /* recommended button width */
#define MIN_BTN_HEIGHT DPI(23) /* recommended button height */

/* flags for the preview menu */
#define SORT_BY_PATH               0x1
#define SORT_BY_SIZE               0x2
#define SORT_BY_CREATION_TIME      0x4
#define SORT_BY_MODIFICATION_TIME  0x8
#define SORT_BY_ACCESS_TIME        0x10
#define SORT_ASCENDING             0x20
#define SORT_DESCENDING            0x40

/* volume list characteristics */
#define LIST_COLUMNS 6
#define C1_DEFAULT_WIDTH 110
#define C2_DEFAULT_WIDTH 110
#define C3_DEFAULT_WIDTH 110
#define C4_DEFAULT_WIDTH 110
#define C5_DEFAULT_WIDTH 110
#define C6_DEFAULT_WIDTH 65

#endif /* _UDEFRAG_GUI_MAIN_H_ */
