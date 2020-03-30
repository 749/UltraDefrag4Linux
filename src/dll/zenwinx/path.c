/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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
 * @note Optimized for speed.
 */
void winx_path_remove_extension(wchar_t *path)
{
    int i;

    if(!path) return;
    
    for(i = wcslen(path) - 1; i >= 0; i--){
        if(path[i] == '\\')
            return; /* filename contains no dot */
        if(path[i] == '.' && i){
            if(path[i - 1] != '\\')
                path[i] = 0;
            return;
        }
    }
}

/**
 * @brief Removes the file name from a path.
 * @details If path contains as input <b>\\??\\C:\\Windows\\Test.txt</b>,
 * then path contains as output <b>\\??\\C:\\Windows</b>.
 * If the path has a trailing backslash, then only that is removed.
 * @param[in,out] path the native ANSI path to be processed.
 */
void winx_path_remove_filename(wchar_t *path)
{
    wchar_t *lb;
    
    if(path){
        lb = wcsrchr(path,'\\');
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
void winx_path_extract_filename(wchar_t *path)
{
    int i,j,n;
    
    if(!path) return;
    
    n = wcslen(path);
    if(!n) return;
    
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
 * @note The returned string should be freed by the winx_free call after its use.
 */
wchar_t *winx_get_module_filename(void)
{
    PROCESS_BASIC_INFORMATION pi;
    NTSTATUS status;
    UNICODE_STRING *us;
    wchar_t *path;
    int size;
    
    RtlZeroMemory(&pi,sizeof(pi));
    status = NtQueryInformationProcess(NtCurrentProcess(),
                    ProcessBasicInformation,&pi,
                    sizeof(pi),NULL);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot query basic process information");
        return NULL;
    }
    
    us = &pi.PebBaseAddress->ProcessParameters->ImagePathName;
    size = us->MaximumLength + sizeof(wchar_t);
    path = winx_tmalloc(size);
    if(path == NULL){
        mtrace();
        return NULL;
    }
    
    memset(path,0,size);
    memcpy(path,us->Buffer,us->Length);
    return path;
}

/**
 * @brief Creates a directory tree.
 * @param[in] path the native path.
 * @return Zero for success,
 * negative value otherwise.
 */
int winx_create_path(wchar_t *path)
{
    /*wchar_t rootdir[] = L"\\??\\X:\\";*/
    winx_volume_information v;
    wchar_t *p;
    unsigned int n;
    
    if(path == NULL)
        return (-1);

    /* path must contain at least \??\X: */
    if(wcsstr(path,L"\\??\\") != path || wcschr(path,':') != (path + 5)){
        etrace("native path must be specified");
        return (-1);
    }

    n = wcslen(L"\\??\\X:\\");
    if(wcslen(path) <= n){
        /* check for volume existence */
        /*
        rootdir[4] = path[4];
        // may fail with access denied status
        return winx_create_directory(rootdir);
        */
        return winx_get_volume_information((char)path[4],&v);
    }
    
    /* skip \??\X:\ */
    p = path + n;
    
    /* create directory tree */
    while((p = wcschr(p,'\\'))){
        *p = 0;
        if(winx_create_directory(path) < 0){
            *p = '\\';
            etrace("cannot create %ws",path);
            return (-1);
        }
        *p = '\\';
        p ++;
    }
    
    /* create target directory */
    if(winx_create_directory(path) < 0){
        etrace("cannot create %ws",path);
        return (-1);
    }
    
    return 0;
}

/** @} */
