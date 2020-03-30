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
 * @file i18n.cpp
 * @brief Internationalization.
 * @addtogroup Internationalization
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                            Global variables
// =======================================================================

wxLocale *g_locale = NULL;

// =======================================================================
//                         Internationalization
// =======================================================================

// macro to add missing languages copied from wxWidgets sources
#define UD_LNG(wxlang, canonical, winlang, winsublang, layout, desc) { \
    wxLanguageInfo info;                                  \
    info.Language = wxlang;                               \
    info.CanonicalName = wxT(canonical);                  \
    info.LayoutDirection = layout;                        \
    info.Description = wxT(desc);                         \
    info.WinLang = winlang, info.WinSublang = winsublang; \
    g_locale->AddLanguage(info);                          \
}

#define UD_UpdateMenuItemLabel(id,label,accel) { \
    if(::strlen(accel)){ \
        wxString ItemLabel = _(label); \
        ItemLabel << wxT("\t") << wxT(accel); \
        m_menuBar->FindItem(id)->SetItemLabel(ItemLabel); \
        if(m_toolBar->FindById(id)){ \
            ItemLabel = _(label); \
            ItemLabel << wxT(" (") << wxT(accel) << wxT(")"); \
            m_toolBar->SetToolShortHelp(id,ItemLabel); \
        } \
    } else { \
        m_menuBar->FindItem(id)->SetItemLabel(_(label)); \
        if(m_toolBar->FindById(id)) \
            m_toolBar->SetToolShortHelp(id,_(label)); \
    } \
}

void App::ResetLocale()
{
    // release previously set locale
    delete g_locale;

    // create a new one
    g_locale = new wxLocale();

    // add translations missing from wxWidgets
    UD_LNG(wxUD_LANGUAGE_ILOKO,             "ilo"  , 0             , 0              , wxLayout_LeftToRight, "Iloko");
    UD_LNG(wxUD_LANGUAGE_KAPAMPANGAN,       "pam"  , 0             , 0              , wxLayout_LeftToRight, "Kapampangan");
    UD_LNG(wxUD_LANGUAGE_NORWEGIAN,         "no"   , LANG_NORWEGIAN, SUBLANG_DEFAULT, wxLayout_LeftToRight, "Norwegian");
    UD_LNG(wxUD_LANGUAGE_WARAY_WARAY,       "war"  , 0             , 0              , wxLayout_LeftToRight, "Waray-Waray");
    UD_LNG(wxUD_LANGUAGE_ACOLI,             "ach"  , 0             , 0              , wxLayout_LeftToRight, "Acoli");
    UD_LNG(wxUD_LANGUAGE_SINHALA_SRI_LANKA, "si_LK", 0             , 0              , wxLayout_LeftToRight, "Sinhala (Sri Lanka)");
    UD_LNG(wxUD_LANGUAGE_SILESIAN,          "szl"  , 0             , 0              , wxLayout_LeftToRight, "Silesian");
}

void App::SetLocale(int id)
{
    ResetLocale();

    // find the most suitable translation
    if(id == wxLANGUAGE_UNKNOWN){
        wxConfigBase *cfg = wxConfigBase::Get();
        if(cfg->HasGroup(wxT("Language"))){
            wxString ver = cfg->Read(
                wxT("/Language/Version"),
                wxT("2.8.12")
            );
            // don't accept it if version is older than 3.0.2
            // as language identifiers got changed since then
            // TODO: adjust on subsequent wxWidgets upgrades
            if(ver != wxT("2.8.12")){
                id = (int)cfg->Read(
                    wxT("/Language/Selected"),
                    (long)wxLANGUAGE_UNKNOWN
                );
            }
        }
    }

    // if still not found, use system language
    if(id == wxLANGUAGE_UNKNOWN)
        id = g_locale->GetSystemLanguage();

    // use English (US) as the last resort
    if(id == wxLANGUAGE_UNKNOWN \
      || id == wxLANGUAGE_ENGLISH)
        id = wxLANGUAGE_ENGLISH_US;

    // apply language selection
    if(!g_locale->Init(id)){
        ResetLocale();
        (void)g_locale->Init(wxLANGUAGE_ENGLISH_US);
    }

    g_locale->AddCatalogLookupPathPrefix(wxT("locale"));

    // locations for development
    g_locale->AddCatalogLookupPathPrefix(wxT("../wxgui/locale"));
    g_locale->AddCatalogLookupPathPrefix(wxT("../../wxgui/locale"));

    g_locale->AddCatalog(wxT("UltraDefrag"));
}

/**
 * @brief Manages translation changes.
 */
void MainFrame::OnLocaleChange(wxCommandEvent& event)
{
    App::SetLocale(event.GetId() - ID_LocaleChange);

    // reset the language menu check mark, just in case
    // SetLocale failed to set the requested language
    int id = g_locale->GetLanguage();
    wxMenuItem *menuItem = m_menuBar->FindItem(ID_LocaleChange + id);
    if(menuItem) menuItem->Check(true);

    // update menu labels and tool bar tool-tips

    // main menu
    m_menuBar->SetMenuLabel(0, _("&Action"));
    m_menuBar->SetMenuLabel(1, _("&Settings"));
    m_menuBar->SetMenuLabel(2, _("&Help"));

    // action menu
    UD_UpdateMenuItemLabel(ID_Analyze    , "&Analyze"              , "F5");
    UD_UpdateMenuItemLabel(ID_Defrag     , "&Defragment"           , "F6");
    UD_UpdateMenuItemLabel(ID_QuickOpt   , "&Quick optimization"   , "F7");
    UD_UpdateMenuItemLabel(ID_FullOpt    , "&Full optimization"    , "Ctrl+F7");
    UD_UpdateMenuItemLabel(ID_MftOpt     , "&Optimize MFT"         , "Shift+F7");
    UD_UpdateMenuItemLabel(ID_Pause      , "Pa&use"                , "Space");
    UD_UpdateMenuItemLabel(ID_Stop       , "&Stop"                 , "Ctrl+C");
    UD_UpdateMenuItemLabel(ID_ShowReport , "&Show report"          , "F8");
    UD_UpdateMenuItemLabel(ID_Repeat     , "Re&peat action"        , "Shift+R");
    UD_UpdateMenuItemLabel(ID_SkipRem    , "Skip removable &media" , "Ctrl+M");
    UD_UpdateMenuItemLabel(ID_Rescan     , "&Rescan drives"        , "Ctrl+D");
    UD_UpdateMenuItemLabel(ID_Repair     , "Repair dri&ves"        , "");
    UD_UpdateMenuItemLabel(ID_Exit       , "E&xit"                 , "Alt+F4");

    // when done sub-menu
    m_subMenuWhenDone->SetItemLabel(_("&When done"));
    UD_UpdateMenuItemLabel(ID_WhenDoneNone      , "&None"      , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneExit      , "E&xit"      , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneStandby   , "Stan&dby"   , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneHibernate , "&Hibernate" , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneLogoff    , "&Logoff"    , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneReboot    , "&Reboot"    , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneShutdown  , "&Shutdown"  , "");

    // settings menu
    m_subMenuLanguage->SetItemLabel(_("&Language"));
    UD_UpdateMenuItemLabel(ID_GuiOptions , "&Options" , "F10");
    m_subMenuSortingConfig->SetItemLabel(_("&Sorting"));
    m_subMenuBootConfig->SetItemLabel(_("&Boot time scan"));

    // language sub-menu
    UD_UpdateMenuItemLabel(ID_LangTranslateOnline  , "Translate &online"    , "");
    UD_UpdateMenuItemLabel(ID_LangTranslateOffline , "Translate o&ffline"   , "");
    UD_UpdateMenuItemLabel(ID_LangOpenFolder       , "&Translations folder" , "");

    // boot time scan sub-menu
    UD_UpdateMenuItemLabel(ID_BootEnable , "&Enable" , "F11");
    UD_UpdateMenuItemLabel(ID_BootScript , "&Script" , "F12");

    // sorting sub-menu
    UD_UpdateMenuItemLabel(ID_SortByPath             , "By &path"                   , "");
    UD_UpdateMenuItemLabel(ID_SortBySize             , "By &size"                   , "");
    UD_UpdateMenuItemLabel(ID_SortByCreationDate     , "By &creation time"          , "");
    UD_UpdateMenuItemLabel(ID_SortByModificationDate , "By last &modification time" , "");
    UD_UpdateMenuItemLabel(ID_SortByLastAccessDate   , "By &last access time"       , "");
    UD_UpdateMenuItemLabel(ID_SortAscending          , "In &ascending order"        , "");
    UD_UpdateMenuItemLabel(ID_SortDescending         , "In &descending order"       , "");

    // help menu
    UD_UpdateMenuItemLabel(ID_HelpContents     , "&Contents"                   , "F1");
    UD_UpdateMenuItemLabel(ID_HelpBestPractice , "Best &practice"              , "F2");
    UD_UpdateMenuItemLabel(ID_HelpFaq          , "&Frequently asked questions" , "F3");
    UD_UpdateMenuItemLabel(ID_HelpLegend       , "Cluster map &legend"         , "");

    // upgrade sub-menu
    m_subMenuUpgrade->SetItemLabel(_("&Upgrade"));
    UD_UpdateMenuItemLabel(ID_HelpUpgradeNone   , "&Never check"                , "");
    UD_UpdateMenuItemLabel(ID_HelpUpgradeStable , "Check &stable releases only" , "");
    UD_UpdateMenuItemLabel(ID_HelpUpgradeAll    , "Check &all releases"         , "");
    UD_UpdateMenuItemLabel(ID_HelpUpgradeCheck  , "&Check now"                  , "");
    UD_UpdateMenuItemLabel(ID_HelpAbout         , "&About"                      , "F4");

    // debug sub-menu
    m_subMenuDebug->SetItemLabel(_("&Debug"));
    UD_UpdateMenuItemLabel(ID_DebugLog  , "Open &log"        , "Alt+L");
    UD_UpdateMenuItemLabel(ID_DebugSend , "Send bug &report" , "");

    // update tool-tips that differ from menu labels
    wxString label = _("&Boot time scan"); label << wxT(" (F11)");
    m_toolBar->SetToolShortHelp(ID_BootEnable,label);
    label = _("Boot time script"); label << wxT(" (F12)");
    m_toolBar->SetToolShortHelp(ID_BootScript,label);
    label = _("&Help"); label << wxT(" (F1)");
    m_toolBar->SetToolShortHelp(ID_HelpContents,label);

    // update list column labels
    wxListItem item; item.SetMask(wxLIST_MASK_TEXT);
    item.SetText(_("Drive"));         m_vList->SetColumn(0,item);
    item.SetText(_("Status"));        m_vList->SetColumn(1,item);
    item.SetText(_("Fragmentation")); m_vList->SetColumn(2,item);
    item.SetText(_("Total space"));   m_vList->SetColumn(3,item);
    item.SetText(_("Free space"));    m_vList->SetColumn(4,item);
    //xgettext:no-c-format
    item.SetText(_("% free"));        m_vList->SetColumn(5,item);

    // set mono-space font for the list unless Burmese translation is selected
    if(g_locale->GetCanonicalName().Left(2) != wxT("my")){
        wxFont font = m_vList->GetFont();
        if(font.SetFaceName(wxT("Courier New")))
            m_vList->SetFont(font);
    } else {
        m_vList->SetFont(*m_vListFont);
    }

    // update list status fields
    for(int i = 0; i < m_vList->GetItemCount(); i++){
        int letter = (int)m_vList->GetItemText(i)[0];
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_UpdateVolumeStatus);
        event.SetInt(letter); GetEventHandler()->ProcessEvent(event);
    }

    // update task-bar icon overlay
    ProcessCommandEvent(this,ID_AdjustTaskbarIconOverlay);

    // update progress counters
    ProcessCommandEvent(this,ID_UpdateStatusBar);

    // update report translation
    App::SaveReportTranslation();
}

// =======================================================================
//                       reports.lng file handling
// =======================================================================

#define UD_AddTranslationString(key, value) { \
    wxString v = value; \
    file.AddLine(wxString::Format( \
        wxT("%hs%ls"), key, ws(v)) \
    ); \
}

void App::SaveReportTranslation()
{
    wxTextFile file;
    file.Create(wxT("reports.lng"));
    UD_AddTranslationString("FRAGMENTED_FILES_ON = ", _("Fragmented files on"));
    UD_AddTranslationString("VISIT_HOMEPAGE      = ", _("Visit our Homepage") );
    UD_AddTranslationString("VIEW_REPORT_OPTIONS = ", _("View report options"));
    UD_AddTranslationString("POWERED_BY_LUA      = ", _("Powered by Lua")     );
    UD_AddTranslationString("FRAGMENTS           = ", _("Fragments")          );
    UD_AddTranslationString("SIZE                = ", _("Size")               );
    UD_AddTranslationString("FILENAME            = ", _("Filename")           );
    UD_AddTranslationString("COMMENT             = ", _("Comment")            );
    UD_AddTranslationString("STATUS              = ", _("Status")             );
    UD_AddTranslationString("LOCKED              = ", _("locked")             );
    UD_AddTranslationString("MOVE_FAILED         = ", _("move failed")        );
    UD_AddTranslationString("INVALID             = ", _("invalid")            );
    file.Write();
    file.Close();
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnLangTranslateOnline(wxCommandEvent& WXUNUSED(event))
{
    wxString url(wxT("https://www.transifex.com/projects/p/ultradefrag/resource/main/"));
    if(!wxLaunchDefaultBrowser(url))
        Utils::ShowError(wxT("Cannot open %ls!"),ws(url));
}

void MainFrame::OnLangTranslateOffline(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("Translation.html"));
}

void MainFrame::OnLangOpenFolder(wxCommandEvent& WXUNUSED(event))
{
    wxString AppPoDir(wxGetCwd() + wxT("/po"));

    if(!wxDirExists(AppPoDir)){
        etrace("po dir not found: %ls",ws(AppPoDir));
    } else {
        if(!wxLaunchDefaultBrowser(AppPoDir))
            Utils::ShowError(wxT("Cannot open %ls!"),ws(AppPoDir));
    }
}

/** @} */
