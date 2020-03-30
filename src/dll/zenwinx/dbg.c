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
 * @file dbg.c
 * @brief Debugging.
 * @details All the debugging messages will be delivered
 * to the Debug View program whenever possible. If logging
 * to the file is enabled, they will be saved there too.
 * Size of the log is limited by available memory. This
 * technique prevents the log file update on disk, thus
 * it makes the disk defragmentation more efficient.
 * @note A few prefixes are defined for the debugging
 * messages. They're listed in ../../include/dbg.h
 * file and are intended for easier analysis of logs. To keep
 * logs clean always use one of those prefixes.
 * @addtogroup Debug
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/*
* Note: to avoid recursion all the code in this file should
* call with care either winx_malloc (handling the out of memory
* condition in a special way) or tracing functions as well.
*/

/* controls whether the messages will be collected or not */
int logging_enabled = 0;

/**
 * @internal
 * @brief Describes
 * a log list entry.
 */
typedef struct _winx_dbg_log_entry {
    struct _winx_dbg_log_entry *next;
    struct _winx_dbg_log_entry *prev;
    winx_time time_stamp;
    char *buffer;
} winx_dbg_log_entry;

/* all the messages will be collected to this list */
winx_dbg_log_entry *dbg_log = NULL;

wchar_t *log_path = NULL;
HANDLE hListSynchEvent = NULL;
HANDLE hLogSynchEvent = NULL;

extern char *reserved_memory;

/*
**************************************************************
*                   auxiliary functions                      *
*     (independent from winx_malloc and tracing calls)       *
**************************************************************
*/

list_entry *dbg_list_insert(list_entry **phead,list_entry *prev,long size)
{
    list_entry *new_item;
    
    new_item = (list_entry *)winx_tmalloc(size);
    if(new_item == NULL)
        return new_item;

    /* is list empty? */
    if(*phead == NULL){
        *phead = new_item;
        new_item->prev = new_item->next = new_item;
        return new_item;
    }

    /* insert as the new head? */
    if(prev == NULL){
        prev = (*phead)->prev;
        *phead = new_item;
    }

    /* insert after the item specified by prev argument */
    new_item->prev = prev;
    new_item->next = prev->next;
    new_item->prev->next = new_item;
    new_item->next->prev = new_item;
    return new_item;
}

int dbg_get_local_time(winx_time *t)
{
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;
    NTSTATUS status;
    
    status = NtQuerySystemTime(&SystemTime);
    if(status != STATUS_SUCCESS) return (-1);
    
    status = RtlSystemTimeToLocalTime(&SystemTime,&LocalTime);
    if(status != STATUS_SUCCESS) return (-1);
    
    RtlTimeToTimeFields(&LocalTime,&TimeFields);
    t->year = TimeFields.Year;
    t->month = TimeFields.Month;
    t->day = TimeFields.Day;
    t->hour = TimeFields.Hour;
    t->minute = TimeFields.Minute;
    t->second = TimeFields.Second;
    t->milliseconds = TimeFields.Milliseconds;
    t->weekday = TimeFields.Weekday;
    return 0;
}


/**
 * @internal
 * @brief Initializes the
 * debugging facility.
 * @return Zero for success,
 * negative value otherwise.
 */
int winx_dbg_init(void)
{
    unsigned int id;
    wchar_t *name;
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS status;

    if(!hListSynchEvent){
        /* attach PID to lock the current process only */
        id = (unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess);
        name = winx_swprintf(L"\\winx_dbg_list_lock_%u",id);
        if(name == NULL) return (-1);
        
        RtlInitUnicodeString(&us,name);
        InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
        status = NtCreateEvent(&hListSynchEvent,STANDARD_RIGHTS_ALL | 0x1ff,
            &oa,SynchronizationEvent,1);
        if(!NT_SUCCESS(status)){
            hListSynchEvent = NULL;
            return (-1);
        }
    }
    if(!hLogSynchEvent){
        /* attach PID to lock the current process only */
        id = (unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess);
        name = winx_swprintf(L"\\winx_dbg_log_lock_%u",id);
        if(name == NULL){
            NtCloseSafe(hListSynchEvent);
            return (-1);
        }
        
        RtlInitUnicodeString(&us,name);
        InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
        status = NtCreateEvent(&hLogSynchEvent,STANDARD_RIGHTS_ALL | 0x1ff,
            &oa,SynchronizationEvent,1);
        if(!NT_SUCCESS(status)){
            hLogSynchEvent = NULL;
            NtCloseSafe(hListSynchEvent);
            return (-1);
        }
    }
    return 0;
}

/**
 * @internal
 * @brief Deinitializes
 * the debugging facility.
 */
void winx_dbg_close(void)
{
    winx_flush_dbg_log(0);
    if(log_path){
        winx_free(log_path);
        log_path = NULL;
    }
    NtCloseSafe(hListSynchEvent);
    NtCloseSafe(hLogSynchEvent);
}

/**
 * @internal
 * @brief Appends a string to the log list.
 */
static void add_dbg_log_entry(char *msg)
{
    LARGE_INTEGER interval;
    winx_dbg_log_entry *new_log_entry = NULL;
    winx_dbg_log_entry *last_log_entry = NULL;

    /* synchronize with other threads */
    interval.QuadPart = MAX_WAIT_INTERVAL;
    if(NtWaitForSingleObject(hListSynchEvent,FALSE,&interval) == WAIT_OBJECT_0){
        if(logging_enabled){
            if(dbg_log) last_log_entry = dbg_log->prev;
            new_log_entry = (winx_dbg_log_entry *)dbg_list_insert((list_entry **)(void *)&dbg_log,
                (list_entry *)last_log_entry,sizeof(winx_dbg_log_entry));
            if(new_log_entry == NULL){
                /* not enough memory */
            } else {
                new_log_entry->buffer = winx_strdup(msg);
                if(new_log_entry->buffer == NULL){
                    /* not enough memory */
                    winx_list_remove((list_entry **)(void *)&dbg_log,(list_entry *)new_log_entry);
                } else {
                    memset(&new_log_entry->time_stamp,0,sizeof(winx_time));
                    (void)dbg_get_local_time(&new_log_entry->time_stamp);
                }
            }
        }
        NtSetEvent(hListSynchEvent,NULL);
    }
}

/**
 * @internal
 * @brief Internal structure used to deliver
 * information to the Debug View program.
 */
typedef struct _DBG_OUTPUT_DEBUG_STRING_BUFFER {
    ULONG ProcessId;
    char  Msg[4096-sizeof(ULONG)];
} DBG_OUTPUT_DEBUG_STRING_BUFFER, *PDBG_OUTPUT_DEBUG_STRING_BUFFER;

#define DBG_OUT_BUFFER_SIZE (4096-sizeof(ULONG))

/**
 * @internal
 * @brief Low-level routine for delivering
 * of debugging messages to the Debug View program.
 * @details OutputDebugString is not safe - being called
 * from DllMain it might crash the application (confirmed on w2k).
 * Because of that we're maintaining this routine.
 */
static void deliver_message(char *string)
{
    HANDLE hEvtBufferReady = NULL;
    HANDLE hEvtDataReady = NULL;
    HANDLE hSection = NULL;
    LPVOID BaseAddress = NULL;
    LARGE_INTEGER SectionOffset;
    ULONG ViewSize = 0;
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS Status;
    LARGE_INTEGER interval;
    DBG_OUTPUT_DEBUG_STRING_BUFFER *dbuffer;
    int length;
    
    /* open debugger's objects */
    RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_BUFFER_READY");
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    Status = NtOpenEvent(&hEvtBufferReady,SYNCHRONIZE,&oa);
    if(!NT_SUCCESS(Status)) goto done;
    
    RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_DATA_READY");
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    Status = NtOpenEvent(&hEvtDataReady,EVENT_MODIFY_STATE,&oa);
    if(!NT_SUCCESS(Status)) goto done;

    RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_BUFFER");
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    Status = NtOpenSection(&hSection,SECTION_ALL_ACCESS,&oa);
    if(!NT_SUCCESS(Status)) goto done;
    SectionOffset.QuadPart = 0;
    Status = NtMapViewOfSection(hSection,NtCurrentProcess(),
        &BaseAddress,0,0,&SectionOffset,(SIZE_T *)&ViewSize,ViewShare,
        0,PAGE_READWRITE);
    if(!NT_SUCCESS(Status)) goto done;
    
    /* send a message */
    /*
    * wait a maximum of 10 seconds for the debug monitor 
    * to finish processing the shared buffer
    */
    interval.QuadPart = -(10000 * 10000);
    if(NtWaitForSingleObject(hEvtBufferReady,FALSE,&interval) != WAIT_OBJECT_0)
        goto done;
    
    /* write the process id into the buffer */
    dbuffer = (DBG_OUTPUT_DEBUG_STRING_BUFFER *)BaseAddress;
    dbuffer->ProcessId = (DWORD)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess);

    (void)strncpy(dbuffer->Msg,string,DBG_OUT_BUFFER_SIZE);
    dbuffer->Msg[DBG_OUT_BUFFER_SIZE - 1] = 0;

    /* ensure that the buffer has new line character */
    length = (int)strlen(dbuffer->Msg);
    if(length > 0){
        if(dbuffer->Msg[length-1] != '\n'){
            if(length == (DBG_OUT_BUFFER_SIZE - 1)){
                dbuffer->Msg[length-1] = '\n';
            } else {
                dbuffer->Msg[length] = '\n';
                dbuffer->Msg[length+1] = 0;
            }
        }
    } else {
        strcpy(dbuffer->Msg,"\n");
    }
    
    /* signal that the buffer contains meaningful data and can be read */
    (void)NtSetEvent(hEvtDataReady,NULL);

done:
    NtCloseSafe(hEvtBufferReady);
    NtCloseSafe(hEvtDataReady);
    if(BaseAddress)
        (void)NtUnmapViewOfSection(NtCurrentProcess(),BaseAddress);
    NtCloseSafe(hSection);
}

/**
 * @internal
 */
typedef struct _NT_STATUS_DESCRIPTION {
    unsigned long status;
    char *desc;
} NT_STATUS_DESCRIPTION, *PNT_STATUS_DESCRIPTION;

/**
 * @internal
 */
NT_STATUS_DESCRIPTION descriptions[] = {
    { STATUS_SUCCESS,                "operation successful"           },
    { STATUS_OBJECT_NAME_INVALID,    "object name invalid"            },
    { STATUS_OBJECT_NAME_NOT_FOUND,  "object name not found"          },
    { STATUS_OBJECT_NAME_COLLISION,  "object name already exists"     },
    { STATUS_OBJECT_PATH_INVALID,    "path is invalid"                },
    { STATUS_OBJECT_PATH_NOT_FOUND,  "path not found"                 },
    { STATUS_OBJECT_PATH_SYNTAX_BAD, "bad syntax in path"             },
    { STATUS_BUFFER_TOO_SMALL,       "buffer is too small"            },
    { STATUS_ACCESS_DENIED,          "access denied"                  },
    { STATUS_NO_MEMORY,              "not enough memory"              },
    { STATUS_UNSUCCESSFUL,           "operation failed"               },
    { STATUS_NOT_IMPLEMENTED,        "not implemented"                },
    { STATUS_INVALID_INFO_CLASS,     "invalid info class"             },
    { STATUS_INFO_LENGTH_MISMATCH,   "info length mismatch"           },
    { STATUS_ACCESS_VIOLATION,       "access violation"               },
    { STATUS_INVALID_HANDLE,         "invalid handle"                 },
    { STATUS_INVALID_PARAMETER,      "invalid parameter"              },
    { STATUS_NO_SUCH_DEVICE,         "device not found"               },
    { STATUS_NO_SUCH_FILE,           "file not found"                 },
    { STATUS_INVALID_DEVICE_REQUEST, "invalid device request"         },
    { STATUS_END_OF_FILE,            "end of file reached"            },
    { STATUS_WRONG_VOLUME,           "wrong volume"                   },
    { STATUS_NO_MEDIA_IN_DEVICE,     "no media in device"             },
    { STATUS_UNRECOGNIZED_VOLUME,    "cannot recognize file system"   },
    { STATUS_VARIABLE_NOT_FOUND,     "environment variable not found" },
    
    /* A file cannot be opened because the share access flags are incompatible. */
    { STATUS_SHARING_VIOLATION,      "file is locked by another process"},
    
    /* A file cannot be moved because target clusters are in use. */
    { STATUS_ALREADY_COMMITTED,      "target clusters are already in use"},
    
    { 0xffffffff,                    NULL                             }
};

/**
 * @internal
 * @brief Returns NT status description.
 * @param[in] status the NT status code.
 * @note This function returns descriptions
 * only for well known codes. Otherwise it
 * returns an empty string.
 * @par Example:
 * @code
 * printf("%s\n",winx_get_status_description(STATUS_ACCESS_VIOLATION));
 * // prints "access violation"
 * @endcode
 */
char *winx_get_status_description(unsigned long status)
{
    int i;
    
    for(i = 0; descriptions[i].desc; i++){
        if(descriptions[i].status == status)
            return descriptions[i].desc;
    }
    return "";
}

/**
 * @internal
 */
enum {
    ENC_ANSI,
    ENC_UTF16
};

/**
 * @internal
 * @brief Retuns error description.
 * @param[in] error the error code.
 * @param[out] encoding pointer to variable
 * receiving the string encoding.
 */
static void *winx_get_error_description(ULONG error,int *encoding)
{
    UNICODE_STRING us;
    NTSTATUS Status;
    HMODULE base_addr;
    MESSAGE_RESOURCE_ENTRY *mre;

    /*
    * ntdll.dll returns wrong messages,
    * so we always use kernel32.dll
    * library giving us a great deal
    * better information.
    */
    RtlInitUnicodeString(&us,L"kernel32.dll");
    Status = LdrGetDllHandle(0,0,&us,&base_addr);
    if(!NT_SUCCESS(Status))
        return NULL; /* this case is usual for boot time executables */
    Status = RtlFindMessage(base_addr,(ULONG)(DWORD_PTR)RT_MESSAGETABLE,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),error,&mre);
    if(!NT_SUCCESS(Status))
        return NULL; /* no appropriate message found */
    if(encoding){
        *encoding = (mre->Flags & MESSAGE_RESOURCE_UNICODE) ? ENC_UTF16 : ENC_ANSI;
    }
    return (void *)(mre->Text);
}

/**
 * @brief Replaces CR and LF
 * characters in a string by spaces.
 * @details Intended for use in winx_dbg_print
 * routine to keep logging as clean as possible.
 */
static void remove_crlf(char *s)
{
    int i;
    
    if(s){
        for(i = 0; s[i]; i++){
            if(s[i] == '\r' || s[i] == '\n')
                s[i] = ' ';
        }
    }
}

/**
 * @brief Delivers a message to the Debug View
 * program and appends it to the log file as well.
 * @param[in] flags one of the flags defined in
 * ../../include/dbg.h file.
 * If NT_STATUS_FLAG is set, the
 * last nt status value will be appended
 * to the debugging message as well as its
 * description. If LAST_ERROR_FLAG is set,
 * the same stuff will be done for the last
 * error value.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 * @note
 * - Not all system API set the last status code.
 * Use strace macro to catch the status for sure.
 */
void winx_dbg_print(int flags, const char *format, ...)
{
    char *msg = NULL;
    char *err_msg = NULL;
    char *ext_msg = NULL;
    char *cnv_msg = NULL;
    ULONG status, error;
    int ns_flag = 0, le_flag = 0;
    int encoding, length;
    va_list arg;
    
    /* save last error codes */
    status = NtCurrentTeb()->LastStatusValue;
    error = NtCurrentTeb()->LastErrorValue;
    
    /* format message */
    if(format){
        va_start(arg,format);
        msg = winx_vsprintf(format,arg);
        va_end(arg);
    }
    if(!msg) return;
    
    /* get rid of trailing new line character */
    length = (int)strlen(msg);
    if(length){
        if(msg[length - 1] == '\n')
            msg[length - 1] = 0;
    }

    if(flags & NT_STATUS_FLAG){
        error = RtlNtStatusToDosError(status);
        ns_flag = 1;
    }
    if(flags & LAST_ERROR_FLAG) le_flag = 1;

    if(ns_flag || le_flag){
        err_msg = winx_get_error_description(error,&encoding);
        if(err_msg == NULL && ns_flag){
            /* for boot time executables we have a good recovery */
            err_msg = winx_get_status_description(status);
            encoding = ENC_ANSI;
        }
        if(err_msg){
            if(encoding == ENC_ANSI){
                ext_msg = winx_sprintf("%s: 0x%x %s: %s",
                     msg,ns_flag ? (UINT)status : (UINT)error,
                     ns_flag ? "status" : "error",err_msg);
                remove_crlf(ext_msg);
                add_dbg_log_entry(ext_msg ? ext_msg : msg);
                deliver_message(ext_msg ? ext_msg : msg);
            } else {
                length = ((int)wcslen((wchar_t *)err_msg) + 1) * sizeof(wchar_t);
                length *= 2; /* enough to hold UTF-8 string */
                cnv_msg = winx_tmalloc(length);
                if(cnv_msg){
                    /* write message to log in UTF-8 encoding */
                    winx_to_utf8(cnv_msg,length,(wchar_t *)err_msg);
                    ext_msg = winx_sprintf("%s: 0x%x %s: %s",
                         msg,ns_flag ? (UINT)status : (UINT)error,
                         ns_flag ? "status" : "error",cnv_msg);
                    remove_crlf(ext_msg);
                    add_dbg_log_entry(ext_msg ? ext_msg : msg);
                    winx_free(ext_msg); ext_msg = NULL;
                    /* send message to debugger in ANSI encoding */
                    _snprintf(cnv_msg,length,"%ls",(wchar_t *)err_msg);
                    cnv_msg[length - 1] = 0;
                    ext_msg = winx_sprintf("%s: 0x%x %s: %s",
                         msg,ns_flag ? (UINT)status : (UINT)error,
                         ns_flag ? "status" : "error",cnv_msg);
                    remove_crlf(ext_msg);
                    deliver_message(ext_msg ? ext_msg : msg);
                } else {
                    goto no_description;
                }
            }
        } else {
no_description:
            ext_msg = winx_sprintf("%s: 0x%x %s",
                 msg,ns_flag ? (UINT)status : (UINT)error,
                 ns_flag ? "status" : "error");
            add_dbg_log_entry(ext_msg ? ext_msg : msg);
            deliver_message(ext_msg ? ext_msg : msg);
        }
    } else {
        add_dbg_log_entry(msg);
        deliver_message(msg);
    }
    
    /* cleanup */
    winx_free(msg);
    winx_free(ext_msg);
    winx_free(cnv_msg);
}

/**
 * @brief Delivers a message to the Debug View
 * program and appends it to the log file as well.
 * @brief Decorates the message by specified
 * character at both sides.
 * @param[in] ch the character to be used for decoration.
 * If zero value is passed, DEFAULT_DBG_PRINT_DECORATION_CHAR is used.
 * @param[in] width the width of the resulting message, in characters.
 * If zero value is passed, DEFAULT_DBG_PRINT_HEADER_WIDTH is used.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 */
void winx_dbg_print_header(char ch, int width, const char *format, ...)
{
    va_list arg;
    char *string;
    char *prefix;
    char *body;
    int length, left;
    char *buffer;
    
    if(ch == 0)
        ch = DEFAULT_DBG_PRINT_DECORATION_CHAR;
    if(width <= 0)
        width = DEFAULT_DBG_PRINT_HEADER_WIDTH;

    if(format){
        va_start(arg,format);
        string = winx_vsprintf(format,arg);
        if(string){
            /* print prefix before decorated body */
            prefix = ""; body = string;
            if(strstr(string,I) == string){
                prefix = I; body += strlen(I);
            } else if(strstr(string,E) == string){
                prefix = E; body += strlen(E);
            } else if(strstr(string,D) == string){
                prefix = D; body += strlen(D);
            }
            length = (int)strlen(body);
            if(length > (width - 4)){
                /* print string not decorated */
                winx_dbg_print(0,"%s",string);
            } else {
                /* allocate buffer for entire string */
                buffer = winx_tmalloc(width + 1);
                if(buffer == NULL){
                    /* print string not decorated */
                    winx_dbg_print(0,"%s",string);
                } else {
                    /* fill buffer by character */
                    memset(buffer,ch,width);
                    buffer[width] = 0;
                    /* paste leading space */
                    left = (width - length - 2) / 2;
                    buffer[left] = 0x20;
                    /* paste string */
                    memcpy(buffer + left + 1,body,length);
                    /* paste closing space */
                    buffer[left + 1 + length] = 0x20;
                    /* print decorated string */
                    winx_dbg_print(0,"%s%s",prefix,buffer);
                    winx_free(buffer);
                }
            }
            winx_free(string);
        }
        va_end(arg);
    }
}

/**
 * @brief Appends all collected debugging
 * information to the log file.
 * @param[in] flags combination of FLUSH_XXX
 * flags defined in zenwinx.h file.
 */
void winx_flush_dbg_log(int flags)
{
    #define DBG_BUFFER_SIZE (100 * 1024) /* 100 KB */
    LARGE_INTEGER interval;
    winx_dbg_log_entry *old_dbg_log, *log_entry;
    char out_of_memory[] = "\r\n*** Out of memory! ***\r\n";
    char crlf[] = "\r\n";
    char *time_stamp;
    WINX_FILE *f;
    int length;

    /* synchronize with other threads */
    if(!(flags & FLUSH_ALREADY_SYNCHRONIZED)){
        interval.QuadPart = MAX_WAIT_INTERVAL;
        if(NtWaitForSingleObject(hLogSynchEvent,FALSE,&interval) != WAIT_OBJECT_0){
            winx_print("\nflush_dbg_log: synchronization failed!\n");
            return;
        }
    }
    
    /* release reserved memory  */
    winx_free(reserved_memory);
    
    /* disable parallel access to dbg_log list */
    interval.QuadPart = MAX_WAIT_INTERVAL;
    if(NtWaitForSingleObject(hListSynchEvent,FALSE,&interval) != WAIT_OBJECT_0) goto done;
    old_dbg_log = dbg_log;
    dbg_log = NULL;
    NtSetEvent(hListSynchEvent,NULL);
    
    if(!old_dbg_log || !log_path) goto done;
    if(log_path[0] == 0) goto done;
    
    /* open log file */
    f = winx_fbopen(log_path,"a",DBG_BUFFER_SIZE);
    if(f == NULL)
        f = winx_fopen(log_path,"a");

    /* save log */
    if(f != NULL){
        winx_printf("\nWriting log file \"%ws\" ...\n",&log_path[4]);

        for(log_entry = old_dbg_log; log_entry; log_entry = log_entry->next){
            if(log_entry->buffer){
                length = (int)strlen(log_entry->buffer);
                if(length){
                    /* get rid of trailing new line character */
                    if(log_entry->buffer[length - 1] == '\n'){
                        log_entry->buffer[length - 1] = 0;
                        length --;
                    }
                    time_stamp = winx_sprintf("%04d-%02d-%02d %02d:%02d:%02d.%03d ",
                        log_entry->time_stamp.year, log_entry->time_stamp.month, log_entry->time_stamp.day,
                        log_entry->time_stamp.hour, log_entry->time_stamp.minute, log_entry->time_stamp.second,
                        log_entry->time_stamp.milliseconds);
                    if(time_stamp){
                        (void)winx_fwrite(time_stamp,sizeof(char),strlen(time_stamp),f);
                        winx_free(time_stamp);
                    }
                    (void)winx_fwrite(log_entry->buffer,sizeof(char),length,f);
                    /* add a proper newline characters */
                    (void)winx_fwrite(crlf,sizeof(char),2,f);
                }
                winx_free(log_entry->buffer);
            }
            if(log_entry->next == old_dbg_log) break;
        }
        
        if(flags & FLUSH_IN_OUT_OF_MEMORY)
            (void)winx_fwrite(out_of_memory,sizeof(char),strlen(out_of_memory),f);
        
        winx_fclose(f);
        winx_list_destroy((list_entry **)(void *)&old_dbg_log);
    }

done:
    /* reserve memory for the out of memory condition handling again */
    reserved_memory = (char *)winx_tmalloc(1024 * 1024);
    
    /* end of synchronization */
    if(!(flags & FLUSH_ALREADY_SYNCHRONIZED))
        NtSetEvent(hLogSynchEvent,NULL);
}

/**
 * @brief Enables or disables
 * debug logging to the file.
 * @param[in] path the path to the logfile.
 * NULL or an empty string forces to flush
 * all collected data to the disk and disable
 * logging to the file.
 */
void winx_set_dbg_log(wchar_t *path)
{
    LARGE_INTEGER interval;
    wchar_t *lb;
    
    if(path == NULL){
        logging_enabled = 0;
    } else {
        logging_enabled = path[0] ? 1 : 0;
    }
    
    if(logging_enabled){
        /* create path */
        lb = wcsrchr(path,'\\');
        if(lb) *lb = 0;
        if(winx_create_path(path) < 0){
            etrace("cannot create directory tree for log path");
            winx_print("\nwinx_set_dbg_log: cannot create directory tree for log path\n");
        }
        if(lb) *lb = '\\';
    }
    
    /* synchronize with other threads */
    interval.QuadPart = MAX_WAIT_INTERVAL;
    if(NtWaitForSingleObject(hLogSynchEvent,FALSE,&interval) != WAIT_OBJECT_0){
        etrace("synchronization failed");
        winx_print("\nwinx_enable_dbg_log: synchronization failed!\n");
        return;
    }
    
    /* flush old log to disk */
    if(path || log_path){
        if(!path || !log_path) winx_flush_dbg_log(FLUSH_ALREADY_SYNCHRONIZED);
        else if(wcscmp(path,log_path)) winx_flush_dbg_log(FLUSH_ALREADY_SYNCHRONIZED);
    }
    
    /* set new log path */
    if(log_path){
        winx_free(log_path);
        log_path = NULL;
    }
    if(logging_enabled){
        itrace("log_path = %ws",path);
        winx_printf("\nUsing log file \"%ws\" ...\n",&path[4]);
        log_path = winx_wcsdup(path);
        if(log_path == NULL){
            mtrace();
            winx_print("\nCannot allocate memory for log path!\n");
        }
    }
    
    /* end of synchronization */
    NtSetEvent(hLogSynchEvent,NULL);
}

/** @} */
