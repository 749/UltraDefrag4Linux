/*
 *  WGX - Windows GUI Extended Library.
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
 * @file shell.c
 * @brief Windows Shell.
 * @addtogroup Shell
 * @{
 */

#include "wgx-internals.h"

/**
 * @brief A lightweight version of the ShellExecuteEx system API.
 */
BOOL WgxShellExecute(HWND hwnd,LPCWSTR lpOperation,LPCWSTR lpFile,
  LPCWSTR lpParameters,LPCWSTR lpDirectory,INT nShowCmd,INT nFlags)
{
    SHELLEXECUTEINFOW se;
    LPCWSTR op, file, args;
    
    op = lpOperation ? lpOperation : L"open";
    file = lpFile ? lpFile : L"";
    args = lpParameters ? lpParameters : L"";
    
    memset(&se,0,sizeof(se));
    se.cbSize = sizeof(se);
    se.fMask = SEE_MASK_FLAG_NO_UI;
    if(nFlags & WSH_NOASYNC)
        se.fMask |= SEE_MASK_FLAG_DDEWAIT;
    se.hwnd = hwnd;
    se.lpVerb = lpOperation;
    se.lpFile = lpFile;
    se.lpParameters = lpParameters;
    se.lpDirectory = lpDirectory;
    se.nShow = nShowCmd;
    if(!ShellExecuteExW(&se)){
        if(nFlags & WSH_SILENT || \
           nFlags & WSH_ALLOW_DEFAULT_ACTION){
            letrace("cannot %ls %ls %ls",op,file,args);
        } else {
            WgxDisplayLastError(hwnd,MB_OK | MB_ICONHAND,
                L"Cannot %ls %ls %ls",op,file,args);
        }
        if(nFlags & WSH_ALLOW_DEFAULT_ACTION){
            /* try the default action */
            se.lpVerb = NULL;
            if(!ShellExecuteExW(&se)){
                if(nFlags & WSH_SILENT){
                    letrace("cannot launch %ls %ls",file,args);
                } else {
                    WgxDisplayLastError(hwnd,MB_OK | MB_ICONHAND,
                        L"Cannot launch %ls %ls",file,args);
                }
                return FALSE;
            }
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

/** @} */
