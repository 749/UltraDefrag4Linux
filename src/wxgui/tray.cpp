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
 * @file tray.cpp
 * @brief System tray icon.
 * @addtogroup SystemTray
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
//                              Popup menu
// =======================================================================

BEGIN_EVENT_TABLE(SystemTrayIcon, wxTaskBarIcon)
    EVT_MENU(ID_ShowHideMenu, SystemTrayIcon::OnMenuShowHide)
    EVT_MENU(ID_PauseMenu, SystemTrayIcon::OnMenuPause)
    EVT_MENU(ID_ExitMenu, SystemTrayIcon::OnMenuExit)
    EVT_TASKBAR_LEFT_UP(SystemTrayIcon::OnLeftButtonUp)
END_EVENT_TABLE()

wxMenu *SystemTrayIcon::CreatePopupMenu()
{
    wxMenu *menu = new wxMenu;

    wxMenuItem *item = new wxMenuItem(NULL,ID_ShowHideMenu,
        g_mainFrame->IsIconized() ? _("Show") : _("Hide"));

    // somehow in wxWidgets 3.0 item->GetFont() fails
    wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    font.MakeBold(); dtrace("%ls",ws(font.GetNativeFontInfoDesc()));

    item->SetFont(font);
    menu->Append(item);
    menu->AppendSeparator();

    item = menu->AppendCheckItem(ID_PauseMenu,_("Pa&use"));
    if(g_mainFrame->m_paused) item->Check(true);
    menu->AppendSeparator();

    menu->Append(ID_ExitMenu,_("E&xit"));
    return menu;
}

void SystemTrayIcon::OnMenuShowHide(wxCommandEvent& WXUNUSED(event))
{
    if(g_mainFrame->IsIconized()){
        g_mainFrame->Show();
        HWND hWindow = (HWND)g_mainFrame->GetHandle();
        if(g_mainFrame->IsMaximized()){
            ::ShowWindow(hWindow,SW_MAXIMIZE);
        } else {
            ::ShowWindow(hWindow,SW_RESTORE);
        }
        ::SetForegroundWindow(hWindow);
    } else {
        g_mainFrame->Iconize();
    }
}

void SystemTrayIcon::OnMenuPause(wxCommandEvent& WXUNUSED(event))
{
    QueueCommandEvent(g_mainFrame,ID_Pause);
}

void SystemTrayIcon::OnMenuExit(wxCommandEvent& WXUNUSED(event))
{
    QueueCommandEvent(g_mainFrame,ID_Exit);
}

void SystemTrayIcon::OnLeftButtonUp(wxTaskBarIconEvent& WXUNUSED(event))
{
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_ShowHideMenu);
    ProcessEvent(event);
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::AdjustSystemTrayIcon(wxCommandEvent& event)
{
    if(CheckOption(wxT("UD_MINIMIZE_TO_SYSTEM_TRAY"))){
        wxString icon(wxT("tray"));
        if(m_busy){
            if(m_paused) icon << wxT("_paused");
            else icon << wxT("_running");
        }

        wxString tooltip = event.GetString().IsEmpty() ? \
            wxT("UltraDefrag") : event.GetString();

        wxIcon i = wxIcon(icon,wxBITMAP_TYPE_ICO_RESOURCE,g_iconSize,g_iconSize);
        if(!m_systemTrayIcon->SetIcon(i,tooltip)){
            etrace("system tray icon setup failed");
            wxSetEnv(wxT("UD_MINIMIZE_TO_SYSTEM_TRAY"),wxT("0"));
        }

        if(m_systemTrayIcon->IsIconInstalled()){
            if(IsIconized()) Hide();
        }
    } else {
        if(m_systemTrayIcon->IsIconInstalled()){
            m_systemTrayIcon->RemoveIcon(); Show();
        }
    }
}

/** @} */
