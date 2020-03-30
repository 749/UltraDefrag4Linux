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
 * @file zenwinx.c
 * @brief Startup and shutdown.
 * @addtogroup StartupAndShutdown
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

void winx_init_case_tables(void);
int winx_create_global_heap(void);
void winx_destroy_global_heap(void);
int winx_dbg_init(void);
void winx_dbg_close(void);
void MarkWindowsBootAsSuccessful(void);
char *winx_get_status_description(unsigned long status);
void kb_close(void);

/**
 * @brief Initializes zenwinx library.
 * @details This routine needs to be called
 * before any use of other routines (except
 * a few ones like winx_print).
 * @return Zero for success, negative value otherwise.
 */
int winx_init_library(void)
{
    winx_init_case_tables();
    if(winx_create_global_heap() < 0)
        return (-1);
    if(winx_dbg_init() < 0)
        return (-1);
    return 0;
}

/**
 * @brief Releases resources 
 * acquired by zenwinx library.
 * @note Call it ONLY if you know
 * what you're doing.
 */
void winx_unload_library(void)
{
    winx_dbg_close();
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
        msg,(UINT)Status,winx_get_status_description(Status));
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
    winx_flush_dbg_log(0);
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
    winx_flush_dbg_log(0);
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
    winx_flush_dbg_log(0);
    Status = NtShutdownSystem(ShutdownPowerOff);
    if(!NT_SUCCESS(Status)){
        print_post_scriptum("winx_shutdown: cannot shut down the computer",Status);
    }
}

/** @} */
