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
 * @file reports.c
 * @brief Reports.
 * @addtogroup Reports
 * @{
 */

#include "main.h"

/**
 * @brief Opens report for a single volume.
 */
static void ShowSingleReport(volume_processing_job *job)
{
    short l_path[] = L"C:\\fraglist.luar";
    char path[] = "C:\\fraglist.luar";
    char cmd[MAX_PATH];
    char buffer[MAX_PATH];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    FILE *f;

    if(job == NULL) return;

    l_path[0] = (short)job->volume_letter;
    path[0] = job->volume_letter;

    if(job->job_type == NEVER_EXECUTED_JOB){
        /* show report if it already exists */
        f = fopen(path,"r");
        if(f) fclose(f);
        else return;
    }

    if(portable_mode == 0){
        (void)WgxShellExecuteW(hWindow,L"view",l_path,NULL,NULL,SW_SHOW);
        return;
    }

    strcpy(cmd,".\\lua5.1a_gui.exe");
    _snprintf(buffer,MAX_PATH,".\\lua5.1a_gui.exe .\\scripts\\udreportcnv.lua \"%s\" . -v",path);
    buffer[MAX_PATH - 1] = 0;

    ZeroMemory(&si,sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    ZeroMemory(&pi,sizeof(pi));

    if(!CreateProcess(cmd,buffer,
      NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,
            "Cannot execute lua5.1a_gui.exe program!");
        return;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

/**
 * @brief Opens reports for all selected volumes.
 */
void ShowReports(void)
{
    volume_processing_job *job;
    LRESULT SelectedItem;
    LV_ITEM lvi;
    char buffer[128];
    int index;
    
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
            job = get_job(buffer[0]);
            if(job)
                ShowSingleReport(job);
        }
        index = (int)SelectedItem;
    }
}

/** @} */
