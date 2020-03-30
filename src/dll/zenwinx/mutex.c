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
 * @file mutex.c
 * @brief Mutexes.
 * @addtogroup Mutexes
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Creates a named mutex.
 * @param[in] name the mutex name.
 * @param[out] phandle pointer to the mutex handle.
 * @return Zero for success, negative value otherwise.
 * @par Example:
 * @code
 * HANDLE h;
 * winx_create_mutex(L"\\BaseNamedObjects\\ultradefrag_mutex",&h);
 * @endcode
 */
int winx_create_mutex(wchar_t *name,HANDLE *phandle)
{
    UNICODE_STRING us;
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;

    DbgCheck2(name,phandle,-1);
    *phandle = NULL;

    RtlInitUnicodeString(&us,name);
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    status = NtCreateMutant(phandle,MUTEX_ALL_ACCESS,&oa,0);
    if(status == STATUS_OBJECT_NAME_COLLISION){
        itrace("%ws already exists",name);
        status = NtOpenMutant(phandle,MUTEX_ALL_ACCESS,&oa);
    }
    if(!NT_SUCCESS(status)){
        *phandle = NULL;
        strace(status,"cannot create/open %ws",name);
        return (-1);
    }
    return 0;
}

/**
 * @brief Opens a named mutex.
 * @param[in] name the mutex name.
 * @param[out] phandle pointer to the mutex handle.
 * @return Zero for success, negative value otherwise.
 * @par Example:
 * @code
 * HANDLE h;
 * winx_open_mutex(L"\\BaseNamedObjects\\ultradefrag_mutex",&h);
 * @endcode
 */
int winx_open_mutex(wchar_t *name,HANDLE *phandle)
{
    UNICODE_STRING us;
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;

    DbgCheck2(name,phandle,-1);
    *phandle = NULL;

    RtlInitUnicodeString(&us,name);
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    status = NtOpenMutant(phandle,MUTEX_ALL_ACCESS,&oa);
    if(!NT_SUCCESS(status)){
        *phandle = NULL;
        strace(status,"cannot open %ws",name);
        return (-1);
    }
    return 0;
}

/**
 * @brief Releases a mutex.
 * @param[in] h the mutex handle.
 * @return Zero for success, 
 * negative value otherwise.
 */
int winx_release_mutex(HANDLE h)
{
    NTSTATUS status;
    
    DbgCheck1(h,-1);
    
    status = NtReleaseMutant(h,NULL);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot release mutex");
        return (-1);
    }
    return 0;
}

/**
 * @brief Destroys a mutex.
 * @details Closes the handle of a named mutex
 * created/opened by winx_create_mutex() or 
 * winx_open_mutex().
 * @param[in] h the mutex handle.
 */
void winx_destroy_mutex(HANDLE h)
{
    if(h) NtClose(h);
}

/** @} */
