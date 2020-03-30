/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

extern int map_blocks_per_line;
extern int map_lines;

/**
 * @brief Initializes structures belonging to all jobs.
 */
int init_jobs(void)
{
    char event_name[64];
    int i;
    
    _snprintf(event_name,64,"udefrag-gui-%u",(int)GetCurrentProcessId());
    event_name[63] = 0;
    hMapEvent = CreateEvent(NULL,FALSE,TRUE,event_name);
    if(hMapEvent == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
            "Cannot create %s event!",
            event_name);
        return (-1);
    }
    
    for(i = 0; i < NUMBER_OF_JOBS; i++){
        memset(&jobs[i],0,sizeof(volume_processing_job));
        jobs[i].pi.completion_status = 1; /* not running */
        jobs[i].job_type = NEVER_EXECUTED_JOB;
        jobs[i].volume_letter = 'a' + i;
    }
    return 0;
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
    char WindowCaption[256];
    char current_operation;

    job = current_job;

    if(job == NULL)
        return;

    memcpy(&job->pi,pi,sizeof(udefrag_progress_info));
    
    VolListUpdateStatusField(job);
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
        (void)sprintf(WindowCaption, "%c:  %c %6.2lf %% (dry run)", 
            udefrag_toupper(job->volume_letter), current_operation, pi->percentage);
    } else {
        (void)sprintf(WindowCaption, "%c:  %c %6.2lf %%",
            udefrag_toupper(job->volume_letter), current_operation, pi->percentage);
    }
    (void)SetWindowText(hWindow, WindowCaption);

    if(WaitForSingleObject(hMapEvent,INFINITE) != WAIT_OBJECT_0){
        WgxDbgPrintLastError("update_progress: wait on hMapEvent failed");
        return;
    }
    if(pi->cluster_map){
        if(job->map.buffer == NULL || pi->cluster_map_size != job->map.size){
            if(job->map.buffer)
                free(job->map.buffer);
            job->map.buffer = malloc(pi->cluster_map_size);
        }
        if(job->map.buffer == NULL){
            WgxDbgPrint("update_progress: cannot allocate %u bytes of memory\n",
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
 * @internal
 * @brief Terminates currently running job.
 */
static int terminator(void *p)
{
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
    int error_code;

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

    /* process all selected volumes */
    index = -1;
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
        index = (int)SelectedItem;
    }

    busy_flag = 0;

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
    DWORD id;
    HANDLE h;
    char *action = "disk analysis";

    h = create_thread(StartJobsThreadProc,(LPVOID)(DWORD_PTR)job_type,&id);
    if(h == NULL){
        if(job_type == DEFRAGMENTATION_JOB)
            action = "disk defragmentation";
        else if(job_type == FULL_OPTIMIZATION_JOB)
            action = "full disk optimization";
        else if(job_type == QUICK_OPTIMIZATION_JOB)
            action = "quick disk optimization";
        else if(job_type == MFT_OPTIMIZATION_JOB)
            action = "MFT optimization";
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,
            "Cannot create thread starting %s!",
            action);
    } else {
        CloseHandle(h);
    }
}

/**
 * @brief Stops all running jobs.
 */
void stop_all_jobs(void)
{
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
        if(j->map.buffer)
            free(j->map.buffer);
        if(j->map.scaled_buffer)
            free(j->map.scaled_buffer);
    }
    if(hMapEvent)
        CloseHandle(hMapEvent);
}

/** @} */
