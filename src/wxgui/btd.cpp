//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
//  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//////////////////////////////////////////////////////////////////////////

/**
 * @file btd.cpp
 * @brief Boot time defragmenter registration tracking.
 * @addtogroup BootTracking
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "prec.h"
#include "main.h"

// =======================================================================
//              Boot time defragmenter registration tracking
// =======================================================================

void *BtdThread::Entry()
{
    itrace("boot registration tracking started");

    HKEY hKey = NULL; HANDLE hEvent = NULL;
    if(::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
      L"SYSTEM\\CurrentControlSet\\Control\\Session Manager",
      0,KEY_NOTIFY,&hKey) != ERROR_SUCCESS){
        letrace("cannot open SMSS key");
        goto done;
    }

    hEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL);
    if(hEvent == NULL){
        letrace("cannot create event for SMSS key tracking");
        goto done;
    }

    while(!g_mainFrame->CheckForTermination(1)){
        LONG error = ::RegNotifyChangeKeyValue(hKey,FALSE,
            REG_NOTIFY_CHANGE_LAST_SET,hEvent,TRUE);
        if(error != ERROR_SUCCESS){
            ::SetLastError(error);
            letrace("RegNotifyChangeKeyValue failed");
            break;
        }
        while(!g_mainFrame->CheckForTermination(1)){
            if(::WaitForSingleObject(hEvent,100) == WAIT_OBJECT_0){
                int result = ::winx_bootex_check(L"defrag_native");
                if(result >= 0){
                    itrace("boot time defragmenter %hs",
                        result > 0 ? "enabled" : "disabled");
                    wxCommandEvent *event = new wxCommandEvent(
                        wxEVT_COMMAND_MENU_SELECTED,ID_BootChange
                    );
                    event->SetInt(result > 0 ? true : false);
                    g_mainFrame->GetEventHandler()->QueueEvent(event);
                }
                break;
            }
        }
    }

done:
    if(hEvent) ::CloseHandle(hEvent);
    if(hKey) ::RegCloseKey(hKey);
    itrace("boot registration tracking stopped");
    return NULL;
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnBootEnable(wxCommandEvent& WXUNUSED(event))
{
    int result;
    if(m_btdEnabled)
        result = ::winx_bootex_unregister(L"defrag_native");
    else
        result = ::winx_bootex_register(L"defrag_native");
    if(result == 0){
        // registration succeeded
        m_btdEnabled = m_btdEnabled ? false : true;
        m_menuBar->Check(ID_BootEnable,m_btdEnabled);
        m_toolBar->ToggleTool(ID_BootEnable,m_btdEnabled);
    } else {
        if(m_btdEnabled) Utils::ShowError(wxT("Cannot disable the boot time defragmenter!"));
        else  Utils::ShowError(wxT("Cannot enable the boot time defragmenter!"));
    }
}

void MainFrame::OnBootChange(wxCommandEvent& event)
{
    m_btdEnabled = (event.GetInt() > 0);
    m_menuBar->Check(ID_BootEnable,m_btdEnabled);
    m_toolBar->ToggleTool(ID_BootEnable,m_btdEnabled);
}

/** @} */
