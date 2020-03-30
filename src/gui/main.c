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
 * @file main.c
 * @brief Main window.
 * @addtogroup MainWindow
 * @{
 */

/*
* Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
* and Dmitri Arkhangelski <dmitriar@gmail.com>.
*/

#include "main.h"

/* global variables */
HINSTANCE hInstance;
char class_name[64];
HWND hWindow = NULL;
HWND hList = NULL;
HWND hMap = NULL;
WGX_FONT wgxFont = {{0},0};
RECT win_rc; /* coordinates of main window */
RECT r_rc;   /* coordinates of restored window */
double fScale = 1.0f;
UINT TaskbarButtonCreatedMsg = 0;

int when_done_action = IDM_WHEN_DONE_NONE;
int shutdown_requested = 0;
int boot_time_defrag_enabled = 0;
extern int init_maximized_window;
extern int allow_map_redraw;
extern int lang_ini_tracking_stopped;

/*
* MSDN states that environment variables
* are limited by 32767 characters,
* including terminal zero.
*/
#define MAX_ENV_VARIABLE_LENGTH 32766
wchar_t env_buffer[MAX_ENV_VARIABLE_LENGTH + 1];
wchar_t env_buffer2[MAX_ENV_VARIABLE_LENGTH + 1];

/* ensure that initial value is greater than any real value */
int previous_list_height = 10000;

/* nonzero value indicates that we are in portable mode */
int portable_mode = 0;

/* nonzero value indicates that boot time defragmenter is installed */
int btd_installed = 0;

int web_statistics_completed = 0;

/* algorithm preview flags controlled through the preview menu */
int job_flags = UD_PREVIEW_MATCHING;

/* forward declarations */
LRESULT CALLBACK MainWindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
static void DestroySynchObjects(void);

/**
 * @brief Initializes all objects
 * synchronizing access to essential data.
 * @return Zero for success,
 * negative value otherwise.
 */
static int InitSynchObjects(void)
{
    hLangMenuEvent = CreateEvent(NULL,FALSE,TRUE,NULL);
    if(hLangMenuEvent == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot create language menu synchronization event!");
        WgxDbgPrintLastError("InitSynchObjects: language menu event creation failed");
        return (-1);
    }
    hTaskbarIconEvent = CreateEvent(NULL,FALSE,TRUE,NULL);
    if(hTaskbarIconEvent == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot create taskbar icon synchronization event!");
        WgxDbgPrintLastError("InitSynchObjects: taskbar icon event creation failed");
        WgxDbgPrint("no taskbar icon overlays will be shown");
        WgxDbgPrint("and no system tray icon will be shown");
        DestroySynchObjects();
        return (-1);
    }
    hMapEvent = CreateEvent(NULL,FALSE,TRUE,NULL);
    if(hMapEvent == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot create cluster map synchronization event!");
        WgxDbgPrintLastError("InitSynchObjects: map event creation failed");
        DestroySynchObjects();
        return (-1);
    }
    hListEvent = CreateEvent(NULL,FALSE,TRUE,NULL);
    if(hListEvent == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot create drives list synchronization event!");
        WgxDbgPrintLastError("InitSynchObjects: list event creation failed");
        DestroySynchObjects();
        return (-1);
    }
    return 0;
}

/**
 * @brief Destroys all objects
 * synchronizing access to essential data.
 */
static void DestroySynchObjects(void)
{
    if(hLangMenuEvent)
        CloseHandle(hLangMenuEvent);
    if(hTaskbarIconEvent)
        CloseHandle(hTaskbarIconEvent);
    if(hMapEvent)
        CloseHandle(hMapEvent);
    if(hListEvent)
        CloseHandle(hListEvent);
}

/**
 * @brief Defines whether GUI is a part of
 * the portable package or regular installation.
 * @return Nonzero value indicates that it is
 * most likely a part of the portable package.
 */
static int IsPortable(void)
{
    wchar_t cd[MAX_PATH];
    wchar_t instdir[MAX_PATH];
    wchar_t path[MAX_PATH];
    HKEY hRegKey = NULL;
    DWORD path_length = (MAX_PATH - 1) * sizeof(wchar_t);
    REGSAM samDesired = KEY_READ;
    OSVERSIONINFO osvi;
    BOOL bIsWindowsXPorLater;
    LONG result;
    int i;
    
    if(!GetModuleFileNameW(NULL,cd,MAX_PATH)){
        WgxDbgPrintLastError("IsPortable: cannot get module file name");
        return 1;
    }
    
    /* make sure we have a terminating null character,
       if the path is truncated on WinXP and earlier */
    cd[MAX_PATH-1] = 0;
    
    /* strip off the file name */
    i = wcslen(cd);
    while(i >= 0) {
        if(cd[i] == '\\'){
            cd[i] = 0;
            break;
        }
        i--;
    }
    
    /* only XP and later support 32-bit registry view, so check for it */
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    bIsWindowsXPorLater = 
       ( (osvi.dwMajorVersion > 5) ||
       ( (osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion >= 1) ));

    if(bIsWindowsXPorLater)
        samDesired |= KEY_WOW64_32KEY;

    /* get install location from uninstall registry key */
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\UltraDefrag",
        0,samDesired,&hRegKey);
    if(result != ERROR_SUCCESS){
        SetLastError((DWORD)result);
        WgxDbgPrintLastError("IsPortable: cannot open registry");
        return 1;
    }
    
    result = RegQueryValueExW(hRegKey,L"InstallLocation",NULL,NULL,(LPBYTE) &instdir,&path_length);
    RegCloseKey(hRegKey);
    if(result != ERROR_SUCCESS){
        SetLastError((DWORD)result);
        WgxDbgPrintLastError("IsPortable: cannot read registry");
        return 1;
    }
    
    /* make sure we have a trailing zero character */
    instdir[path_length / sizeof(wchar_t)] = 0;
    
    /* strip off any double quotes */
    if(instdir[0] == '"'){
        (void)wcsncpy(path,&instdir[1],wcslen(instdir)-2);
        path[wcslen(instdir)-2] = 0;
    } else {
        (void)wcscpy(path,instdir);
    }
    
    if(udefrag_wcsicmp(path,cd) == 0){
        WgxDbgPrint("Install location \"%ws\" matches \"%ws\", so it isn't portable\n",path,cd);
        return 0;
    }
    
    WgxDbgPrint("Install location \"%ws\" differs from \"%ws\", so it is portable\n",path,cd);
    return 1;
}

/**
 * @brief Defines whether boot time defragmenter is installed or not.
 * @return Nonzero value indicates that it is installed.
 */
static int IsBtdInstalled(void)
{
    char sysdir[MAX_PATH];
    char path[MAX_PATH];
    FILE *f;
    
    if(!GetSystemDirectory(sysdir,MAX_PATH)){
        WgxDbgPrintLastError("IsBtdInstalled: cannot get system directory");
        return 1; /* let's assume that it is installed */
    }
    
    _snprintf(path,MAX_PATH,"%s\\defrag_native.exe",sysdir);
    path[MAX_PATH - 1] = 0;

    f = fopen(path,"rb");
    if(f == NULL){
        WgxDbgPrint("Cannot open %s => the boot time defragmenter is not installed\n",path);
        return 0;
    }
    
    fclose(f);
    return 1;
}

/**
 * @brief Registers main window class.
 * @return Zero for success, negative value otherwise.
 */
static int RegisterMainWindowClass(void)
{
    WNDCLASSEX wc;
    
    /* make it unique to be able to center configuration dialog over */
    _snprintf(class_name,sizeof(class_name),"udefrag-gui-%u",(int)GetCurrentProcessId());
    class_name[sizeof(class_name) - 1] = 0;
    
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = MainWindowProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hIconSm       = NULL;
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_MENU + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = class_name;
    
    if(!RegisterClassEx(&wc)){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "RegisterMainWindowClass failed!");
        return (-1);
    }
    return 0;
}

/**
 * @brief Defines initial
 * coordinates of main window.
 */
static void InitMainWindowCoordinates(void)
{
    int center_on_the_screen = 0;
    int screen_width, screen_height;
    int width, height;
    HDC hDC;

    if(scale_by_dpi){
        hDC = GetDC(NULL);
        if(hDC){
            fScale = (double)GetDeviceCaps(hDC,LOGPIXELSX) / 96.0f;
            ReleaseDC(NULL,hDC);
        }
    }

    if(r_rc.left == UNDEFINED_COORD)
        center_on_the_screen = 1;
    else if(r_rc.top == UNDEFINED_COORD)
        center_on_the_screen = 1;
    else if(restore_default_window_size)
        center_on_the_screen = 1;
    
    if(center_on_the_screen){
        /* center default sized window on the screen */
        width = DPI(DEFAULT_WIDTH);
        height = DPI(DEFAULT_HEIGHT);
        screen_width = GetSystemMetrics(SM_CXSCREEN);
        screen_height = GetSystemMetrics(SM_CYSCREEN);
        if(screen_width < width || screen_height < height){
            r_rc.left = r_rc.top = 0;
        } else {
            r_rc.left = (screen_width - width) / 2;
            r_rc.top = (screen_height - height) / 2;
        }
        r_rc.right = r_rc.left + width;
        r_rc.bottom = r_rc.top + height;
        restore_default_window_size = 0; /* because already done */
    }

    WgxCheckWindowCoordinates(&r_rc,130,50);
    memcpy((void *)&win_rc,(void *)&r_rc,sizeof(RECT));
}

static RECT prev_rc = {0,0,0,0};

/**
 * @brief Adjust positions of controls
 * in accordance with main window dimensions.
 */
void ResizeMainWindow(int force)
{
    int vlist_height, sbar_height;
    int spacing;
    int toolbar_height;
    RECT rc;
    int w, h;
    int min_list_height;
    int max_list_height;
    int expand_list = 0;
    
    if(hStatus == NULL || hMainMenu == NULL || hToolbar == NULL)
        return; /* this happens on early stages of window initialization */
    
    if(!GetClientRect(hWindow,&rc)){
        WgxDbgPrint("GetClientRect failed in RepositionMainWindowControls()!\n");
        return;
    }
    
    if(list_height == 0)
        list_height = DPI(VLIST_HEIGHT);
    
    /* correct invalid list heights */
    min_list_height = GetMinVolListHeight();
    if(list_height < min_list_height)
        list_height = min_list_height;

    max_list_height = GetMaxVolListHeight();
    if(list_height > max_list_height)
        list_height = max_list_height;
    
    vlist_height = list_height;//DPI(VLIST_HEIGHT);
    spacing = DPI(SPACING);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;

    /* prevent resizing if the current size is equal to previous */
    if(!force && (prev_rc.right - prev_rc.left == w) && (prev_rc.bottom - prev_rc.top == h))
        return; /* this usually encounters when user minimizes window and then restores it */
    memcpy((void *)&prev_rc,(void *)&rc,sizeof(RECT));
    
    if(GetWindowRect(hToolbar,&rc))
        toolbar_height = rc.bottom - rc.top;
    else
        toolbar_height = 24 + 2 * GetSystemMetrics(SM_CYEDGE);
    
    if(vlist_height > previous_list_height){
        expand_list = 1;
        previous_list_height = vlist_height;
    }

    list_height = vlist_height = ResizeVolList(0,toolbar_height,w,vlist_height,expand_list);
    previous_list_height = vlist_height;
    /* do it twice for a proper list redraw when maximized window gets restored */
    list_height = vlist_height = ResizeVolList(0,toolbar_height,w,vlist_height,0);
    previous_list_height = vlist_height;
    sbar_height = ResizeStatusBar(h,w);
    ResizeMap(0,toolbar_height + vlist_height + 1,w,h - toolbar_height - vlist_height - sbar_height);
    
    /* redraw menu bar */
    DrawMenuBar(hWindow);
    
    /* redraw toolbar */
    SendMessage(hToolbar,TB_AUTOSIZE,0,0);
}

/**
 * @brief Creates main window.
 * @return Zero for success,
 * negative value otherwise.
 */
int CreateMainWindow(int nShowCmd)
{
    char *caption;
    HACCEL hAccelTable;
    MSG msg;
    udefrag_progress_info pi;
    
    /* register class */
    if(RegisterMainWindowClass() < 0)
        return (-1);

    TaskbarButtonCreatedMsg = RegisterWindowMessage("TaskbarButtonCreated");
    if(TaskbarButtonCreatedMsg == 0)
        WgxDbgPrintLastError("CreateMainWindow: cannot register TaskbarButtonCreated message");

    if(dry_run == 0){
        if(portable_mode) caption = VERSIONINTITLE_PORTABLE;
        else caption = VERSIONINTITLE;
    } else {
        if(portable_mode) caption = VERSIONINTITLE_PORTABLE " (dry run)";
        else caption = VERSIONINTITLE " (dry run)";
    }
    
    /* create main window */
    InitMainWindowCoordinates();
    hWindow = CreateWindowEx(
            WS_EX_APPWINDOW,
            class_name,caption,
            WS_OVERLAPPEDWINDOW,
            win_rc.left,win_rc.top,
            win_rc.right - win_rc.left,
            win_rc.bottom - win_rc.top,
            NULL,NULL,hInstance,NULL);
    if(hWindow == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot create main window!");
        return (-1);
    }
    
    /* create menu */
    if(CreateMainMenu() < 0)
        return (-1);
    
    /* create toolbar */
    if(CreateToolbar() < 0)
        return (-1);
    
    /* create controls */
    hList = CreateWindowEx(WS_EX_CLIENTEDGE,
            "SysListView32","",
            WS_CHILD | WS_VISIBLE \
            | LVS_REPORT | LVS_SHOWSELALWAYS \
            | LVS_NOSORTHEADER,
            0,0,100,100,
            hWindow,NULL,hInstance,NULL);
    if(hList == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot create disk list control!");
        return (-1);
    }

    hMap = CreateWindowEx(WS_EX_CLIENTEDGE,
            "Static","",
            WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
            0,100,100,100,
            hWindow,NULL,hInstance,NULL);
    if(hMap == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot create cluster map control!");
        return (-1);
    }
    
    CreateStatusBar();
    if(hStatus == NULL)
        return (-1);
    
    /* set font */
    InitFont();
    WgxSetFont(hList,&wgxFont);

    /* initialize controls */
    InitVolList();
    InitMap();

    /* maximize window if required */
    if(init_maximized_window)
        SendMessage(hWindow,WM_MAXIMIZE_MAIN_WINDOW,0,0);
    
    /* resize controls */
    ResizeMainWindow(1);
    
    /* fill list of volumes */
    UpdateVolList(); /* after a complete map initialization! */

    /* reset status bar */
    memset(&pi,0,sizeof(udefrag_progress_info));
    UpdateStatusBar(&pi);
    
    WgxSetIcon(hInstance,hWindow,IDI_APP);
    
    /* load i18n resources */
    ApplyLanguagePack();
    BuildLanguageMenu();
    
    /* show main window on the screen */
    ShowWindow(hWindow,init_maximized_window ? SW_MAXIMIZE : nShowCmd);
    UpdateWindow(hWindow);

    /* load accelerators */
    hAccelTable = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_MAIN_ACCELERATOR));
    if(hAccelTable == NULL){
        WgxDbgPrintLastError("CreateMainWindow: accelerators cannot be loaded");
    }
    
    SetFocus(hList);

    /* go to the message loop */
    while(GetMessage(&msg,NULL,0,0)){
        if(!TranslateAccelerator(hWindow,hAccelTable,&msg)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

/**
 * @brief Opens a page of the handbook, either
 * from local storage or from the network.
 */
void OpenWebPage(char *page, char *anchor)
{
    int i;
    wchar_t path[MAX_PATH];
    wchar_t exe[MAX_PATH] = {0};
    char cd[MAX_PATH];
    HINSTANCE hApp;

    (void)_snwprintf(path,MAX_PATH,L".\\handbook\\%hs",page);

    if (anchor != NULL){
        if(GetModuleFileName(NULL,cd,MAX_PATH)){
            cd[MAX_PATH-1] = 0;

            i = strlen(cd) - 1;
            while(i >= 0) {
                if(cd[i] == '\\'){
                    cd[i] = 0;
                    break;
                }
                i--;
            }

            (void)FindExecutableW(path,NULL,exe);

            (void)_snwprintf(path,MAX_PATH,L"file://%hs\\handbook\\%hs#%hs",cd,page,anchor);
        }
    }
    path[MAX_PATH - 1] = 0;

    if (anchor != NULL && exe[0] != 0)
        hApp = ShellExecuteW(hWindow,NULL,exe,path,NULL,SW_SHOW);
    else
        hApp = ShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_SHOW);
    if((int)(LONG_PTR)hApp <= 32){
        if (anchor != NULL)
            (void)_snwprintf(path,MAX_PATH,L"http://ultradefrag.sourceforge.net/handbook/%hs#%hs",page,anchor);
        else
            (void)_snwprintf(path,MAX_PATH,L"http://ultradefrag.sourceforge.net/handbook/%hs",page);
        path[MAX_PATH - 1] = 0;
        (void)WgxShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_SHOW);
    }
}

/**
 * @brief Opens a page of the translation wiki.
 * @param[in] islang 1 indicates a language page, 0 not
 */
void OpenTranslationWebPage(wchar_t *page, int islang)
{
    wchar_t path[MAX_PATH] = {0};

    if(islang == 0 )
        (void)_snwprintf(path,MAX_PATH,L"http://ultradefrag.wikispaces.com/%ls",page);
    else
        (void)_snwprintf(path,MAX_PATH,L"http://ultradefrag.wikispaces.com/%ls.lng",page);
    path[MAX_PATH - 1] = 0;
    (void)WgxShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_SHOW);
}

/**
 * @brief Opens the log file.
 */
void OpenLog(void)
{
    /* getenv() may give wrong results as stated in MSDN */
    if(!GetEnvironmentVariableW(L"UD_LOG_FILE_PATH",env_buffer2,MAX_ENV_VARIABLE_LENGTH + 1)){
        WgxDbgPrintLastError("OpenLog: cannot query UD_LOG_FILE_PATH environment variable");
        MessageBox(hWindow,"The log_file_path option is not set.","Cannot open log file!",MB_OK | MB_ICONHAND);
    } else {
        udefrag_flush_dbg_log();
        (void)WgxShellExecuteW(hWindow,L"open",env_buffer2,NULL,NULL,SW_SHOW);
    }
}

/**
 * @internal
 * @brief Updates the global win_rc structure.
 * @return Zero for success, negative value otherwise.
 */
static int UpdateMainWindowCoordinates(void)
{
    RECT rc;

    if(GetWindowRect(hWindow,&rc)){
        if((HIWORD(rc.bottom)) != 0xffff){
            memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
            return 0;
        }
    }
    return (-1);
}

/**
 * @brief Defines whether the cursor 
 * is between list and map controls.
 * @return Nonzero value indicates that
 * the cursor is between the controls.
 */
static int IsCursorBetweenControls(void)
{
    POINT pt;
    RECT rc;
    
    /* get cursor's position */
    if(!GetCursorPos(&pt)){
        WgxDbgPrintLastError("IsCursorBetweenControls: cannot get cursor position");
        return 0;
    }
    
    /* convert screen coordinates to list view control coordinates */
    if(!MapWindowPoints(NULL,hList,&pt,1)){
        WgxDbgPrintLastError("IsCursorBetweenControls: MapWindowPoints failed");
        return 0;
    }
    
    /* get dimensions of the list view control */
    if(!GetWindowRect(hList,&rc)){
        WgxDbgPrintLastError("IsCursorBetweenControls: cannot get height of the list view control");
        return 0;
    }
    rc.bottom -= rc.top;
    rc.right -= rc.left;
    
    if(pt.x >= 0 && pt.x <= rc.right \
        && pt.y >= rc.bottom \
        && pt.y <= rc.bottom + 3) return 1;

    return 0;
}

/*
* The splitter control routines.
* Adopted from http://www.catch22.net/tuts/splitter -
* a public domain piece of code.
*/

/**
 * @brief The last position of the cursor.
 */
static int old_y = -4;

/**
 * @brief Drag mode indicator.
 */
static int drag_mode = 0;

/**
 * @brief Draws moving line of the splitter.
 * @param[in] y the y coordinate of the cursor
 * relative to the main window's client area.
 * @note Calling this routine twice restores background.
 */
static void DrawXorBar(int y)
{
    HDC hdc;
    HBITMAP hBitmap;
    HBRUSH  hBrush, hOldBrush;
    RECT rc, list_rc;
    POINT pt;
    int x, width, height;
    static WORD _dotPatternBmp[8] = { 
        0x00aa, 0x0055, 0x00aa, 0x0055, 
        0x00aa, 0x0055, 0x00aa, 0x0055
    };
    
    /* get dimensions of the main window */
    if(!GetWindowRect(hWindow,&rc)){
        WgxDbgPrintLastError("DrawXorBar: cannot get main window dimensions");
        return;
    }

    /* get dimensions of the list view control */
    if(!GetWindowRect(hList,&list_rc)){
        WgxDbgPrintLastError("DrawXorBar: cannot get list dimensions");
        return;
    }
    
    width = list_rc.right - list_rc.left - GetSystemMetrics(SM_CXEDGE) * 2;
    height = 4;

    pt.x = 0;
    pt.y = y - 2;
    if(!ClientToScreen(hWindow,&pt)){
        WgxDbgPrintLastError("DrawXorBar: ClientToScreen failed");
        return;
    }

    x = (rc.right - rc.left - width) / 2;
    y = pt.y - rc.top;

    /* create brush */
    hBitmap = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
    if(hBitmap == NULL){
        WgxDbgPrintLastError("DrawXorBar: cannot create bitmap");
        return;
    }
    hBrush = CreatePatternBrush(hBitmap);
    if(hBrush == NULL){
        WgxDbgPrintLastError("DrawXorBar: cannot create brush");
        DeleteObject(hBitmap);
        return;
    }
    
    hdc = GetWindowDC(hWindow);
    SetBrushOrgEx(hdc,x,y,0);
    hOldBrush = (HBRUSH)SelectObject(hdc,hBrush);

    /* draw line */
    PatBlt(hdc,x,y,width,height,PATINVERT);

    /* cleanup */
    SelectObject(hdc,hOldBrush);
    ReleaseDC(hWindow,hdc);
    DeleteObject(hBrush);
    DeleteObject(hBitmap);
}

/**
 * @brief Handles the beginning
 * of the list resize operation.
 */
static void ResizeListBegin(short y)
{
    if(IsCursorBetweenControls()){
        drag_mode = 1;
        SetCapture(hWindow);
        DrawXorBar(y);
        old_y = y;
    }
}

/**
 * @brief Handles the middle part
 * of the list resize operation.
 */
static void ResizeListMove(short y)
{
    POINT pt;
    
    if(drag_mode == 0 || y == old_y) return;
    
    /* calculate list height */
    pt.x = pt.y = 0;
    if(!MapWindowPoints(hList,hWindow,&pt,1)){
        WgxDbgPrintLastError("ResizeListMove: MapWindowPoints failed");
    } else {
        list_height = y - pt.y;
        if(list_height >= 0 && list_height <= GetMaxVolListHeight()){
            DrawXorBar(old_y);
            DrawXorBar(y);
            old_y = y;
        }
    }
}

/**
 * @brief Handles the end
 * of the list resize operation.
 */
static void ResizeListEnd(short y)
{
    POINT pt;
    
    if(drag_mode){
        DrawXorBar(old_y);
        old_y = y;
        drag_mode = 0;
        pt.x = pt.y = 0;
        if(!MapWindowPoints(hList,hWindow,&pt,1)){
            WgxDbgPrintLastError("ResizeListEnd: MapWindowPoints failed");
        } else {
            list_height = y - pt.y;
            ResizeMainWindow(1);
        }
        ReleaseCapture();
    }
}

/**
 * @brief Main window procedure.
 */
LRESULT CALLBACK MainWindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    MINMAXINFO *mmi;
    BOOL size_changed;
    RECT rc;
    wchar_t path[MAX_PATH];
    CHOOSEFONT cf;
    UINT i, id;
    MENUITEMINFOW mi;
    wchar_t lang_name[MAX_PATH];
    wchar_t *report_opts_path;
    FILE *f;
    int flag, disable_latest_version_check_old;
    
    /* handle shell restart */
    if(uMsg == TaskbarButtonCreatedMsg){
        /* set taskbar icon overlay */
        if(show_taskbar_icon_overlay){
            if(WaitForSingleObject(hTaskbarIconEvent,INFINITE) != WAIT_OBJECT_0){
                WgxDbgPrintLastError("MainWindowProc: wait on hTaskbarIconEvent failed");
            } else {
                if(job_is_running)
                    SetTaskbarIconOverlay(IDI_BUSY,"JOB_IS_RUNNING");
                SetEvent(hTaskbarIconEvent);
            }
        }
        return 0;
    }

    switch(uMsg){
    case WM_CREATE:
        /* initialize main window */
        return 0;
    case WM_NOTIFY:
        VolListNotifyHandler(lParam);
        return 0;
    case WM_SETCURSOR:
        /* show NS arrows between list and map controls */
        if(IsCursorBetweenControls()){
            SetCursor(LoadCursor(NULL,IDC_SIZENS));
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        ResizeListBegin((short)HIWORD(lParam));
        return 0;
    case WM_LBUTTONUP:
        ResizeListEnd((short)HIWORD(lParam));
        return 0;
    case WM_MOUSEMOVE:
        if(wParam & MK_LBUTTON)
            ResizeListMove((short)HIWORD(lParam));
        return 0;
    case WM_COMMAND:
        switch(LOWORD(wParam)){
        /* Action menu handlers */
        case IDM_ANALYZE:
            start_selected_jobs(ANALYSIS_JOB);
            return 0;
        case IDM_DEFRAG:
            start_selected_jobs(DEFRAGMENTATION_JOB);
            return 0;
        case IDM_QUICK_OPTIMIZE:
            start_selected_jobs(QUICK_OPTIMIZATION_JOB);
            return 0;
        case IDM_FULL_OPTIMIZE:
            start_selected_jobs(FULL_OPTIMIZATION_JOB);
            return 0;
        case IDM_OPTIMIZE_MFT:
            start_selected_jobs(MFT_OPTIMIZATION_JOB);
            return 0;
        case IDM_STOP:
            stop_all_jobs();
            return 0;
        case IDM_REPEAT_ACTION:
            if(repeat_action){
                repeat_action = 0;
                CheckMenuItem(hMainMenu,
                    IDM_REPEAT_ACTION,
                    MF_BYCOMMAND | MF_UNCHECKED);
                SendMessage(hToolbar,TB_CHECKBUTTON,IDM_REPEAT_ACTION,MAKELONG(FALSE,0));
            } else {
                repeat_action = 1;
                CheckMenuItem(hMainMenu,
                    IDM_REPEAT_ACTION,
                    MF_BYCOMMAND | MF_CHECKED);
                SendMessage(hToolbar,TB_CHECKBUTTON,IDM_REPEAT_ACTION,MAKELONG(TRUE,0));
            }
            return 0;
        case IDM_IGNORE_REMOVABLE_MEDIA:
            if(skip_removable){
                skip_removable = 0;
                CheckMenuItem(hMainMenu,
                    IDM_IGNORE_REMOVABLE_MEDIA,
                    MF_BYCOMMAND | MF_UNCHECKED);
            } else {
                skip_removable = 1;
                CheckMenuItem(hMainMenu,
                    IDM_IGNORE_REMOVABLE_MEDIA,
                    MF_BYCOMMAND | MF_CHECKED);
            }
        case IDM_RESCAN:
            if(!busy_flag)
                UpdateVolList();
            return 0;
        case IDM_REPAIR:
            if(!busy_flag)
                RepairSelectedVolumes();
            return 0;
        case IDM_OPEN_LOG:
            OpenLog();
            return 0;
        case IDM_REPORT_BUG:
            (void)WgxShellExecuteW(hWindow,L"open",
                L"http://sourceforge.net/p/ultradefrag/bugs/",
                NULL,NULL,SW_SHOW);
            return 0;
        case IDM_EXIT:
            goto done;
        /* Reports menu handler */
        case IDM_SHOW_REPORT:
            ShowReports();
            return 0;
        /* Settings menu handlers */
        case IDM_TRANSLATIONS_CHANGE_LOG:
            OpenTranslationWebPage(L"Change+Log", 0);
            return 0;
        case IDM_TRANSLATIONS_REPORT:
            OpenTranslationWebPage(L"Translation+Report", 0);
            return 0;
        case IDM_TRANSLATIONS_FOLDER:
            (void)WgxShellExecuteW(hWindow,L"open",L"explorer.exe",
                L"/select, \".\\i18n\\translation.template\"",NULL,SW_SHOW);
            return 0;
        case IDM_TRANSLATIONS_SUBMIT:
            if(GetPrivateProfileStringW(L"Language",L"Selected",NULL,lang_name,MAX_PATH,L".\\lang.ini")>0)
                OpenTranslationWebPage(lang_name, 1);
            return 0;
        case IDM_CFG_GUI_FONT:
            memset(&cf,0,sizeof(cf));
            cf.lStructSize = sizeof(CHOOSEFONT);
            cf.lpLogFont = &wgxFont.lf;
            cf.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
            cf.hwndOwner = hWindow;
            if(ChooseFont(&cf)){
                WgxDestroyFont(&wgxFont);
                if(WgxCreateFont("",&wgxFont)){
                    WgxSetFont(hList,&wgxFont);
                    ResizeMainWindow(1);
                    WgxSaveFont(".\\options\\font.lua",&wgxFont);
                }
            }
            return 0;
        case IDM_CFG_GUI_SETTINGS:
            if(portable_mode)
                WgxShellExecuteW(hWindow,L"open",L"notepad.exe",L".\\options\\guiopts.lua",NULL,SW_SHOW);
            else
                WgxShellExecuteW(hWindow,L"Edit",L".\\options\\guiopts.lua",NULL,NULL,SW_SHOW);
            return 0;
        case IDM_CFG_BOOT_ENABLE:
            if(!GetWindowsDirectoryW(path,MAX_PATH)){
                WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot retrieve the Windows directory path!");
            } else {
                if(boot_time_defrag_enabled){
                    boot_time_defrag_enabled = 0;
                    CheckMenuItem(hMainMenu,
                        IDM_CFG_BOOT_ENABLE,
                        MF_BYCOMMAND | MF_UNCHECKED);
                    SendMessage(hToolbar,TB_CHECKBUTTON,IDM_CFG_BOOT_ENABLE,MAKELONG(FALSE,0));
                    (void)wcscat(path,L"\\System32\\boot-off.cmd");
                } else {
                    boot_time_defrag_enabled = 1;
                    CheckMenuItem(hMainMenu,
                        IDM_CFG_BOOT_ENABLE,
                        MF_BYCOMMAND | MF_CHECKED);
                    SendMessage(hToolbar,TB_CHECKBUTTON,IDM_CFG_BOOT_ENABLE,MAKELONG(TRUE,0));
                    (void)wcscat(path,L"\\System32\\boot-on.cmd");
                }
                (void)WgxShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_HIDE);
            }
            return 0;
        case IDM_CFG_BOOT_SCRIPT:
            if(!GetWindowsDirectoryW(path,MAX_PATH)){
                WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot retrieve the Windows directory path");
            } else {
                (void)wcscat(path,L"\\System32\\ud-boot-time.cmd");
                (void)WgxShellExecuteW(hWindow,L"edit",path,NULL,NULL,SW_SHOW);
            }
            return 0;
        case IDM_CFG_REPORTS:
            /* open custom options or default if they aren't exist */
            report_opts_path = L".\\options\\udreportopts.lua";
            f = fopen(".\\options\\udreportopts-custom.lua","r");
            if(f != NULL){
                fclose(f);
                report_opts_path = L".\\options\\udreportopts-custom.lua";
            }
            if(portable_mode)
                WgxShellExecuteW(hWindow,L"open",L"notepad.exe",report_opts_path,NULL,SW_SHOW);
            else
                WgxShellExecuteW(hWindow,L"Edit",report_opts_path,NULL,NULL,SW_SHOW);
            return 0;
        /* Help menu handlers */
        case IDM_CONTENTS:
            OpenWebPage("index.html", NULL);
            return 0;
        case IDM_BEST_PRACTICE:
            OpenWebPage("Tips.html", NULL);
            return 0;
        case IDM_FAQ:
            OpenWebPage("FAQ.html", NULL);
            return 0;
        case IDM_CM_LEGEND:
            OpenWebPage("GUI.html", "cluster_map_legend");
            return 0;
        case IDM_CHECK_UPDATE:
            disable_latest_version_check_old = disable_latest_version_check;
            disable_latest_version_check = 0;
            CheckForTheNewVersion();
            disable_latest_version_check = disable_latest_version_check_old;
            return 0;
        case IDM_ABOUT:
            AboutBox();
            return 0;
        case IDM_SELECT_ALL:
            if(!busy_flag)
                SelectAllDrives();
            return 0;
        default:
            id = LOWORD(wParam);
            /* handle language menu */
            if(id > IDM_LANGUAGE && id < IDM_CFG_GUI){
                /* get name of selected language */
                memset(&mi,0,sizeof(MENUITEMINFOW));
                mi.cbSize = sizeof(MENUITEMINFOW);
                mi.fMask = MIIM_TYPE;
                mi.fType = MFT_STRING;
                mi.dwTypeData = lang_name;
                mi.cch = MAX_PATH;
                if(!GetMenuItemInfoW(hMainMenu,id,FALSE,&mi)){
                    WgxDbgPrintLastError("MainWindowProc: cannot get selected language");
                    return 0;
                }
                
                /* move check mark */
                for(i = IDM_LANGUAGE + 1; i < IDM_CFG_GUI; i++){
                    if(CheckMenuItem(hMainMenu,i,MF_BYCOMMAND | MF_UNCHECKED) == -1)
                        break;
                }
                CheckMenuItem(hMainMenu,id,MF_BYCOMMAND | MF_CHECKED);

                /* save selection to lang.ini file */
                WritePrivateProfileStringW(L"Language",L"Selected",lang_name,L".\\lang.ini");
                
                /* update all gui controls */
                if(lang_ini_tracking_stopped){
                    /* this hangs app if lang.ini change tracking runs in parallel thread */
                    ApplyLanguagePack();
                }
                return 0;
            }
            /* handle when done submenu */
            if(id > IDM_WHEN_DONE && id < IDM_EXIT){
                /* move check mark */
                for(i = IDM_WHEN_DONE + 1; i < IDM_EXIT; i++){
                    if(CheckMenuItem(hMainMenu,i,MF_BYCOMMAND | MF_UNCHECKED) == -1)
                        break;
                }
                CheckMenuItem(hMainMenu,id,MF_BYCOMMAND | MF_CHECKED);
                
                /* save choice */
                when_done_action = id;
            }
            /* handle preview submenu */
            if(id > IDM_PREVIEW && id < IDM_PREVIEW_LAST_ITEM){
                switch(id){
                /* case IDM_PREVIEW_MOVE_FRONT:
                    flag = UD_PREVIEW_MOVE_FRONT;
                    break; */
                case IDM_PREVIEW_LARGEST:
                    /* "find largest" menu item affects "find matching" item too */
                    id = IDM_PREVIEW_MATCHING;
                case IDM_PREVIEW_MATCHING:
                    flag = UD_PREVIEW_MATCHING;
                    break;
                /* case IDM_PREVIEW_SKIP_PARTIAL:
                    flag = UD_PREVIEW_SKIP_PARTIAL;
                    break; */
                default:
                    flag = 0;
                    break;
                }
                if(job_flags & flag){
                    CheckMenuItem(hMainMenu,id,MF_BYCOMMAND | MF_UNCHECKED);
                    job_flags ^= flag;
                } else {
                    CheckMenuItem(hMainMenu,id,MF_BYCOMMAND | MF_CHECKED);
                    job_flags |= flag;
                }
                
                /* set "find largest" menu state opposed to "find matching" state */
                if(job_flags & UD_PREVIEW_MATCHING)
                    CheckMenuItem(hMainMenu,IDM_PREVIEW_LARGEST,MF_BYCOMMAND | MF_UNCHECKED);
                else
                    CheckMenuItem(hMainMenu,IDM_PREVIEW_LARGEST,MF_BYCOMMAND | MF_CHECKED);
            }
            break;
        }
        break;
    case WM_SIZE:
        /* resize main window and its controls */
        if(wParam == SIZE_MAXIMIZED)
            maximized_window = 1;
        else if(wParam != SIZE_MINIMIZED)
            maximized_window = 0;
        if(UpdateMainWindowCoordinates() >= 0){
            if(!maximized_window)
                memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
            ResizeMainWindow(0);
        } else {
            WgxDbgPrint("Wrong window dimensions on WM_SIZE message!\n");
        }
        return 0;
    case WM_GETMINMAXINFO:
        /* set min size to avoid overlaying controls */
        mmi = (MINMAXINFO *)lParam;
        mmi->ptMinTrackSize.x = DPI(MIN_WIDTH);
        mmi->ptMinTrackSize.y = DPI(MIN_HEIGHT);
        return 0;
    case WM_MOVE:
        /* update coordinates of the main window */
        size_changed = TRUE;
        if(GetWindowRect(hWindow,&rc)){
            if((HIWORD(rc.bottom)) != 0xffff){
                if((rc.right - rc.left == win_rc.right - win_rc.left) &&
                    (rc.bottom - rc.top == win_rc.bottom - win_rc.top))
                        size_changed = FALSE;
            }
        }
        UpdateMainWindowCoordinates();
        if(!maximized_window && !size_changed)
            memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
        return 0;
    case WM_MAXIMIZE_MAIN_WINDOW:
        /* maximize window */
        ShowWindow(hWnd,SW_MAXIMIZE);
        return 0;
    case WM_DESTROY:
        goto done;
    }
    return DefWindowProc(hWnd,uMsg,wParam,lParam);

done:
    UpdateMainWindowCoordinates();
    if(!maximized_window)
        memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
    VolListGetColumnWidths();
    exit_pressed = 1;
    stop_all_jobs();
    PostQuitMessage(0);
    return 0;
}

/**
 * @internal
 * @brief Updates web statistics of the program use.
 */
DWORD WINAPI UpdateWebStatisticsThreadProc(LPVOID lpParameter)
{
    int tracking_enabled = 1;
    
    /* getenv() may give wrong results as stated in MSDN */
    if(!GetEnvironmentVariableW(L"UD_DISABLE_USAGE_TRACKING",env_buffer,MAX_ENV_VARIABLE_LENGTH + 1)){
        if(GetLastError() != ERROR_ENVVAR_NOT_FOUND)
            WgxDbgPrintLastError("UpdateWebStatisticsThreadProc: cannot get %%UD_DISABLE_USAGE_TRACKING%%!");
    } else {
        if(wcscmp(env_buffer,L"1") == 0)
            tracking_enabled = 0;
    }
    
    if(tracking_enabled){
#ifndef _WIN64
        IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/gui-x86.html","UA-15890458-1");
#else
    #if defined(_IA64_)
        IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/gui-ia64.html","UA-15890458-1");
    #else
        IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/gui-x64.html","UA-15890458-1");
    #endif
#endif
    }

    web_statistics_completed = 1;
    return 0;
}

/**
 * @brief Starts web statistics request delivering.
 */
void start_web_statistics(void)
{
    if(!WgxCreateThread(UpdateWebStatisticsThreadProc,NULL)){
        WgxDbgPrintLastError("Cannot run UpdateWebStatisticsThreadProc");
        web_statistics_completed = 1;
    }
}

/**
 * @brief Waits for web statistics request completion.
 */
void stop_web_statistics()
{
    while(!web_statistics_completed) Sleep(100);
}

/**
 * @brief Entry point.
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    HMODULE hComCtlDll;
    typedef BOOL (WINAPI *ICCE_PROC)(LPINITCOMMONCONTROLSEX lpInitCtrls);
    ICCE_PROC pInitCommonControlsEx = NULL;
    INITCOMMONCONTROLSEX icce;
    int result;
    
    WgxSetDbgPrintHandler(udefrag_dbg_print);
    hInstance = GetModuleHandle(NULL);
    
    /* check for admin rights - they're strongly required */
    if(!WgxCheckAdminRights()){
        MessageBox(NULL,"Administrative rights are needed "
          "to run the program!","UltraDefrag",
          MB_OK | MB_ICONHAND);
        return 1;
    }

    /* show crash info when the program crashed last time */
    StartCrashInfoCheck();
    
    /* handle initialization failure */
    if(udefrag_init_failed()){
        MessageBoxA(NULL,"Send bug report to the authors please.",
            "UltraDefrag initialization failed!",MB_OK | MB_ICONHAND);
        StopCrashInfoCheck();
        return 1;
    }
    
    /* define whether we are in portable mode or not */
    portable_mode = IsPortable();
    btd_installed = IsBtdInstalled();

    /* get preferences */
    GetPrefs();

    /* save preferences to update the config file to the recent format */
    SavePrefs();
    
    if(InitSynchObjects() < 0){
        DeleteEnvironmentVariables();
        StopCrashInfoCheck();
        return 1;
    }

    start_web_statistics();
    CheckForTheNewVersion();

    /* InitCommonControlsEx may be not available on NT 4 */
    hComCtlDll = LoadLibrary("comctl32.dll");
    if(hComCtlDll){
        pInitCommonControlsEx = (ICCE_PROC)GetProcAddress(hComCtlDll,"InitCommonControlsEx");
    }
    if(pInitCommonControlsEx){
        icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icce.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
        pInitCommonControlsEx(&icce);
    } else {
        InitCommonControls();
    }
    
    init_jobs();
    
    /* track changes in guiopts.lua file; synchronized with map redraw */
    StartPrefsChangesTracking();
    StartBootExecChangesTracking();
    StartLangIniChangesTracking();
    StartI18nFolderChangesTracking();
    
    if(CreateMainWindow(nShowCmd) < 0){
        StopPrefsChangesTracking();
        StopBootExecChangesTracking();
        StopLangIniChangesTracking();
        StopI18nFolderChangesTracking();
        release_jobs();
        WgxDestroyResourceTable(i18n_table);
        stop_web_statistics();
        DeleteEnvironmentVariables();
        DestroySynchObjects();
        StopCrashInfoCheck();
        return 3;
    }

    /* release all resources */
    StopPrefsChangesTracking();
    StopBootExecChangesTracking();
    StopLangIniChangesTracking();
    StopI18nFolderChangesTracking();
    release_jobs();
    ReleaseVolList();
    ReleaseMap();

    /* save settings */
    SavePrefs();
    DeleteEnvironmentVariables();
    stop_web_statistics();
    StopCrashInfoCheck();
    
    if(shutdown_requested){
        result = ShutdownOrHibernate();
        WgxDestroyFont(&wgxFont);
        WgxDestroyResourceTable(i18n_table);
        DestroySynchObjects();
        return result;
    }
    
    WgxDestroyFont(&wgxFont);
    WgxDestroyResourceTable(i18n_table);
    DestroySynchObjects();
    return 0;
}

/** @} */
