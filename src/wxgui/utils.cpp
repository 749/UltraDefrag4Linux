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
 * @file utils.cpp
 * @brief Auxiliary utilities.
 * @addtogroup Utils
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
//                         Auxiliary utilities
// =======================================================================

/**
 * @brief Defines whether the user
 * has administrative rights or not.
 */
bool Utils::CheckAdminRights(void)
{
    PSID psid = NULL;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = {SECURITY_NT_AUTHORITY};
    if(!::AllocateAndInitializeSid(&SystemSidAuthority,2,
      SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,
      0,0,0,0,0,0,&psid)){
        letrace("cannot create the security identifier");
        return false;
    }

    BOOL is_member = false;
    if(!::CheckTokenMembership(NULL,psid,&is_member)){
        letrace("cannot check token membership");
        if(psid) ::FreeSid(psid);
        return false;
    }

    if(!is_member) itrace("the user is not a member of administrators group");
    if(psid) ::FreeSid(psid);
    return (is_member == 0 ? false : true);
}

/**
 * @brief Downloads a file from the web.
 * @return Path to the downloaded file.
 * @note If the program terminates before
 * the file download completion it crashes.
 */
bool Utils::DownloadFile(const wxString& url, const wxString& path)
{
    itrace("downloading %ls",ws(url));

    /*
    * URLDownloadToCacheFileW cannot be used
    * here because it may immediately delete
    * the file after its creation.
    */
    HRESULT result = ::URLDownloadToFileW(
        NULL,ws(url),ws(path),0,NULL);
    if(result != S_OK){
        etrace("URLDownloadToFile failed "
            "with code 0x%x",(UINT)result);
        return false;
    }

    return true;
}

/**
 * @brief Sends a request to Google Analytics
 * service gathering statistics of the use
 * of the program.
 * @details Based on http://code.google.com/apis/analytics/docs/
 * and http://www.vdgraaf.info/google-analytics-without-javascript.html
 */
void Utils::GaRequest(const wxString& path, const wxString& id)
{
    srand((unsigned int)time(NULL));
    int utmn = (rand() << 16) + rand();
    int utmhid = (rand() << 16) + rand();
    int cookie = (rand() << 16) + rand();
    int random = (rand() << 16) + rand();
    __int64 today = (__int64)time(NULL);

    wxString url;
    url << wxT("http://www.google-analytics.com/__utm.gif?utmwv=4.6.5");
    url << wxString::Format(wxT("&utmn=%u"),utmn);
    url << wxT("&utmhn=ultradefrag.sourceforge.net");
    url << wxString::Format(wxT("&utmhid=%u&utmr=-"),utmhid);
    url << wxT("&utmp=") << path;
    url << wxT("&utmac=") << id;
    url << wxString::Format(wxT("&utmcc=__utma%%3D%u.%u.%I64u.%I64u.%I64u.") \
        wxT("50%%3B%%2B__utmz%%3D%u.%I64u.27.2.utmcsr%%3Dgoogle.com%%7Cutmccn%%3D") \
        wxT("(referral)%%7Cutmcmd%%3Dreferral%%7Cutmcct%%3D%%2F%%3B"),
        cookie,random,today,today,today,cookie,today);

    itrace("downloading %ls",ws(url));

    wchar_t file[MAX_PATH + 1]; file[MAX_PATH] = 0;
    HRESULT result = ::URLDownloadToCacheFileW(
        NULL,ws(url),file,MAX_PATH,0,NULL);
    if(result != S_OK){
        etrace("URLDownloadToCacheFile failed "
               "with code 0x%x",(UINT)result);
        return;
    }

    (void)::DeleteFile(file);
}

/**
 * @brief Creates a bitmap
 * from a png resource.
 */
wxBitmap Utils::LoadPngResource(const wchar_t *name)
{
    HRSRC resource = ::FindResource(NULL,name,RT_RCDATA);
    if(!resource){
        letrace("cannot find %ls resource",name);
        return NULL;
    }

    HGLOBAL handle = ::LoadResource(NULL,resource);
    if(!handle){
        letrace("cannot load %ls resource",name);
        return NULL;
    }

    char *data = (char *)::LockResource(handle);
    if(!data){
        letrace("cannot lock %ls resource",name);
        return NULL;
    }

    DWORD size = ::SizeofResource(NULL,resource);
    if(!size){
        letrace("cannot get size of %ls resource",name);
        return NULL;
    }

    wxMemoryInputStream is(data,size);
    return wxBitmap(wxImage(is,wxBITMAP_TYPE_PNG,-1),-1);
}

/**
 * @brief Opens the UltraDefrag Handbook,
 * either its local copy or from the web.
 */
void Utils::OpenHandbook(const wxString& page, const wxString& anchor)
{
    wxString path;
    path = wxT("./handbook/") + page;

    if(wxFileExists(path)){
        path = wxGetCwd();
        path.Replace(wxT("\\"),wxT("/"));
        if(!anchor.IsEmpty()){
            /*
            * wxLaunchDefaultBrowser
            * is unable to open a local
            * page with anchor appended.
            * So, we're making a redirector
            * and opening it instead.
            */
            wxString redirector(wxT("./handbook/"));
            redirector << page << wxT(".") << anchor << wxT(".html");
            if(!wxFileExists(redirector)){
                wxTextFile file;
                file.Create(redirector);
                file.AddLine(wxT("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">"));
                file.AddLine(wxT("<html><head><meta http-equiv=\"Refresh\" content=\"0; URL=") \
                    + page + wxT("#") + anchor + wxT("\">"));
                file.AddLine(wxT("</head><body>"));
                file.AddLine(wxT("Redirecting... if the page has not been redirected automatically click "));
                file.AddLine(wxT("<a href=\"") + page + wxT("#") + anchor + wxT("\">here</a>."));
                file.AddLine(wxT("</body></html>"));
                file.Write();
                file.Close();
            }
            path << wxT("/") << redirector;
        } else {
            path << wxT("/handbook/") << page;
        }
    } else {
        path = wxT("https://ultradefrag.net");
        path << wxT("/handbook/v7/") << page;
        if(!anchor.IsEmpty())
            path << wxT("#") << anchor;
    }

    itrace("%ls",ws(path));
    if(path.Left(4) == wxT("http")) {
        if(!wxLaunchDefaultBrowser(path))
            ShowError(wxT("Cannot open %ls!"),ws(path));
    } else {
        ShellExec(path,wxT("open"));
    }
}

/**
 * @brief Sets priority for the current process.
 * @param[in] priority process priority class.
 * Read MSDN article on SetPriorityClass for details.
 */
bool Utils::SetProcessPriority(int priority)
{
    HANDLE hProcess = ::OpenProcess(PROCESS_SET_INFORMATION,
        FALSE,::GetCurrentProcessId());
    if(!hProcess){
        letrace("cannot open current process");
        return false;
    }

    BOOL result = ::SetPriorityClass(hProcess,(DWORD)priority);
    if(!result) letrace("cannot set process priority");

    ::CloseHandle(hProcess);
    return (result != 0 ? true : false);
}

/**
 * @brief Windows ShellExecute analog.
 */
void Utils::ShellExec(
    const wxString& file,
    const wxString& action,
    const wxString& parameters,
    int show, int flags)
{
    SHELLEXECUTEINFO se;
    memset(&se,0,sizeof(se));
    se.cbSize = sizeof(se);

    se.fMask = SEE_MASK_FLAG_NO_UI;
    if(flags & SHELLEX_NOASYNC)
        se.fMask |= SEE_MASK_FLAG_DDEWAIT;

    wchar_t *action_string = winx_wcsdup(ws(action));
    wchar_t *file_string = winx_wcsdup(ws(file));
    wchar_t *param_string = parameters.IsEmpty() ? NULL :
        winx_wcsdup(ws(parameters));

    se.lpVerb = action_string;
    se.lpFile = file_string;
    se.lpParameters = param_string;
    se.nShow = show;

    if(!ShellExecuteEx(&se)){
        letrace("cannot %ls %ls %ls",
            ws(action), ws(file),
            ws(parameters));
        if(!(flags & SHELLEX_SILENT)){
            ShowError(wxT("Cannot %ls %ls %ls"),
                ws(action), ws(file),
                ws(parameters));
        }
    }

    winx_free(action_string);
    winx_free(file_string);
    winx_free(param_string);
}

/**
 * @brief wxMessageDialog analog,
 * but with custom button labels
 * and with ability to center dialog
 * over the parent window.
 * @param[in] parent the parent window.
 * @param[in] caption the dialog caption.
 * @param[in] icon one of the wxART_xxx constants.
 * @param[in] text1 the OK button label.
 * @param[in] text2 the Cancel button label.
 * @param[in] format the format specification.
 * @param[in] ... the parameters.
 */
int Utils::MessageDialog(wxFrame *parent,
    const wxString& caption, const wxArtID& icon,
    const wxString& text1, const wxString& text2,
    const wxChar *format, ...)
{
    wxChar *cmessage;
    wxString message;

    va_list args;
    va_start(args,format);

#if wxUSE_UNICODE
    cmessage = winx_vswprintf(format,args);
    if(cmessage) message << wxString::Format(wxT("%ls"),cmessage);
    else message << wxT("Not enough memory!");
#else
    cmessage = winx_vsprintf(format,args);
    if(cmessage) message << wxString::Format(wxT("%hs"),cmessage);
    else message << wxT("Not enough memory!");
#endif

    winx_free(cmessage);

    va_end(args);

    wxDialog dlg(parent,wxID_ANY,caption);

    // wxAppProvider performs double icon conversion:
    // once from icon to bitmap and then back to icon;
    // since it causes the icon to look untidy we're using
    // direct icon loading here
    LPCWSTR id = NULL;
    if(icon == wxART_QUESTION) id = IDI_QUESTION;
    else if(icon == wxART_WARNING) id = IDI_EXCLAMATION;
    else if(icon == wxART_ERROR) id = IDI_HAND;
    else if(icon == wxART_INFORMATION) id = IDI_ASTERISK;

    HICON hIcon = NULL;
    if(id){
        hIcon = ::LoadIcon(NULL,id);
        if(!hIcon) letrace("cannot load icon for \"%ls\"",ws(icon));
    }

    wxIcon messageIcon;
    if(hIcon){
        int size = ::GetSystemMetrics(SM_CXICON);
        messageIcon.SetSize(size, size);
        messageIcon.SetHICON((WXHICON)hIcon);
    } else {
        messageIcon = wxArtProvider::GetIcon(icon,wxART_MESSAGE_BOX);
    }

    wxStaticBitmap *pic = new wxStaticBitmap(&dlg,wxID_ANY,messageIcon);
    wxStaticText *msg = new wxStaticText(&dlg,wxID_ANY,message,
        wxDefaultPosition,wxDefaultSize,wxALIGN_CENTRE);

    wxGridBagSizer* contents = new wxGridBagSizer(0, 0);

    contents->Add(pic, wxGBPosition(0, 0), wxDefaultSpan,
        wxBOTTOM | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,
        LARGE_SPACING);
    contents->Add(msg, wxGBPosition(0, 1), wxDefaultSpan,
        (wxALL & ~wxTOP) | wxALIGN_CENTER_HORIZONTAL | \
        wxALIGN_CENTER_VERTICAL,LARGE_SPACING);

    wxButton *ok = new wxButton(&dlg,wxID_OK,text1);
    wxButton *cancel = new wxButton(&dlg,wxID_CANCEL,text2);

    // Burmese needs Padauk font for display
    if(g_locale->GetCanonicalName().Left(2) == wxT("my")){
        wxFont textFont = msg->GetFont();
        if(!textFont.SetFaceName(wxT("Padauk"))){
            etrace("Padauk font needed for correct Burmese text display not found");
        } else {
            textFont.SetPointSize(textFont.GetPointSize() + 2);
            msg->SetFont(textFont);
            ok->SetFont(textFont);
            cancel->SetFont(textFont);
        }
    }

    wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->Add(ok,wxSizerFlags(1));
    buttons->AddSpacer(LARGE_SPACING);
    buttons->Add(cancel,wxSizerFlags(1));

    contents->Add(buttons, wxGBPosition(1, 0), wxGBSpan(1, 2),
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);

    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->AddSpacer(LARGE_SPACING);
    hbox->Add(contents,wxSizerFlags());
    hbox->AddSpacer(LARGE_SPACING);

    wxBoxSizer *space = new wxBoxSizer(wxVERTICAL);
    space->AddSpacer(LARGE_SPACING);
    space->Add(hbox,wxSizerFlags());
    space->AddSpacer(LARGE_SPACING);

    dlg.SetSizerAndFit(space);
    space->SetSizeHints(&dlg);

    if(!parent->IsIconized()) dlg.Center();
    else dlg.CenterOnScreen();

    return dlg.ShowModal();
}

/**
 * @brief Shows an error and
 * invites to open log file.
 */
void Utils::ShowError(const wxChar *format, ...)
{
    wxChar *message;
    
    va_list args;
    va_start(args,format);

#if wxUSE_UNICODE
    message = winx_vswprintf(format,args);
#else
    message = winx_vsprintf(format,args);
#endif

    va_end(args);

    wxString log = _("Open &log");
    log.Replace(wxT("&"),wxT(""));

    if(MessageDialog(g_mainFrame,_("Error!"),
      wxART_ERROR,log,_("&Cancel"),message) == wxID_OK)
    {
        QueueCommandEvent(g_mainFrame,ID_DebugLog);
    }
    
    winx_free(message);
}

/** @} */
