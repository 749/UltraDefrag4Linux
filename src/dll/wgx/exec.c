/*
 *  WGX - Windows GUI Extended Library.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file exec.c
 * @brief Processes and Threads.
 * @addtogroup Exec
 * @{
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "wgx.h"

#define MAX_CMD_LENGTH 32768

/**
 * @brief Lightweight version
 * of CreateProcess system API.
 */
BOOL WgxCreateProcess(char *cmd,char *args)
{
    char *command = NULL;
    char *cmdline = NULL;
    int error = ERROR_SUCCESS;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    /* validate arguments */
    if(cmd == NULL){
        error = ERROR_INVALID_PARAMETER;
        WgxDbgPrint("WgxCreateProcess: cmd == NULL");
        goto done;
    }
    if(args == NULL)
        args = "";
    
    /* allocate memory */
    command = malloc(MAX_CMD_LENGTH);
    cmdline = malloc(MAX_CMD_LENGTH);
    if(command == NULL || cmdline == NULL){
        error = ERROR_NOT_ENOUGH_MEMORY;
        WgxDbgPrint("WgxCreateProcess: not enough memory for %s",cmd);
        goto done;
    }
    
    /* expand environment strings in command */
    if(!ExpandEnvironmentStrings(cmd,command,MAX_CMD_LENGTH)){
        error = GetLastError();
        WgxDbgPrintLastError("WgxCreateProcess: cannot expand environment strings in %s",cmd);
        goto done;
    }
    
    /* build command line */
    _snprintf(cmdline,MAX_CMD_LENGTH,"%s %s",command,args);
    cmdline[MAX_CMD_LENGTH - 1] = 0;
    
    /* create process */
    ZeroMemory(&si,sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    ZeroMemory(&pi,sizeof(pi));

    if(!CreateProcess(command,cmdline,
      NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
        error = GetLastError();
        WgxDbgPrintLastError("WgxCreateProcess: cannot create process for %s",cmd);
        goto done;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
done:
    if(command) free(command);
    if(cmdline) free(cmdline);
    SetLastError(error);
    return (error == ERROR_SUCCESS) ? TRUE : FALSE;
}

/**
 * @brief Lightweight version
 * of CreateThread system API.
 */
BOOL WgxCreateThread(LPTHREAD_START_ROUTINE routine,LPVOID param)
{
    DWORD id;
    HANDLE h;
    
    h = CreateThread(NULL,0,routine,param,0,&id);
    if(h == NULL)
        return FALSE;
    CloseHandle(h);
    return TRUE;
}

/** @} */
