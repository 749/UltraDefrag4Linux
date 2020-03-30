/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
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
 * @file crash.c
 * @brief Crash information handling.
 * @addtogroup CrashInfo
 * @{
 */

/*
* Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
* and Dmitri Arkhangelski <dmitriar@gmail.com>.
*/

#include "main.h"

#define EVENT_BUFFER_SIZE (64 * 1024) /* 64 KB */

int crash_info_check_stopped = 0;
int stop_crash_info_check = 0;

HANDLE hLogFile = NULL;
DWORD last_time_stamp = 0; /* time stamp of the last processed event */
int crash_info_collected = 0;

DWORD WINAPI CrashInfoCheckingProc(LPVOID lpParameter);
static void CollectCrashInfo(void);
static void ShowCrashInfo(void);

/**
 * @brief Launches the code showing the crash
 * information when the program crashed last time.
 */
void StartCrashInfoCheck(void)
{
    if(!WgxCreateThread(CrashInfoCheckingProc,NULL)){
        letrace("cannot create thread for the crash info checking");
        crash_info_check_stopped = 1;
    }
}

/**
 * @brief Terminates the code showing the crash
 * information when the program crashed last time.
 */
void StopCrashInfoCheck(void)
{
    stop_crash_info_check = 1;
    while(!crash_info_check_stopped)
        Sleep(100);
}

/**
 * @internal
 * @brief StartCrashInfoCheck thread routine.
 */
DWORD WINAPI CrashInfoCheckingProc(LPVOID lpParameter)
{
    char *msg = "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"
                "\r\n"
                "         If this file contains the UltraDefrag crash information\r\n"
                "           you can help to fix the bug which caused the crash\r\n"
                "           by submitting this information to the bug tracker:\r\n"
                "        http://sourceforge.net/tracker/?group_id=199532&atid=969870\r\n"
                "\r\n"
                "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"
                "\r\n";
    DWORD bytes_written;
    char time_stamp[32];
    
    /* open crash-info.log file for exclusive access */
    hLogFile = CreateFile("crash-info.log",GENERIC_WRITE,
        FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hLogFile == NULL){
        letrace("cannot create crash-info.log file");
        goto done;
    }
    
    /* write header to the file */
    if(!WriteFile(hLogFile,msg,strlen(msg),&bytes_written,NULL)){
        letrace("cannot write to crash-info.log file");
    }

    /* get time stamp of the last processed event */
    GetPrivateProfileString("LastProcessedEvent","TimeStamp","0",time_stamp,32,".\\crash-info.ini");
    last_time_stamp = atoi(time_stamp);
    if(stop_crash_info_check) goto done;
    
    /* collect information on all the events not processed yet */
    CollectCrashInfo();
    if(stop_crash_info_check) goto done;
    
    /* show the collected information */
    ShowCrashInfo();
    
    /* save the time stamp of the last processed event */
    _snprintf(time_stamp,32,"%u",last_time_stamp);
    time_stamp[31] = 0;
    WritePrivateProfileString("LastProcessedEvent","TimeStamp",time_stamp,".\\crash-info.ini");
    
done:
    if(hLogFile) CloseHandle(hLogFile);
    crash_info_check_stopped = 1;
    return 0;
}

/**
 * @brief Collects the crash information.
 */
static void CollectCrashInfo(void)
{
    char *buffer = NULL;
    HANDLE hLog = NULL;
    DWORD bytes_to_read;
    DWORD bytes_read;
    DWORD bytes_needed;
    BOOL result;
    DWORD error;
    PEVENTLOGRECORD rec;
    char *data;
    DWORD bytes_written;
    DWORD new_time_stamp;
    
    new_time_stamp = last_time_stamp;

    bytes_to_read = EVENT_BUFFER_SIZE;
    buffer = malloc(bytes_to_read);
    if(buffer == NULL){
        mtrace();
        goto done;
    }
    
    hLog = OpenEventLog(NULL,"Application");
    if(hLog == NULL){
        letrace("cannot open the Application event log");
        goto done;
    }
    
    while(!stop_crash_info_check){
        result = ReadEventLog(hLog,
            EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
            0,buffer,bytes_to_read,&bytes_read,&bytes_needed);
        if(!result){
            /* handle errors */
            error = GetLastError();
            switch(error){
            case ERROR_INSUFFICIENT_BUFFER:
                itrace("larger buffer of %u bytes is needed",bytes_needed);
                bytes_to_read = bytes_needed;
                free(buffer);
                buffer = malloc(bytes_to_read);
                if(buffer == NULL){
                    mtrace();
                    goto done;
                }
                break;
            case ERROR_HANDLE_EOF:
                goto done;
            default:
                letrace("ReadEventLog failed");
                goto done;
            }
        } else {
            /* handle events */
            rec = (PEVENTLOGRECORD)buffer;
            while(!stop_crash_info_check){
                if((char *)rec >= buffer + bytes_read) break;
                /* skip records older than already processed before */
                if(rec->TimeGenerated <= last_time_stamp) break;
                /* handle Application Error events only */
                if(rec->EventType == EVENTLOG_ERROR_TYPE && (rec->EventID & 0xFFFF) == 1000){
                    if(rec->DataLength > 0){
                        data = malloc(rec->DataLength + 1);
                        if(data == NULL){
                            etrace("not enough memory for event data");
                        } else {
                            memcpy(data,(char *)rec + rec->DataOffset,rec->DataLength);
                            data[rec->DataLength] = 0;
                            /* handle UltraDefrag GUI and command line tool crashes only */
                            if(strstr(data,"ultradefrag.exe") || strstr(data,"udefrag.exe")){
                                trace(I"Crashed in the past: %s",data);
                                if(!WriteFile(hLogFile,data,rec->DataLength,&bytes_written,NULL)){
                                    letrace("cannot write to crash-info.log file");
                                } else {
                                    crash_info_collected = 1;
                                    if(rec->TimeGenerated > new_time_stamp)
                                        new_time_stamp = rec->TimeGenerated;
                                }
                            }
                            free(data);
                        }
                    }
                }
                rec = (PEVENTLOGRECORD)((char *)rec + rec->Length);
            }
        }
    }

done:
    last_time_stamp = new_time_stamp;
    if(hLog) CloseEventLog(hLog);
    free(buffer);
}

/**
 * @brief Shows the crash information.
 */
static void ShowCrashInfo(void)
{
    if(crash_info_collected){
        (void)WgxShellExecute(hWindow,L"open",L".\\crash-info.log",
            NULL,NULL,SW_SHOW,WSH_ALLOW_DEFAULT_ACTION);
    }
}

/** @} */
