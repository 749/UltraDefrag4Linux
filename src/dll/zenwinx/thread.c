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
 * @file thread.c
 * @brief Threads.
 * @addtogroup Threads
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Creates a thread and starts them.
 * @param[in] start_addr the starting address of the thread.
 * @param[in] parameter pointer to data passed to thread routine.
 * @param[out] phandle the thread handle pointer. May be NULL.
 * @return Zero for success, negative value otherwise.
 * @note Look at the following example for the thread function prototype.
 * @par Example:
 * @code
 * DWORD WINAPI thread_proc(LPVOID parameter)
 * {
 *     // do something
 *     winx_exit_thread(0);
 *     return 0;
 * }
 * winx_create_thread(thread_proc,NULL);
 * @endcode
 */
int winx_create_thread(PTHREAD_START_ROUTINE start_addr,PVOID parameter,HANDLE *phandle)
{
    NTSTATUS Status;
    HANDLE hThread;
    HANDLE *ph;

    DbgCheck1(start_addr,"winx_create_thread",-1);

    /*
    * Implementation is very easy, because we have required call
    * on all of the supported versions of Windows.
    */
    if(phandle) ph = phandle;
    else ph = &hThread;
    Status = RtlCreateUserThread(NtCurrentProcess(),NULL,
                    0,0,0,0,start_addr,parameter,ph,NULL);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_create_thread: cannot create thread");
        return (-1);
    }
    if(phandle == NULL) NtCloseSafe(*ph);
    return 0;
}

/**
 * @brief Terminates the current thread.
 * @param[in] status the exit status.
 * @note This routine causes a small memory leak,
 * because it doesn't deallocate the initial stack.
 * On the other hand, such deallocation seems to be
 * not easy and even if we'll find a proper solution
 * for, let's say XP, we cannot guarantee its work
 * on other systems.
 */
void winx_exit_thread(NTSTATUS status)
{
    NTSTATUS Status = ZwTerminateThread(NtCurrentThread(),status);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_exit_thread: cannot terminate thread");
    }
}

/** @} */
