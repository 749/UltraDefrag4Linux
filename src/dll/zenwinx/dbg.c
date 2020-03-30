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
 * @file dbg.c
 * @brief Debugging.
 * @details All debugging messages will be delivered
 * to the Debug View Program when possible. If logging
 * to the file is enabled, they will be saved there too.
 * Size of the log is limited by available memory, 
 * which is rational since logging is rarely needed
 * and RAM is usually smaller than free space available on disk.
 * @addtogroup Debug
 * @{
 */

#include "zenwinx.h"

winx_spin_lock *dbg_lock = NULL;
int debug_print_enabled = 1;

/* forward declarations */
int winx_debug_print(char *string);
static int deliver_message_to_the_debugger(char *string);
static int init_dbg_log(void);
static void add_dbg_log_entry(char *string);
void flush_dbg_log(int already_synchronized);
static void close_dbg_log(void);

/* Initialization routines */

/**
 * @internal
 * @brief Initializes the synchronization objects used
 * in the debugging routines.
 * @return Zero for success, negative value otherwise.
 */
int winx_init_synch_objects(void)
{
    if(dbg_lock == NULL)
        dbg_lock = winx_init_spin_lock("winx_dbg_lock");
    if(dbg_lock == NULL)
        return (-1);
    return init_dbg_log();
}

/**
 * @internal
 * @brief Destroys the synchronization
 * objects used in the debugging routines.
 */
void winx_destroy_synch_objects(void)
{
    close_dbg_log();
    winx_destroy_spin_lock(dbg_lock);
    dbg_lock = NULL;
}


/* Auxiliary routines */

typedef struct _NT_STATUS_DESCRIPTION {
    unsigned long status;
    char *description;
} NT_STATUS_DESCRIPTION, *PNT_STATUS_DESCRIPTION;

NT_STATUS_DESCRIPTION status_descriptions[] = {
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
 * @brief Retrieves a human readable
 * explanation of the NT status code.
 * @param[in] status the NT status code.
 * @return A pointer to string containing
 * the status explanation.
 * @note This function returns explanations
 * only for well known codes. Otherwise 
 * it returns an empty string.
 * @par Example:
 * @code
 * printf("%s\n",winx_get_error_description(STATUS_ACCESS_VIOLATION));
 * // prints "access violation"
 * @endcode
 */
char *winx_get_error_description(unsigned long status)
{
    int i;
    
    for(i = 0;; i++){
        if(status_descriptions[i].description == NULL) break;
        if(status_descriptions[i].status == status)
            return status_descriptions[i].description;
    }
    return "";
}


/* Generic debug print routines */

/**
 * @brief DbgPrint() native equivalent.
 * @note DbgPrint() exported by ntdll library cannot
 * be captured by DbgPrint loggers, therefore we are
 * using our own routine for debugging purposes.
 */
void winx_dbg_print(char *format, ...)
{
    va_list arg;
    char *string;
    
    if(format){
        va_start(arg,format);
        string = winx_vsprintf(format,arg);
        if(string){
            winx_debug_print(string);
            winx_heap_free(string);
        }
        va_end(arg);
    }
}

/**
 * @brief Delivers an error message
 * to the DbgView program.
 * @details The error message includes 
 * NT status and its explanation.
 * @param[in] status the NT status code.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 * @par Example:
 * @code
 * NTSTATUS Status = NtCreateFile(...);
 * if(!NT_SUCCESS(Status)){
 *     winx_dbg_print_ex(Status,"Cannot create %s file",filename);
 * }
 * @endcode
 */
void winx_dbg_print_ex(unsigned long status,char *format, ...)
{
    va_list arg;
    char *string;
    
    if(format){
        va_start(arg,format);
        string = winx_vsprintf(format,arg);
        if(string){
            /*
            * FormatMessage may result in unreadable strings
            * if a proper locale is not set in DbgView program.
            */
            winx_dbg_print("%s: 0x%x: %s",string,(UINT)status,
                winx_get_error_description(status));
            winx_heap_free(string);
        }
        va_end(arg);
    }
}

/**
 * @brief Delivers formatted message, decorated by specified
 * character at both sides, to DbgView program.
 * @param[in] ch character to be used for decoration.
 * If zero value is passed, DEFAULT_DBG_PRINT_DECORATION_CHAR is used.
 * @param[in] width the width of the resulting message, in characters.
 * If zero value is passed, DEFAULT_DBG_PRINT_HEADER_WIDTH is used.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 */
void winx_dbg_print_header(char ch, int width, char *format, ...)
{
    va_list arg;
    char *string;
    int length;
    char *buffer;
    int left;
    
    if(ch == 0)
        ch = DEFAULT_DBG_PRINT_DECORATION_CHAR;
    if(width <= 0)
        width = DEFAULT_DBG_PRINT_HEADER_WIDTH;

    if(format){
        va_start(arg,format);
        string = winx_vsprintf(format,arg);
        if(string){
            length = strlen(string);
            if(length > (width - 4)){
                /* print string not decorated */
                winx_debug_print(string);
            } else {
                /* allocate buffer for entire string */
                buffer = winx_heap_alloc(width + 1);
                if(buffer == NULL){
                    /* print string not decorated */
                    winx_debug_print(string);
                } else {
                    /* fill buffer by character */
                    memset(buffer,ch,width);
                    buffer[width] = 0;
                    /* paste leading space */
                    left = (width - length - 2) / 2;
                    buffer[left] = 0x20;
                    /* paste string */
                    memcpy(buffer + left + 1,string,length);
                    /* paste closing space */
                    buffer[left + 1 + length] = 0x20;
                    /* print decorated string */
                    winx_debug_print(buffer);
                    winx_heap_free(buffer);
                }
            }
            winx_heap_free(string);
        }
        va_end(arg);
    }
}

/**
 * @internal
 * @brief Delivers a message to the Debug View program and
 * saves it to the log list if logging to the file is enabled.
 * @param[in] string the string to be delivered.
 * @return Zero for success, negative value otherwise.
 */
int winx_debug_print(char *string)
{
    int result = 0;
    
    if(string == NULL)
        return (-1);

    /* synchronize with other threads, wait no more than 11 sec */
    if(dbg_lock != NULL){
        if(dbg_lock->hEvent && winx_acquire_spin_lock(dbg_lock,11000) < 0)
            return (-1);
    }
    
    /* return immediately if debug print is disabled */
    if(debug_print_enabled){
        /* add message to the log list */
        add_dbg_log_entry(string);
        result = deliver_message_to_the_debugger(string);
    }

    /* end of synchronization */
    winx_release_spin_lock(dbg_lock);
    return result;
}

/**
 * @internal
 * @brief Internal structure used to deliver
 * information to the Debug View program.
 */
typedef struct _DBG_OUTPUT_DEBUG_STRING_BUFFER {
    ULONG ProcessId;
    UCHAR Msg[4096-sizeof(ULONG)];
} DBG_OUTPUT_DEBUG_STRING_BUFFER, *PDBG_OUTPUT_DEBUG_STRING_BUFFER;

#define DBG_BUFFER_SIZE (4096-sizeof(ULONG))

/**
 * @internal
 * @brief Low-level routine for delivering
 * of debugging messages to the Debug View program.
 * @return Zero for success, negative value otherwise.
 */
static int deliver_message_to_the_debugger(char *string)
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
    int result = (-1);
    
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

    (void)strncpy(dbuffer->Msg,string,DBG_BUFFER_SIZE);
    dbuffer->Msg[DBG_BUFFER_SIZE - 1] = 0;

    /* ensure that the buffer has new line character */
    length = strlen(dbuffer->Msg);
    if(length > 0){
        if(dbuffer->Msg[length-1] != '\n'){
            if(length == (DBG_BUFFER_SIZE - 1)){
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
    result = 0;

done:
    NtCloseSafe(hEvtBufferReady);
    NtCloseSafe(hEvtDataReady);
    if(BaseAddress)
        (void)NtUnmapViewOfSection(NtCurrentProcess(),BaseAddress);
    NtCloseSafe(hSection);
    return result;
}

/* Logging to the file */

typedef struct _winx_dbg_log_entry {
    struct _winx_dbg_log_entry *next;
    struct _winx_dbg_log_entry *prev;
    winx_time time_stamp;
    char *buffer;
} winx_dbg_log_entry;

winx_dbg_log_entry *dbg_log = NULL;
winx_spin_lock *file_lock = NULL;
int logging_enabled = 0;
char *log_path = NULL;

/**
 * @internal
 * @brief Initializes the logging to the file.
 * @return Zero for success, negative value otherwise.
 */
static int init_dbg_log(void)
{
    if(file_lock == NULL)
        file_lock = winx_init_spin_lock("winx_dbg_logfile_lock");
    return (file_lock == NULL) ? (-1) : (0);
}

/**
 * @internal
 * @brief Adds a string to the debug log.
 */
static void add_dbg_log_entry(char *string)
{
    winx_dbg_log_entry *new_log_entry = NULL, *last_log_entry = NULL;

    if(logging_enabled){
        if(dbg_log)
            last_log_entry = dbg_log->prev;
        new_log_entry = (winx_dbg_log_entry *)winx_list_insert_item((list_entry **)(void *)&dbg_log,
            (list_entry *)last_log_entry,sizeof(winx_dbg_log_entry));
        if(new_log_entry == NULL){
            /* not enough memory */
        } else {
            new_log_entry->buffer = winx_strdup(string);
            if(new_log_entry->buffer == NULL){
                /* not enough memory */
                winx_list_remove_item((list_entry **)(void *)&dbg_log,(list_entry *)new_log_entry);
            } else {
                memset(&new_log_entry->time_stamp,0,sizeof(winx_time));
                (void)winx_get_local_time(&new_log_entry->time_stamp);
            }
        }
    }
}

/**
 * @internal
 * @brief Appends all collected debugging
 * information to the log file.
 * @param[in] already_synchronized an internal
 * flag, used in winx_enable_dbg_log only.
 * Should be always set to zero in other cases.
 */
void flush_dbg_log(int already_synchronized)
{
    WINX_FILE *f;
    char *path;
    winx_dbg_log_entry *log_entry;
    int length;
    char crlf[] = "\r\n";
    char *time_stamp;

    /* synchronize with other threads */
    if(!already_synchronized && file_lock != NULL){
        if(file_lock->hEvent && winx_acquire_spin_lock(file_lock,INFINITE) < 0){
            DebugPrint("flush_dbg_log: synchronization failed");
            winx_print("\nflush_dbg_log: synchronization failed!\n");
            return;
        }
    }

    /* disable parallel access to dbg_log list */
    if(dbg_lock != NULL){
        if(dbg_lock->hEvent && winx_acquire_spin_lock(dbg_lock,11000) < 0){
            if(!already_synchronized)
                winx_release_spin_lock(file_lock);
            return;
        }
    }
    debug_print_enabled = 0;
    winx_release_spin_lock(dbg_lock);
    
    if(dbg_log == NULL || log_path == NULL)
        goto done;
    
    if(log_path[0] == 0)
        goto done;

    /* save log */
    f = winx_fbopen(log_path,"a",DBG_BUFFER_SIZE);
    if(f == NULL){
        /* recreate path if it does not exist */
        path = winx_strdup(log_path);
        if(path == NULL){
            DebugPrint("flush_dbg_log: cannot allocate memory for log path");
            winx_print("\nflush_dbg_log: cannot allocate memory for log path\n");
        } else {
            winx_path_remove_filename(path);
            if(winx_create_path(path) < 0){
                DebugPrint("flush_dbg_log: cannot create directory tree for log path");
                winx_print("\nflush_dbg_log: cannot create directory tree for log path\n");
            }
            winx_heap_free(path);
            f = winx_fbopen(log_path,"a",DBG_BUFFER_SIZE);
        }
    }
    if(f != NULL){
        winx_printf("\nWriting log file \"%s\" ...\n",&log_path[4]);
        
        for(log_entry = dbg_log; log_entry; log_entry = log_entry->next){
            if(log_entry->buffer){
                length = strlen(log_entry->buffer);
                if(length > 0){
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
                        winx_heap_free(time_stamp);
                    }
                    (void)winx_fwrite(log_entry->buffer,sizeof(char),length,f);
                    /* add a proper newline characters */
                    (void)winx_fwrite(crlf,sizeof(char),2,f);
                }
            }
            if(log_entry->next == dbg_log) break;
        }
        winx_fclose(f);
        winx_list_destroy((list_entry **)(void *)&dbg_log);
    }

done:
    /* enable access to dbg_log list again */
    if(dbg_lock == NULL){
        debug_print_enabled = 1;
    } else if(!dbg_lock->hEvent || winx_acquire_spin_lock(dbg_lock,11000) == 0){
        debug_print_enabled = 1;
        winx_release_spin_lock(dbg_lock);
    }

    /* end of synchronization */
    if(!already_synchronized)
        winx_release_spin_lock(file_lock);
}

/**
 * @brief Enables debug logging to the file.
 * @param[in] path the path to the logfile.
 * NULL or an empty string forces to flush
 * all collected data to the disk and disable
 * logging to the file.
 */
void winx_enable_dbg_log(char *path)
{
    if(path == NULL){
        logging_enabled = 0;
    } else {
        logging_enabled = path[0] ? 1 : 0;
    }
    
    /* synchronize with other threads */
    if(file_lock != NULL){
        if(file_lock->hEvent && winx_acquire_spin_lock(file_lock,INFINITE) < 0){
            DebugPrint("winx_enable_dbg_log: synchronization failed");
            winx_print("\nwinx_enable_dbg_log: synchronization failed!\n");
            return;
        }
    }
    
    /* flush old log to disk */
    if(!logging_enabled || log_path == NULL){
        flush_dbg_log(1);
    } else {
        if(strcmp(path,log_path))
            flush_dbg_log(1);
    }
    
    if(logging_enabled){
        DebugPrint("winx_enable_dbg_log: log_path = %s",path);
        winx_printf("\nUsing log file \"%s\" ...\n",&path[4]);
    }
    
    /* set new log path */
    if(log_path)
        winx_heap_free(log_path);
    if(logging_enabled){
        log_path = winx_strdup(path);
        if(log_path == NULL){
            DebugPrint("winx_enable_dbg_log: cannot allocate memory for log path");
            winx_print("\nCannot allocate memory for log path!\n");
        }
    } else {
        log_path = NULL;
    }
    
    /* end of synchronization */
    winx_release_spin_lock(file_lock);
}

/**
 * @brief Disables debug logging to the file.
 */
void winx_disable_dbg_log(void)
{
    winx_enable_dbg_log(NULL);
}

/**
 * @internal
 * @brief Closes logging to the file.
 */
static void close_dbg_log(void)
{
    flush_dbg_log(0);
    if(log_path){
        winx_heap_free(log_path);
        log_path = NULL;
    }
    winx_destroy_spin_lock(file_lock);
    file_lock = NULL;
}

/** @} */
