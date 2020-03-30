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
 * @file report.cpp
 * @brief Disk fragmentation report.
 * @addtogroup Report
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
//                            Event handlers
// =======================================================================

void MainFrame::OnShowReport(wxCommandEvent& WXUNUSED(event))
{
    if(m_busy) return;

    wxFileName lua(wxT(".\\lua5.1a_gui.exe")); lua.Normalize();

    long i = m_vList->GetFirstSelected();
    while(i != -1){
        char letter = (char)m_vList->GetItemText(i)[0];
        wxString path = wxString::Format(
            wxT(".\\reports\\fraglist_%c.luar"),
            winx_tolower(letter));
        wxFileName report(path); report.Normalize();
        if(report.FileExists()){
            wxString args = wxString::Format(
                wxT(".\\scripts\\udreportcnv.lua \"%ls\" -v"),
                ws(report.GetFullPath()));
            Utils::ShellExec(lua.GetFullPath(),wxT("open"),args);
        }
        i = m_vList->GetNextSelected(i);
    }
}

/** @} */
