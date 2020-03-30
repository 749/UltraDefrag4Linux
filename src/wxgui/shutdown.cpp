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
 * @file shutdown.cpp
 * @brief Shutdown.
 * @addtogroup Shutdown
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "prec.h"
#include "main.h"
#include <powrprof.h>

#ifndef SHTDN_REASON_MAJOR_OTHER
#define SHTDN_REASON_MAJOR_OTHER   0x00000000
#endif
#ifndef SHTDN_REASON_MINOR_OTHER
#define SHTDN_REASON_MINOR_OTHER   0x00000000
#endif
#ifndef SHTDN_REASON_FLAG_PLANNED
#define SHTDN_REASON_FLAG_PLANNED  0x80000000
#endif
#ifndef EWX_FORCEIFHUNG
#define EWX_FORCEIFHUNG     0x00000010
#endif

enum {
    WHEN_DONE_STANDBY,
    WHEN_DONE_HIBERNATE,
    WHEN_DONE_LOGOFF,
    WHEN_DONE_REBOOT,
    WHEN_DONE_SHUTDOWN
};

class ShutdownDialog: public wxDialog {
public:
    ShutdownDialog(wxWindow* parent,const wxString& title)
      : wxDialog(parent,wxID_ANY,title){ m_timer = new wxTimer(this,wxID_ANY); }
    ~ShutdownDialog() { m_timer->Stop(); delete m_timer; }

    void OnTimer(wxTimerEvent& event);

    wxTimer *m_timer;

    unsigned long seconds;
    wxString      ctext;
    wxStaticText *counter;

private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ShutdownDialog, wxDialog)
    EVT_TIMER(wxID_ANY, ShutdownDialog::OnTimer)
END_EVENT_TABLE()

// =======================================================================
//                      Shutdown confirmation dialog
// =======================================================================

void ShutdownDialog::OnTimer(wxTimerEvent& event)
{
    if(!seconds){
        EndModal(wxID_OK);
        return;
    }

    seconds --;
    counter->SetLabel(wxString::Format(ctext,seconds));
}

int MainFrame::ShowShutdownDialog(int action)
{
    ShutdownDialog dlg(this,_("Please confirm"));

    wxStaticBitmap *icon = new wxStaticBitmap(&dlg,wxID_ANY,wxICON(shutdown));

    wxString q;
    switch(action){
    case WHEN_DONE_HIBERNATE:
        q = _("Do you really want to hibernate when done?");
        //: This expands to "60 seconds until hibernation"
        //: Make sure that "%lu" is included in the translated string at the correct position
        dlg.ctext = _("%lu seconds until hibernation");
        break;
    case WHEN_DONE_LOGOFF:
        q = _("Do you really want to log off when done?");
        //: This expands to "60 seconds until logoff"
        //: Make sure that "%lu" is included in the translated string at the correct position
        dlg.ctext = _("%lu seconds until logoff");
        break;
    case WHEN_DONE_REBOOT:
        q = _("Do you really want to reboot when done?");
        //: This expands to "60 seconds until reboot"
        //: Make sure that "%lu" is included in the translated string at the correct position
        dlg.ctext = _("%lu seconds until reboot");
        break;
    default:
        q = _("Do you really want to shutdown when done?");
        //: This expands to "60 seconds until shutdown"
        //: Make sure that "%lu" is included in the translated string at the correct position
        dlg.ctext = _("%lu seconds until shutdown");
    }

    wxString s;
    wxGetEnv(wxT("UD_SECONDS_FOR_SHUTDOWN_REJECTION"),&s);
    if(!s.ToULong(&dlg.seconds))
        dlg.seconds = DEFAULT_SECONDS_FOR_SHUTDOWN_REJECTION;

    wxStaticText *question = new wxStaticText(&dlg,wxID_ANY,q);
    dlg.counter = new wxStaticText(&dlg,wxID_ANY,
        wxString::Format(dlg.ctext,dlg.seconds));

    wxButton *yes = new wxButton(&dlg,wxID_OK,_("&Yes"));
    wxButton *no = new wxButton(&dlg,wxID_CANCEL,_("&No"));

    // Burmese needs Padauk font for display
    if(g_locale->GetCanonicalName().Left(2) == wxT("my")){
        wxFont textFont = question->GetFont();
        if(!textFont.SetFaceName(wxT("Padauk"))){
            etrace("Padauk font needed for correct Burmese text display not found");
        } else {
            textFont.SetPointSize(textFont.GetPointSize() + 2);
            question->SetFont(textFont);
            dlg.counter->SetFont(textFont);
            yes->SetFont(textFont);
            no->SetFont(textFont);
        }
    }

    wxSizerFlags flags; flags.Center();

    wxBoxSizer *text = new wxBoxSizer(wxVERTICAL);
    text->Add(question,flags);
    text->Add(dlg.counter,flags);

    wxBoxSizer *top = new wxBoxSizer(wxHORIZONTAL);
    top->AddSpacer(LARGE_SPACING);
    top->Add(icon,flags);
    top->AddSpacer(LARGE_SPACING);
    top->Add(text,flags);
    top->AddSpacer(LARGE_SPACING);

    wxBoxSizer *bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->AddSpacer(LARGE_SPACING);
    bottom->Add(yes,wxSizerFlags(1).Center());
    bottom->AddSpacer(LARGE_SPACING);
    bottom->Add(no,wxSizerFlags(1).Center());
    bottom->AddSpacer(LARGE_SPACING);

    wxSizer *contents = new wxBoxSizer(wxVERTICAL);
    contents->AddSpacer(LARGE_SPACING);
    contents->Add(top,wxSizerFlags().Left());
    contents->AddSpacer(LARGE_SPACING);
    contents->Add(bottom,flags);
    contents->AddSpacer(LARGE_SPACING);
    dlg.SetSizerAndFit(contents);

    if(!IsIconized()) dlg.Center();
    else dlg.CenterOnScreen();

    no->SetFocus();

    dlg.m_timer->Start(1000);

    return dlg.ShowModal();
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::Shutdown(wxCommandEvent& WXUNUSED(event))
{
    int action = 0;

    if(m_menuBar->IsChecked(ID_WhenDoneNone)){
        // nothing to do
        return;
    } else if(m_menuBar->IsChecked(ID_WhenDoneExit)){
        // just close the window
        ProcessCommandEvent(this,ID_Exit);
        return;
    } else if(m_menuBar->IsChecked(ID_WhenDoneStandby)){
        action = WHEN_DONE_STANDBY;
    } else if(m_menuBar->IsChecked(ID_WhenDoneHibernate)){
        action = WHEN_DONE_HIBERNATE;
    } else if(m_menuBar->IsChecked(ID_WhenDoneLogoff)){
        action = WHEN_DONE_LOGOFF;
    } else if(m_menuBar->IsChecked(ID_WhenDoneReboot)){
        action = WHEN_DONE_REBOOT;
    } else if(m_menuBar->IsChecked(ID_WhenDoneShutdown)){
        action = WHEN_DONE_SHUTDOWN;
    }

    if(action != WHEN_DONE_STANDBY && \
      CheckOption(wxT("UD_SECONDS_FOR_SHUTDOWN_REJECTION"))){
        if(ShowShutdownDialog(action) != wxID_OK) return;
    }

    ::winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
    ::winx_flush_dbg_log(0);

    // close the window after the request completion
    QueueCommandEvent(this,ID_Exit);

    // There is an opinion that SetSuspendState call
    // is more reliable than SetSystemPowerState:
    // http://msdn.microsoft.com/en-us/library/aa373206%28VS.85%29.aspx
    if(action == WHEN_DONE_STANDBY){
        // suspend, request permission from apps and drivers
        if(!SetSuspendState(FALSE,FALSE,FALSE)){
            letrace("cannot suspend the system");
            Utils::ShowError(wxT("Cannot suspend the system!"));
        }
        return;
    }
    if(action == WHEN_DONE_HIBERNATE){
        // hibernate, request permission from apps and drivers
        if(!SetSuspendState(TRUE,FALSE,FALSE)){
            letrace("cannot hibernate the computer");
            Utils::ShowError(wxT("Cannot hibernate the computer!"));
        }
        return;
    }

    // Shutdown command works better on remote
    // computers since it shows no confirmation.
    wxFileName shutdown(wxT("%windir%\\system32\\shutdown.exe"));
    wxFileName shell(wxT("%windir%\\system32\\cmd.exe"));
    shutdown.Normalize(); shell.Normalize();

    if(action == WHEN_DONE_LOGOFF){
        if(shutdown.FileExists()){
            Utils::ShellExec(shell.GetFullPath(),wxT("open"),wxT("/K shutdown -l"));
        } else {
            if(!ExitWindowsEx(EWX_LOGOFF | EWX_FORCEIFHUNG,
              SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | \
              SHTDN_REASON_FLAG_PLANNED)){
                letrace("cannot log the user off");
                Utils::ShowError(wxT("Cannot log the user off!"));
            }
        }
    } else if(action == WHEN_DONE_REBOOT){
        if(shutdown.FileExists()){
            Utils::ShellExec(shell.GetFullPath(),wxT("open"),wxT("/K shutdown -r -t 0"));
        } else {
            if(!ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG,
              SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | \
              SHTDN_REASON_FLAG_PLANNED)){
                letrace("cannot reboot the computer");
                Utils::ShowError(wxT("Cannot reboot the computer!"));
            }
        }
    } else if(action == WHEN_DONE_SHUTDOWN){
        if(shutdown.FileExists()){
            Utils::ShellExec(shell.GetFullPath(),wxT("open"),wxT("/K shutdown -s -t 0"));
        } else {
            if(!ExitWindowsEx(EWX_POWEROFF | EWX_FORCEIFHUNG,
              SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | \
              SHTDN_REASON_FLAG_PLANNED)){
                letrace("cannot shut the computer down");
                Utils::ShowError(wxT("Cannot shut the computer down!"));
            }
        }
    }
}

/** @} */
