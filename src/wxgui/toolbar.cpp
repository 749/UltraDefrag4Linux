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
 * @file toolbar.cpp
 * @brief Toolbar.
 * @addtogroup Toolbar
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
//                       Tool bar for main window
// =======================================================================

#define UD_MakeToolItem(id, icon) { \
    wxString string; \
    string.Printf(wxT("%hs%u"),#icon,g_iconSize); \
    wxBitmap pic = Utils::LoadPngResource(ws(string)); \
    if(pic.IsOk()){ \
        wxImage img = pic.ConvertToImage(); \
        wxBitmap bmp(img.ConvertToGreyscale()); \
        m_toolBar->AddTool(id,wxEmptyString,pic,bmp); \
    } \
}

#define UD_MakeToolCheckItem(id, icon) { \
    wxString string; \
    string.Printf(wxT("%hs%u"),#icon,g_iconSize); \
    wxBitmap pic = Utils::LoadPngResource(ws(string)); \
    if(pic.IsOk()){ \
        wxImage img = pic.ConvertToImage(); \
        wxBitmap bmp(img.ConvertToGreyscale()); \
        m_toolBar->AddCheckTool(id,wxEmptyString,pic,bmp); \
    } \
}

/**
 * @brief Initializes tool bar.
 */
void MainFrame::InitToolbar()
{
    m_toolBar = CreateToolBar(wxTB_FLAT);
    m_toolBar->SetToolBitmapSize(wxSize(g_iconSize,g_iconSize));

    UD_MakeToolItem(ID_Analyze         , glass   );
    UD_MakeToolItem(ID_Defrag          , defrag  );
    UD_MakeToolItem(ID_QuickOpt        , quick   );
    UD_MakeToolItem(ID_FullOpt         , full    );
    UD_MakeToolItem(ID_MftOpt          , mft     );
    UD_MakeToolCheckItem(ID_Pause      , pause   );
    UD_MakeToolItem(ID_Stop            , stop    );
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_ShowReport      , report  );
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_GuiOptions      , gear    );
    m_toolBar->AddSeparator();
    UD_MakeToolCheckItem(ID_BootEnable , terminal);
    UD_MakeToolItem(ID_BootScript      , script  );
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_HelpContents    , help    );

    m_toolBar->Realize();

    // initial settings (must be after realize)
    // m_toolBar->ToggleTool(ID_XXX,m_xxx);
}

/** @} */
