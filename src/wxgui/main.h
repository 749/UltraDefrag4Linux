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

#ifndef _UDEFRAG_GUI_MAIN_H_
#define _UDEFRAG_GUI_MAIN_H_

// =======================================================================
//                               Headers
// =======================================================================

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/artprov.h>
#include <wx/cmdline.h>
#include <wx/confbase.h>
#include <wx/dir.h>
#include <wx/display.h>
#include <wx/dynlib.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/gbsizer.h>
#include <wx/hyperlink.h>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/mstream.h>
#include <wx/settings.h>
#include <wx/splitter.h>
#include <wx/sysopt.h>
#include <wx/taskbar.h>
#include <wx/textfile.h>
#include <wx/thread.h>
#include <wx/toolbar.h>
#include <wx/uri.h>

/*
* Next definition is very important for mingw:
* _WIN32_IE must be no less than 0x0400
* to include some important constant definitions.
*/
#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif
#include <commctrl.h>

typedef enum {
    TBPF_NOPROGRESS	= 0,
    TBPF_INDETERMINATE	= 0x1,
    TBPF_NORMAL	= 0x2,
    TBPF_ERROR	= 0x4,
    TBPF_PAUSED	= 0x8
} TBPFLAG;

#if defined(__GNUC__)
extern "C" {
HRESULT WINAPI URLDownloadToFileW(
    /* LPUNKNOWN */ void *lpUnkcaller,
    LPCWSTR szURL,
    LPCWSTR szFileName,
    DWORD dwReserved,
    /*IBindStatusCallback*/ void *pBSC
);

HRESULT WINAPI URLDownloadToCacheFileW(
    /* LPUNKNOWN */ void *lpUnkcaller,
    LPCWSTR szURL,
    LPWSTR szFileName,
    DWORD cchFileName,
    DWORD dwReserved,
    /*IBindStatusCallback*/ void *pBSC
);
}
#endif

#include "../include/dbg.h"
#include "../include/version.h"
#include "../dll/zenwinx/zenwinx.h"
#include "../dll/udefrag/udefrag.h"

// =======================================================================
//                              Constants
// =======================================================================

#define LIST_COLUMNS 6 // number of columns in the list of volumes

#define USAGE_TRACKING_ACCOUNT_ID wxT("UA-15890458-1")
#define TEST_TRACKING_ACCOUNT_ID  wxT("UA-70148850-1")

#ifndef _WIN64
  #define TRACKING_ID wxT("gui-x86")
#else
 #if defined(_IA64_)
  #define TRACKING_ID wxT("gui-ia64")
 #else
  #define TRACKING_ID wxT("gui-x64")
 #endif
#endif

// append html extension to the tracking id, for historical reasons
#define USAGE_TRACKING_PATH wxT("/appstat/") TRACKING_ID wxT(".html")

#define TEST_TRACKING_PATH \
    wxT("/") wxT(wxUD_ABOUT_VERSION) \
    wxT("/") TRACKING_ID wxT("/")

#define GA_REQUEST(type) ::Utils::GaRequest(type##_PATH, type##_ACCOUNT_ID)

enum {
    // file menu identifiers

    // NOTE: they share a single event handler
    ID_Analyze = 1,
    ID_Defrag,
    ID_QuickOpt,
    ID_FullOpt,
    ID_MftOpt,

    ID_Pause,
    ID_Stop,

    ID_ShowReport,

    ID_Repeat,

    ID_SkipRem,
    ID_Rescan,

    ID_Repair,

    ID_WhenDoneNone,
    ID_WhenDoneExit,
    ID_WhenDoneStandby,
    ID_WhenDoneHibernate,
    ID_WhenDoneLogoff,
    ID_WhenDoneReboot,
    ID_WhenDoneShutdown,

    ID_Exit,

    // settings menu identifiers
    ID_LangTranslateOnline,
    ID_LangTranslateOffline,
    ID_LangOpenFolder,

    ID_GuiOptions,

    ID_BootEnable,
    ID_BootScript,

    ID_SortByPath,
    ID_SortBySize,
    ID_SortByCreationDate,
    ID_SortByModificationDate,
    ID_SortByLastAccessDate,

    ID_SortAscending,
    ID_SortDescending,

    // help menu identifiers
    ID_HelpContents,
    ID_HelpBestPractice,
    ID_HelpFaq,
    ID_HelpLegend,

    ID_DebugLog,
    ID_DebugSend,

    // NOTE: they share a single event handler
    ID_HelpUpgradeNone,
    ID_HelpUpgradeStable,
    ID_HelpUpgradeAll,
    ID_HelpUpgradeCheck,

    ID_HelpAbout,

    // event identifiers
    ID_AdjustListColumns,
    ID_AdjustListHeight,
    ID_AdjustSystemTrayIcon,
    ID_AdjustTaskbarIconOverlay,
    ID_BootChange,
    ID_CacheJob,
    ID_DefaultAction,
    ID_DiskProcessingFailure,
    ID_JobCompletion,
    ID_PopulateList,
    ID_ReadUserPreferences,
    ID_RedrawMap,
    ID_SelectAll,
    ID_SetWindowTitle,
    ID_ShowUpgradeDialog,
    ID_Shutdown,
    ID_UpdateStatusBar,
    ID_UpdateVolumeInformation,
    ID_UpdateVolumeStatus,

    // tray icon menu identifiers
    ID_ShowHideMenu,
    ID_PauseMenu,
    ID_ExitMenu,

    // language selection menu item, must always be last in the list
    ID_LocaleChange
};

#define MAIN_WINDOW_DEFAULT_WIDTH  640
#define MAIN_WINDOW_DEFAULT_HEIGHT 480
#define MAIN_WINDOW_MIN_WIDTH      500
#define MAIN_WINDOW_MIN_HEIGHT     375
#define DEFAULT_LIST_HEIGHT        130
#define MIN_PANEL_HEIGHT            40

// dialog layout constants
#define SMALL_SPACING  DPI(5)
#define LARGE_SPACING  DPI(11)

#define DEFAULT_DRY_RUN          0
#define DEFAULT_FREE_COLOR_R   255
#define DEFAULT_FREE_COLOR_G   255
#define DEFAULT_FREE_COLOR_B   255
#define DEFAULT_GRID_COLOR_R     0
#define DEFAULT_GRID_COLOR_G     0
#define DEFAULT_GRID_COLOR_B     0
#define DEFAULT_GRID_LINE_WIDTH  1
#define DEFAULT_MAP_BLOCK_SIZE   4
#define DEFAULT_MINIMIZE_TO_SYSTEM_TRAY          0
#define DEFAULT_SECONDS_FOR_SHUTDOWN_REJECTION  60
#define DEFAULT_SHOW_MENU_ICONS                  1
#define DEFAULT_SHOW_PROGRESS_IN_TASKBAR         1
#define DEFAULT_SHOW_TASKBAR_ICON_OVERLAY        1

/* user defined language IDs
   Important: never change their order when adding new translations
   or the selection of the user will be broken */
enum {
    wxUD_LANGUAGE_ILOKO = wxLANGUAGE_USER_DEFINED+1,
    wxUD_LANGUAGE_KAPAMPANGAN,
    wxUD_LANGUAGE_NORWEGIAN,
    wxUD_LANGUAGE_WARAY_WARAY,
    wxUD_LANGUAGE_ACOLI,
    wxUD_LANGUAGE_SINHALA_SRI_LANKA,
    wxUD_LANGUAGE_SILESIAN,
    wxUD_LANGUAGE_BASQUE,
    wxUD_LANGUAGE_LAST          // must always be last in the list
};

// =======================================================================
//                          Macro definitions
// =======================================================================

/*
* Convert wxString to formats acceptable by vararg functions.
* NOTE: Use it to pass strings to functions only as returned
* objects may be temporary!
*/
#define ansi(s) ((const char *)s.mb_str())
#define ws(s) ((const wchar_t *)s.wc_str())

/* converts pixels from 96 DPI to the current one */
#define DPI(x) ((int)((double)x * g_scaleFactor))

/*
* Immediately processes a command event.
* NOTE: Call it from the main thread only!
*/
#define ProcessCommandEvent(window,id) { \
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,id); \
    window->GetEventHandler()->ProcessEvent(event); \
}

/*
* Safely delivers a command event to the event queue.
*/
#define QueueCommandEvent(window,id) { \
    window->GetEventHandler()->QueueEvent( \
        new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED,id) \
    ); \
}

// =======================================================================
//                            Declarations
// =======================================================================

class StatThread: public wxThread {
public:
    StatThread() : wxThread(wxTHREAD_JOINABLE) { Create(); Run(); }
    ~StatThread() { Wait(); }

    virtual void *Entry();
};

class Log: public wxLog {
public:
    Log()  { delete SetActiveTarget(this); };
    ~Log() { SetActiveTarget(NULL); };

    virtual void DoLogTextAtLevel(wxLogLevel level, const wxString& msg);
};

class App: public wxApp {
public:
    virtual bool OnInit();
    virtual int  OnExit();
    virtual void OnInitCmdLine(wxCmdLineParser& parser) {
        /* wxWidgets raises an "assert failure" message without them */
        parser.AddSwitch(wxT("s"),wxT("setup"),wxT("setup"));
        parser.AddSwitch(wxT("v"),wxT("verbose"),wxT("verbose"));
    }

    /* wxLANGUAGE_UNKNOWN forces to use the most suitable locale */
    static void InitLocale() { SetLocale(wxLANGUAGE_UNKNOWN); }
    static void SetLocale(int id);
    static void ResetLocale();
    static void SaveReportTranslation();

    static void AttachDebugger();

private:
    void Cleanup();

    Log *m_log;
    StatThread *m_statThread;
};

class BtdThread: public wxThread {
public:
    BtdThread() : wxThread(wxTHREAD_JOINABLE) { Create(); Run(); }
    ~BtdThread() { Wait(); }

    virtual void *Entry();
};

class ConfigThread: public wxThread {
public:
    ConfigThread() : wxThread(wxTHREAD_JOINABLE) { Create(); Run(); }
    ~ConfigThread() { Wait(); }

    virtual void *Entry();
};

class JobThread: public wxThread {
public:
    JobThread() : wxThread(wxTHREAD_JOINABLE) {
        m_launch = false; Create(); Run();
    }
    ~JobThread() { Wait(); }

    virtual void *Entry();

    bool m_launch;
    wxArrayString *m_volumes;
    udefrag_job_type m_jobType;
    int m_mapSize;

private:
    void ProcessVolume(int index);
    static void ProgressCallback(udefrag_progress_info *pi, void *p);
    static int Terminator(void *p);

    char m_letter;
};

class ListThread: public wxThread {
public:
    ListThread() : wxThread(wxTHREAD_JOINABLE) {
        m_rescan = true; Create(); Run();
    }
    ~ListThread() { Wait(); }

    virtual void *Entry();

    bool m_rescan;
};

class UpgradeThread: public wxThread {
public:
    UpgradeThread(int level) : wxThread(wxTHREAD_JOINABLE) {
        m_level = level; m_check = true; Create(); Run();
    }
    ~UpgradeThread() { Wait(); }

    virtual void *Entry();

    bool m_check;
    int m_level;

private:
    int ParseVersionString(const char *version);
};

class SystemTrayIcon: public wxTaskBarIcon {
public:
    virtual wxMenu *CreatePopupMenu();

    void OnMenuShowHide(wxCommandEvent& event);
    void OnMenuPause(wxCommandEvent& event);
    void OnMenuExit(wxCommandEvent& event);

    void OnLeftButtonUp(wxTaskBarIconEvent& event);

    DECLARE_EVENT_TABLE()
};

class DrivesList: public wxListView {
public:
    DrivesList(wxWindow* parent, long style)
      : wxListView(parent,wxID_ANY,
        wxDefaultPosition,wxDefaultSize,style) {}
    ~DrivesList() {}

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnMouse(wxMouseEvent& event);
    void OnSelectionChange(wxListEvent& event);

    DECLARE_EVENT_TABLE()
};

class ClusterMap: public wxWindow {
public:
    ClusterMap(wxWindow* parent);
    ~ClusterMap();

    void OnEraseBackground(wxEraseEvent& event);
    void OnPaint(wxPaintEvent& event);

private:
    char *ScaleMap(int scaled_size);

    int m_width;
    int m_height;
    HDC m_cacheDC;
    HBITMAP m_cacheBmp;

    HBRUSH m_brushes[SPACE_STATES];

    DECLARE_EVENT_TABLE()
};

typedef struct _JobsCacheEntry {
    udefrag_job_type jobType;
    udefrag_progress_info pi;
    char *clusterMap;
    bool stopped;
} JobsCacheEntry;

WX_DECLARE_HASH_MAP(int, JobsCacheEntry*, \
    wxIntegerHash, wxIntegerEqual, JobsCache);

class MainFrame: public wxFrame {
public:
    MainFrame();
    ~MainFrame();

    WXLRESULT MSWWindowProc(WXUINT msg,WXWPARAM wParam,WXLPARAM lParam);

    bool CheckForTermination(int time);

    // action menu handlers
    void OnStartJob(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);

    void OnShowReport(wxCommandEvent& event);

    void OnRepeat(wxCommandEvent& event);

    void OnSkipRem(wxCommandEvent& event);
    void OnRescan(wxCommandEvent& event);

    void OnRepair(wxCommandEvent& event);

    void OnExit(wxCommandEvent& event);

    // settings menu handlers
    void OnLangTranslateOnline(wxCommandEvent& event);
    void OnLangTranslateOffline(wxCommandEvent& event);
    void OnLangOpenFolder(wxCommandEvent& event);

    void OnGuiOptions(wxCommandEvent& event);

    void OnBootEnable(wxCommandEvent& event);
    void OnBootScript(wxCommandEvent& event);

    // help menu handlers
    void OnHelpContents(wxCommandEvent& event);
    void OnHelpBestPractice(wxCommandEvent& event);
    void OnHelpFaq(wxCommandEvent& event);
    void OnHelpLegend(wxCommandEvent& event);

    void OnDebugLog(wxCommandEvent& event);
    void OnDebugSend(wxCommandEvent& event);

    void OnHelpUpgrade(wxCommandEvent& event);
    void OnHelpAbout(wxCommandEvent& event);

    // event handlers
    void OnActivate(wxActivateEvent& event);
    void OnMove(wxMoveEvent& event);
    void OnSize(wxSizeEvent& event);

    void AdjustListColumns(wxCommandEvent& event);
    void AdjustListHeight(wxCommandEvent& event);
    void AdjustSystemTrayIcon(wxCommandEvent& event);
    void AdjustTaskbarIconOverlay(wxCommandEvent& event);
    void CacheJob(wxCommandEvent& event);
    void OnBootChange(wxCommandEvent& event);
    void OnDefaultAction(wxCommandEvent& event);
    void OnDiskProcessingFailure(wxCommandEvent& event);
    void OnJobCompletion(wxCommandEvent& event);
    void OnListSize(wxSizeEvent& event);
    void OnLocaleChange(wxCommandEvent& event);
    void OnSplitChanged(wxSplitterEvent& event);
    void PopulateList(wxCommandEvent& event);
    void ReadUserPreferences(wxCommandEvent& event);
    void RedrawMap(wxCommandEvent& event);
    void SelectAll(wxCommandEvent& event);
    void SetWindowTitle(wxCommandEvent& event);
    void ShowUpgradeDialog(wxCommandEvent& event);
    void Shutdown(wxCommandEvent& event);
    void UpdateStatusBar(wxCommandEvent& event);
    void UpdateVolumeInformation(wxCommandEvent& event);
    void UpdateVolumeStatus(wxCommandEvent& event);

    // common routines
    int  CheckOption(const wxString& name);
    void SetTaskbarProgressState(TBPFLAG flag);
    void SetTaskbarProgressValue(ULONGLONG completed, ULONGLONG total);

    bool m_repeat;
    bool m_skipRem;
    bool m_busy;
    bool m_paused;
    bool m_stopped;

    // overall progress counters
    int m_selected;
    int m_processed;

    SystemTrayIcon *m_systemTrayIcon;

    JobThread *m_jobThread;
    JobsCache m_jobsCache;
    JobsCacheEntry *m_currentJob;

private:
    void InitMenu();
    void InitToolbar();
    void InitStatusBar();
    void InitVolList();
    void ReadAppConfiguration();
    void ReadUserPreferences();
    void ReleasePause();
    void RemoveTaskbarIconOverlay();
    void SaveAppConfiguration();
    void SetPause();
    void SetTaskbarIconOverlay(
        const wxString& icon,
        const wxString& text);
    int ShowShutdownDialog(int action);

    int  m_x;
    int  m_y;
    int  m_width;
    int  m_height;
    bool m_saved;
    bool m_maximized;
    int  m_separatorPosition;

    // list column widths ratios:
    // 0.5 means that a column acquires
    // a half of the entire list width
    double m_r[LIST_COLUMNS];

    // list column widths:
    // used to check whether the user
    // has changed them or not
    int m_w[LIST_COLUMNS];

    // list height
    int m_vListHeight;

    wxFont *m_vListFont;

    wxString   *m_title;
    wxToolBar  *m_toolBar;
    wxMenuBar  *m_menuBar;
    wxMenuItem *m_subMenuWhenDone;
    wxMenuItem *m_subMenuLanguage;
    wxMenuItem *m_subMenuBootConfig;
    wxMenuItem *m_subMenuSortingConfig;
    wxMenuItem *m_subMenuDebug;
    wxMenuItem *m_subMenuUpgrade;
    wxMenu     *m_menuLanguage;

    wxSplitterWindow *m_splitter;
    DrivesList       *m_vList;
    ClusterMap       *m_cMap;

    wxBitmap m_repeatButtonBitmap;

    bool m_btdEnabled;
    BtdThread *m_btdThread;

    ConfigThread    *m_configThread;
    ListThread      *m_listThread;
    UpgradeThread   *m_upgradeThread;

    DECLARE_EVENT_TABLE()
};

class Utils {
public:
    static bool CheckAdminRights(void);
    static bool DownloadFile(const wxString& url, const wxString& path);
    static void GaRequest(const wxString& path, const wxString& id);
    static wxBitmap LoadPngResource(const wchar_t *name);
    static int MessageDialog(wxFrame *parent,
        const wxString& caption, const wxArtID& icon,
        const wxString& text1, const wxString& text2,
        const wxString& format, ...);
    static void OpenHandbook(const wxString& page,
        const wxString& anchor = wxEmptyString);
    static bool SetProcessPriority(int priority);
    static void ShellExec(const wxString& file,
        const wxString& action = wxT("open"),
        const wxString& parameters = wxEmptyString,
        int show = SW_SHOW, int flags = 0);
    static void ShowError(const wxString& format, ...);
};

/* flags for Utils::ShellExec */
#define SHELLEX_SILENT  0x1
#define SHELLEX_NOASYNC 0x2

// =======================================================================
//                           Global variables
// =======================================================================

extern MainFrame *g_mainFrame;
extern wxLocale *g_locale;
extern double g_scaleFactor;
extern int g_iconSize;
extern HANDLE g_synchEvent;

#endif /* _UDEFRAG_GUI_MAIN_H_ */
