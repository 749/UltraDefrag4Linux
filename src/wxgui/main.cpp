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
 * @file main.cpp
 * @brief Main window.
 * @addtogroup MainWindow
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "prec.h"
#include "main.h"

#if !defined(__GNUC__)
#include <new.h> // for _set_new_handler
#endif

// Uncomment to test crash reporting facilities.
// NOTE: on Windows 7 you should reset Fault Tolerant
// Heap protection from time to time via the following
// command: rundll32 fthsvc.dll,FthSysprepSpecialize
// Otherwise some of crash tests will fail.
// #define CRASH_TESTS

// =======================================================================
//                            Global variables
// =======================================================================

MainFrame *g_mainFrame = NULL;
double g_scaleFactor = 1.0f;   // DPI-aware scaling factor
int g_iconSize;                // small icon size
HANDLE g_synchEvent = NULL;    // synchronization for threads
UINT g_TaskbarIconMsg;         // taskbar icon overlay setup on shell restart

// =======================================================================
//                             Web statistics
// =======================================================================

void *StatThread::Entry()
{
    bool enabled = true; wxString s;
    if(wxGetEnv(wxT("UD_DISABLE_USAGE_TRACKING"),&s))
        if(s.Cmp(wxT("1")) == 0) enabled = false;

    if(enabled){
        GA_REQUEST(USAGE_TRACKING);
#ifdef SEND_TEST_REPORTS
        GA_REQUEST(TEST_TRACKING);
#endif
    }

    return NULL;
}

// =======================================================================
//                    Application startup and shutdown
// =======================================================================

/**
 * @brief Sets icon for system modal message boxes.
 */
BOOL CALLBACK DummyDlgProc(HWND hWnd,
    UINT msg,WPARAM wParam,LPARAM lParam)
{
    HINSTANCE hInst;

    switch(msg){
    case WM_INITDIALOG:
        // set icon for system modal message boxes
        hInst = (HINSTANCE)GetModuleHandle(NULL);
        if(hInst){
            HICON hIcon = LoadIcon(hInst,wxT("appicon"));
            if(hIcon) (void)SetClassLongPtr( \
                hWnd,GCLP_HICON,(LONG_PTR)hIcon);
        }
        // kill our window before showing it :)
        (void)EndDialog(hWnd,1);
        return FALSE;
    case WM_CLOSE:
        // handle it too, just for safety
        (void)EndDialog(hWnd,1);
        return TRUE;
    }
    return FALSE;
}

#if !defined(__GNUC__)
static int out_of_memory_handler(size_t n)
{
    int choice = MessageBox(
        g_mainFrame ? (HWND)g_mainFrame->GetHandle() : NULL,
        wxT("Try to release some memory by closing\n")
        wxT("other applications and click Retry then\n")
        wxT("or click Cancel to terminate the program."),
        wxT("UltraDefrag: out of memory!"),
        MB_RETRYCANCEL | MB_ICONHAND | MB_SYSTEMMODAL);
    if(choice == IDCANCEL){
        winx_flush_dbg_log(FLUSH_IN_OUT_OF_MEMORY);
        if(g_mainFrame) // remove system tray icon
            delete g_mainFrame->m_systemTrayIcon;
        exit(3); return 0;
    }
    return 1;
}
#endif

/**
 * @brief Initializes the application.
 */
bool App::OnInit()
{
    m_mutex = NULL;
    
    // initialize wxWidgets
    SetAppName(wxT("UltraDefrag"));
    wxInitAllImageHandlers();
    if(!wxApp::OnInit())
        return false;

    // set icon for system modal message boxes
    (void)DialogBox(
        (HINSTANCE)GetModuleHandle(NULL),
        wxT("dummy_dialog"),NULL,
        (DLGPROC)DummyDlgProc
    );

    // initialize udefrag library
    if(::udefrag_init_library() < 0){
        wxLogError(wxT("Initialization failed!"));
        return false;
    }

    // set out of memory handler
#if !defined(__GNUC__)
    winx_set_killer(out_of_memory_handler);
    _set_new_handler(out_of_memory_handler);
    _set_new_mode(1);
#endif

    // enable crash handling
#ifdef ATTACH_DEBUGGER
    AttachDebugger();
#endif

    // uncomment to test out of memory condition
    /*for(int i = 0; i < 1000000000; i++)
        char *p = new char[1024];*/

#ifdef CRASH_TESTS
#ifndef _WIN64
    wchar_t *s1 = new wchar_t[1024];
    wcscpy(s1,wxT("hello"));
    delete s1;
    wcscpy(s1,wxT("world"));
    delete s1;
#else
    // the code above fails to crash
    // on Windows XP 64-bit edition
    void *p = NULL;
    *(char *)p = 0;
#endif
#endif

    // initialize debug log
    wxFileName logpath(wxT(".\\logs\\ultradefrag.log"));
    logpath.Normalize();
    m_logPath = new wxString(logpath.GetFullPath());
    wxSetEnv(wxT("UD_LOG_FILE_PATH"),*m_logPath);
    ::udefrag_set_log_file_path();

    // initialize logging
    m_log = new Log();

    // forbid installation / upgrade while UltraDefrag is running
    m_mutex = CreateMutex(NULL,FALSE,L"Global\\ultradefrag_mutex");
    if(m_mutex == NULL) letrace("cannot create the mutex");
    
    // use global config object for internal settings
    wxFileConfig *cfg = new wxFileConfig(wxT(""),wxT(""),
        wxT("gui.ini"),wxT(""),wxCONFIG_USE_RELATIVE_PATH);
    wxConfigBase::Set(cfg);

    // enable i18n support
    InitLocale();

    // save report translation on setup
    wxString cmdLine(GetCommandLine());
    if(cmdLine.Find(wxT("--setup")) != wxNOT_FOUND){
        SaveReportTranslation();
        ::winx_flush_dbg_log(0);
        delete m_log;
        return false;
    }

    // start web statistics
    m_statThread = new StatThread();

    // check for administrative rights
    if(!Utils::CheckAdminRights()){
        wxMessageDialog dlg(NULL,
            wxT("Administrative rights are needed to run the program!"),
            wxT("UltraDefrag"),wxOK | wxICON_ERROR | wxCENTRE
        );
        dlg.ShowModal(); Cleanup();
        return false;
    }

    // create synchronization event
    g_synchEvent = ::CreateEvent(NULL,TRUE,FALSE,NULL);
    if(!g_synchEvent){
        letrace("cannot create synchronization event");
        wxMessageDialog dlg(NULL,
            wxT("Cannot create synchronization event!"),
            wxT("UltraDefrag"),wxOK | wxICON_ERROR | wxCENTRE
        );
        dlg.ShowModal(); Cleanup();
        return false;
    }

    // keep things DPI-aware
    HDC hdc = ::GetDC(NULL);
    if(hdc){
        g_scaleFactor = (double)::GetDeviceCaps(hdc,LOGPIXELSX) / 96.0f;
        ::ReleaseDC(NULL,hdc);
    }
    g_iconSize = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X);
    if(g_iconSize < 20) g_iconSize = 16;
    else if(g_iconSize < 24) g_iconSize = 20;
    else if(g_iconSize < 32) g_iconSize = 24;
    else g_iconSize = 32;

    // support taskbar icon overlay setup on shell restart
    g_TaskbarIconMsg = ::RegisterWindowMessage(wxT("TaskbarButtonCreated"));
    if(!g_TaskbarIconMsg) letrace("cannot register TaskbarButtonCreated message");

    // create main window
    g_mainFrame = new MainFrame();
    g_mainFrame->Show(true);
    SetTopWindow(g_mainFrame);
    return true;
}

/**
 * @brief Frees application resources.
 */
void App::Cleanup()
{
    // flush configuration to disk
    delete wxConfigBase::Set(NULL);

    // stop web statistics
    delete m_statThread;

    // deinitialize logging
    ::winx_flush_dbg_log(0);
    delete m_logPath;
    delete m_log;
    
    // release resources
    if(m_mutex) CloseHandle(m_mutex);
}

/**
 * @brief Deinitializes the application.
 */
int App::OnExit()
{
    Cleanup();
    return wxApp::OnExit();
}

IMPLEMENT_APP(App)

// =======================================================================
//                             Main window
// =======================================================================

/**
 * @brief Initializes main window.
 */
MainFrame::MainFrame()
    :wxFrame(NULL,wxID_ANY,wxT("UltraDefrag"))
{
    g_mainFrame = this;
    m_vList = NULL;
    m_cMap = NULL;
    m_currentJob = NULL;
    m_busy = false;
    m_paused = false;
    m_sizeAdjustmentEnabled = false;
    m_upgradeAvailable = false;
    m_upgradeLinkOpened = false;
    m_upgradeOfferId = 1;

    // set main window icon
    wxIconBundle icons;
    int sizes[] = {16,20,22,24,26,32,40,48,64};
    for(int i = 0; i < sizeof(sizes) / sizeof(int); i++){
        icons.AddIcon(wxIcon(wxT("appicon"),
            wxBITMAP_TYPE_ICO_RESOURCE,
            sizes[i],sizes[i])
        );
    }
    SetIcons(icons);

    // read configuration
    ReadAppConfiguration();
    ProcessCommandEvent(this,ID_ReadUserPreferences);

    // set main window title
    wxString instdir;
    if(wxGetEnv(wxT("UD_INSTALL_DIR"),&instdir)){
        wxFileName path(wxGetCwd()); path.Normalize();
        wxString cd = path.GetFullPath();
        itrace("current directory: %ls",ws(cd));
        itrace("installation directory: %ls",ws(instdir));
        if(cd.CmpNoCase(instdir) == 0){
            itrace("current directory matches "
                "installation location, so it isn't portable");
            m_title = new wxString(wxT(VERSIONINTITLE));
        } else {
            itrace("current directory differs from "
                "installation location, so it is portable");
            m_title = new wxString(wxT(VERSIONINTITLE_PORTABLE));
        }
    } else {
        m_title = new wxString(wxT(VERSIONINTITLE_PORTABLE));
    }
    ProcessCommandEvent(this,ID_SetWindowTitle);

    // set main window size and position
    SetSize(m_width,m_height);
    if(!m_saved){
        CenterOnScreen();
        GetPosition(&m_x,&m_y);
    }
    Move(m_x,m_y);
    if(m_maximized) Maximize(true);

    // create menu, tool and status bars
    InitMenu(); InitToolbar(); InitStatusBar();

    // create list of volumes and cluster map; don't use
    // live update style to avoid horizontal scrollbar
    // appearance on list resizing; use wxSP_NO_XP_THEME
    // to draw borders correctly on Windows XP theme
    m_splitter = new wxSplitterWindow(this,wxID_ANY,
        wxDefaultPosition,wxDefaultSize,wxSP_3D |
        wxSP_NO_XP_THEME/* | wxSP_LIVE_UPDATE*/ |
        wxCLIP_CHILDREN
    );
    m_splitter->SetMinimumPaneSize(DPI(MIN_PANEL_HEIGHT));

    m_vList = new DrivesList(m_splitter,wxLC_REPORT | \
        wxLC_NO_SORT_HEADER | wxLC_HRULES | wxLC_VRULES | wxBORDER_NONE);
    //LONG_PTR style = ::GetWindowLongPtr((HWND)m_vList->GetHandle(),GWL_STYLE);
    //style |= LVS_SHOWSELALWAYS; ::SetWindowLongPtr((HWND)m_vList->GetHandle(),GWL_STYLE,style);

    m_cMap = new ClusterMap(m_splitter);

    m_splitter->SplitHorizontally(m_vList,m_cMap);

    int height = GetClientSize().GetHeight();
    int maxPanelHeight = height - DPI(MIN_PANEL_HEIGHT) - m_splitter->GetSashSize();
    if(m_separatorPosition < DPI(MIN_PANEL_HEIGHT)) m_separatorPosition = DPI(MIN_PANEL_HEIGHT);
    else if(m_separatorPosition > maxPanelHeight) m_separatorPosition = maxPanelHeight;
    m_splitter->SetSashPosition(m_separatorPosition);

    // update frame layout so we'll be able
    // to initialize list of volumes and
    // cluster map properly
    wxSizeEvent evt(wxSize(m_width,m_height));
    GetEventHandler()->ProcessEvent(evt);
    m_splitter->UpdateSize();

    InitVolList();
    m_vList->SetFocus();

    // finally we have all map dimensions
    // set, so it's time to adjust frame
    // size accordingly to avoid gaps
    // between map cells and map borders
    m_sizeAdjustmentEnabled = true;
    ProcessCommandEvent(this,ID_AdjustMinFrameSize);
    evt.SetSize(wxSize(m_width,m_height));
    GetEventHandler()->ProcessEvent(evt);

    // populate list of volumes
    m_listThread = new ListThread();

    // check the boot time defragmenter presence
    wxFileName btdFile(wxT("%SystemRoot%\\system32\\defrag_native.exe"));
    btdFile.Normalize();
    bool btd = btdFile.FileExists();
    m_menuBar->Enable(ID_BootEnable,btd);
    m_menuBar->Enable(ID_BootScript,btd);
    m_toolBar->EnableTool(ID_BootEnable,btd);
    m_toolBar->EnableTool(ID_BootScript,btd);
    if(btd && ::winx_bootex_check(L"defrag_native") > 0){
        m_menuBar->Check(ID_BootEnable,true);
        m_toolBar->ToggleTool(ID_BootEnable,true);
        m_btdEnabled = true;
    } else {
        m_btdEnabled = false;
    }

    // launch threads for time consuming operations
    m_btdThread = btd ? new BtdThread() : NULL;
    m_configThread = new ConfigThread();

    wxConfigBase *cfg = wxConfigBase::Get();
    int ulevel = (int)cfg->Read(wxT("/Upgrade/Level"),1);
    wxMenuItem *item = m_menuBar->FindItem(ID_HelpUpgradeNone + ulevel);
    if(item) item->Check();

    m_upgradeThread = new UpgradeThread(ulevel);

    m_rdiThread = new RefreshDrivesInfoThread();

    // set system tray icon
    m_systemTrayIcon = new SystemTrayIcon();
    if(!m_systemTrayIcon->IsOk()){
        etrace("system tray icon initialization failed");
        wxSetEnv(wxT("UD_MINIMIZE_TO_SYSTEM_TRAY"),wxT("0"));
    }
    ProcessCommandEvent(this,ID_AdjustSystemTrayIcon);

    // set localized text
    ProcessCommandEvent(this,ID_LocaleChange \
        + g_locale->GetLanguage());

    // allow disk processing
    m_jobThread = new JobThread();
}

/**
 * @brief Deinitializes main window.
 */
MainFrame::~MainFrame()
{
    // show upgrade notification
    if(m_upgradeAvailable){
        ProcessCommandEvent(this,ID_ShowUpgradeDialog);
    }
    
    // terminate threads
    ProcessCommandEvent(this,ID_Stop);
    ::SetEvent(g_synchEvent);
    delete m_btdThread;
    delete m_configThread;
    delete m_jobThread;
    delete m_listThread;
    delete m_rdiThread;

    // save configuration
    SaveAppConfiguration();
    delete m_upgradeThread;

    // remove system tray icon
    delete m_systemTrayIcon;

    // free resources
    ::CloseHandle(g_synchEvent);
    delete m_title;
}

/**
 * @brief Returns true if the program
 * is going to be terminated.
 * @param[in] time timeout interval,
 * in milliseconds.
 */
bool MainFrame::CheckForTermination(int time)
{
    DWORD result = ::WaitForSingleObject(g_synchEvent,(DWORD)time);
    if(result == WAIT_FAILED){
        letrace("synchronization failed");
        return true;
    }
    return result == WAIT_OBJECT_0 ? true : false;
}

// =======================================================================
//                             Event table
// =======================================================================

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    // action menu
    EVT_MENU_RANGE(ID_Analyze, ID_MftOpt,
                   MainFrame::OnStartJob)
    EVT_MENU(ID_Pause, MainFrame::OnPause)
    EVT_MENU(ID_Stop,  MainFrame::OnStop)

    EVT_MENU(ID_ShowReport, MainFrame::OnShowReport)

    EVT_MENU(ID_SkipRem, MainFrame::OnSkipRem)
    EVT_MENU(ID_Rescan,  MainFrame::OnRescan)

    EVT_MENU(ID_Repair,  MainFrame::OnRepair)

    EVT_MENU(ID_Exit, MainFrame::OnExit)

    // settings menu
    EVT_MENU_RANGE(ID_LocaleChange, ID_LocaleChange \
        + wxUD_LANGUAGE_LAST, MainFrame::OnLocaleChange)

    EVT_MENU(ID_GuiOptions, MainFrame::OnGuiOptions)

    EVT_MENU(ID_BootEnable, MainFrame::OnBootEnable)
    EVT_MENU(ID_BootScript, MainFrame::OnBootScript)

    // help menu
    EVT_MENU(ID_HelpContents,     MainFrame::OnHelpContents)
    EVT_MENU(ID_HelpBestPractice, MainFrame::OnHelpBestPractice)
    EVT_MENU(ID_HelpFaq,          MainFrame::OnHelpFaq)
    EVT_MENU(ID_HelpLegend,       MainFrame::OnHelpLegend)

    EVT_MENU(ID_DebugLog,  MainFrame::OnDebugLog)
    EVT_MENU(ID_DebugSend, MainFrame::OnDebugSend)

    EVT_MENU_RANGE(ID_HelpUpgradeNone,
                   ID_HelpUpgradeCheck,
                   MainFrame::OnHelpUpgrade)
    EVT_MENU(ID_HelpAbout, MainFrame::OnHelpAbout)

    // event handlers
    EVT_ACTIVATE(MainFrame::OnActivate)
    EVT_MOVE(MainFrame::OnMove)
    EVT_SIZE(MainFrame::OnSize)

    EVT_MENU(ID_AdjustFrameSize,   MainFrame::AdjustFrameSize)
    EVT_MENU(ID_AdjustListColumns, MainFrame::AdjustListColumns)
    EVT_MENU(ID_AdjustListHeight,  MainFrame::AdjustListHeight)
    EVT_MENU(ID_AdjustMinFrameSize,MainFrame::AdjustMinFrameSize)
    EVT_MENU(ID_AdjustSystemTrayIcon,     MainFrame::AdjustSystemTrayIcon)
    EVT_MENU(ID_AdjustTaskbarIconOverlay, MainFrame::AdjustTaskbarIconOverlay)
    EVT_MENU(ID_BootChange,        MainFrame::OnBootChange)
    EVT_MENU(ID_CacheJob,          MainFrame::CacheJob)
    EVT_MENU(ID_DefaultAction,     MainFrame::OnDefaultAction)
    EVT_MENU(ID_DiskProcessingFailure, MainFrame::OnDiskProcessingFailure)
    EVT_MENU(ID_JobCompletion,     MainFrame::OnJobCompletion)
    EVT_MENU(ID_PopulateList,      MainFrame::PopulateList)
    EVT_MENU(ID_ReadUserPreferences,   MainFrame::ReadUserPreferences)
    EVT_MENU(ID_RedrawMap,         MainFrame::RedrawMap)
    EVT_MENU(ID_RefreshDrivesInfo, MainFrame::RefreshDrivesInfo)
    EVT_MENU(ID_RefreshFrame,      MainFrame::RefreshFrame)
    EVT_MENU(ID_SelectAll,         MainFrame::SelectAll)
    EVT_MENU(ID_SetWindowTitle,    MainFrame::SetWindowTitle)
    EVT_MENU(ID_ShowUpgradeDialog, MainFrame::ShowUpgradeDialog)
    EVT_MENU(ID_Shutdown,          MainFrame::Shutdown)
    EVT_MENU(ID_UpdateStatusBar,   MainFrame::UpdateStatusBar)
    EVT_MENU(ID_UpdateVolumeInformation, MainFrame::UpdateVolumeInformation)
    EVT_MENU(ID_UpdateVolumeStatus,      MainFrame::UpdateVolumeStatus)
END_EVENT_TABLE()

// =======================================================================
//                            Event handlers
// =======================================================================

WXLRESULT MainFrame::MSWWindowProc(WXUINT msg,WXWPARAM wParam,WXLPARAM lParam)
{
    if(msg == g_TaskbarIconMsg){
        // handle shell restart
        QueueCommandEvent(this,ID_AdjustTaskbarIconOverlay);
        return 0;
    }
    return wxFrame::MSWWindowProc(msg,wParam,lParam);
}

void MainFrame::SetWindowTitle(wxCommandEvent& event)
{
    if(event.GetString().IsEmpty()){
        if(CheckOption(wxT("UD_DRY_RUN"))){
            SetTitle(*m_title + wxT(" (dry run)"));
        } else {
            SetTitle(*m_title);
        }
    } else {
        SetTitle(event.GetString());
    }
}

void MainFrame::OnActivate(wxActivateEvent& event)
{
    /* suggested by Brian Gaff */
    if(event.GetActive() && m_vList)
        m_vList->SetFocus();
    event.Skip();
}

void MainFrame::OnMove(wxMoveEvent& event)
{
    if(!IsMaximized() && !IsIconized()){
        GetPosition(&m_x,&m_y);
        GetSize(&m_width,&m_height);
    }

    // hide window on minimization if system tray icon is turned on
    if(CheckOption(wxT("UD_MINIMIZE_TO_SYSTEM_TRAY")) && IsIconized()) Hide();

    event.Skip();
}

void MainFrame::AdjustMinFrameSize(wxCommandEvent& WXUNUSED(event))
{
    int min_width = DPI(MAIN_WINDOW_MIN_WIDTH);
    int min_height = DPI(MAIN_WINDOW_MIN_HEIGHT);

    int map_width, map_height;
    m_cMap->GetClientSize(&map_width,&map_height);
    int block_size = CheckOption(wxT("UD_MAP_BLOCK_SIZE"));
    int line_width = CheckOption(wxT("UD_GRID_LINE_WIDTH"));
    int cell_size = block_size + line_width;

    if(cell_size > 1){
        int dx = (min_width - (m_width - map_width) - line_width) % cell_size;
        int dy = (min_height - (m_height - map_height) - line_width) % cell_size;
        if(dx) min_width += cell_size - dx; if(dy) min_height += cell_size - dy;
    }
    SetMinSize(wxSize(min_width,min_height));
}

void MainFrame::AdjustFrameSize(wxCommandEvent& event)
{
    if(IsMaximized() || IsIconized()){
        m_sizeAdjustmentEnabled = true;
        return;
    }

    int block_size = CheckOption(wxT("UD_MAP_BLOCK_SIZE"));
    int line_width = CheckOption(wxT("UD_GRID_LINE_WIDTH"));
    int cell_size = block_size + line_width;
    if(cell_size < 2){ m_sizeAdjustmentEnabled = true; return; }

    bool resize_required = false;

    int map_width, map_height, dx, dy, flags;
    m_cMap->GetClientSize(&map_width,&map_height);
    dx = (map_width - line_width) % cell_size;
    dy = (map_height - line_width) % cell_size;
    flags = event.GetInt();
    if(dx){
        if(flags & FRAME_WIDTH_INCREASED)
            m_width += cell_size - dx;
        else
            m_width -= dx;
        resize_required = true;
    }
    if(dy){
        if(flags & FRAME_HEIGHT_INCREASED)
            m_height += cell_size - dy;
        else
            m_height -= dy;
        resize_required = true;
    }

    int min_width = GetMinWidth();
    int min_height = GetMinHeight();
    while(m_width < min_width){
        m_width += cell_size;
        resize_required = true;
    }
    while(m_height < min_height){
        m_height += cell_size;
        resize_required = true;
    }

    if(resize_required){
        SetSize(m_width,m_height);
        ProcessCommandEvent(this,ID_RefreshFrame);
    }

    m_sizeAdjustmentEnabled = true;
}

void MainFrame::OnSize(wxSizeEvent& event)
{
    if(!IsMaximized() && !IsIconized()){
        int old_width = m_width;
        int old_height = m_height;
        GetSize(&m_width,&m_height);
        if(m_sizeAdjustmentEnabled){
            wxCommandEvent *event = new wxCommandEvent(
                wxEVT_COMMAND_MENU_SELECTED,ID_AdjustFrameSize
            );
            int flags = 0;
            if(m_width > old_width) flags |= FRAME_WIDTH_INCREASED;
            if(m_height > old_height) flags |= FRAME_HEIGHT_INCREASED;
            event->SetInt(flags); GetEventHandler()->QueueEvent(event);
            m_sizeAdjustmentEnabled = false;
        }
    }
    if(m_cMap) m_cMap->Refresh();
    event.Skip();
}

// =======================================================================
//                            Menu handlers
// =======================================================================

void MainFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

// help menu handlers
void MainFrame::OnHelpContents(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("index.html"));
}

void MainFrame::OnHelpBestPractice(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("Tips.html"));
}

void MainFrame::OnHelpFaq(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("FAQ.html"));
}

void MainFrame::OnHelpLegend(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("GUI.html"),wxT("cluster_map_legend"));
}

/** @} */
