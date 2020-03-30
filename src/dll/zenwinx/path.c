/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2012 Stefan Pendl (stefanpe@users.sourceforge.net).
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
 * @file path.c
 * @brief Paths.
 * @addtogroup Paths
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Removes the extension from a path.
 * @details If path contains as input <b>\\??\\C:\\Windows\\Test.txt</b>,
 * then path contains as output <b>\\??\\C:\\Windows\\Test</b>.
 * If the file name contains no dot or is starting with a dot,
 * then path keeps unchanged.
 * @param[in,out] path the native ANSI path to be processed.
 */
void winx_path_remove_extension(char *path)
{
    int i;

#if 0
    char *lb, *dot;

    /* slower */
    if(path){
        lb = strrchr(path,'\\');
        dot = strrchr(path,'.');
        if(lb && dot){ /* both backslash and a dot exist */
            if(dot < lb || dot == lb + 1)
                return; /* filename contains either no dot or a leading dot */
            *dot = 0;
        }
    }
#else
    if(path == NULL)
        return;
    
    /* faster */
    for(i = strlen(path) - 1; i >= 0; i--){
        if(path[i] == '\\')
            return; /* filename contains no dot */
        if(path[i] == '.' && i){
            if(path[i - 1] != '\\')
                path[i] = 0;
            return;
        }
    }
#endif
}

/**
 * @brief Removes the file name from a path.
 * @details If path contains as input <b>\\??\\C:\\Windows\\Test.txt</b>,
 * then path contains as output <b>\\??\\C:\\Windows</b>.
 * If the path has a trailing backslash, then only that is removed.
 * @param[in,out] path the native ANSI path to be processed.
 */
void winx_path_remove_filename(char *path)
{
    char *lb;
    
    if(path){
        lb = strrchr(path,'\\');
        if(lb) *lb = 0;
    }
}

/**
 * @brief Extracts the file name from a path.
 * @details If path contains as input <b>\\??\\C:\\Windows\\Test.txt</b>,
 * path contains as output <b>Test.txt</b>.
 * If path contains as input <b>\\??\\C:\\Windows\\</b>,
 * path contains as output <b>Windows\\</b>.
 * @param[in,out] path the native ANSI path to be processed.
 */
void winx_path_extract_filename(char *path)
{
    int i,j,n;
    
    if(path == NULL)
        return;
    
    n = strlen(path);
    if(n == 0)
        return;
    
    for(i = n - 1; i >= 0; i--){
        if(path[i] == '\\' && (i != n - 1)){
            /* path[i+1] points to filename */
            i++;
            for(j = 0; path[i]; i++, j++)
                path[j] = path[i];
            path[j] = 0;
            return;
        }
    }
}

/**
 * @brief Gets the fully quallified path of the current module.
 * @details This routine is the native equivalent of GetModuleFileName.
 * @param[out] path receives the native path of the current executable.
 * @note path must be MAX_PATH characters long.
 */
void winx_get_module_filename(char *path)
{
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION ProcessInformation;
    ANSI_STRING as = {0};
    
    if(path == NULL)
        return;
    
    path[0] = 0;
    
    /* retrieve process executable path */
    RtlZeroMemory(&ProcessInformation,sizeof(ProcessInformation));
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                    ProcessBasicInformation,&ProcessInformation,
                    sizeof(ProcessInformation),
                    NULL);
    /* extract path and file name */
    if(NT_SUCCESS(Status)){
        /* convert Unicode path to ANSI */
        if(RtlUnicodeStringToAnsiString(&as,&ProcessInformation.PebBaseAddress->ProcessParameters->ImagePathName,TRUE) == STATUS_SUCCESS){
            /* avoid buffer overflow */
            if(as.Length < (MAX_PATH-4)){
                /* add native path prefix to path */
                (void)strcpy(path,"\\??\\");
                (void)strncat(path,as.Buffer,as.Length);
            } else {
                DebugPrint("winx_get_module_filename: path is too long");
            }
            RtlFreeAnsiString(&as);
        } else {
            DebugPrint("winx_get_module_filename: cannot convert unicode to ansi path: not enough memory");
        }
    } else {
        DebugPrintEx(Status,"winx_get_module_filename: cannot query process basic information");
    }
}

/**
 * @brief Creates directory tree.
 * @param[in] path the native path.
 * @return Zero for success,
 * negative value otherwise.
 */
int winx_create_path(char *path)
{
    char *p;
    unsigned int n;
    /*char rootdir[] = "\\??\\X:\\";*/
    winx_volume_information v;
    
    if(path == NULL)
        return (-1);

    /* path must contain at least \??\X: */
    if(strstr(path,"\\??\\") != path || strchr(path,':') != (path + 5)){
        DebugPrint("winx_create_path: native path must be specified");
        return (-1);
    }

    n = strlen("\\??\\X:\\");
    if(strlen(path) <= n){
        /* check for volume existence */
        /*
        rootdir[4] = path[4];
        // may fail with access denied status
        return winx_create_directory(rootdir);
        */
        return winx_get_volume_information(path[4],&v);
    }
    
    /* skip \??\X:\ */
    p = path + n;
    
    /* create directory tree */
    while((p = strchr(p,'\\'))){
        *p = 0;
        if(winx_create_directory(path) < 0){
            DebugPrint("winx_create_path failed");
            *p = '\\';
            return (-1);
        }
        *p = '\\';
        p ++;
    }
    
    /* create target directory */
    if(winx_create_directory(path) < 0){
        DebugPrint("winx_create_path failed");
        return (-1);
    }
    
    return 0;
}

/** @} */
