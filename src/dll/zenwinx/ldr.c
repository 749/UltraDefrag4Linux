/*
 *  ZenWINX - WIndows Native eXtended library.
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
 * @file ldr.c
 * @brief GetProcAddress.
 * @addtogroup Loader
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Retrieves the address of an exported function
 * or variable from the specified dynamic-link library (DLL).
 * @param[in] libname the library name.
 * @param[in] funcname the function or variable name.
 * @return The address of the requested function or variable.
 * NULL indicates failure.
 * @note The specified dynamic-link library 
 * must be loaded before this call.
 */
void *winx_get_proc_address(wchar_t *libname,char *funcname)
{
    UNICODE_STRING us;
    ANSI_STRING as;
    NTSTATUS status;
    HMODULE base_addr;
    void *proc_addr = NULL;

    DbgCheck2(libname,funcname,NULL);
    
    RtlInitUnicodeString(&us,libname);
    status = LdrGetDllHandle(0,0,&us,&base_addr);
    if(!NT_SUCCESS(status)){
        etrace("cannot get %ls handle: %x",libname,(UINT)status);
        return NULL;
    }
    RtlInitAnsiString(&as,funcname);
    status = LdrGetProcedureAddress(base_addr,&as,0,&proc_addr);
    if(!NT_SUCCESS(status)){
        if(strcmp(funcname,"RtlGetVersion")) /* reduce amount of debugging output on NT4 */
            etrace("cannot get address of %s: %x",funcname,(UINT)status);
        return NULL;
    }
    return proc_addr;
}

/** @} */
