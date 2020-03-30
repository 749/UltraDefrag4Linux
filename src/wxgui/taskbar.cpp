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
 * @file taskbar.cpp
 * @brief Taskbar icon.
 * @details Taskbar icon overlay and progress bar are
 * supported by Windows 7 and more recent Windows editions.
 * On older Windows editions these routines do nothing.
 * @addtogroup Taskbar
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

#define LPTHUMBBUTTON LPVOID

class ITaskbarList: public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE HrInit(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddTab(HWND hwnd) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteTab(HWND hwnd) = 0;
    virtual HRESULT STDMETHODCALLTYPE ActivateTab(HWND hwnd) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetActiveAlt(HWND hwnd) = 0;
};

class ITaskbarList2: public ITaskbarList {
public:
    virtual HRESULT STDMETHODCALLTYPE MarkFullscreenWindow(HWND hwnd,BOOL fFullscreen) = 0;
};

class ITaskbarList3: public ITaskbarList2 {
public:
    virtual HRESULT STDMETHODCALLTYPE SetProgressValue(HWND hwnd,ULONGLONG ullCompleted,ULONGLONG ullTotal) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProgressState(HWND hwnd,TBPFLAG tbpFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE RegisterTab(HWND hwndTab,HWND hwndMDI) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnregisterTab(HWND hwndTab) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTabOrder(HWND hwndTab,HWND hwndInsertBefore) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTabActive(HWND hwndTab,HWND hwndMDI,DWORD dwReserved) = 0;
    virtual HRESULT STDMETHODCALLTYPE ThumbBarAddButtons(HWND hwnd,UINT cButtons,LPTHUMBBUTTON pButton) = 0;
    virtual HRESULT STDMETHODCALLTYPE ThumbBarUpdateButtons(HWND hwnd,UINT cButtons,LPTHUMBBUTTON pButton) = 0;
    virtual HRESULT STDMETHODCALLTYPE ThumbBarSetImageList(HWND hwnd,HIMAGELIST himl) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetOverlayIcon(HWND hwnd,HICON hIcon,LPCWSTR pszDescription) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetThumbnailTooltip(HWND hwnd,LPCWSTR pszTip) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetThumbnailClip(HWND hwnd,RECT *prcClip) = 0;
};

#define CLSID_TaskbarList {0x56FDF344, 0xFD6D, 0x11d0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}}
#define IID_ITaskbarList3 {0xEA1AFB91, 0x9E28, 0x4B86, {0x90, 0xE9, 0x9E, 0x9F, 0x8A, 0x5E, 0xEF, 0xAF}}

// =======================================================================
//                         Auxiliary routines
// =======================================================================

static ITaskbarList3 *GetTaskbarInstance()
{
    if(winx_get_os_version() < WINDOWS_7)
        return NULL; // interface introduced in Win7

    ::CoInitialize(NULL);

    const GUID clsid = CLSID_TaskbarList;
    const GUID iid = IID_ITaskbarList3;
    ITaskbarList3 *taskBar = NULL;

    HRESULT result = ::CoCreateInstance(
        clsid,NULL,CLSCTX_INPROC_SERVER,
        iid,reinterpret_cast<void **>(&taskBar)
    );
    if(!SUCCEEDED(result) || !taskBar){
        etrace("failed with code 0x%x",(UINT)result);
        return NULL;
    }

    return taskBar;
}

static void ReleaseTaskbarInstance(ITaskbarList3 *taskBar)
{
    if(winx_get_os_version() >= WINDOWS_7){
        if(taskBar) taskBar->Release();
        ::CoUninitialize();
    }
}

// =======================================================================
//                        Taskbar icon overlay
// =======================================================================

void MainFrame::SetTaskbarIconOverlay(const wxString& icon, const wxString& text)
{
    if(!CheckOption(wxT("UD_SHOW_TASKBAR_ICON_OVERLAY"))) return;

    ITaskbarList3 *taskBar = GetTaskbarInstance();

    if(taskBar){
        wxIcon i(icon,wxBITMAP_TYPE_ICO_RESOURCE,g_iconSize,g_iconSize);
        HRESULT result = taskBar->SetOverlayIcon(
            (HWND)g_mainFrame->GetHandle(),
            (HICON)i.GetHICON(),ws(text)
        );
        if(!SUCCEEDED(result))
            etrace("failed with code 0x%x",(UINT)result);
    }

    ReleaseTaskbarInstance(taskBar);
}

void MainFrame::RemoveTaskbarIconOverlay()
{
    ITaskbarList3 *taskBar = GetTaskbarInstance();

    if(taskBar){
        HRESULT result = taskBar->SetOverlayIcon(
            (HWND)g_mainFrame->GetHandle(),NULL,NULL
        );
        if(!SUCCEEDED(result))
            etrace("failed with code 0x%x",(UINT)result);
    }

    ReleaseTaskbarInstance(taskBar);
}

// =======================================================================
//                     Taskbar progress indication
// =======================================================================

void MainFrame::SetTaskbarProgressState(TBPFLAG flag)
{
    ITaskbarList3 *taskBar = GetTaskbarInstance();

    if(taskBar){
        HRESULT result = taskBar->SetProgressState(
            (HWND)g_mainFrame->GetHandle(),flag
        );
        if(!SUCCEEDED(result))
            etrace("failed with code 0x%x",(UINT)result);
    }

    ReleaseTaskbarInstance(taskBar);
}

void MainFrame::SetTaskbarProgressValue(ULONGLONG completed, ULONGLONG total)
{
    ITaskbarList3 *taskBar = GetTaskbarInstance();

    if(taskBar){
        HRESULT result = taskBar->SetProgressValue(
            (HWND)g_mainFrame->GetHandle(),completed,total
        );
        if(!SUCCEEDED(result))
            etrace("failed with code 0x%x",(UINT)result);
    }

    ReleaseTaskbarInstance(taskBar);
}

// =======================================================================
//                           Event handlers
// =======================================================================

void MainFrame::AdjustTaskbarIconOverlay(wxCommandEvent& WXUNUSED(event))
{
    if(!CheckOption(wxT("UD_SHOW_TASKBAR_ICON_OVERLAY"))){
        RemoveTaskbarIconOverlay(); return;
    }

    if(m_busy){
        if(m_paused){
            SetTaskbarIconOverlay(
                wxT("overlay_paused"),
                _("The job is paused")
            );
        } else {
            SetTaskbarIconOverlay(
                wxT("overlay_running"),
                _("The job is running")
            );
        }
    } else {
        RemoveTaskbarIconOverlay();
    }
}

/** @} */
