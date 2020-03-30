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
 * @file lock.c
 * @brief Locks.
 * @details Spin locks are intended for thread-safe
 * synchronization of access to shared data.
 * @addtogroup Locks
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/*
* winx_acquire_spin_lock and winx_release_spin_lock
* are used in debugging routines, therefore winx_dbg_xxx
* calls must be avoided there to avoid recursion.
*/

/**
 * @brief Initializes a spin lock.
 * @param[in] name the spin lock name.
 * @return Pointer to the intialized spin lock.
 * NULL indicates failure.
 */
winx_spin_lock *winx_init_spin_lock(char *name)
{
    wchar_t *fullname;
    unsigned int id;
    winx_spin_lock *sl;
    
    /* attach PID to lock the current process only */
    id = (unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess);
    fullname = winx_swprintf(L"\\%hs_%u",name,id);
    if(fullname == NULL){
        etrace("not enough memory for %s",name);
        return NULL;
    }

    sl = winx_malloc(sizeof(winx_spin_lock));
    
    if(winx_create_event(fullname,SynchronizationEvent,&sl->hEvent) < 0){
        etrace("cannot create synchronization event");
        winx_free(sl);
        winx_free(fullname);
        return NULL;
    }
    
    winx_free(fullname);
    
    if(winx_release_spin_lock(sl) < 0){
        winx_destroy_event(sl->hEvent);
        winx_free(sl);
        return NULL;
    }
    
    return sl;
}

/**
 * @brief Acquires a spin lock.
 * @param[in] sl pointer to the spin lock.
 * @param[in] msec the timeout interval.
 * If INFINITE constant is passed, 
 * the interval never elapses.
 * @return Zero for success,
 * negative value otherwise.
 */
int winx_acquire_spin_lock(winx_spin_lock *sl,int msec)
{
    LARGE_INTEGER interval;
    NTSTATUS status;

    if(sl == NULL)
        return (-1);
    
    if(sl->hEvent == NULL)
        return (-1);

    if(msec != INFINITE)
        interval.QuadPart = -((signed long)msec * 10000);
    else
        interval.QuadPart = MAX_WAIT_INTERVAL;
    status = NtWaitForSingleObject(sl->hEvent,FALSE,&interval);
    if(status != WAIT_OBJECT_0)
        return (-1);

    return 0;
}

/**
 * @brief Releases a spin lock.
 * @return Zero for success,
 * negative value otherwise.
 */
int winx_release_spin_lock(winx_spin_lock *sl)
{
    NTSTATUS status;
    
    if(sl == NULL)
        return (-1);
    
    if(sl->hEvent == NULL)
        return (-1);
    
    status = NtSetEvent(sl->hEvent,NULL);
    if(status != STATUS_SUCCESS)
        return (-1);
    
    return 0;
}

/**
 * @brief Destroys a spin lock.
 */
void winx_destroy_spin_lock(winx_spin_lock *sl)
{
    if(sl){
        winx_destroy_event(sl->hEvent);
        winx_free(sl);
    }
}

/** @} */
