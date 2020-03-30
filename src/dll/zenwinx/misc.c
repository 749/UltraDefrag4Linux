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
 * @file misc.c
 * @brief Miscellaneous system functions.
 * @addtogroup Miscellaneous
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Suspends the execution of the current thread.
 * @param[in] msec the time interval, in milliseconds.
 * If an INFINITE constant is passed, the time-out
 * interval never elapses.
 */
void winx_sleep(int msec)
{
    LARGE_INTEGER Interval;

    if(msec != INFINITE){
        /* System time units are 100 nanoseconds. */
        Interval.QuadPart = -((signed long)msec * 10000);
    } else {
        /* Approximately 292000 years hence */
        Interval.QuadPart = MAX_WAIT_INTERVAL;
    }
    /* don't litter debugging log in case of errors */
    (void)NtDelayExecution(0/*FALSE*/,&Interval);
}

/**
 * @brief Returns the version of Windows.
 * @return major_version_number * 10 + minor_version_number.
 * @note
 * - Works fine on NT 4.0 and later systems. Otherwise always returns 40.
 * - Useless on Windows 9x. Though, the complete zenwinx library is useless
 * there since there are many required calls missing in ntdll library 
 * on windows 9x.
 * @par Example:
 * @code 
 * if(winx_get_os_version() >= WINDOWS_XP){
 *     // we are running on XP or later system
 * }
 * @endcode
 */
int winx_get_os_version(void)
{
    /*NTSTATUS (__stdcall *func_RtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation);*/
    NTSTATUS (__stdcall *func_RtlGetVersion)(OSVERSIONINFOW *version_info);
    OSVERSIONINFOW ver;
    
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);

    if(winx_get_proc_address(L"ntdll.dll","RtlGetVersion",
      (void *)&func_RtlGetVersion) < 0) return 40;
    /* it seems to be impossible for it to fail */
    (void)func_RtlGetVersion(&ver);
    return (ver.dwMajorVersion * 10 + ver.dwMinorVersion);
}

/**
 * @brief Retrieves the path of the Windows directory.
 * @param[out] buffer pointer to the buffer
 * receiving the null-terminated path.
 * @param[in] length the length of the buffer, in characters.
 * @return Zero for success, negative value otherwise.
 * @note This function retrieves a native path, like this 
 *       \\??\\C:\\WINDOWS
 */
int winx_get_windows_directory(char *buffer, int length)
{
    short buf[MAX_PATH + 1];

    DbgCheck2(buffer,(length > 0),"winx_get_windows_directory",-1);
    
    if(winx_query_env_variable(L"SystemRoot",buf,MAX_PATH) < 0) return (-1);
    (void)_snprintf(buffer,length - 1,"\\??\\%ws",buf);
    buffer[length - 1] = 0;
    return 0;
}

/**
 * @brief Queries a symbolic link.
 * @param[in] name the name of symbolic link.
 * @param[out] buffer pointer to the buffer
 * receiving the null-terminated target.
 * @param[in] length of the buffer, in characters.
 * @return Zero for success, negative value otherwise.
 * @par Example:
 * @code
 * winx_query_symbolic_link(L"\\??\\C:",buffer,BUFFER_LENGTH);
 * // now the buffer may contain \Device\HarddiskVolume1 or something like that
 * @endcode
 */
int winx_query_symbolic_link(short *name, short *buffer, int length)
{
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING uStr;
    NTSTATUS Status;
    HANDLE hLink;
    ULONG size;

    DbgCheck3(name,buffer,(length > 0),"winx_query_symbolic_link",-1);
    
    RtlInitUnicodeString(&uStr,name);
    InitializeObjectAttributes(&oa,&uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
    Status = NtOpenSymbolicLinkObject(&hLink,SYMBOLIC_LINK_QUERY,&oa);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_query_symbolic_link: cannot open %ls",name);
        return (-1);
    }
    uStr.Buffer = buffer;
    uStr.Length = 0;
    uStr.MaximumLength = length * sizeof(short);
    size = 0;
    Status = NtQuerySymbolicLinkObject(hLink,&uStr,&size);
    (void)NtClose(hLink);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_query_symbolic_link: cannot query %ls",name);
        return (-1);
    }
    buffer[length - 1] = 0;
    return 0;
}

/**
 * @brief Sets a system error mode.
 * @param[in] mode the process error mode.
 * @return Zero for success, negative value otherwise.
 * @note 
 * - Mode constants aren't the same as in Win32 SetErrorMode() call.
 * - Use INTERNAL_SEM_FAILCRITICALERRORS constant to 
 *   disable the critical-error-handler message box. After that
 *   you can for example try to read a missing floppy disk without 
 *   any popup windows displaying error messages.
 * - winx_set_system_error_mode(1) call is equal to SetErrorMode(0).
 * - Other mode constants can be found in ReactOS sources, but
 *   they need to be tested carefully because they were never
 *   officially documented.
 * @par Example:
 * @code
 * winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS);
 * @endcode
 */
int winx_set_system_error_mode(unsigned int mode)
{
    NTSTATUS Status;

    Status = NtSetInformationProcess(NtCurrentProcess(),
                    ProcessDefaultHardErrorMode,
                    (PVOID)&mode,
                    sizeof(int));
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_set_system_error_mode: "
                "cannot set system error mode %u",mode);
        return (-1);
    }
    return 0;
}

/**
 * @brief Loads a driver.
 * @param[in] driver_name the name of the driver,
 * exactly as written in system registry.
 * @return Zero for success, negative value otherwise.
 * @note When the driver is already loaded this function
 * completes successfully.
 */
int winx_load_driver(short *driver_name)
{
    UNICODE_STRING us;
    short driver_key[MAX_PATH];
    NTSTATUS Status;

    DbgCheck1(driver_name,"winx_load_driver",-1);

    (void)_snwprintf(driver_key,MAX_PATH,L"%ls%ls",
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\",driver_name);
    driver_key[MAX_PATH - 1] = 0;
    RtlInitUnicodeString(&us,driver_key);
    Status = NtLoadDriver(&us);
    if(!NT_SUCCESS(Status) && Status != STATUS_IMAGE_ALREADY_LOADED){
        DebugPrintEx(Status,"winx_load_driver: cannot load %ws",driver_name);
        return (-1);
    }
    return 0;
}

/**
 * @brief Unloads a driver.
 * @param[in] driver_name the name of the driver,
 * exactly as written in system registry.
 * @return Zero for success, negative value otherwise.
 */
int winx_unload_driver(short *driver_name)
{
    UNICODE_STRING us;
    short driver_key[MAX_PATH];
    NTSTATUS Status;

    DbgCheck1(driver_name,"winx_unload_driver",-1);

    (void)_snwprintf(driver_key,MAX_PATH,L"%ls%ls",
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\",driver_name);
    driver_key[MAX_PATH - 1] = 0;
    RtlInitUnicodeString(&us,driver_key);
    Status = NtUnloadDriver(&us);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_unload_driver: cannot unload %ws",driver_name);
        return (-1);
    }
    return 0;
}

/**
 * @brief Retrieves the Windows boot options.
 * @return Pointer to Unicode string containing all Windows
 * boot options. NULL indicates failure.
 * @note After a use of returned string it should be freed
 * by winx_heap_free() call.
 */
short *winx_get_windows_boot_options(void)
{
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS status;
    HANDLE hKey;
    KEY_VALUE_PARTIAL_INFORMATION *data;
    short *data_buffer = NULL;
    DWORD data_size = 0;
    DWORD data_size2 = 0;
    DWORD data_length;
    BOOLEAN empty_value = FALSE;
    short *boot_options;
    int buffer_size;
    
    /* 1. open HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control registry key */
    RtlInitUnicodeString(&us,L"\\Registry\\Machine\\SYSTEM\\"
                             L"CurrentControlSet\\Control");
    InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
    status = NtOpenKey(&hKey,KEY_QUERY_VALUE,&oa);
    if(status != STATUS_SUCCESS){
        DebugPrintEx(status,"winx_get_windows_boot_options: cannot open %ws",us.Buffer);
        winx_printf("winx_get_windows_boot_options: cannot open %ls: %x\n\n",us.Buffer,(UINT)status);
        return NULL;
    }
    
    /* 2. read SystemStartOptions value */
    RtlInitUnicodeString(&us,L"SystemStartOptions");
    status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
            NULL,0,&data_size);
    if(status != STATUS_BUFFER_TOO_SMALL){
        DebugPrintEx(status,"winx_get_windows_boot_options: cannot query SystemStartOptions value size");
        winx_printf("winx_get_windows_boot_options: cannot query SystemStartOptions value size: %x\n\n",(UINT)status);
        return NULL;
    }
    data_size += sizeof(short);
    data = winx_heap_alloc(data_size);
    if(data == NULL){
        DebugPrint("winx_get_windows_boot_options: cannot allocate %u bytes of memory",data_size);
        winx_printf("winx_get_windows_boot_options: cannot allocate %u bytes of memory\n\n",data_size);
        return NULL;
    }
    
    RtlZeroMemory(data,data_size);
    status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
            data,data_size,&data_size2);
    if(status != STATUS_SUCCESS){
        DebugPrintEx(status,"winx_get_windows_boot_options: cannot query SystemStartOptions value");
        winx_printf("winx_get_windows_boot_options: cannot query SystemStartOptions value: %x\n\n",(UINT)status);
        winx_heap_free(data);
        return NULL;
    }
    data_buffer = (short *)(data->Data);
    data_length = data->DataLength >> 1;
    if(data_length == 0) empty_value = TRUE;
    
    if(!empty_value){
        data_buffer[data_length - 1] = 0;
        buffer_size = data_length * sizeof(short);
    } else {
        buffer_size = 1 * sizeof(short);
    }

    boot_options = winx_heap_alloc(buffer_size);
    if(!boot_options){
        DebugPrint("winx_get_windows_boot_options: cannot allocate %u bytes of memory",buffer_size);
        winx_printf("winx_get_windows_boot_options: cannot allocate %u bytes of memory\n\n",buffer_size);
        winx_heap_free(data);
        return NULL;
    }

    if(!empty_value){
        memcpy((void *)boot_options,(void *)data_buffer,buffer_size);
        DebugPrint("winx_get_windows_boot_options: %ls - %u",data_buffer,data_size);
        //winx_printf("winx_get_windows_boot_options: %ls - %u\n\n",data_buffer,data_size);
    } else {
        boot_options[0] = 0;
    }
    
    winx_heap_free(data);
    return boot_options;
}

/**
 * @brief Determines whether Windows is in Safe Mode or not.
 * @return Positive value indicates the presence of the Safe Mode.
 * Zero value indicates a normal boot. Negative value indicates
 * indeterminism caused by impossibility of an appropriate check.
 */
int winx_windows_in_safe_mode(void)
{
    short *boot_options;
    int safe_boot = 0;
    
    boot_options = winx_get_windows_boot_options();
    if(!boot_options) return (-1);
    
    /* search for SAFEBOOT */
    _wcsupr(boot_options);
    if(wcsstr(boot_options,L"SAFEBOOT")) safe_boot = 1;
    winx_heap_free(boot_options);

    return safe_boot;
}

/**
 * @internal
 * @brief Marks Windows boot as successful.
 * @note
 * - Based on http://www.osronline.com/showthread.cfm?link=185567
 * - Is used internally by winx_shutdown and winx_reboot.
 */
void MarkWindowsBootAsSuccessful(void)
{
    char bootstat_file_path[MAX_PATH];
    WINX_FILE *f_bootstat;
    char boot_success_flag = 1;
    
    /*
    * We have decided to avoid the use of related RtlXxx calls,
    * since they're undocumented (as usually), therefore may
    * bring us a lot of surprises.
    */
    if(winx_get_windows_directory(bootstat_file_path,MAX_PATH) < 0){
        DebugPrint("MarkWindowsBootAsSuccessful(): cannot retrieve the Windows directory path");
        winx_printf("\nMarkWindowsBootAsSuccessful(): cannot retrieve the Windows directory path\n\n");
        winx_sleep(3000);
        return;
    }
    (void)strncat(bootstat_file_path,"\\bootstat.dat",
            MAX_PATH - strlen(bootstat_file_path) - 1);

    /* open the bootstat.dat file */
    f_bootstat = winx_fopen(bootstat_file_path,"r+");
    if(f_bootstat == NULL){
        /* it seems that we have system prior to XP */
        return;
    }

    /* set BootSuccessFlag to 0x1 (look at BOOT_STATUS_DATA definition in ntndk.h for details) */
    f_bootstat->woffset.QuadPart = 0xa;
    (void)winx_fwrite(&boot_success_flag,sizeof(char),1,f_bootstat);
    
    /* close the file */
    winx_fclose(f_bootstat);
}

/** @} */
