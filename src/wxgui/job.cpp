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
 * @file job.cpp
 * @brief Volume processing.
 * @addtogroup VolumeProcessing
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "prec.h"
#include "main.h"

#define UD_EnableTool(id) { \
    m_menuBar->Enable(id,true); \
    m_toolBar->EnableTool(id,true); \
}

#define UD_DisableTool(id) { \
    m_menuBar->Enable(id,false); \
    m_toolBar->EnableTool(id,false); \
}

// =======================================================================
//                              Jobs cache
// =======================================================================

void MainFrame::CacheJob(wxCommandEvent& event)
{
    int index = event.GetInt();
    JobsCacheEntry *cacheEntry = m_jobsCache[index];
    JobsCacheEntry *newEntry = (JobsCacheEntry *)event.GetClientData();

    if(!cacheEntry){
        m_jobsCache[index] = newEntry;
    } else {
        delete [] cacheEntry->clusterMap;
        memcpy(cacheEntry,newEntry,sizeof(JobsCacheEntry));
        delete newEntry;
    }

    m_currentJob = m_jobsCache[index];
}

// =======================================================================
//                          Job startup thread
// =======================================================================

void JobThread::ProgressCallback(udefrag_progress_info *pi, void *p)
{
    // update window title and tray icon tooltip
    char op = 'O';
    if(pi->current_operation == VOLUME_ANALYSIS) op = 'A';
    if(pi->current_operation == VOLUME_DEFRAGMENTATION) op = 'D';

    wxString title = wxString::Format(wxT("%c:  %c %6.2lf %%"),
        winx_toupper(g_mainFrame->m_jobThread->m_letter),op,pi->percentage
    );
    if(g_mainFrame->CheckOption(wxT("UD_DRY_RUN"))) title += wxT(" (dry run)");

    wxCommandEvent *event = new wxCommandEvent(
        wxEVT_COMMAND_MENU_SELECTED,ID_SetWindowTitle
    );
    event->SetString(title.c_str()); // make a deep copy
    g_mainFrame->GetEventHandler()->QueueEvent(event);

    event = new wxCommandEvent(
        wxEVT_COMMAND_MENU_SELECTED,ID_AdjustSystemTrayIcon
    );
    event->SetString(title.c_str()); // make another deep copy
    g_mainFrame->GetEventHandler()->QueueEvent(event);

    // set overall progress
    if(g_mainFrame->m_jobThread->m_jobType == ANALYSIS_JOB \
      || pi->current_operation != VOLUME_ANALYSIS)
    {
        if(g_mainFrame->CheckOption(wxT("UD_SHOW_PROGRESS_IN_TASKBAR"))){
            g_mainFrame->SetTaskbarProgressState(TBPF_NORMAL);
            if(pi->clusters_to_process){
                g_mainFrame->SetTaskbarProgressValue(
                    (pi->clusters_to_process / g_mainFrame->m_selected) * \
                    g_mainFrame->m_processed + \
                    pi->processed_clusters / g_mainFrame->m_selected,
                    pi->clusters_to_process
                );
            } else {
                g_mainFrame->SetTaskbarProgressValue(0,1);
            }
        } else {
            g_mainFrame->SetTaskbarProgressState(TBPF_NOPROGRESS);
        }
    }

    // save progress information to the jobs cache
    int letter = (int)(g_mainFrame->m_jobThread->m_letter);
    JobsCacheEntry *cacheEntry = new JobsCacheEntry;
    cacheEntry->jobType = g_mainFrame->m_jobThread->m_jobType;
    memcpy(&cacheEntry->pi,pi,sizeof(udefrag_progress_info));
    cacheEntry->clusterMap = new char[pi->cluster_map_size];
    if(pi->cluster_map_size){
        memcpy(cacheEntry->clusterMap,
            pi->cluster_map,
            pi->cluster_map_size
        );
    }
    cacheEntry->stopped = g_mainFrame->m_stopped;
    event = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED,ID_CacheJob);
    event->SetInt(letter); event->SetClientData((void *)cacheEntry);
    g_mainFrame->GetEventHandler()->QueueEvent(event);

    // update progress indicators
    event = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED,ID_UpdateVolumeStatus);
    event->SetInt(letter); g_mainFrame->GetEventHandler()->QueueEvent(event);
    QueueCommandEvent(g_mainFrame,ID_RedrawMap);
    QueueCommandEvent(g_mainFrame,ID_UpdateStatusBar);
}

int JobThread::Terminator(void *p)
{
    while(g_mainFrame->m_paused) ::Sleep(300);
    return g_mainFrame->m_stopped;
}

void JobThread::ProcessVolume(int index)
{
    // update volume capacity information
    wxCommandEvent *event = new wxCommandEvent(
        wxEVT_COMMAND_MENU_SELECTED,ID_UpdateVolumeInformation
    );
    event->SetInt((int)m_letter);
    g_mainFrame->GetEventHandler()->QueueEvent(event);

    // process volume
    int result = udefrag_validate_volume(m_letter,FALSE);
    if(result == 0){
        result = udefrag_start_job(m_letter,m_jobType,0,m_mapSize,
            reinterpret_cast<udefrag_progress_callback>(ProgressCallback),
            reinterpret_cast<udefrag_terminator>(Terminator),NULL
        );
    }

    if(result < 0 && !g_mainFrame->m_stopped){
        wxCommandEvent *event = new wxCommandEvent(
            wxEVT_COMMAND_MENU_SELECTED,ID_DiskProcessingFailure
        );
        event->SetInt(result);
        event->SetString(((*m_volumes)[index]).c_str()); // make a deep copy
        g_mainFrame->GetEventHandler()->QueueEvent(event);
    }

    // update volume dirty status
    event = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED,ID_UpdateVolumeInformation);
    event->SetInt((int)m_letter); g_mainFrame->GetEventHandler()->QueueEvent(event);
}

void *JobThread::Entry()
{
    while(!g_mainFrame->CheckForTermination(200)){
        if(m_launch){
            // do the job
            g_mainFrame->m_selected = (int)m_volumes->Count();
            g_mainFrame->m_processed = 0;

            for(int i = 0; i < g_mainFrame->m_selected; i++){
                if(g_mainFrame->m_stopped) break;

                m_letter = (char)((*m_volumes)[i][0]);
                ProcessVolume(i);

                /* advance overall progress to processed/selected */
                g_mainFrame->m_processed ++;
                if(g_mainFrame->CheckOption(wxT("UD_SHOW_PROGRESS_IN_TASKBAR"))){
                    g_mainFrame->SetTaskbarProgressState(TBPF_NORMAL);
                    g_mainFrame->SetTaskbarProgressValue(
                        g_mainFrame->m_processed, g_mainFrame->m_selected
                    );
                } else {
                    g_mainFrame->SetTaskbarProgressState(TBPF_NOPROGRESS);
                }
            }

            // complete the job
            QueueCommandEvent(g_mainFrame,ID_JobCompletion);
            delete m_volumes; m_launch = false;
        }
    }

    return NULL;
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnStartJob(wxCommandEvent& event)
{
    if(m_busy) return;

    // if nothing is selected in the list return
    m_jobThread->m_volumes = new wxArrayString;
    long i = m_vList->GetFirstSelected();
    while(i != -1){
        m_jobThread->m_volumes->Add(m_vList->GetItemText(i));
        i = m_vList->GetNextSelected(i);
    }
    if(!m_jobThread->m_volumes->Count()) return;

    // lock everything till the job completion
    m_busy = true; m_paused = false; m_stopped = false;
    UD_DisableTool(ID_Analyze);
    UD_DisableTool(ID_Defrag);
    UD_DisableTool(ID_QuickOpt);
    UD_DisableTool(ID_FullOpt);
    UD_DisableTool(ID_MftOpt);
    UD_DisableTool(ID_SkipRem);
    UD_DisableTool(ID_Rescan);
    UD_DisableTool(ID_Repair);
    UD_DisableTool(ID_ShowReport);
    m_subMenuSortingConfig->Enable(false);

    ReleasePause();

    ProcessCommandEvent(this,ID_AdjustSystemTrayIcon);
    ProcessCommandEvent(this,ID_AdjustTaskbarIconOverlay);
    /* set overall progress: normal 0% */
    if(CheckOption(wxT("UD_SHOW_PROGRESS_IN_TASKBAR"))){
        SetTaskbarProgressValue(0,1);
        SetTaskbarProgressState(TBPF_NORMAL);
    }

    // set sorting parameters
    if(m_menuBar->IsChecked(ID_SortByPath)){
        wxSetEnv(wxT("UD_SORTING"),wxT("path"));
    } else if(m_menuBar->IsChecked(ID_SortBySize)){
        wxSetEnv(wxT("UD_SORTING"),wxT("size"));
    } else if(m_menuBar->IsChecked(ID_SortByCreationDate)){
        wxSetEnv(wxT("UD_SORTING"),wxT("c_time"));
    } else if(m_menuBar->IsChecked(ID_SortByModificationDate)){
        wxSetEnv(wxT("UD_SORTING"),wxT("m_time"));
    } else if(m_menuBar->IsChecked(ID_SortByLastAccessDate)){
        wxSetEnv(wxT("UD_SORTING"),wxT("a_time"));
    }
    if(m_menuBar->IsChecked(ID_SortAscending)){
        wxSetEnv(wxT("UD_SORTING_ORDER"),wxT("asc"));
    } else {
        wxSetEnv(wxT("UD_SORTING_ORDER"),wxT("desc"));
    }

    // launch the job
    switch(event.GetId()){
    case ID_Analyze:
        m_jobThread->m_jobType = ANALYSIS_JOB;
        break;
    case ID_Defrag:
        m_jobThread->m_jobType = DEFRAGMENTATION_JOB;
        break;
    case ID_QuickOpt:
        m_jobThread->m_jobType = QUICK_OPTIMIZATION_JOB;
        break;
    case ID_FullOpt:
        m_jobThread->m_jobType = FULL_OPTIMIZATION_JOB;
        break;
    default:
        m_jobThread->m_jobType = MFT_OPTIMIZATION_JOB;
        break;
    }
    int width, height; m_cMap->GetClientSize(&width,&height);
    int block_size = CheckOption(wxT("UD_MAP_BLOCK_SIZE"));
    int line_width = CheckOption(wxT("UD_GRID_LINE_WIDTH"));
    int cell_size = block_size + line_width;
    int blocks_per_line = (width - line_width) / cell_size;
    int lines = (height - line_width) / cell_size;
    m_jobThread->m_mapSize = blocks_per_line * lines;
    m_jobThread->m_launch = true;
}

void MainFrame::OnJobCompletion(wxCommandEvent& WXUNUSED(event))
{
    // unlock everything after the job completion
    UD_EnableTool(ID_Analyze);
    UD_EnableTool(ID_Defrag);
    UD_EnableTool(ID_QuickOpt);
    UD_EnableTool(ID_FullOpt);
    UD_EnableTool(ID_MftOpt);
    UD_EnableTool(ID_SkipRem);
    UD_EnableTool(ID_Rescan);
    UD_EnableTool(ID_Repair);
    UD_EnableTool(ID_ShowReport);
    m_subMenuSortingConfig->Enable(true);
    m_busy = false;

    // XXX: sometimes reenabled buttons remain gray
    // on Windows 7, at least on a virtual machine,
    // so we have to refresh the toolbar ourselves
    m_toolBar->Refresh(); m_toolBar->Update();

    ReleasePause();

    ProcessCommandEvent(this,ID_AdjustSystemTrayIcon);
    ProcessCommandEvent(this,ID_SetWindowTitle);
    ProcessCommandEvent(this,ID_AdjustTaskbarIconOverlay);
    SetTaskbarProgressState(TBPF_NOPROGRESS);

    // shutdown when requested
    if(!m_stopped) ProcessCommandEvent(this,ID_Shutdown);
}

void MainFrame::SetPause()
{
    m_menuBar->Check(ID_Pause,true);
    m_toolBar->ToggleTool(ID_Pause,true);

    Utils::SetProcessPriority(IDLE_PRIORITY_CLASS);

    ProcessCommandEvent(this,ID_AdjustSystemTrayIcon);
    ProcessCommandEvent(this,ID_AdjustTaskbarIconOverlay);
}

void MainFrame::ReleasePause()
{
    m_menuBar->Check(ID_Pause,false);
    m_toolBar->ToggleTool(ID_Pause,false);

    Utils::SetProcessPriority(NORMAL_PRIORITY_CLASS);

    ProcessCommandEvent(this,ID_AdjustSystemTrayIcon);
    ProcessCommandEvent(this,ID_AdjustTaskbarIconOverlay);
}

void MainFrame::OnPause(wxCommandEvent& WXUNUSED(event))
{
    m_paused = m_paused ? false : true;
    if(m_paused) SetPause(); else ReleasePause();
}

void MainFrame::OnStop(wxCommandEvent& WXUNUSED(event))
{
    m_paused = false;
    ReleasePause();
    m_stopped = true;
}

void RecoveryConsole::OnTerminate(int pid, int status)
{
    g_refreshDrivesInfo = true;
}

void MainFrame::OnRepair(wxCommandEvent& WXUNUSED(event))
{
    if(m_busy) return;

    wxString args;
    long i = m_vList->GetFirstSelected();
    while(i != -1){
        char letter = (char)m_vList->GetItemText(i)[0];
        args << wxString::Format(wxT(" %c:"),letter);
        i = m_vList->GetNextSelected(i);
    }

    if(args.IsEmpty()) return;

    /*
    create command line to check disk for corruption:
    CHKDSK {drive} /F ................. check the drive and correct problems
    PING -n {seconds + 1} localhost ... pause for the specified number of seconds
    */
    wxFileName path(wxT("%windir%\\system32\\cmd.exe"));
    path.Normalize(); wxString cmd(path.GetFullPath());
    cmd << wxT(" /C ( ");
    cmd << wxT("for %D in ( ") << args << wxT(" ) do ");
    cmd << wxT("@echo. ");
    cmd << wxT("& echo chkdsk %D ");
    cmd << wxT("& echo. ");
    cmd << wxT("& chkdsk %D /F ");
    cmd << wxT("& echo. ");
    cmd << wxT("& echo ------------------------------------------------- ");
    cmd << wxT("& ping -n 11 localhost >nul ");
    cmd << wxT(") ");
    cmd << wxT("& echo. ");
    cmd << wxT("& pause");

    itrace("command line: %ls", ws(cmd));

    RecoveryConsole *rc = new RecoveryConsole();
    if(!wxExecute(cmd,wxEXEC_ASYNC,rc)){
        Utils::ShowError(wxT("Cannot execute cmd.exe program!"));
        delete rc;
    }
}

void MainFrame::OnDefaultAction(wxCommandEvent& WXUNUSED(event))
{
    long i = m_vList->GetFirstSelected();
    if(i != -1){
        volume_info v;
        char letter = (char)m_vList->GetItemText(i)[0];
        if(udefrag_get_volume_information(letter,&v) >= 0){
            if(v.is_dirty){
                ProcessCommandEvent(this,ID_Repair);
                return;
            }
        }
        ProcessCommandEvent(this,ID_Analyze);
    }
}

void MainFrame::OnDiskProcessingFailure(wxCommandEvent& event)
{
    wxString caption;
    switch(m_jobThread->m_jobType){
    case ANALYSIS_JOB:
        caption = wxString::Format(
            wxT("Analysis of %ls failed."),
            ws(event.GetString())
        );
        break;
    case DEFRAGMENTATION_JOB:
        caption = wxString::Format(
            wxT("Defragmentation of %ls failed."),
            ws(event.GetString())
        );
        break;
    default:
        caption = wxString::Format(
            wxT("Optimization of %ls failed."),
            ws(event.GetString())
        );
        break;
    }

    int error = event.GetInt();
    wxString msg = caption + wxT("\n") \
        + wxString::Format(wxT("%hs"),
        udefrag_get_error_description(error));

    Utils::ShowError(wxT("%ls"),ws(msg));
}

/** @} */
