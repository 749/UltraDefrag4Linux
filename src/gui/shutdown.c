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
    wchar_t *text1 = NULL, *text2 = NULL;
    wchar_t buffer[256];
    wchar_t text[256];
    int text1_width, text1_height;
    int text2_width, text2_height;
    int text_block_width;
    int text_block_height;
    int top_width, top_height;
    int bottom_width, bottom_height;
    int client_width, client_height;
    int icon_y, text1_y;
    int button1_x;
    int button_width = MIN_BTN_WIDTH;
    int button_height = MIN_BTN_HEIGHT;
    int buttons_spacing = SMALL_SPACING;
    int width, height;
    BOOL result;
    RECT rc;
    
    /* get dimensions of texts */
    switch(when_done_action){
    case IDM_WHEN_DONE_HIBERNATE:
        text1 = WgxGetResourceString(i18n_table,"REALLY_HIBERNATE_WHEN_DONE");
        break;
    case IDM_WHEN_DONE_LOGOFF:
        text1 = WgxGetResourceString(i18n_table,"REALLY_LOGOFF_WHEN_DONE");
        break;
    case IDM_WHEN_DONE_REBOOT:
        text1 = WgxGetResourceString(i18n_table,"REALLY_REBOOT_WHEN_DONE");
        break;
    case IDM_WHEN_DONE_SHUTDOWN:
        text1 = WgxGetResourceString(i18n_table,"REALLY_SHUTDOWN_WHEN_DONE");
        break;
    default:
        text1 = _wcsdup(L"");
        break;
    }
    if(text1 == NULL){
        WgxDbgPrint("ResizeShutdownConfirmDialog: cannot allocate memory for text1");
        return;
    }
    result = WgxGetTextDimensions(text1,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_MESSAGE), &text1_width, &text1_height);
    if(result == FALSE)
        goto done;

    _snwprintf(buffer,sizeof(buffer)/sizeof(wchar_t),L"%u %ws",seconds_for_shutdown_rejection,counter_msg);
    buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
    text2 = buffer;
    result = WgxGetTextDimensions(text2,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_DELAY_MSG), &text2_width, &text2_height);
    if(result == FALSE)
        goto done;
    /* TODO: do it more accurately, without the need of extra spacing */
    text2_width += DPI(20);
    
    /* calculate dimensions of buttons */
    if(!GetWindowTextW(GetDlgItem(hwnd,IDC_YES_BUTTON),text,sizeof(text)/sizeof(wchar_t))){
        WgxDbgPrint("ResizeShutdownConfirmDialog: cannot get Yes button text");
        goto done;
    }
    text[sizeof(text)/sizeof(wchar_t) - 1] = 0;
    result = WgxGetTextDimensions(text,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_YES_BUTTON), &width, &height);
    if(result == FALSE)
        goto done;
    if(width + 2 * BTN_H_SPACING > button_width)
        button_width = width + 2 * BTN_H_SPACING;
    if(height + 2 * BTN_V_SPACING > button_height)
        button_height = height + 2 * BTN_V_SPACING;

    if(!GetWindowTextW(GetDlgItem(hwnd,IDC_NO_BUTTON),text,sizeof(text)/sizeof(wchar_t))){
        WgxDbgPrint("ResizeShutdownConfirmDialog: cannot get No button text");
        goto done;
    }
    text[sizeof(text)/sizeof(wchar_t) - 1] = 0;
    result = WgxGetTextDimensions(text,
        use_custom_font_in_dialogs ? wgxFont.hFont : NULL,
        GetDlgItem(hwnd,IDC_NO_BUTTON), &width, &height);
    if(result == FALSE)
        goto done;
    if(width + 2 * BTN_H_SPACING > button_width)
        button_width = width + 2 * BTN_H_SPACING;
    if(height + 2 * BTN_V_SPACING > button_height)
        button_height = height + 2 * BTN_V_SPACING;

    /* calculate dimensions of the entire text block */
    text_block_width = max(text1_width,text2_width);
    text_block_height = text1_height + SMALL_SPACING + text2_height;
    
    top_width = ICON_SIZE + LARGE_SPACING + text_block_width;
    top_height = max(ICON_SIZE,text_block_height);
    bottom_width = button_width * 2 + SMALL_SPACING;
    bottom_height = button_height;
    if(top_width < bottom_width){
        text_block_width += bottom_width - top_width;
        top_width = bottom_width;
    }
    
    /* calculate dimensions of the entire client area */
    client_width = top_width + MARGIN * 2;
    client_height = top_height + LARGE_SPACING + bottom_height + MARGIN * 2;
    
    rc.left = 0;
    rc.right = client_width;
    rc.top = 0;
    rc.bottom = client_height;
    if(!AdjustWindowRect(&rc,WS_CAPTION | WS_DLGFRAME,FALSE)){
        WgxDbgPrintLastError("ResizeShutdownConfirmDialog: cannot calculate window dimensions");
        goto done;
    }
            
    /* resize main window */
    MoveWindow(hwnd,0,0,rc.right - rc.left,rc.bottom - rc.top,FALSE);
            
    /* reposition controls */
    if(ICON_SIZE <= text_block_height){
        icon_y = (text_block_height - ICON_SIZE) / 2 + MARGIN;
        text1_y = MARGIN;
    } else {
        icon_y = MARGIN;
        text1_y = (ICON_SIZE - text_block_height) / 2 + MARGIN;
    }
    SetWindowPos(GetDlgItem(hwnd,IDC_SHUTDOWN_ICON),NULL,
        MARGIN,
        icon_y,
        ICON_SIZE,
        ICON_SIZE,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_MESSAGE),NULL,
        MARGIN + ICON_SIZE + LARGE_SPACING,
        text1_y,
        text_block_width,
        text1_height + SMALL_SPACING,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_DELAY_MSG),NULL,
        MARGIN + ICON_SIZE + LARGE_SPACING,
        text1_y + text1_height + SMALL_SPACING,
        text_block_width,
        text2_height + SMALL_SPACING,
        SWP_NOZORDER);
    button1_x = (client_width - button_width * 2 - SMALL_SPACING) / 2;
    if((client_width - button_width * 2 - SMALL_SPACING) % 2)
        buttons_spacing ++;
    SetWindowPos(GetDlgItem(hwnd,IDC_YES_BUTTON),NULL,
        button1_x,
        MARGIN + top_height + LARGE_SPACING,
        button_width,
        button_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_NO_BUTTON),NULL,
        button1_x + button_width + buttons_spacing,
        MARGIN + top_height + LARGE_SPACING,
        button_width,
        button_height,
        SWP_NOZORDER);

    /* set localized texts */
    SetWindowTextW(GetDlgItem(hwnd,IDC_MESSAGE),text1);
    SetWindowTextW(GetDlgItem(hwnd,IDC_DELAY_MSG),text2);
    
    /* center window on the screen */
    WgxCenterWindow(hwnd);

done:
    free(text1);
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
            WgxDbgPrintLastError("ShutdownConfirmDlgProc: SetTimer failed");
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
 * @return Error code. Zero indicates success.
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
                return 0;
            /* in case of errors we'll shutdown anyway */
            /* to avoid situation when pc works a long time without any control */
        }
        break;
    case IDM_WHEN_DONE_EXIT:
        return 0; /* nothing to do */
    }
    
    /* set SE_SHUTDOWN privilege */
    if(!OpenProcessToken(GetCurrentProcess(), 
    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"ShutdownOrHibernate: cannot open process token!");
        return 4;
    }
    
    LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
    AdjustTokenPrivileges(hToken,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);         
    if(GetLastError() != ERROR_SUCCESS){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"ShutdownOrHibernate: cannot set shutdown privilege!");
        return 5;
    }
    
    /*
    * Save log file before any action.
    */
    udefrag_flush_dbg_log();
    
    
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
            WgxDbgPrintLastError("ShutdownOrHibernate: cannot load powrprof.dll");
        } else {
            pSetSuspendState = (SET_SUSPEND_STATE_PROC)GetProcAddress(hPowrProfDll,"SetSuspendState");
            if(pSetSuspendState == NULL)
                WgxDbgPrintLastError("ShutdownOrHibernate: cannot get SetSuspendState address inside powrprof.dll");
        }
        if(pSetSuspendState == NULL)
            WgxDbgPrint("Therefore SetSystemPowerState API will be used instead of SetSuspendState.\n");
    }

    switch(when_done_action){
    case IDM_WHEN_DONE_STANDBY:
        /* suspend, request permission from apps and drivers */
        if(pSetSuspendState)
            result = pSetSuspendState(FALSE,FALSE,FALSE);
        else
            result = SetSystemPowerState(TRUE,FALSE);
        if(!result){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot suspend the system!");
            return 6;
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
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot hibernate the computer!");
            return 7;
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
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot log the user off!");
            return 8;
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
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot reboot the computer!");
            return 9;
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
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot shut down the computer!");
            return 10;
        }
        break;
    }
    
    return 0;
}

/** @} */
