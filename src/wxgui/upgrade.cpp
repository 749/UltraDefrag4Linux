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
 * @file upgrade.cpp
 * @brief Upgrade.
 * @addtogroup Upgrade
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "prec.h"
#include "main.h"

enum {
    UPGRADE_NONE = 0,
    UPGRADE_STABLE,
    UPGRADE_ALL
};

#define VERSION_URL        "http://ultradefrag.net/version.ini"
#define STABLE_VERSION_URL "http://ultradefrag.net/stable-version.ini"

// =======================================================================
//                          Upgrade handling
// =======================================================================

void *UpgradeThread::Entry()
{
    while(!g_mainFrame->CheckForTermination(200)){
        if(m_check && m_level){
            wxFileName target(wxT(".\\tmp"));
            target.Normalize();
            wxString dir(target.GetFullPath());
            if(!wxDirExists(dir)) wxMkdir(dir);

            wxString url(wxT(""));
            wxString path(dir);

            if(m_level == UPGRADE_ALL){
                url << wxT(VERSION_URL);
                path << wxT("\\version.ini");
            } else {
                url << wxT(STABLE_VERSION_URL);
                path << wxT("\\stable-version.ini");
            }

            if(Utils::DownloadFile(url,path)){
                wxTextFile file; file.Open(path);
                wxString lv = file.GetFirstLine();
                lv.Trim(true); lv.Trim(false);
                int last = ParseVersionString(ansi(lv));

                const char *cv = VERSIONINTITLE;
                int current = ParseVersionString(&cv[12]);

                itrace("latest version : %hs",ansi(lv));
                itrace("current version: %hs",&cv[12]);

                if(last && current && last > current){
                    wxCommandEvent *event = new wxCommandEvent(
                        wxEVT_COMMAND_MENU_SELECTED,ID_ShowUpgradeDialog
                    );
                    event->SetString(lv.c_str()); // make a deep copy
                    g_mainFrame->GetEventHandler()->QueueEvent(event);
                }

                file.Close(); wxRemoveFile(path);
            }
            m_check = false;
        }
    }

    return NULL;
}

/**
 * @brief Parses a version string and generates an integer for comparison.
 * @return An integer representing the version. Zero indicates that
 * the version string parsing failed.
 */
int UpgradeThread::ParseVersionString(const char *version)
{
    char *string = _strdup(version);
    if(!string) return 0;

    _strlwr(string);

    // version numbers (major, minor, revision, unstable version)
    int mj, mn, rev, uv;

    int res = sscanf(string,"%u.%u.%u alpha%u",&mj,&mn,&rev,&uv);
    if(res == 4){
        uv += 100;
    } else {
        res = sscanf(string,"%u.%u.%u beta%u",&mj,&mn,&rev,&uv);
        if(res == 4){
            uv += 200;
        } else {
            res = sscanf(string,"%u.%u.%u rc%u",&mj,&mn,&rev,&uv);
            if(res == 4){
                uv += 300;
            } else {
                res = sscanf(string,"%u.%u.%u",&mj,&mn,&rev);
                if(res == 3){
                    uv = 999;
                } else {
                    etrace("parsing of '%hs' failed",version);
                    return 0;
                }
            }
        }
    }

    free(string);

    /* 5.0.0 > 4.99.99 rc10*/
    return mj * 10000000 + mn * 100000 + rev * 1000 + uv;
}

// =======================================================================
//                           Event handlers
// =======================================================================

void MainFrame::OnHelpUpgrade(wxCommandEvent& event)
{
    switch(event.GetId()){
        case ID_HelpUpgradeNone:
            // disable checks
            m_upgradeThread->m_level = 0;
            break;
        case ID_HelpUpgradeStable:
        case ID_HelpUpgradeAll:
            // enable the check...
            m_upgradeThread->m_level = event.GetId() - ID_HelpUpgradeNone;
            // ...and check it now
        case ID_HelpUpgradeCheck:
            m_upgradeThread->m_check = true;
    }
}

void MainFrame::ShowUpgradeDialog(wxCommandEvent& event)
{
    wxString message = wxString();
    //: This expands to "Release 7.0.0 is available for download!"
    //: Make sure that "%ls" is included in the translated string at the correct position
    message.Printf(_("Release %ls is available for download!"),ws(event.GetString()));

    if(Utils::MessageDialog(this,_("You can upgrade me ^-^"),
      wxART_INFORMATION,_("&Upgrade"),_("&Cancel"),ts(message)) == wxID_OK)
    {
        wxString url(wxT("https://ultradefrag.net"));
        if(!wxLaunchDefaultBrowser(url))
            Utils::ShowError(wxT("Cannot open %ls!"),ws(url));
    }
}

/** @} */
