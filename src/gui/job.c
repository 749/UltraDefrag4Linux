/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * @file job.c
 * @brief Volume processing jobs.
 * @addtogroup Job
 * @{
 */

/*
* Revised by Stefan Pendl, 2010, 2011
* <stefanpe@users.sourceforge.net>
*/

#include "main.h"

/* each volume letter may have a single job assigned */
#define NUMBER_OF_JOBS ('z' - 'a' + 1)

volume_processing_job jobs[NUMBER_OF_JOBS];

/* currently selected job */
volume_processing_job *current_job = NULL;

/* synchronizes access to internal map representation */
HANDLE hMapEvent = NULL;

/* nonzero value indicates that some job is running */
int busy_flag = 0;

/* forces to stop all running jobs */
int stop_pressed;

/* nonzero value indicates that the main window has been closed */
int exit_pressed = 0;

/* this flag is used for taskbar icon redraw synchonization */
int job_is_running = 0;

int pause_flag = 0;

/* overall progress counters */
int selected_volumes;
int processed_volumes;

extern int map_blocks_per_line;
extern int map_lines;

/**
 * @brief Initializes structures belonging to all jobs.
 */
void init_jobs(void)
{
    int i;
    
    for(i = 0; i < NUMBER_OF_JOBS; i++){
        memset(&jobs[i],0,sizeof(volume_processing_job));
        jobs[i].pi.completion_status = 1; /* not running */
        jobs[i].job_type = NEVER_EXECUTED_JOB;
        jobs[i].volume_letter = 'a' + i;
    }
}

/**
 * @brief Get job assigned to volume letter.
 */
volume_processing_job *get_job(char volume_letter)
{
    /* validate volume letter */
    volume_letter = udefrag_tolower(volume_letter);
    if(volume_letter < 'a' || volume_letter > 'z')
        return NULL;
    
    return &jobs[volume_letter - 'a'];
}

/**
 * @brief Updates status field 
 * of the list for all volumes.
 */
void update_status_of_all_jobs(void)
{
    int i;

    for(i = 0; i < NUMBER_OF_JOBS; i++){
        if(jobs[i].job_type != NEVER_EXECUTED_JOB)
            VolListUpdateStatusField(&jobs[i]);
    }
}

/**
 * @internal
 * @brief Updates progress indicators
 * for the currently running job.
 */
static void update_progress(udefrag_progress_info *pi, void *p)
{
    volume_processing_job *job;
    wchar_t WindowCaption[256];
    char current_operation;

    job = current_job;

    if(job == NULL)
        return;

    memcpy(&job->pi,pi,sizeof(udefrag_progress_info));
    
    VolListUpdateStatusField(job);
    VolListUpdateFragmentationField(job);
    UpdateStatusBar(pi);
    
    switch(pi->current_operation){
    case VOLUME_ANALYSIS:
        current_operation = 'A';
        break;
    case VOLUME_DEFRAGMENTATION:
        current_operation = 'D';
        break;
    default:
        current_operation = 'O';
        break;
    }
    
    if(dry_run){
        (void)swprintf(WindowCaption, L"%c:  %c %6.2lf %% (dry run)", 
            udefrag_toupper(job->volume_letter), current_operation, pi->percentage);
    } else {
        (void)swprintf(WindowCaption, L"%c:  %c %6.2lf %%",
            udefrag_toupper(job->volume_letter), current_operation, pi->percentage);
    }
    (void)SetWindowTextW(hWindow, WindowCaption);
    
    /* update tray icon tooltip */
    if(minimize_to_system_tray)
        SetSystemTrayIconTooltip(WindowCaption);

    if(WaitForSingleObject(hMapEvent,INFINITE) != WAIT_OBJECT_0){
        letrace("wait on hMapEvent failed");
        return;
    }
    if(pi->cluster_map){
        if(job->map.buffer == NULL || pi->cluster_map_size != job->map.size){
            free(job->map.buffer);
            job->map.buffer = malloc(pi->cluster_map_size);
        }
        if(job->map.buffer == NULL){
            etrace("cannot allocate %u bytes of memory",
                pi->cluster_map_size);
            job->map.size = 0;
        } else {
            job->map.size = pi->cluster_map_size;
            memcpy(job->map.buffer,pi->cluster_map,pi->cluster_map_size);
        }
    }
    SetEvent(hMapEvent);

    if(pi->cluster_map && job->map.buffer)
        RedrawMap(job,1);
    
    /* set overall progress */
    if(current_job->job_type == ANALYSIS_JOB \
      || pi->current_operation != VOLUME_ANALYSIS){
        if(WaitForSingleObject(hTaskbarIconEvent,INFINITE) != WAIT_OBJECT_0){
            letrace("wait on hTaskbarIconEvent failed");
        } else {
            if(show_progress_in_taskbar){
                WgxSetTaskbarProgressState(hWindow,TBPF_NORMAL);
                if(pi->clusters_to_process){
                    WgxSetTaskbarProgressValue(hWindow,
                        (pi->clusters_to_process / selected_volumes) * processed_volumes + \
                        pi->processed_clusters / selected_volumes,
                        pi->clusters_to_process);
                } else {
                    WgxSetTaskbarProgressValue(hWindow,0,1);
                }
            } else {
                WgxSetTaskbarProgressState(hWindow,TBPF_NOPROGRESS);
            }
            SetEvent(hTaskbarIconEvent);
        }
    }
    
    if(pi->completion_status != 0/* && !stop_pressed*/){
        if(dry_run == 0){
            if(portable_mode) SetWindowText(hWindow,VERSIONINTITLE_PORTABLE);
            else SetWindowText(hWindow,VERSIONINTITLE);
        } else {
            if(portable_mode) SetWindowText(hWindow,VERSIONINTITLE_PORTABLE " (dry run)");
            else SetWindowText(hWindow,VERSIONINTITLE " (dry run)");
        }
    }
}

/**
 * @brief Puts the job into sleep.
 */
void SetPause(void)
{
    pause_flag = 1;
    CheckMenuItem(hMainMenu,IDM_PAUSE,
        MF_BYCOMMAND | MF_CHECKED);
    SendMessage(hToolbar,TB_CHECKBUTTON,IDM_PAUSE,MAKELONG(TRUE,0));
    WgxSetProcessPriority(IDLE_PRIORITY_CLASS);

    /* set taskbar icon overlay and notification area icon */
    if(WaitForSingleObject(hTaskbarIconEvent,INFINITE) != WAIT_OBJECT_0){
        letrace("wait on hTaskbarIconEvent failed");
    } else {
        SetTaskbarIconOverlay(IDI_PAUSED,"JOB_IS_PAUSED");
        ShowSystemTrayIcon(NIM_MODIFY);
        SetEvent(hTaskbarIconEvent);
    }
}

/**
 * @brief Wakes the job from sleep.
 */
void ReleasePause(void)
{
    pause_flag = 0;
    CheckMenuItem(hMainMenu,IDM_PAUSE,
        MF_BYCOMMAND | MF_UNCHECKED);
    SendMessage(hToolbar,TB_CHECKBUTTON,IDM_PAUSE,MAKELONG(FALSE,0));
    WgxSetProcessPriority(NORMAL_PRIORITY_CLASS);

    /* set taskbar icon overlay and notification area icon */
    if(WaitForSingleObject(hTaskbarIconEvent,INFINITE) != WAIT_OBJECT_0){
        letrace("wait on hTaskbarIconEvent failed");
    } else {
        if(job_is_running){
            SetTaskbarIconOverlay(IDI_BUSY,"JOB_IS_RUNNING");
        } else {
            RemoveTaskbarIconOverlay();
        }
        ShowSystemTrayIcon(NIM_MODIFY);
        SetEvent(hTaskbarIconEvent);
    }
}

/**
 * @internal
 * @brief Terminates currently running job.
 */
static int terminator(void *p)
{
    while(pause_flag) Sleep(300);
    return stop_pressed;
}

/**
 * @internal
 * @brief Displays detailed information
 * about volume validation failure.
 */
static void DisplayInvalidVolumeError(int error_code)
{
    char buffer[512];

    if(error_code == UDEFRAG_UNKNOWN_ERROR){
        MessageBoxA(NULL,"Disk is missing or some error has been encountered.\n"
                         "Enable logs or use DbgView program to get more information.",
                         "The disk cannot be processed!",MB_OK | MB_ICONHAND);
    } else {
        (void)_snprintf(buffer,sizeof(buffer),"%s\n%s",
                udefrag_get_error_description(error_code),
                "Enable logs or use DbgView program to get more information.");
        buffer[sizeof(buffer) - 1] = 0;
        MessageBoxA(NULL,buffer,"The disk cannot be processed!",MB_OK | MB_ICONHAND);
    }
}

/**
 * @internal
 * @brief Displays detailed information
 * about volume processing failure.
 */
static void DisplayDefragError(int error_code)
{
    char buffer[512];
    char *caption = "Disk optimization failed!";
    
    switch(current_job->job_type){
    case ANALYSIS_JOB:
        caption = "Disk analysis failed!";
        break;
    case DEFRAGMENTATION_JOB:
        caption = "Disk defragmentation failed!";
        break;
    default:
        break;
    }
    (void)_snprintf(buffer,sizeof(buffer),"%s\n%s",
            udefrag_get_error_description(error_code),
            "Enable logs or use DbgView program to get more information.");
    buffer[sizeof(buffer) - 1] = 0;
    MessageBoxA(NULL,buffer,caption,MB_OK | MB_ICONHAND);
}

/**
 * @internal
 * @brief Runs job for a single volume.
 */
static void ProcessSingleVolume(volume_processing_job *job)
{
    volume_info v;
    int error_code;
    int index;

    if(job == NULL)
        return;

    /* refresh capacity information of the volume (bug #2036873) */
    VolListRefreshItem(job);
    /*ShowProgress();
    SetProgress(L"A 0.00 %",0);*/

    /* validate the volume before any processing */
    error_code = udefrag_validate_volume(job->volume_letter,FALSE);
    if(error_code < 0){
        /* handle error */
        DisplayInvalidVolumeError(error_code);
    } else {
        /* process the volume */
        current_job = job;
        if(repeat_action) job_flags |= UD_JOB_REPEAT;
        else job_flags &= ~UD_JOB_REPEAT;
        error_code = udefrag_start_job(job->volume_letter, job->job_type,
                job_flags,map_blocks_per_line * map_lines,update_progress,
                terminator, NULL);
        if(error_code < 0 && !exit_pressed){
            DisplayDefragError(error_code);
        }
        /* update dirty volume mark */
        index = get_job_index(job);
        if(index != -1){
            if(udefrag_get_volume_information(job->volume_letter,&v) >= 0)
                SetVolumeDirtyStatus(index,&v);
        }
    }
}

/**
 * @internal
 * @brief start_selected_jobs thread routine.
 */
DWORD WINAPI StartJobsThreadProc(LPVOID lpParameter)
{
    volume_processing_job *job;
    LRESULT SelectedItem;
    LV_ITEM lvi;
    char buffer[64];
    int index;
    TBBUTTONINFO tbi;
    UINT id;
    udefrag_job_type job_type = (udefrag_job_type)(DWORD_PTR)lpParameter;
    
    /* return immediately if we are busy */
    if(busy_flag) return 0;
    busy_flag = 1;
    stop_pressed = 0;
    
    /* return immediately if there are no volumes selected */
    if(SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED) == -1){
        busy_flag = 0;
        return 0;
    }

    /* disable menu entries */
    EnableMenuItem(hMainMenu,IDM_ANALYZE,MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMainMenu,IDM_DEFRAG,MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMainMenu,IDM_QUICK_OPTIMIZE,MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMainMenu,IDM_FULL_OPTIMIZE,MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMainMenu,IDM_OPTIMIZE_MFT,MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMainMenu,IDM_REPEAT_ACTION,MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMainMenu,IDM_SHOW_REPORT,MF_BYCOMMAND | MF_GRAYED);
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_ANALYZE,MAKELONG(FALSE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_DEFRAG,MAKELONG(FALSE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_QUICK_OPTIMIZE,MAKELONG(FALSE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_FULL_OPTIMIZE,MAKELONG(FALSE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_OPTIMIZE_MFT,MAKELONG(FALSE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_REPEAT_ACTION,MAKELONG(FALSE,0));
    /*SendMessage(hToolbar,TB_SETSTATE,IDM_REPEAT_ACTION,MAKELONG(TBSTATE_INDETERMINATE,0));*/
    tbi.cbSize = sizeof(TBBUTTONINFO);
    tbi.dwMask = TBIF_IMAGE;
    tbi.iImage = 2; /* grayed repeat icon */
    SendMessage(hToolbar,TB_SETBUTTONINFO,IDM_REPEAT_ACTION,(LRESULT)&tbi);
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_SHOW_REPORT,MAKELONG(FALSE,0));
    
    /* set check mark in front of the selected optimization method */
    switch(job_type){
    case FULL_OPTIMIZATION_JOB:
        id = IDM_FULL_OPTIMIZE;
        break;
    case QUICK_OPTIMIZATION_JOB:
        id = IDM_QUICK_OPTIMIZE;
        break;
    case MFT_OPTIMIZATION_JOB:
        id = IDM_OPTIMIZE_MFT;
        break;
    default:
        id = 0;
        break;
    }
    if(id) CheckMenuItem(hMainMenu,id,MF_BYCOMMAND | MF_CHECKED);
    
    /* set taskbar icon overlay and notification area icon */
    if(WaitForSingleObject(hTaskbarIconEvent,INFINITE) != WAIT_OBJECT_0){
        letrace("wait on hTaskbarIconEvent failed");
    } else {
        SetTaskbarIconOverlay(IDI_BUSY,"JOB_IS_RUNNING");
        /* set overall progress: normal 0% */
        if(show_progress_in_taskbar){
            WgxSetTaskbarProgressValue(hWindow,0,1);
            WgxSetTaskbarProgressState(hWindow,TBPF_NORMAL);
        }
        job_is_running = 1;
        ShowSystemTrayIcon(NIM_MODIFY);
        SetEvent(hTaskbarIconEvent);
    }

    /* count selected volumes */
    index = -1; selected_volumes = 0;
    while(1){
        SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,(WPARAM)index,LVNI_SELECTED);
        if(SelectedItem == -1 || SelectedItem == index) break;
        selected_volumes ++;
        index = (int)SelectedItem;
    }
    /* avoid division by zero error in update_progress */
    if(selected_volumes == 0)
        selected_volumes ++;

    /* process all selected volumes */
    index = -1; processed_volumes = 0;
    while(1){
        SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,(WPARAM)index,LVNI_SELECTED);
        if(SelectedItem == -1 || SelectedItem == index) break;
        if(stop_pressed || exit_pressed) break;
        lvi.iItem = (int)SelectedItem;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = buffer;
        lvi.cchTextMax = 63;
        if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi)){
            job = get_job(buffer[0]);
            if(job){
                job->job_type = job_type;
                ProcessSingleVolume(job);
            }
        }
        processed_volumes ++;
        /* advance overall progress to processed/selected */
        if(WaitForSingleObject(hTaskbarIconEvent,INFINITE) != WAIT_OBJECT_0){
            letrace("wait on hTaskbarIconEvent failed");
        } else {
            if(show_progress_in_taskbar){
                WgxSetTaskbarProgressState(hWindow,TBPF_NORMAL);
                WgxSetTaskbarProgressValue(hWindow,processed_volumes,selected_volumes);
            } else {
                WgxSetTaskbarProgressState(hWindow,TBPF_NOPROGRESS);
            }
            SetEvent(hTaskbarIconEvent);
        }
        index = (int)SelectedItem;
    }

    busy_flag = 0;
    
    /* remove check mark set in front of the selected optimization method */
    if(id) CheckMenuItem(hMainMenu,id,MF_BYCOMMAND | MF_UNCHECKED);

    /* enable menu entries */
    EnableMenuItem(hMainMenu,IDM_ANALYZE,MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMainMenu,IDM_DEFRAG,MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMainMenu,IDM_QUICK_OPTIMIZE,MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMainMenu,IDM_FULL_OPTIMIZE,MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMainMenu,IDM_OPTIMIZE_MFT,MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMainMenu,IDM_REPEAT_ACTION,MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMainMenu,IDM_SHOW_REPORT,MF_BYCOMMAND | MF_ENABLED);
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_ANALYZE,MAKELONG(TRUE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_DEFRAG,MAKELONG(TRUE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_QUICK_OPTIMIZE,MAKELONG(TRUE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_FULL_OPTIMIZE,MAKELONG(TRUE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_OPTIMIZE_MFT,MAKELONG(TRUE,0));
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_REPEAT_ACTION,MAKELONG(TRUE,0));
    /*SendMessage(hToolbar,TB_SETSTATE,IDM_REPEAT_ACTION,MAKELONG(TBSTATE_ENABLED,0));
    if(repeat_action)
        SendMessage(hToolbar,TB_CHECKBUTTON,IDM_REPEAT_ACTION,MAKELONG(TRUE,0));
    else
        SendMessage(hToolbar,TB_CHECKBUTTON,IDM_REPEAT_ACTION,MAKELONG(FALSE,0));
    */
    tbi.cbSize = sizeof(TBBUTTONINFO);
    tbi.dwMask = TBIF_IMAGE;
    tbi.iImage = 1; /* normal repeat icon */
    SendMessage(hToolbar,TB_SETBUTTONINFO,IDM_REPEAT_ACTION,(LRESULT)&tbi);
    SendMessage(hToolbar,TB_ENABLEBUTTON,IDM_SHOW_REPORT,MAKELONG(TRUE,0));
    
    /* remove taskbar icon overlay; change notification area icon */
    if(WaitForSingleObject(hTaskbarIconEvent,INFINITE) != WAIT_OBJECT_0){
        letrace("wait on hTaskbarIconEvent failed");
    } else {
        job_is_running = 0;
        RemoveTaskbarIconOverlay();
        /* remove the taskbar progress indicator */
        WgxSetTaskbarProgressState(hWindow,TBPF_NOPROGRESS);
        ShowSystemTrayIcon(NIM_MODIFY);
        SetEvent(hTaskbarIconEvent);
    }

    /* check the when done action state */
    if(!exit_pressed && !stop_pressed){
        if(when_done_action != IDM_WHEN_DONE_NONE){
            shutdown_requested = 1;
            SendMessage(hWindow,WM_COMMAND,IDM_EXIT,0);
        }
    }
    return 0;
}

/**
 * @brief Runs sequentially all selected jobs.
 */
void start_selected_jobs(udefrag_job_type job_type)
{
    char *action = "disk analysis";

    ReleasePause();
    
    if(!WgxCreateThread(StartJobsThreadProc,(LPVOID)(DWORD_PTR)job_type)){
        if(job_type == DEFRAGMENTATION_JOB)
            action = "disk defragmentation";
        else if(job_type == FULL_OPTIMIZATION_JOB)
            action = "full disk optimization";
        else if(job_type == QUICK_OPTIMIZATION_JOB)
            action = "quick disk optimization";
        else if(job_type == MFT_OPTIMIZATION_JOB)
            action = "MFT optimization";
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,
            L"Cannot create thread starting %s!",action);
    }
}

/**
 * @brief Stops all running jobs.
 */
void stop_all_jobs(void)
{
    ReleasePause();
    stop_pressed = 1;
}

/**
 * @brief Frees resources allocated for all jobs.
 */
void release_jobs(void)
{
    volume_processing_job *j;
    int i;
    
    for(i = 0; i < NUMBER_OF_JOBS; i++){
        j = &jobs[i];
        if(j->map.hdc)
            (void)DeleteDC(j->map.hdc);
        if(j->map.hbitmap)
            (void)DeleteObject(j->map.hbitmap);
        free(j->map.buffer);
        free(j->map.scaled_buffer);
    }
}

/**
 * @internal
 * @brief Buffers for RepairSelectedVolumes.
 */
#define MAX_CMD_LENGTH 32768
char args[MAX_CMD_LENGTH];
char buffer[MAX_CMD_LENGTH];

/**
 * @brief Tries to repair all selected volumes.
 */
void RepairSelectedVolumes(void)
{
    LRESULT SelectedItem;
    LV_ITEM lvi;
    char letter;
    int index;
    int counter = 0;

    strcpy(args,"");

    index = -1;
    while(1){
        SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,(WPARAM)index,LVNI_SELECTED);
        if(SelectedItem == -1 || SelectedItem == index) break;
        lvi.iItem = (int)SelectedItem;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = buffer;
        lvi.cchTextMax = 127;
        if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi)){
            letter = buffer[0];
            _snprintf(buffer,MAX_CMD_LENGTH,"%s %c:",args,letter);
            buffer[MAX_CMD_LENGTH - 1] = 0;
            strcpy(args,buffer);
            counter ++;
        }
        index = (int)SelectedItem;
    }
    
    if(counter == 0) return;

    /*
    create command line to check disk for corruption:
    CHKDSK {drive} /F ................. check the drive and correct problems
    PING -n {seconds + 1} localhost ... pause for the specified seconds
    */
    _snprintf(buffer,MAX_CMD_LENGTH,
        "/C ( for %%D in ( %s ) do @echo. & echo chkdsk %%D & echo. & chkdsk %%D /F & echo. & echo %s & ping -n 11 localhost >nul ) & echo. & pause",
        args,"-------------------------------------------------");
    buffer[MAX_CMD_LENGTH - 1] = 0;
    strcpy(args,buffer);
    
    itrace("Command Line: %s", args);

    if(!WgxCreateProcess("%windir%\\system32\\cmd.exe",args)){
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,
            L"Cannot execute cmd.exe program!");
    }
}

/** @} */
