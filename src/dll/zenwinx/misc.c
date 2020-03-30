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
    typedef NTSTATUS (__stdcall *RTLGETVERSION_PROC)(OSVERSIONINFOW *version_info);
    RTLGETVERSION_PROC pRtlGetVersion;
    OSVERSIONINFOW ver;
    
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);

    pRtlGetVersion = (RTLGETVERSION_PROC)winx_get_proc_address(L"ntdll.dll","RtlGetVersion");
    if(pRtlGetVersion == NULL) return 40;
    /* it seems to be impossible for it to fail */
    (void)pRtlGetVersion(&ver);
    return (ver.dwMajorVersion * 10 + ver.dwMinorVersion);
}

/**
 * @brief Retrieves the path of the Windows directory.
 * @return The native path of the Windows directory.
 * NULL indicates failure.
 * @note 
 * - This function retrieves the native path, like this 
 *       \\??\\C:\\WINDOWS
 * - The returned string should be freed by the winx_free
 * call after its use.
 */
wchar_t *winx_get_windows_directory(void)
{
    wchar_t *windir;
    wchar_t *path = NULL;
    int size, result;

    windir = winx_getenv(L"SystemRoot");
    if(windir){
        winx_swprintf(path,size,result,L"\\??\\%ws",windir);
        if(path == NULL)
            mtrace();
        winx_free(windir);
    }
    return path;
}

/**
 * @brief Queries a symbolic link.
 * @param[in] name the symbolic link name.
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
int winx_query_symbolic_link(wchar_t *name, wchar_t *buffer, int length)
{
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING us;
    NTSTATUS status;
    HANDLE hLink;
    ULONG size;

    DbgCheck3(name,buffer,(length > 0),-1);
    
    RtlInitUnicodeString(&us,name);
    InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
    status = NtOpenSymbolicLinkObject(&hLink,SYMBOLIC_LINK_QUERY,&oa);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot open %ls",name);
        return (-1);
    }
    us.Buffer = buffer;
    us.Length = 0;
    us.MaximumLength = length * sizeof(wchar_t);
    size = 0;
    status = NtQuerySymbolicLinkObject(hLink,&us,&size);
    (void)NtClose(hLink);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot query %ls",name);
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
 * - The mode constants aren't the same as in Win32 SetErrorMode() call.
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
    NTSTATUS status;

    status = NtSetInformationProcess(NtCurrentProcess(),
                    ProcessDefaultHardErrorMode,
                    (PVOID)&mode,
                    sizeof(int));
    if(!NT_SUCCESS(status)){
        strace(status,"cannot set system error mode %u",mode);
        return (-1);
    }
    return 0;
}

/**
 * @brief Retrieves the Windows boot options.
 * @return Pointer to Unicode string containing all Windows
 * boot options. NULL indicates failure.
 * @note After a use of the returned string it should be freed
 * by winx_free() call.
 */
wchar_t *winx_get_windows_boot_options(void)
{
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS status;
    HANDLE hKey;
    KEY_VALUE_PARTIAL_INFORMATION *data;
    wchar_t *data_buffer = NULL;
    DWORD data_size = 0;
    DWORD data_size2 = 0;
    DWORD data_length;
    BOOLEAN empty_value = FALSE;
    wchar_t *boot_options;
    int buffer_size;
    
    /* 1. open HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control registry key */
    RtlInitUnicodeString(&us,L"\\Registry\\Machine\\SYSTEM\\"
                             L"CurrentControlSet\\Control");
    InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
    status = NtOpenKey(&hKey,KEY_QUERY_VALUE,&oa);
    if(status != STATUS_SUCCESS){
        strace(status,"cannot open %ws",us.Buffer);
        winx_printf("%s: cannot open %ls: %x\n\n",__FUNCTION__,us.Buffer,(UINT)status);
        return NULL;
    }
    
    /* 2. read SystemStartOptions value */
    RtlInitUnicodeString(&us,L"SystemStartOptions");
    status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
            NULL,0,&data_size);
    if(status != STATUS_BUFFER_TOO_SMALL){
        strace(status,"cannot query SystemStartOptions value size");
        winx_printf("%s: cannot query SystemStartOptions value size: %x\n\n",__FUNCTION__,(UINT)status);
        return NULL;
    }
    data_size += sizeof(wchar_t);
    data = winx_tmalloc(data_size);
    if(data == NULL){
        etrace("cannot allocate %u bytes of memory",data_size);
        winx_printf("%s: cannot allocate %u bytes of memory\n\n",__FUNCTION__,data_size);
        return NULL;
    }
    
    RtlZeroMemory(data,data_size);
    status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
            data,data_size,&data_size2);
    if(status != STATUS_SUCCESS){
        strace(status,"cannot query SystemStartOptions value");
        winx_printf("%s: cannot query SystemStartOptions value: %x\n\n",__FUNCTION__,(UINT)status);
        winx_free(data);
        return NULL;
    }
    data_buffer = (wchar_t *)(data->Data);
    data_length = data->DataLength >> 1;
    if(data_length == 0) empty_value = TRUE;
    
    if(!empty_value){
        data_buffer[data_length - 1] = 0;
        buffer_size = data_length * sizeof(wchar_t);
    } else {
        buffer_size = 1 * sizeof(wchar_t);
    }

    boot_options = winx_tmalloc(buffer_size);
    if(!boot_options){
        etrace("cannot allocate %u bytes of memory",buffer_size);
        winx_printf("%s: cannot allocate %u bytes of memory\n\n",__FUNCTION__,buffer_size);
        winx_free(data);
        return NULL;
    }

    if(!empty_value){
        memcpy((void *)boot_options,(void *)data_buffer,buffer_size);
        itrace("%ls - %u",data_buffer,data_size);
        //winx_printf("%s: %ls - %u\n\n",__FUNCTION__,data_buffer,data_size);
    } else {
        boot_options[0] = 0;
    }
    
    winx_free(data);
    return boot_options;
}

/**
 * @brief Determines whether Windows is in Safe Mode or not.
 * @return Positive value indicates the presence of the Safe Mode.
 * Zero value indicates a normal boot. Negative value indicates
 * indeterminism caused by impossibility of the appropriate check.
 */
int winx_windows_in_safe_mode(void)
{
    wchar_t *boot_options;
    int safe_boot = 0;
    
    boot_options = winx_get_windows_boot_options();
    if(!boot_options) return (-1);
    
    /* search for SAFEBOOT */
    _wcsupr(boot_options);
    if(wcsstr(boot_options,L"SAFEBOOT")) safe_boot = 1;
    winx_free(boot_options);

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
    wchar_t *windir;
    wchar_t path[MAX_PATH + 1];
    WINX_FILE *f;
    char boot_success_flag = 1;
    
    /*
    * We have decided to avoid the use of the related RtlXxx calls,
    * since they're undocumented (as usually), therefore may
    * bring us a lot of surprises.
    */
    windir = winx_get_windows_directory();
    if(windir == NULL){
        etrace("cannot retrieve the Windows directory path");
        winx_printf("\n%s: cannot retrieve the Windows directory path\n\n",__FUNCTION__);
        winx_sleep(3000);
        return;
    }
    _snwprintf(path,MAX_PATH,L"%ws\\bootstat.dat",windir);
    path[MAX_PATH] = 0;
    winx_free(windir);

    /*
    * Set BootSuccessFlag to 0x1 (look at 
    * BOOT_STATUS_DATA definition in ntndk.h
    * file for details).
    */
    f = winx_fopen(path,"r+");
    if(f != NULL){
        f->woffset.QuadPart = 0xa;
        (void)winx_fwrite(&boot_success_flag,sizeof(char),1,f);
        winx_fclose(f);
    } else {
        /* It seems that we have system prior to XP. */
    }
}

/** @} */
