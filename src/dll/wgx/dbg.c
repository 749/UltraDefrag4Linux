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
 * @file dbg.c
 * @brief Debugging.
 * @details All the provided routines are safe,
 * in contrary to OutputDebugString which cannot
 * be called safely from DllMain - it may crash
 * the application in some cases (confirmed on w2k).
 * @addtogroup Debug
 * @{
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "wgx.h"

/* should be enough for any message */
#define DBG_BUFFER_SIZE (64 * 1024)

WGX_DBG_PRINT_HANDLER DbgPrintHandler = NULL;

/**
 * @brief Sets a routine to be called
 * for delivering of debugging messages.
 * @param[in] h address of the routine.
 * May be NULL if no delivering is needed.
 * @note The routine should support
 * <b>: $LE</b> magic sequence at end of the
 * format string triggering inclusion of the
 * last error code as well as its description
 * to the message.
 */
void WgxSetDbgPrintHandler(WGX_DBG_PRINT_HANDLER h)
{
    DbgPrintHandler = h;
}

/**
 * @brief Sends formatted string to DbgView program.
 */
void WgxDbgPrint(char *format, ...)
{
    char *msg;
    va_list arg;
    int length;
    
    if(!format || !DbgPrintHandler) return;

    msg = malloc(DBG_BUFFER_SIZE);
    if(msg == NULL){
        DbgPrintHandler("Cannot allocate memory for WgxDbgPrint!");
        return;
    }

    /* store formatted string into buffer */
    va_start(arg,format);
    memset(msg,0,DBG_BUFFER_SIZE);
    length = _vsnprintf(msg,DBG_BUFFER_SIZE - 1,format,arg);
    (void)length;
    msg[DBG_BUFFER_SIZE - 1] = 0;
    va_end(arg);

    /* send formatted string to the debugger */
    DbgPrintHandler("%s",msg);
    free(msg);
}

/**
 * @brief Sends formatted string to DbgView program,
 * with attached description of the last Win32 error.
 */
void WgxDbgPrintLastError(char *format, ...)
{
    char *msg;
    va_list arg;
    unsigned int length;
    char *seq = ": $LE";
    DWORD error = GetLastError();
    
    if(!format || !DbgPrintHandler) return;

    msg = malloc(DBG_BUFFER_SIZE);
    if(msg == NULL){
        DbgPrintHandler("Cannot allocate memory for WgxDbgPrintLastError!");
        return;
    }

    /* store formatted string into buffer */
    va_start(arg,format);
    memset(msg,0,DBG_BUFFER_SIZE);
    length = _vsnprintf(msg,DBG_BUFFER_SIZE - 1,format,arg);
    (void)length;
    msg[DBG_BUFFER_SIZE - 1] = 0;
    va_end(arg);
    
    /* append ": $LE" magic sequence */
    length = strlen(msg);
    if(length < DBG_BUFFER_SIZE - strlen(seq)){
        strcat(msg,seq);
    }

    /* send formatted string to the debugger */
    SetLastError(error);
    DbgPrintHandler("%s",msg);
    free(msg);
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
    char *msg;
    va_list arg;
    unsigned int length;
    char *seq = ": $LE";
    DWORD error = GetLastError();
    wchar_t *umsg, *desc = NULL, *text;
    #define SM_BUFFER_SIZE 32
    wchar_t b[SM_BUFFER_SIZE];
    int result;
    
    if(format == NULL)
        return 0;

    msg = malloc(DBG_BUFFER_SIZE);
    if(msg == NULL){
        if(DbgPrintHandler)
            DbgPrintHandler("Cannot allocate memory for WgxDisplayLastError (case 1)!");
        return 0;
    }

    /* store formatted string into buffer */
    va_start(arg,format);
    memset(msg,0,DBG_BUFFER_SIZE);
    length = _vsnprintf(msg,DBG_BUFFER_SIZE - 1,format,arg);
    (void)length;
    msg[DBG_BUFFER_SIZE - 1] = 0;
    va_end(arg);

    /* send formatted string to the debugger */
    if(DbgPrintHandler){
        length = strlen(msg);
        if(length < DBG_BUFFER_SIZE - strlen(seq)){
            strcat(msg,seq);
        }
        SetLastError(error);
        DbgPrintHandler("%s",msg);
        msg[length] = 0;
    }    
    
    /* display message box */
    umsg = malloc(DBG_BUFFER_SIZE * sizeof(wchar_t));
    if(umsg == NULL){
        if(DbgPrintHandler)
            DbgPrintHandler("Cannot allocate memory for WgxDisplayLastError (case 2)!");
        free(msg);
        return 0;
    }
    _snwprintf(umsg,DBG_BUFFER_SIZE,L"%hs",msg);
    umsg[DBG_BUFFER_SIZE - 1] = 0;
    
    if(!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPWSTR)(void *)&desc,0,NULL)){
        if(error == ERROR_COMMITMENT_LIMIT)
            (void)_snwprintf(b,SM_BUFFER_SIZE,L"Not enough memory.");
        else
            (void)_snwprintf(b,SM_BUFFER_SIZE,L"Error code = 0x%x",(UINT)error);
        b[SM_BUFFER_SIZE - 1] = 0;
        desc = NULL;
        text = b;
    } else {
        text = desc;
    }

    result = MessageBoxW(hParent,text,umsg,msgbox_flags);
    
    /* cleanup */
    if(desc) LocalFree(desc);
    free(umsg);
    free(msg);
    return result;
}

/** @} */
