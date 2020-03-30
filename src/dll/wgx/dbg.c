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
 * @file dbg.c
 * @brief Debugging.
 * @addtogroup Debug
 * @{
 */

#include "wgx-internals.h"

WGX_TRACE_HANDLER InternalTraceHandler = NULL;

/**
 * @brief Sets an internal trace handler
 * intended for being called each time
 * Wgx library produces debugging output.
 * @param[in] h address of the routine.
 * May be NULL if no debugging output
 * is needed.
 */
void WgxSetInternalTraceHandler(WGX_TRACE_HANDLER h)
{
    InternalTraceHandler = h;
}

/**
 * @brief Displays message box with formatted string in caption
 * and with description of the last Win32 error inside the window.
 * @param[in] hParent handle to the parent window.
 * @param[in] msgbox_flags flags passed to MessageBox routine.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 * @return Return value is the same as MessageBox returns.
 */
int WgxDisplayLastError(HWND hParent,UINT msgbox_flags, wchar_t *format, ...)
{
    va_list arg;
    wchar_t *msg;
    wchar_t *desc;
    #define BUFFER_SIZE 32
    wchar_t buffer[BUFFER_SIZE];
    DWORD error = GetLastError();
    int result;
    
    if(!format)
        return 0;

    va_start(arg,format);
    msg = wgx_vswprintf(format,arg);
    va_end(arg);

    if(msg == NULL){
        etrace("not enough memory");
        return 0;
    }

    /* produce debugging output */
    if(InternalTraceHandler){
        SetLastError(error);
        InternalTraceHandler(LAST_ERROR_FLAG,E"%ws",msg);
    }    
    
    /* display message box */
    if(!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPWSTR)(void *)&desc,0,NULL)){
        if(error == ERROR_COMMITMENT_LIMIT)
            (void)_snwprintf(buffer,BUFFER_SIZE,L"Not enough memory.");
        else
            (void)_snwprintf(buffer,BUFFER_SIZE,L"Error code = 0x%x",(UINT)error);
        buffer[BUFFER_SIZE - 1] = 0;
        result = MessageBoxW(hParent,buffer,msg,msgbox_flags);
    } else {
        result = MessageBoxW(hParent,desc,msg,msgbox_flags);
        LocalFree(desc);
    }
    
    /* cleanup */
    free(msg);
    return result;
}

/** @} */
