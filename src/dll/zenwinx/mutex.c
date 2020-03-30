/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2012 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file mutex.c
 * @brief Mutexes.
 * @addtogroup Mutexes
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Creates a named mutex.
 * @param[in] name the name of the mutex.
 * @param[out] phandle pointer to the handle of the mutex.
 * @return Zero for success, negative value otherwise.
 * @par Example:
 * @code
 * HANDLE h;
 * winx_create_mutex(L"\\BaseNamedObjects\\ultradefrag_mutex",&h);
 * @endcode
 */
int winx_create_mutex(short *name,HANDLE *phandle)
{
    UNICODE_STRING us;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;

    DbgCheck2(name,phandle,"winx_create_mutex",-1);
    *phandle = NULL;

    RtlInitUnicodeString(&us,name);
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    Status = NtCreateMutant(phandle,MUTEX_ALL_ACCESS,&oa,0/*FALSE*/);
    if(Status == STATUS_OBJECT_NAME_COLLISION){
        DebugPrint("winx_create_mutex: %ws already exists",name);
        Status = NtOpenMutant(phandle,MUTEX_ALL_ACCESS,&oa);
    }
    if(!NT_SUCCESS(Status)){
        *phandle = NULL;
        DebugPrintEx(Status,"cannot create/open %ws mutex",name);
        return (-1);
    }
    return 0;
}

/**
 * @brief Opens a named mutex.
 * @param[in] name the name of the mutex.
 * @param[out] phandle pointer to the handle of the mutex.
 * @return Zero for success, negative value otherwise.
 * @par Example:
 * @code
 * HANDLE h;
 * winx_open_mutex(L"\\BaseNamedObjects\\ultradefrag_mutex",&h);
 * @endcode
 */
int winx_open_mutex(short *name,HANDLE *phandle)
{
    UNICODE_STRING us;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;

    DbgCheck2(name,phandle,"winx_open_mutex",-1);
    *phandle = NULL;

    RtlInitUnicodeString(&us,name);
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    Status = NtOpenMutant(phandle,MUTEX_ALL_ACCESS,&oa);
    if(!NT_SUCCESS(Status)){
        *phandle = NULL;
        DebugPrintEx(Status,"cannot open %ws mutex",name);
        return (-1);
    }
    return 0;
}

/**
 * @brief Releases a mutex.
 * @param[in] h the handle of the mutex.
 * @return Zero for success, 
 * negative value otherwise.
 */
int winx_release_mutex(HANDLE h)
{
    NTSTATUS Status;
    
    DbgCheck1(h,"winx_release_mutex",-1);
    
    Status = NtReleaseMutant(h,NULL);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"cannot release mutex");
        return (-1);
    }
    return 0;
}

/**
 * @brief Destroys a mutex.
 * @details Closes handle of a named mutex
 * created/opened by winx_create_mutex() or 
 * winx_open_mutex().
 * @param[in] h the handle of the mutex.
 */
void winx_destroy_mutex(HANDLE h)
{
    if(h) NtClose(h);
}

/** @} */
