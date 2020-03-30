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
 * @file about.cpp
 * @brief About box.
 * @addtogroup AboutBox
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

#define HOMEPAGE wxT("http://ultradefrag.sourceforge.net")

class HomePageLink: public wxGenericHyperlinkCtrl {
public:
    HomePageLink(wxWindow* parent,const wxString& title,const wxString& url)
      : wxGenericHyperlinkCtrl(parent,wxID_ANY,title,url) {
            // use custom paint handler to get rid of annoying border
            GetEventHandler()->Connect(wxEVT_PAINT,
                wxPaintEventHandler(HomePageLink::OnPaint)
            );
        }
    ~HomePageLink() {}

    void OnKeyUp(wxKeyEvent& event);
    void OnPaint(wxPaintEvent& event);

private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(HomePageLink, wxGenericHyperlinkCtrl)
    EVT_KEY_UP(HomePageLink::OnKeyUp)
END_EVENT_TABLE()

// =======================================================================
//                            Event handlers
// =======================================================================

void HomePageLink::OnKeyUp(wxKeyEvent& event)
{
    switch(event.GetKeyCode()){
    case WXK_ESCAPE:
        GetParent()->Destroy();
        return;
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        wxLaunchDefaultBrowser(HOMEPAGE);
        return;
    }
    event.Skip();
}

// copied from %WXWIDGETSDIR%\src\generic\hyperlinkg.cpp
void HomePageLink::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    dc.SetFont(GetFont());
    dc.SetTextForeground(GetForegroundColour());
    dc.SetTextBackground(GetBackgroundColour());

    dc.DrawText(GetLabel(), GetLabelRect().GetTopLeft());
    // draw no border around even when the link has focus
}

void MainFrame::OnHelpAbout(wxCommandEvent& WXUNUSED(event))
{
    wxDialog dlg(this,wxID_ANY,_("About UltraDefrag"));

    wxStaticBitmap *bmp = new wxStaticBitmap(&dlg,wxID_ANY,wxBITMAP(ship));

    wxStaticText *version = new wxStaticText(&dlg,wxID_ANY,wxT(VERSIONINTITLE));
    wxFont fontBig(*wxNORMAL_FONT);
    fontBig.SetPointSize(fontBig.GetPointSize() + 2);
    fontBig.SetWeight(wxFONTWEIGHT_BOLD);
    version->SetFont(fontBig);

    wxStaticText *copyright = new wxStaticText(&dlg,wxID_ANY,
        wxT("(C) 2007-2016 UltraDefrag development team"));
    wxStaticText *description = new wxStaticText(&dlg,wxID_ANY,
        _("An open source defragmentation utility."));
    wxStaticText *credits = new wxStaticText(&dlg,wxID_ANY,
        _("Credits and licenses are listed in the handbook."));

    HomePageLink *homepage = new HomePageLink(&dlg,
        _("UltraDefrag website"),HOMEPAGE);

    // Burmese needs Padauk font for display
    if(g_locale->GetCanonicalName().Left(2) == wxT("my")){
        wxFont textFont = description->GetFont();
        if(!textFont.SetFaceName(wxT("Padauk"))){
            etrace("Padauk font needed for correct Burmese text display not found");
        } else {
            textFont.SetPointSize(textFont.GetPointSize() + 2);
            version->SetFont(textFont);
            copyright->SetFont(textFont);
            description->SetFont(textFont);
            credits->SetFont(textFont);
            homepage->SetFont(textFont);
        }
    }

    wxSizerFlags flags; flags.Center();

    wxSizer *bmpblock = new wxBoxSizer(wxVERTICAL);
    bmpblock->AddSpacer(SMALL_SPACING);
    bmpblock->Add(bmp,flags);
    bmpblock->AddSpacer(SMALL_SPACING);

    wxSizer *text = new wxBoxSizer(wxVERTICAL);
    text->AddSpacer(LARGE_SPACING);
    text->Add(version,flags);
    text->AddSpacer(LARGE_SPACING);
    text->Add(copyright,flags);
    text->AddSpacer(LARGE_SPACING);
    text->Add(description,flags);
    text->Add(credits,flags);
    text->AddSpacer(LARGE_SPACING);
    text->Add(homepage,flags);
    text->AddSpacer(LARGE_SPACING);

    wxSizer *contents = new wxBoxSizer(wxHORIZONTAL);
    contents->AddSpacer(SMALL_SPACING);
    contents->Add(bmpblock,wxSizerFlags().Center());
    contents->AddSpacer(SMALL_SPACING);
    contents->AddSpacer(SMALL_SPACING);
    contents->Add(text,wxSizerFlags(1).Center());
    contents->AddSpacer(SMALL_SPACING);
    dlg.SetSizerAndFit(contents);

    if(!IsIconized()) dlg.Center();
    else dlg.CenterOnScreen();

    homepage->SetFocus();

    dlg.ShowModal();
}

/** @} */
