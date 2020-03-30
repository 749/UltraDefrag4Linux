//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2016 Dmitri Arkhangelski (dmitriar@gmail.com).
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//////////////////////////////////////////////////////////////////////////

/**
 * @file crash.cpp
 * @brief Crash handling.
 * @addtogroup CrashHandling
 * @{
 */

// =======================================================================
//                            Declarations
// =======================================================================

#include <signal.h>
#include "main.h"

#define STATUS_FATAL_APP_EXIT        0x40000015
#define STATUS_HEAP_CORRUPTION       0xC0000374
#ifndef STATUS_STACK_BUFFER_OVERRUN
#define STATUS_STACK_BUFFER_OVERRUN  0xC0000409
#endif

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo);
void __cdecl AbortHandler(int signal);

// =======================================================================
//                          Global variables
// =======================================================================

udefrag_shared_data *sd = NULL;

#define MAX_CMD_LENGTH 32768
wchar_t cmd[MAX_CMD_LENGTH];

// =======================================================================
//                           Crash handling
// =======================================================================

/**
 * @brief Attaches UltraDefrag debugger.
 */
void App::AttachDebugger(void)
{
    HANDLE hEvent = NULL;

    // launch UltraDefrag debugger
    STARTUPINFO si; PROCESS_INFORMATION pi;
    memset(&si,0,sizeof(si)); si.cb = sizeof(si);
    si.dwFlags = STARTF_FORCEOFFFEEDBACK;
    memset(&pi,0,sizeof(pi));
    wcscpy(cmd,wxT("udefrag-dbg.exe"));
    if(CreateProcess(NULL,cmd,NULL,NULL,
      FALSE,0,NULL,NULL,&si,&pi) == FALSE){
        letrace("cannot launch UltraDefrag debugger");
        return;
    }

    // set up shared memory to transfer debugging information
    wchar_t path[64]; int id = pi.dwProcessId;
    _snwprintf(path,64,wxT("udefrag-shared-mem-%u"),id);
    HANDLE hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE,
        NULL,PAGE_READWRITE,0,sizeof(udefrag_shared_data),path);
    if(hSharedMemory == NULL){
        letrace("cannot set up shared memory");
        goto done;
    }
    sd = (udefrag_shared_data *)MapViewOfFile(hSharedMemory,
        FILE_MAP_ALL_ACCESS,0,0,sizeof(udefrag_shared_data));
    if(sd == NULL){
        letrace("cannot map shared memory");
        CloseHandle(hSharedMemory);
        goto done;
    }

    // initialize shared data
    sd->ready = false;

    // create synchronization event
    _snwprintf(path,64,wxT("udefrag-synch-event-%u"),id);
    hEvent = CreateEvent(NULL,FALSE,FALSE,path);
    if(hEvent == NULL){
        letrace("cannot create synchronization event");
        UnmapViewOfFile(sd); sd = NULL;
        CloseHandle(hSharedMemory);
        goto done;
    }

    // force debugger to acquire shared objects right now
    if(DuplicateHandle(GetCurrentProcess(),hSharedMemory,
      pi.hProcess,NULL,FILE_MAP_ALL_ACCESS,FALSE,0) == FALSE){
        letrace("cannot duplicate shared memory handle");
    }
    if(DuplicateHandle(GetCurrentProcess(),hEvent,
      pi.hProcess,NULL,EVENT_ALL_ACCESS,FALSE,0) == FALSE){
        letrace("cannot duplicate synchronization event handle");
    }

    // memory corruption will either raise a structured
    // exception which we'll catch directly via our custom
    // exception handler or CRT will detect it and call
    // abort() routine instead

#if !defined(__GNUC__)
    // force abort() to pass control to our exception handler
    _set_abort_behavior(_CALL_REPORTFAULT,_CALL_REPORTFAULT);
    _set_abort_behavior(0,_WRITE_ABORT_MSG);
#endif

    // catch SIGABRT, just for safety
    (void)signal(SIGABRT,AbortHandler);

    // still, on MinGW sometimes abnormal termination will
    // not be caught, especially in console applications
    // which tend to hang instead (tested on x86 Windows 7)

    // set our custom exception handler
    AddVectoredExceptionHandler(1, \
        (PVECTORED_EXCEPTION_HANDLER) \
        ExceptionHandler
    );

done:
    // cleanup
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

/**
 * @brief Handles SIGABRT signal.
 */
void __cdecl AbortHandler(int signal)
{
    RaiseException(STATUS_FATAL_APP_EXIT,0,0,NULL);
}

/**
 * @brief Handles all exceptions.
 * @note Synchronization API shouldn't
 * be used here as Windows may suspend
 * execution of other processes/threads
 * meanwhile.
 */
LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
    DWORD exception_code = ExceptionInfo-> \
        ExceptionRecord->ExceptionCode;
    void *exception_address = ExceptionInfo-> \
        ExceptionRecord->ExceptionAddress;

    switch(exception_code){
    case EXCEPTION_ACCESS_VIOLATION:
    case STATUS_HEAP_CORRUPTION:
    case STATUS_FATAL_APP_EXIT:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_GUARD_PAGE:
    case EXCEPTION_STACK_OVERFLOW:
    case STATUS_STACK_BUFFER_OVERRUN:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        // handle the most interesting cases
        if(sd){
            // send crash report
            sd->version = SHARED_DATA_VERSION;
            sd->exception_code = exception_code;
            sd->exception_address = exception_address;
            wcscpy(sd->tracking_id,TRACKING_ID);
            sd->ready = true;

            // terminate process safely
            TerminateProcess(GetCurrentProcess(),3);
        }
        break;
    default:
        // pass on everything else
        // to frame based handlers
        break;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

/** @} */
