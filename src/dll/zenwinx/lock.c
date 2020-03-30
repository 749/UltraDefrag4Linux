/*
 *  ZenWINX - WIndows Native eXtended library.
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
 * @file lock.c
 * @brief Locks.
 * @details Spin locks are intended for thread-safe
 * synchronization of access to shared data.
 * @addtogroup Locks
 * @{
 */

#include "zenwinx.h"

/*
* winx_acquire_spin_lock and winx_release_spin_lock
* are used in debugging routines, therefore winx_dbg_xxx
* calls must be avoided there to avoid recursion.
*/

/**
 * @brief Initializes a spin lock.
 * @param[in] name the name of the spin lock.
 * @return Pointer to intialized spin lock.
 * NULL indicates failure.
 */
winx_spin_lock *winx_init_spin_lock(char *name)
{
    winx_spin_lock *sl;
    char *buffer;
    int length;
    wchar_t *fullname;
    
    /* attach PID to lock the current process only */
    buffer = winx_sprintf("\\%s_%u",name,
        (unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess));
    if(buffer == NULL){
        DebugPrint("winx_init_spin_lock: not enough memory for ansi full name");
        return NULL;
    }

    length = strlen(buffer);
    fullname = winx_heap_alloc((length + 1) * sizeof(wchar_t));
    if(fullname == NULL){
        DebugPrint("winx_init_spin_lock: not enough memory for unicode full name");
        winx_heap_free(buffer);
        return NULL;
    }
    
    if(_snwprintf(fullname,length + 1,L"%hs",buffer) < 0){
        DebugPrint("winx_init_spin_lock: full name conversion to unicode failed");
        winx_heap_free(buffer);
        return NULL;
    }
    
    fullname[length] = 0;
    winx_heap_free(buffer);
    
    sl = winx_heap_alloc(sizeof(winx_spin_lock));
    if(sl == NULL){
        DebugPrint("winx_init_spin_lock: cannot allocate memory for %s",name);
        winx_heap_free(fullname);
        return NULL;
    }
    
    if(winx_create_event(fullname,SynchronizationEvent,&sl->hEvent) < 0){
        DebugPrint("winx_init_spin_lock: cannot create synchronization event");
        winx_heap_free(sl);
        winx_heap_free(fullname);
        return NULL;
    }
    
    winx_heap_free(fullname);
    
    if(winx_release_spin_lock(sl) < 0){
        winx_destroy_event(sl->hEvent);
        winx_heap_free(sl);
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
        if(sl->hEvent)
            winx_destroy_event(sl->hEvent);
        winx_heap_free(sl);
    }
}

/** @} */
