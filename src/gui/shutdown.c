/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2015 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file shutdown.c
 * @brief Shutdown.
 * @addtogroup Shutdown
 * @{
 */

/*
* Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
* and Dmitri Arkhangelski <dmitriar@gmail.com>.
*/

#include "main.h"

typedef BOOLEAN (WINAPI *SET_SUSPEND_STATE_PROC)(BOOLEAN Hibernate,BOOLEAN ForceCritical,BOOLEAN DisableWakeEvent);

/**
 * @brief Adjusts position of controls inside
 * the shutdown confirmation dialog to make it
 * nice looking independent from text lengths
 * and font size.
 */
static void ResizeShutdownConfirmDialog(HWND hwnd,wchar_t *counter_msg)
{
    int text1_width, text1_height;
    int text2_width, text2_height;
    int btn1_width, btn1_height;
    int btn2_width, btn2_height;
    int buttons_spacing = SMALL_SPACING;
    int width, height, delta;
    int client_width, client_height;
    int icon_y, block_y;
    int btns_x, btns_y;
    wchar_t *text;
    HFONT hFont;
    RECT rc;

    /* set text labels */
    switch(when_done_action){
    case IDM_WHEN_DONE_HIBERNATE:
        WgxSetText(GetDlgItem(hwnd,IDC_MESSAGE),
            i18n_table,"REALLY_HIBERNATE_WHEN_DONE");
        break;
    case IDM_WHEN_DONE_LOGOFF:
        WgxSetText(GetDlgItem(hwnd,IDC_MESSAGE),
            i18n_table,"REALLY_LOGOFF_WHEN_DONE");
        break;
    case IDM_WHEN_DONE_REBOOT:
        WgxSetText(GetDlgItem(hwnd,IDC_MESSAGE),
            i18n_table,"REALLY_REBOOT_WHEN_DONE");
        break;
    case IDM_WHEN_DONE_SHUTDOWN:
        WgxSetText(GetDlgItem(hwnd,IDC_MESSAGE),
            i18n_table,"REALLY_SHUTDOWN_WHEN_DONE");
        break;
    default:
        SetWindowTextW(GetDlgItem(hwnd,IDC_MESSAGE),L"");
        break;
    }
    text = wgx_swprintf(L"%u %ws",seconds_for_shutdown_rejection,counter_msg);
    if(text == NULL){
        mtrace();
        return;
    }
    SetWindowTextW(GetDlgItem(hwnd,IDC_DELAY_MSG),text);
    free(text);
    
    /* get dimensions of controls */
    hFont = use_custom_font_in_dialogs ? wgxFont.hFont : NULL;
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_MESSAGE),
        hFont,&text1_width, &text1_height)) return;
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_DELAY_MSG),
        hFont,&text2_width, &text2_height)) return;
    /* TODO: do it more accurately, without the need of extra spacing */
    text2_width += DPI(20);
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_YES_BUTTON),
        hFont,&btn1_width, &btn1_height)) return;
    if(!WgxGetControlDimensions(GetDlgItem(hwnd,IDC_NO_BUTTON),
        hFont,&btn2_width, &btn2_height)) return;
    
    /* adjust dimensions of buttons */
    width = max(btn1_width,btn2_width);
    btn1_width = btn2_width = max(width,MIN_BTN_WIDTH);
    height = max(btn1_height,btn2_height);
    btn1_height = btn2_height = max(height,MIN_BTN_HEIGHT);

    /* adjust controls */
    width = max(text1_width,text2_width);
    text1_width = text2_width = width;
    delta = btn1_width * 2 + buttons_spacing;
    delta -= ICON_SIZE + LARGE_SPACING + width;
    if(delta > 0){
        text1_width += delta;
        text2_width += delta;
    } else {
        if(delta % 2)
            buttons_spacing ++;
    }
    
    /* calculate dimensions of the entire client area */
    client_width = ICON_SIZE + LARGE_SPACING + text1_width + MARGIN * 2;
    client_height = max(ICON_SIZE,text1_height + SMALL_SPACING + text2_height);
    client_height += LARGE_SPACING + btn1_height + MARGIN * 2;

    /* resize main window */
    rc.left = 0, rc.right = client_width;
    rc.top = 0, rc.bottom = client_height;
    if(!AdjustWindowRect(&rc,WS_CAPTION | WS_DLGFRAME,FALSE)){
        letrace("cannot calculate window dimensions");
        return;
    }
    MoveWindow(hwnd,0,0,rc.right - rc.left,rc.bottom - rc.top,FALSE);

    /* reposition controls */
    delta = text1_height + SMALL_SPACING + text2_height - ICON_SIZE;
    if(delta > 0){
        icon_y = MARGIN + delta / 2;
        block_y = MARGIN;
    } else {
        icon_y = MARGIN;
        block_y = MARGIN - delta / 2;
    }
    btns_x = client_width - MARGIN * 2 - btn1_width * 2 - buttons_spacing;
    btns_x = MARGIN + btns_x / 2;
    btns_y = MARGIN + max(ICON_SIZE,ICON_SIZE + delta) + LARGE_SPACING;
    SetWindowPos(GetDlgItem(hwnd,IDC_SHUTDOWN_ICON),NULL,
        MARGIN,icon_y,ICON_SIZE,ICON_SIZE,SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_MESSAGE),NULL,
        MARGIN + ICON_SIZE + LARGE_SPACING,
        block_y,text1_width,text1_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_DELAY_MSG),NULL,
        MARGIN + ICON_SIZE + LARGE_SPACING,
        block_y + text1_height + SMALL_SPACING,
        text2_width,text2_height,SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_YES_BUTTON),NULL,
        btns_x,btns_y,btn1_width,btn1_height,SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_NO_BUTTON),NULL,
        btns_x + btn1_width + buttons_spacing,
        btns_y,btn2_width,btn2_height,SWP_NOZORDER);

    /* center window on the screen */
    WgxCenterWindow(hwnd);
}

/**
 * @brief Asks user whether he really wants
 * to shutdown/hibernate the computer.
 */
BOOL CALLBACK ShutdownConfirmDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    static UINT_PTR timer;
    static UINT counter;
    #define TIMER_ID 0x16748382
    #define MAX_TEXT_LENGTH 127
    static wchar_t buffer[MAX_TEXT_LENGTH + 1];
    static wchar_t counter_msg[MAX_TEXT_LENGTH + 1];
    wchar_t *text = NULL;

    switch(msg){
    case WM_INITDIALOG:
        if(use_custom_font_in_dialogs)
            WgxSetFont(hWnd,&wgxFont);
        WgxSetText(hWnd,i18n_table,"PLEASE_CONFIRM");
        WgxSetText(GetDlgItem(hWnd,IDC_YES_BUTTON),i18n_table,"YES");
        WgxSetText(GetDlgItem(hWnd,IDC_NO_BUTTON),i18n_table,"NO");
        switch(when_done_action){
        case IDM_WHEN_DONE_HIBERNATE:
            text = WgxGetResourceString(i18n_table,"SECONDS_TILL_HIBERNATION");
            break;
        case IDM_WHEN_DONE_LOGOFF:
            text = WgxGetResourceString(i18n_table,"SECONDS_TILL_LOGOFF");
            break;
        case IDM_WHEN_DONE_REBOOT:
            text = WgxGetResourceString(i18n_table,"SECONDS_TILL_REBOOT");
            break;
        case IDM_WHEN_DONE_SHUTDOWN:
            text = WgxGetResourceString(i18n_table,"SECONDS_TILL_SHUTDOWN");
            break;
        }
        if(text){
            wcsncpy(counter_msg,text,MAX_TEXT_LENGTH);
            counter_msg[MAX_TEXT_LENGTH] = 0;
            free(text);
        } else {
            counter_msg[0] = 0;
        }

        /* set timer */
        counter = seconds_for_shutdown_rejection;
        if(counter == 0)
            (void)EndDialog(hWnd,1);
        timer = (UINT_PTR)SetTimer(hWnd,TIMER_ID,1000,NULL);
        if(timer == 0){
            /* message box will prevent shutdown which is dangerous */
            letrace("SetTimer failed");
            /* force shutdown to avoid situation when pc works a long time without any control */
            (void)EndDialog(hWnd,1);
        }
        
        /* adjust positions of controls to make the window nice looking */
        ResizeShutdownConfirmDialog(hWnd,counter_msg);

        /* shutdown will be rejected by pressing the space key */
        (void)SetFocus(GetDlgItem(hWnd,IDC_NO_BUTTON));
        
        /* bring dialog to foreground */
        SetForegroundWindow(hWnd);
        return FALSE;
    case WM_TIMER:
        counter --;
        _snwprintf(buffer,MAX_TEXT_LENGTH + 1,L"%u %ls",counter,counter_msg);
        buffer[MAX_TEXT_LENGTH] = 0;
        SetWindowTextW(GetDlgItem(hWnd,IDC_DELAY_MSG),buffer);
        if(counter == 0){
            (void)KillTimer(hWnd,TIMER_ID);
            (void)EndDialog(hWnd,1);
        }
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)){
        case IDC_YES_BUTTON:
            (void)KillTimer(hWnd,TIMER_ID);
            (void)EndDialog(hWnd,1);
            break;
        case IDC_NO_BUTTON:
            (void)KillTimer(hWnd,TIMER_ID);
            (void)EndDialog(hWnd,0);
            break;
        }
        return TRUE;
    case WM_CLOSE:
        (void)KillTimer(hWnd,TIMER_ID);
        (void)EndDialog(hWnd,0);
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Shuts computer down or hibernates
 * them respect to user preferences.
 * @return Zero for success, nonzero value otherwise.
 */
int ShutdownOrHibernate(void)
{
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp;
    HMODULE hPowrProfDll;
    SET_SUSPEND_STATE_PROC pSetSuspendState = NULL;
    BOOL shutdown_cmd_present;
    BOOL result;

    switch(when_done_action){
    case IDM_WHEN_DONE_HIBERNATE:
    case IDM_WHEN_DONE_LOGOFF:
    case IDM_WHEN_DONE_REBOOT:
    case IDM_WHEN_DONE_SHUTDOWN:
        if(seconds_for_shutdown_rejection){
            if(DialogBoxW(hInstance,MAKEINTRESOURCEW(IDD_SHUTDOWN),NULL,(DLGPROC)ShutdownConfirmDlgProc) == 0)
                return EXIT_SUCCESS;
            /* in case of errors we'll shutdown anyway */
            /* to avoid situation when pc works a long time without any control */
        }
        break;
    case IDM_WHEN_DONE_EXIT:
        /* nothing to do */
        return EXIT_SUCCESS;
    }
    
    /* set SE_SHUTDOWN privilege */
    if(!OpenProcessToken(GetCurrentProcess(), 
    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,L"ShutdownOrHibernate: cannot open process token!");
        return EXIT_FAILURE;
    }
    
    LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
    AdjustTokenPrivileges(hToken,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);         
    if(GetLastError() != ERROR_SUCCESS){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,L"ShutdownOrHibernate: cannot set shutdown privilege!");
        return EXIT_FAILURE;
    }
    
    /*
    * Save log file before any action.
    */
    udefrag_flush_dbg_log(0);
    
    
    /*
    * Shutdown command works better on remote
    * computers since it shows no confirmation.
    */
    shutdown_cmd_present = WgxCreateProcess("%windir%\\system32\\shutdown.exe","/?");
    
    /*
    * There is an opinion that SetSuspendState call
    * is more reliable than SetSystemPowerState:
    * http://msdn.microsoft.com/en-us/library/aa373206%28VS.85%29.aspx
    * On the other hand, it is missing on NT4.
    */
    if(when_done_action == IDM_WHEN_DONE_STANDBY || when_done_action == IDM_WHEN_DONE_HIBERNATE){
        hPowrProfDll = LoadLibrary("powrprof.dll");
        if(hPowrProfDll == NULL){
            letrace("cannot load powrprof.dll library");
        } else {
            pSetSuspendState = (SET_SUSPEND_STATE_PROC)GetProcAddress(hPowrProfDll,"SetSuspendState");
            if(pSetSuspendState == NULL)
                letrace("cannot get SetSuspendState address inside powrprof.dll");
        }
        if(pSetSuspendState == NULL)
            itrace("therefore SetSystemPowerState API will be used instead of SetSuspendState");
    }

    switch(when_done_action){
    case IDM_WHEN_DONE_STANDBY:
        /* suspend, request permission from apps and drivers */
        if(pSetSuspendState)
            result = pSetSuspendState(FALSE,FALSE,FALSE);
        else
            result = SetSystemPowerState(TRUE,FALSE);
        if(!result){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,L"UltraDefrag: cannot suspend the system!");
            return EXIT_FAILURE;
        }
        break;
    case IDM_WHEN_DONE_HIBERNATE:
        /* hibernate, request permission from apps and drivers */
        if(pSetSuspendState){
            result = pSetSuspendState(TRUE,FALSE,FALSE);
        } else {
            /* the second parameter must be FALSE, dmitriar's windows xp hangs otherwise */
            result = SetSystemPowerState(FALSE,FALSE);
        }
        if(!result){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,L"UltraDefrag: cannot hibernate the computer!");
            return EXIT_FAILURE;
        }
        break;
    case IDM_WHEN_DONE_LOGOFF:
        if(shutdown_cmd_present){
            result = WgxCreateProcess("%windir%\\system32\\cmd.exe","/K shutdown -l");
        } else {
            result = ExitWindowsEx(EWX_LOGOFF | EWX_FORCEIFHUNG,
                SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | \
                SHTDN_REASON_FLAG_PLANNED);
        }
        if(!result){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,L"UltraDefrag: cannot log the user off!");
            return EXIT_FAILURE;
        }
        break;
    case IDM_WHEN_DONE_REBOOT:
        if(shutdown_cmd_present){
            result = WgxCreateProcess("%windir%\\system32\\cmd.exe","/K shutdown -r -t 0");
        } else {
            result = ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG,
                SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | \
                SHTDN_REASON_FLAG_PLANNED);
        }
        if(!result){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,L"UltraDefrag: cannot reboot the computer!");
            return EXIT_FAILURE;
        }
        break;
    case IDM_WHEN_DONE_SHUTDOWN:
        if(shutdown_cmd_present){
            result = WgxCreateProcess("%windir%\\system32\\cmd.exe","/K shutdown -s -t 0");
        } else {
            result = ExitWindowsEx(EWX_POWEROFF | EWX_FORCEIFHUNG,
                SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | \
                SHTDN_REASON_FLAG_PLANNED);
        }
        if(!result){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,L"UltraDefrag: cannot shut down the computer!");
            return EXIT_FAILURE;
        }
        break;
    }
    
    return EXIT_SUCCESS;
}

/** @} */
