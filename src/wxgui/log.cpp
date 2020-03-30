//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2015 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file log.cpp
 * @brief Logging.
 * @addtogroup Logging
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                                Logging
// =======================================================================

void Log::DoLogTextAtLevel(wxLogLevel level, const wxString& msg)
{
    switch(level){
    case wxLOG_FatalError:
        // XXX: fatal errors pass by actually
        trace(E"%ls",ws(msg));
        winx_flush_dbg_log(0);
        break;
    case wxLOG_Error:
        trace(E"%ls",ws(msg));
        break;
    case wxLOG_Warning:
    case wxLOG_Info:
        trace(D"%ls",ws(msg));
        break;
    default:
        trace(I"%ls",ws(msg));
        break;
    }
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnDebugLog(wxCommandEvent& WXUNUSED(event))
{
    wxString logpath;
    if(!wxGetEnv(wxT("UD_LOG_FILE_PATH"),&logpath)){
        wxMessageBox(wxT("Logging to a file is turned off."),
            wxT("Cannot open log file!"),wxOK | wxICON_HAND,this);
    } else {
        wxFileName file(logpath);
        file.Normalize();
        ::winx_flush_dbg_log(0);
        logpath = file.GetFullPath();
        if(!wxLaunchDefaultBrowser(logpath))
            Utils::ShowError(wxT("Cannot open %ls!"),ws(logpath));
    }
}

void MainFrame::OnDebugSend(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("Troubleshooting.html"));
}

/** @} */
