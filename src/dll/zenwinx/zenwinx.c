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
 * @file zenwinx.c
 * @brief Startup and shutdown.
 * @addtogroup StartupAndShutdown
 * @{
 */

#include "zenwinx.h"

void kb_close(void);
int winx_create_global_heap(void);
void winx_destroy_global_heap(void);
int winx_init_synch_objects(void);
void winx_destroy_synch_objects(void);
void MarkWindowsBootAsSuccessful(void);
char *winx_get_error_description(unsigned long status);
void flush_dbg_log(int already_synchronized);

/**
 * @internal
 * @brief Internal variable indicating
 * whether library initialization failed
 * or not. Intended to be checked by
 * winx_init_failed routine.
 */
int initialization_failed = 1;

#ifndef STATIC_LIB
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
    if(dwReason == DLL_PROCESS_ATTACH){
        winx_init_library(NULL);
    } else if(dwReason == DLL_PROCESS_DETACH){
        winx_unload_library();
    }
    return 1;
}
#endif

/**
 * @brief Initializes zenwinx library.
 * @details If the library is linked statically
 * this routine needs to be called explicitly
 * before any use of other routines (except a few
 * ones like winx_print).
 * @param[in] peb pointer to the Process Environment Block.
 * May be NULL.
 * @return Zero for success, negative value otherwise.
 * @par Example 1 - a native application:
 * @code
 * void __stdcall NtProcessStartup(PPEB Peb)
 * {
 *     winx_init_library(Peb);
 *     winx_kb_init();
 *
 *     // do something
 *     winx_printf("Hello!");
 *     winx_sleep(1000);
 *
 *     winx_exit(0);
 * }
 * @endcode
 * @par Example 2 - a command line tool (zenwinx is linked statically):
 * @code
 * int main(void)
 * {
 *     winx_init_library(NULL);
 *
 *     // do something
 *     printf("Hello!");
 *     winx_sleep(1000);
 *
 *     winx_unload_library();
 *     return 0;
 * }
 * @endcode
 * @par Example 3 - a command line tool (zenwinx is linked dynamically):
 * @code
 * int main(void)
 * {
 *     // do something
 *     printf("Hello!");
 *     winx_sleep(1000);
 *
 *     return 0;
 * }
 * @endcode
 */
int winx_init_library(void *peb)
{
    PRTL_USER_PROCESS_PARAMETERS pp;

    /*  normalize and get the process parameters */
    if(peb){
        pp = RtlNormalizeProcessParams(((PPEB)peb)->ProcessParameters);
        /* breakpoint if we were requested to do so */
        if(pp){
            if(pp->DebugFlags)
                DbgBreakPoint();
        }
    }

    if(winx_create_global_heap() < 0)
        return (-1);
    if(winx_init_synch_objects() < 0)
        return (-1);
    initialization_failed = 0;
    return 0;
}

/**
 * @brief Defines whether zenwinx library has 
 * been initialized successfully or not.
 * @details Useful for non-native applications
 * making no direct call to winx_init_library.
 * @return Boolean value.
 */
int winx_init_failed(void)
{
    return initialization_failed;
}

/**
 * @brief Frees resources allocated by zenwinx library.
 * @details If the library is linked statically
 * to the non-native application this routine 
 * needs to be called explicitly.
 */
void winx_unload_library(void)
{
    winx_destroy_synch_objects();
    winx_destroy_global_heap();
}

/**
 * @internal
 * @brief Displays error message when
 * either debug print or memory
 * allocation may be not available.
 * @param[in] msg the error message.
 * @param[in] Status the NT status code.
 * @note Intended to be used after winx_exit,
 * winx_shutdown and winx_reboot failures.
 */
static void print_post_scriptum(char *msg,NTSTATUS Status)
{
    char buffer[256];

    _snprintf(buffer,sizeof(buffer),"\n%s: %x: %s\n\n",
        msg,(UINT)Status,winx_get_error_description(Status));
    buffer[sizeof(buffer) - 1] = 0;
    /* winx_printf cannot be used here */
    winx_print(buffer);
}

/**
 * @brief Terminates the calling native process.
 * @details This routine releases all resources
 * used by zenwinx library before the process termination.
 * @param[in] exit_code the exit status.
 */
void winx_exit(int exit_code)
{
    NTSTATUS Status;
    
    kb_close();
    flush_dbg_log(0);
    winx_unload_library();
    Status = NtTerminateProcess(NtCurrentProcess(),exit_code);
    if(!NT_SUCCESS(Status)){
        print_post_scriptum("winx_exit: cannot terminate process",Status);
    }
}

/**
 * @brief Reboots the computer.
 * @note If SE_SHUTDOWN privilege adjusting fails
 * then the computer will not be rebooted and the program 
 * will continue the execution after this call.
 */
void winx_reboot(void)
{
    NTSTATUS Status;
    
    kb_close();
    MarkWindowsBootAsSuccessful();
    (void)winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
    flush_dbg_log(0);
    Status = NtShutdownSystem(ShutdownReboot);
    if(!NT_SUCCESS(Status)){
        print_post_scriptum("winx_reboot: cannot reboot the computer",Status);
    }
}

/**
 * @brief Shuts down the computer.
 * @note If SE_SHUTDOWN privilege adjusting fails
 * then the computer will not be shut down and the program 
 * will continue the execution after this call.
 */
void winx_shutdown(void)
{
    NTSTATUS Status;
    
    kb_close();
    MarkWindowsBootAsSuccessful();
    (void)winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
    flush_dbg_log(0);
    Status = NtShutdownSystem(ShutdownPowerOff);
    if(!NT_SUCCESS(Status)){
        print_post_scriptum("winx_shutdown: cannot shut down the computer",Status);
    }
}

/** @} */
