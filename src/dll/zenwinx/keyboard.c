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
 * @file keyboard.c
 * @brief Keyboard input.
 * @addtogroup Keyboard
 * @{
 */

/*
* Revised by Stefan Pendl, 2010
* <stefanpe@users.sourceforge.net>
*/

#include "zenwinx.h"

/*
* This delay affects primarily text typing
* speed when winx_prompt() is used to get
* user input.
*/
#define MAX_TYPING_DELAY 10 /* msec */

#define MAX_NUM_OF_KEYBOARDS 100

/*
* Keyboard queue is needed to keep all
* key hits, including composite ones
* like Pause/Break.
*/
#define KB_QUEUE_LENGTH      100

typedef struct _KEYBOARD {
    int device_number; /* for debugging purposes */
    HANDLE hKbDevice;
    HANDLE hKbEvent;
} KEYBOARD, *PKEYBOARD;

KEYBOARD kb[MAX_NUM_OF_KEYBOARDS] = {{0}};
int number_of_keyboards = 0;

HANDLE hKbSynchEvent = NULL;
KEYBOARD_INPUT_DATA kids[KB_QUEUE_LENGTH] = {{0}};
int start_index = 0;
int n_written = 0;
int stop_kb_wait_for_input = 0;
int kb_wait_for_input_threads = 0;
#define STOP_KB_WAIT_INTERVAL 100 /* ms */

/* prototypes */
void kb_close(void);
static int kb_check(HANDLE hKbDevice);
static int kb_open_internal(int device_number);
char *winx_get_error_description(unsigned long status);

/**
 * @internal
 * @brief Waits for user input on
 * the specified keyboard.
 */
static DWORD WINAPI kb_wait_for_input(LPVOID p)
{
    LARGE_INTEGER ByteOffset;
    IO_STATUS_BLOCK iosb;
    KEYBOARD_INPUT_DATA kid;
    NTSTATUS Status;
    LARGE_INTEGER interval;
    LARGE_INTEGER synch_interval;
    int index;
    char buffer[128];
    KEYBOARD *kbd = (KEYBOARD *)p;
    
    /*
    * Either debug print or winx_printf,
    * or memory allocation aren't available here...
    */
    
    kb_wait_for_input_threads ++;
    interval.QuadPart = -((signed long)STOP_KB_WAIT_INTERVAL * 10000);

    while(!stop_kb_wait_for_input){
        ByteOffset.QuadPart = 0;
        /* make a read request */
        Status = NtReadFile(kbd->hKbDevice,kbd->hKbEvent,NULL,NULL,
            &iosb,&kid,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
        /* wait for key hits */
        if(NT_SUCCESS(Status)){
            do {
                Status = NtWaitForSingleObject(kbd->hKbEvent,FALSE,&interval);
                if(stop_kb_wait_for_input){
                    if(Status == STATUS_TIMEOUT){
                        /* cancel the pending operation */
                        Status = NtCancelIoFile(kbd->hKbDevice,&iosb);
                        if(NT_SUCCESS(Status)){
                            Status = NtWaitForSingleObject(kbd->hKbEvent,FALSE,NULL);
                            if(NT_SUCCESS(Status)) Status = iosb.Status;
                        }
                        if(!NT_SUCCESS(Status)){
                            _snprintf(buffer,sizeof(buffer),"\nNtCancelIoFile for KeyboadClass%u failed: %x!\n%s\n",
                                kbd->device_number,(UINT)Status,winx_get_error_description((ULONG)Status));
                            buffer[sizeof(buffer) - 1] = 0;
                            winx_print(buffer);
                        }
                    }
                    goto done;
                }
            } while(Status == STATUS_TIMEOUT);
            if(NT_SUCCESS(Status)) Status = iosb.Status;
        }
        /* here we have either an input gathered or an error */
        if(!NT_SUCCESS(Status)){
            _snprintf(buffer,sizeof(buffer),"\nCannot read the KeyboadClass%u device: %x!\n%s\n",
                kbd->device_number,(UINT)Status,winx_get_error_description((ULONG)Status));
            buffer[sizeof(buffer) - 1] = 0;
            winx_print(buffer);
            goto done;
        } else {
            /* synchronize with other threads */
            if(hKbSynchEvent){
                synch_interval.QuadPart = MAX_WAIT_INTERVAL;
                Status = NtWaitForSingleObject(hKbSynchEvent,FALSE,&synch_interval);
                if(Status != WAIT_OBJECT_0){
                    _snprintf(buffer,sizeof(buffer),"\nkb_wait_for_input: synchronization failed: %x!\n%s\n",
                        (UINT)Status,winx_get_error_description((ULONG)Status));
                    buffer[sizeof(buffer) - 1] = 0;
                    winx_print(buffer);
                }
            }

            /* push new item to the keyboard queue */
            if(start_index < 0 || start_index >= KB_QUEUE_LENGTH){
                winx_print("\nkb_wait_for_input: unexpected condition #1!\n\n");
                start_index = 0;
            }
            if(n_written < 0 || n_written > KB_QUEUE_LENGTH){
                winx_print("\nkb_wait_for_input: unexpected condition #2!\n\n");
                n_written = 0;
            }

            index = start_index + n_written;
            if(index >= KB_QUEUE_LENGTH)
                index -= KB_QUEUE_LENGTH;

            if(n_written == KB_QUEUE_LENGTH)
                start_index ++;
            else
                n_written ++;
            
            memcpy(&kids[index],&kid,sizeof(KEYBOARD_INPUT_DATA));
            
            /* release synchronization event */
            if(hKbSynchEvent)
                (void)NtSetEvent(hKbSynchEvent,NULL);
        }
    }

done:
    kb_wait_for_input_threads --;
    winx_exit_thread(0);
    return 0;
}

/**
 * @brief Prepares all existing keyboards
 * for work with user input related procedures.
 * @details If checking of first keyboard fails
 * it waits ten seconds for the initialization.
 * This is needed for wireless devices.
 * @return Zero for success, negative value otherwise.
 */
int winx_kb_init(void)
{
    short event_name[64];
    int i, j;
    
    /* create synchronization event for safe access to kids array */
    _snwprintf(event_name,64,L"\\winx_kb_synch_event_%u",
        (unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess));
    event_name[63] = 0;
    
    if(hKbSynchEvent == NULL){
        (void)winx_create_event(event_name,SynchronizationEvent,&hKbSynchEvent);
        if(hKbSynchEvent) (void)NtSetEvent(hKbSynchEvent,NULL);
    }
    
    if(hKbSynchEvent == NULL){
        winx_printf("\nCannot create %ws event!\n\n",event_name);
        return (-1);
    }

    /* initialize kb array */
    memset((void *)kb,0,sizeof(kb));
    number_of_keyboards = 0;
    
    /* check all the keyboards and wait ten seconds
       for any keyboard that fails detection.
       required for USB devices, which can change ports */
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++) {
        if (kb_open_internal(i) == -1) {
            if (i < 2) {
                winx_printf("Wait 10 seconds for keyboard initialization ");
                
                for(j = 0; j < 10; j++){
                    winx_sleep(1000);
                    winx_printf(".");
                }
                winx_printf("\n\n");

                (void)kb_open_internal(i);
            }
        }
    }
    
    /* start threads waiting for user input */
    stop_kb_wait_for_input = 0;
    kb_wait_for_input_threads = 0;
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
        if(kb[i].hKbDevice == NULL) break;
        if(winx_create_thread(kb_wait_for_input,(LPVOID)&kb[i],NULL) < 0){
            winx_printf("\nCannot create thread gathering input from \\Device\\KeyboardClass%u\n\n",
                kb[i].device_number);
            /* stop all threads */
            stop_kb_wait_for_input = 1;
            while(kb_wait_for_input_threads)
                winx_sleep(STOP_KB_WAIT_INTERVAL);
            return (-1);
        }
    }
    
    if(kb[0].hKbDevice) return 0; /* success, at least one keyboard found */
    else return (-1);
}

/**
 * @internal
 * @brief Closes all opened keyboards.
 */
void kb_close(void)
{
    int i;
    
    /*
    * Either debug print or memory
    * allocation calls aren't available here...
    */
    
    /* stop threads waiting for user input */
    stop_kb_wait_for_input = 1;
    while(kb_wait_for_input_threads)
        winx_sleep(STOP_KB_WAIT_INTERVAL);
    
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
        if(kb[i].hKbDevice == NULL) break;
        NtCloseSafe(kb[i].hKbDevice);
        NtCloseSafe(kb[i].hKbEvent);
        /* don't reset device_number member here */
        number_of_keyboards --;
    }
    
    /* destroy synchronization event */
    winx_destroy_event(hKbSynchEvent);
}

/**
 * @internal
 * @brief Checks the console for keyboard input.
 * @details Tries to read from all keyboard devices 
 * until specified time-out expires.
 * @param[out] pKID pointer to the structure receiving keyboard input.
 * @param[in] msec_timeout time-out interval in milliseconds.
 * @return Zero if some key was pressed, negative value otherwise.
 */
int kb_read(PKEYBOARD_INPUT_DATA pKID,int msec_timeout)
{
    int attempts = 0;
    ULONGLONG xtime = 0;
    LARGE_INTEGER synch_interval;
    NTSTATUS Status;
    
    DbgCheck1(pKID,"kb_read",-1);
    
    if(msec_timeout != INFINITE){
        attempts = msec_timeout / MAX_TYPING_DELAY + 1;
        xtime = winx_xtime();
    }
    
    while(number_of_keyboards){
        /* synchronize with other threads */
        if(hKbSynchEvent){
            synch_interval.QuadPart = MAX_WAIT_INTERVAL;
            Status = NtWaitForSingleObject(hKbSynchEvent,FALSE,&synch_interval);
            if(Status != WAIT_OBJECT_0){
                winx_printf("\nkb_read: synchronization failed: 0x%x\n",(UINT)Status);
                winx_printf("%s\n\n",winx_get_error_description((ULONG)Status));
            }
        }

        /* pop item from the keyboard queue */
        if(n_written > 0){
            memcpy(pKID,&kids[start_index],sizeof(KEYBOARD_INPUT_DATA));
            start_index ++;
            if(start_index >= KB_QUEUE_LENGTH)
                start_index = 0;
            n_written --;
            if(hKbSynchEvent)
                (void)NtSetEvent(hKbSynchEvent,NULL);
            return 0;
        }

        /* release synchronization event */
        if(hKbSynchEvent)
            (void)NtSetEvent(hKbSynchEvent,NULL);

        winx_sleep(MAX_TYPING_DELAY);
        if(msec_timeout != INFINITE){
            attempts --;
            if(attempts == 0) break;
            if(xtime && (winx_xtime() - xtime >= msec_timeout)) break;
        }
    }
    return (-1);
}

/*
**************************************************************
*                   internal functions                       *
**************************************************************
*/

/**
 * @internal
 * @brief Opens the keyboard.
 * @param[in] device_number the number of the keyboard device.
 * @return Zero for success, negative value otherwise.
 */
static int kb_open_internal(int device_number)
{
    short device_name[32];
    short event_name[32];
    UNICODE_STRING uStr;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE hKbDevice = NULL;
    HANDLE hKbEvent = NULL;
    int i;

    (void)_snwprintf(device_name,32,L"\\Device\\KeyboardClass%u",device_number);
    device_name[31] = 0;
    RtlInitUnicodeString(&uStr,device_name);
    InitializeObjectAttributes(&ObjectAttributes,&uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
    Status = NtCreateFile(&hKbDevice,
                GENERIC_READ | FILE_RESERVE_OPFILTER | FILE_READ_ATTRIBUTES/*0x80100080*/,
                &ObjectAttributes,&IoStatusBlock,NULL,FILE_ATTRIBUTE_NORMAL/*0x80*/,
                0,FILE_OPEN/*1*/,FILE_DIRECTORY_FILE/*1*/,NULL,0);
    if(!NT_SUCCESS(Status)){
        if(device_number < 2){
            DebugPrintEx(Status,"Cannot open the keyboard %ws",device_name);
            winx_printf("\nCannot open the keyboard %ws: %x!\n",
                device_name,(UINT)Status);
            winx_printf("%s\n",winx_get_error_description((ULONG)Status));
            
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND && winx_get_os_version() < WINDOWS_XP) return (-2);
        }
        return (-1);
    }
    
    /* ensure that we have opened a really connected keyboard */
    if(kb_check(hKbDevice) < 0){
        DebugPrintEx(Status,"Invalid keyboard device %ws",device_name);
        winx_printf("\nInvalid keyboard device %ws: %x!\n",device_name,(UINT)Status);
        winx_printf("%s\n",winx_get_error_description((ULONG)Status));
        NtCloseSafe(hKbDevice);
        return (-1);
    }
    
    /* create a special event object for internal use */
    (void)_snwprintf(event_name,32,L"\\kb_event%u",device_number);
    event_name[31] = 0;
    RtlInitUnicodeString(&uStr,event_name);
    InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
    Status = NtCreateEvent(&hKbEvent,STANDARD_RIGHTS_ALL | 0x1ff/*0x1f01ff*/,
        &ObjectAttributes,SynchronizationEvent,0/*FALSE*/);
    if(!NT_SUCCESS(Status)){
        NtCloseSafe(hKbDevice);
        DebugPrintEx(Status,"Cannot create kb_event%u",device_number);
        winx_printf("\nCannot create kb_event%u: %x!\n",device_number,(UINT)Status);
        winx_printf("%s\n",winx_get_error_description((ULONG)Status));
        return (-1);
    }
    
    /* add information to kb array */
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
        if(kb[i].hKbDevice == NULL){
            kb[i].hKbDevice = hKbDevice;
            kb[i].hKbEvent = hKbEvent;
            kb[i].device_number = device_number;
            number_of_keyboards ++;
            winx_printf("Keyboard device found: %ws.\n",device_name);
            return 0;
        }
    }

    winx_printf("\nkb array is full!\n");
    return (-1);
}

#define LIGHTING_REPEAT_COUNT 0x5

/**
 * @internal
 * @brief Light up the keyboard indicators.
 * @param[in] hKbDevice the handle of the keyboard device.
 * @param[in] LedFlags the flags specifying
 * which indicators must be lighten up.
 * @return Zero for success, negative value otherwise.
 */
static int kb_light_up_indicators(HANDLE hKbDevice,USHORT LedFlags)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    KEYBOARD_INDICATOR_PARAMETERS kip;

    kip.LedFlags = LedFlags;
    kip.UnitId = 0;

    Status = NtDeviceIoControlFile(hKbDevice,NULL,NULL,NULL,
            &iosb,IOCTL_KEYBOARD_SET_INDICATORS,
            &kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS),NULL,0);
    if(NT_SUCCESS(Status)){
        Status = NtWaitForSingleObject(hKbDevice,FALSE,NULL);
        if(NT_SUCCESS(Status)) Status = iosb.Status;
    }
    if(!NT_SUCCESS(Status) || Status == STATUS_PENDING) return (-1);
    
    return 0;
}

/**
 * @internal
 * @brief Checks the keyboard for an existence.
 * @param[in] hKbDevice the handle of the keyboard device.
 * @return Zero for success, negative value otherwise.
 */
static int kb_check(HANDLE hKbDevice)
{
    USHORT LedFlags;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    KEYBOARD_INDICATOR_PARAMETERS kip;
    int i;
    
    /* try to get LED flags */
    RtlZeroMemory(&kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS));
    Status = NtDeviceIoControlFile(hKbDevice,NULL,NULL,NULL,
            &iosb,IOCTL_KEYBOARD_QUERY_INDICATORS,NULL,0,
            &kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS));
    if(NT_SUCCESS(Status)){
        Status = NtWaitForSingleObject(hKbDevice,FALSE,NULL);
        if(NT_SUCCESS(Status)) Status = iosb.Status;
    }
    if(!NT_SUCCESS(Status) || Status == STATUS_PENDING) return (-1);

    LedFlags = kip.LedFlags;
    
    /* light up LED's */
    for(i = 0; i < LIGHTING_REPEAT_COUNT; i++){
        (void)kb_light_up_indicators(hKbDevice,KEYBOARD_NUM_LOCK_ON);
        winx_sleep(100);
        (void)kb_light_up_indicators(hKbDevice,KEYBOARD_CAPS_LOCK_ON);
        winx_sleep(100);
        (void)kb_light_up_indicators(hKbDevice,KEYBOARD_SCROLL_LOCK_ON);
        winx_sleep(100);
    }

    (void)kb_light_up_indicators(hKbDevice,LedFlags);
    return 0;
}

/** @} */
