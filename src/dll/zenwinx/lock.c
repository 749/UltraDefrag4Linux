/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file lock.c
 * @brief Locks.
 * @details Locks can be used to 
 * synchronize access to shared data.
 * @addtogroup Locks
 * @{
 */

#include "prec.h"
#include "zenwinx.h"

/*
* winx_acquire_lock and winx_release_lock
* are intended for use from debugging routines,
* so avoid use of tracing facilities there
* to avoid recursion.
*/

/**
 * @brief Creates a lock.
 * @param[in] name the name of the lock.
 * @param[out] phandle pointer to variable
 * to store the lock's handle into.
 * @return Zero for success,
 * a negative value otherwise.
 */
int winx_create_lock(wchar_t *name,HANDLE *phandle)
{
    unsigned int id;
    wchar_t *fullname;
    int result;
    
    DbgCheck2(name,phandle,-1);
    
    /* attach PID to lock the current process only */
    id = (unsigned int)(DWORD_PTR)(NtCurrentTeb()-> \
        ClientId.UniqueProcess);
    fullname = winx_swprintf(L"\\%ls_%u",name,id);
    if(fullname == NULL){
        etrace("not enough memory for %ls",name);
        *phandle = NULL;
        return (-1);
    }

    result = winx_create_event(fullname, \
        SynchronizationEvent,phandle);
    winx_free(fullname);

    if(result < 0)
        return result;

    if(winx_release_lock(*phandle) < 0){
        etrace("cannot release %ls",name);
        winx_destroy_event(*phandle);
        *phandle = NULL;
        return (-1);
    }
    
    return 0;
}

/**
 * @brief Acquires a lock.
 * @param[in] h the lock's handle.
 * @param[in] msec the timeout interval,
 * in milliseconds. If the INFINITE constant
 * is passed, the interval never elapses.
 * @return Zero for success,
 * a negative value otherwise.
 */
int winx_acquire_lock(HANDLE h,int msec)
{
    LARGE_INTEGER interval;
    NTSTATUS s;

    if(msec != INFINITE)
        interval.QuadPart = -((signed long)msec * 10000);
    else
        interval.QuadPart = MAX_WAIT_INTERVAL;

    s = NtWaitForSingleObject(h,FALSE,&interval);
    return (s == WAIT_OBJECT_0) ? 0 : (-1);
}

/**
 * @brief Releases a lock.
 * @param[in] h the lock's handle.
 * @return Zero for success,
 * a negative value otherwise.
 */
int winx_release_lock(HANDLE h)
{
    NTSTATUS s = NtSetEvent(h,NULL);
    return (s == STATUS_SUCCESS) ? 0 : (-1);
}

/**
 * @brief Destroys a lock.
 * @param[in] h the lock's handle.
 */
void winx_destroy_lock(HANDLE h)
{
    winx_destroy_event(h);
}

/** @} */
