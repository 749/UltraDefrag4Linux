/*
 *  WGX - Windows GUI Extended Library.
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
 * @file dbg.c
 * @brief Debugging.
 * @addtogroup Debug
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "wgx.h"

/* look at /src/dll/zenwinx/dbg.c for details */
#define DBG_BUFFER_SIZE (4096-sizeof(ULONG))

/**
 * @brief Sends formatted string to DbgView program.
 */
void WgxDbgPrint(char *format, ...)
{
    char *buffer;
    va_list arg;
    int length;
    
    if(format == NULL)
        return;

    buffer = malloc(DBG_BUFFER_SIZE);
    if(buffer == NULL){
        OutputDebugString("Cannot allocate memory for WgxDbgPrint!");
        return;
    }

    /* store formatted string into buffer */
    va_start(arg,format);
    memset(buffer,0,DBG_BUFFER_SIZE);
    length = _vsnprintf(buffer,DBG_BUFFER_SIZE - 1,format,arg);
    (void)length;
    buffer[DBG_BUFFER_SIZE - 1] = 0;
    va_end(arg);

    /* send formatted string to the debugger */
    OutputDebugString(buffer);

    /* cleanup */
    free(buffer);
}

/**
 * @brief Sends formatted string to DbgView program,
 * with attached description of the last Win32 error.
 */
void WgxDbgPrintLastError(char *format, ...)
{
    char *buffer;
    va_list arg;
    int length;
    LPVOID msg;
    DWORD error = GetLastError();
    
    if(format == NULL)
        return;

    buffer = malloc(DBG_BUFFER_SIZE);
    if(buffer == NULL){
        OutputDebugString("Cannot allocate memory for WgxDbgPrintLastError!");
        return;
    }

    /* store formatted string into buffer */
    va_start(arg,format);
    memset(buffer,0,DBG_BUFFER_SIZE);
    length = _vsnprintf(buffer,DBG_BUFFER_SIZE - 1,format,arg);
    (void)length;
    buffer[DBG_BUFFER_SIZE - 1] = 0;
    va_end(arg);

    /* send formatted string to the debugger */
    OutputDebugString(buffer);
    OutputDebugString(": ");
    
    /* send last error description to the debugger */
    if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&msg,0,NULL)){
        if(error == ERROR_COMMITMENT_LIMIT)
            (void)sprintf(buffer,"Not enough memory.");
        else
            (void)sprintf(buffer,"Error code = 0x%x",(UINT)error);
        OutputDebugString(buffer);
    } else {
        OutputDebugString((LPCTSTR)msg);
        LocalFree(msg);
    }
    
    /* cleanup */
    OutputDebugString("\n");
    free(buffer);
}

/**
 * @brief Displays message box with formatted string in caption,
 * with description of the last Win32 error inside the window.
 * @param[in] hParent handle to the parent window.
 * @param[in] msgbox_flags flags passed to MessageBox routine.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 * @return Return value is the same as MessageBox returns.
 */
int WgxDisplayLastError(HWND hParent,UINT msgbox_flags, char *format, ...)
{
    char *buffer;
    va_list arg;
    int length;
    char b[32];
    char *text;
    LPVOID msg;
    DWORD error = GetLastError();
    int result;
    
    if(format == NULL)
        return 0;

    buffer = malloc(DBG_BUFFER_SIZE);
    if(buffer == NULL){
        OutputDebugString("Cannot allocate memory for WgxDisplayLastError!");
        return 0;
    }

    /* store formatted string into buffer */
    va_start(arg,format);
    memset(buffer,0,DBG_BUFFER_SIZE);
    length = _vsnprintf(buffer,DBG_BUFFER_SIZE - 1,format,arg);
    (void)length;
    buffer[DBG_BUFFER_SIZE - 1] = 0;
    va_end(arg);

    /* send formatted string to the debugger */
    OutputDebugString(buffer);
    OutputDebugString(": ");
    
    /* send last error description to the debugger */
    if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&msg,0,NULL)){
        if(error == ERROR_COMMITMENT_LIMIT)
            (void)_snprintf(b,sizeof(b),"Not enough memory.");
        else
            (void)_snprintf(b,sizeof(b),"Error code = 0x%x",(UINT)error);
        b[sizeof(b) - 1] = 0;
        msg = NULL;
        text = b;
    } else {
        text = (char *)msg;
    }
    OutputDebugString(text);
    OutputDebugString("\n");

    /* display message box */
    result = MessageBox(hParent,text,buffer,msgbox_flags);
    
    /* cleanup */
    if(msg) LocalFree(msg);
    free(buffer);
    return result;
}

/** @} */
