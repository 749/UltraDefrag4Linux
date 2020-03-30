/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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

#include "zenwinx.h"

/*
* This delay is needed for non-PS/2
* keyboards initialization.
*/
#define KB_INIT_DELAY 10 /* sec */

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

KEYBOARD kb[MAX_NUM_OF_KEYBOARDS];
int number_of_keyboards = 0;

HANDLE hKbSynchEvent = NULL;
KEYBOARD_INPUT_DATA kids[KB_QUEUE_LENGTH];
int start_index = 0;
int n_written = 0;
int stop_kb_wait_for_input = 0;
int kb_wait_for_input_threads = 0;
#define STOP_KB_WAIT_INTERVAL 100 /* ms */

int kb_read_time_elapsed = 0;

/* prototypes */
char *winx_get_status_description(unsigned long status);
void kb_close(void);

/*
**************************************************************
*                   auxiliary functions                      *
**************************************************************
*/

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
    KEYBOARD *kbd = (KEYBOARD *)p;
    
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
                            winx_printf("\nNtCancelIoFile for KeyboadClass%u failed: %x!\n%s\n",
                                kbd->device_number,(UINT)Status,winx_get_status_description((ULONG)Status));
                        }
                    }
                    goto done;
                }
            } while(Status == STATUS_TIMEOUT);
            if(NT_SUCCESS(Status)) Status = iosb.Status;
        }
        /* here we have either an input gathered or an error */
        if(!NT_SUCCESS(Status)){
            winx_printf("\nCannot read the KeyboadClass%u device: %x!\n%s\n",
                kbd->device_number,(UINT)Status,winx_get_status_description((ULONG)Status));
            goto done;
        } else {
            /* synchronize with other threads */
            synch_interval.QuadPart = MAX_WAIT_INTERVAL;
            Status = NtWaitForSingleObject(hKbSynchEvent,FALSE,&synch_interval);
            if(Status != WAIT_OBJECT_0){
                winx_printf("\nkb_wait_for_input: synchronization failed: %x!\n%s\n",
                    (UINT)Status,winx_get_status_description((ULONG)Status));
                goto done;
            }

            /* push new item to the keyboard queue */
            if(start_index < 0 || start_index >= KB_QUEUE_LENGTH){
                winx_printf("\nkb_wait_for_input: unexpected condition #1!\n\n");
                start_index = 0;
            }
            if(n_written < 0 || n_written > KB_QUEUE_LENGTH){
                winx_printf("\nkb_wait_for_input: unexpected condition #2!\n\n");
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
            (void)NtSetEvent(hKbSynchEvent,NULL);
        }
    }

done:
    kb_wait_for_input_threads --;
    winx_exit_thread(0);
    return 0;
}

#define LIGHTING_REPEAT_COUNT 0x5
#define LIGHTING_REPEAT_DELAY 100 /* msec */

/**
 * @internal
 * @brief Lights up the keyboard indicators.
 * @param[in] hKbDevice the handle of the keyboard device.
 * @param[in] LedFlags the flags specifying
 * which indicators must be lighten up.
 * @return Zero for success, negative value otherwise.
 */
static int kb_light_up_indicators(HANDLE hKbDevice,USHORT LedFlags)
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    KEYBOARD_INDICATOR_PARAMETERS kip;

    kip.LedFlags = LedFlags;
    kip.UnitId = 0;

    status = NtDeviceIoControlFile(hKbDevice,NULL,NULL,NULL,
            &iosb,IOCTL_KEYBOARD_SET_INDICATORS,
            &kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS),NULL,0);
    if(NT_SUCCESS(status)){
        status = NtWaitForSingleObject(hKbDevice,FALSE,NULL);
        if(NT_SUCCESS(status)) status = iosb.Status;
    }
    if(!NT_SUCCESS(status) || status == STATUS_PENDING){
        strace(status,"cannot light up the keyboard"
            " indicators 0x%x",(UINT)LedFlags);
        return (-1);
    }
    
    return 0;
}

/**
 * @internal
 * @brief Checks the keyboard for existence.
 * @param[in] hKbDevice the handle of the keyboard device.
 * @return Zero for success, negative value otherwise.
 */
static int kb_check(HANDLE hKbDevice)
{
    USHORT LedFlags;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    KEYBOARD_INDICATOR_PARAMETERS kip;
    int i;
    
    /* try to get LED flags */
    RtlZeroMemory(&kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS));
    status = NtDeviceIoControlFile(hKbDevice,NULL,NULL,NULL,
            &iosb,IOCTL_KEYBOARD_QUERY_INDICATORS,NULL,0,
            &kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS));
    if(NT_SUCCESS(status)){
        status = NtWaitForSingleObject(hKbDevice,FALSE,NULL);
        if(NT_SUCCESS(status)) status = iosb.Status;
    }
    if(!NT_SUCCESS(status) || status == STATUS_PENDING){
        strace(status,"cannot get keyboard indicators state");
        return (-1);
    }

    LedFlags = kip.LedFlags;
    
    /* light up LED's */
    for(i = 0; i < LIGHTING_REPEAT_COUNT; i++){
        (void)kb_light_up_indicators(hKbDevice,KEYBOARD_NUM_LOCK_ON);
        winx_sleep(LIGHTING_REPEAT_DELAY);
        (void)kb_light_up_indicators(hKbDevice,KEYBOARD_CAPS_LOCK_ON);
        winx_sleep(LIGHTING_REPEAT_DELAY);
        (void)kb_light_up_indicators(hKbDevice,KEYBOARD_SCROLL_LOCK_ON);
        winx_sleep(LIGHTING_REPEAT_DELAY);
    }

    (void)kb_light_up_indicators(hKbDevice,LedFlags);
    return 0;
}

/**
 * @internal
 * @brief Opens a keyboard device.
 * @param[in] device_number the number of the keyboard device.
 * @return Zero for success, negative value otherwise.
 */
static int kb_open_device(int device_number)
{
    wchar_t device_name[32];
    wchar_t event_name[32];
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    HANDLE hKbDevice = NULL;
    HANDLE hKbEvent = NULL;
    int i;

    (void)_snwprintf(device_name,32,L"\\Device\\KeyboardClass%u",device_number);
    device_name[31] = 0;
    RtlInitUnicodeString(&us,device_name);
    InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
    status = NtCreateFile(&hKbDevice,
                GENERIC_READ | FILE_RESERVE_OPFILTER | FILE_READ_ATTRIBUTES/*0x80100080*/,
                &oa,&iosb,NULL,FILE_ATTRIBUTE_NORMAL/*0x80*/,
                0,FILE_OPEN/*1*/,FILE_DIRECTORY_FILE/*1*/,NULL,0);
    if(!NT_SUCCESS(status)){
        /* don't litter logs */
        if(status != STATUS_OBJECT_NAME_NOT_FOUND){
            strace(status,"cannot open %ws",device_name);
            winx_printf("\nCannot open the keyboard %ws: %x!\n",
                device_name,(UINT)status);
            winx_printf("%s\n",winx_get_status_description((ULONG)status));
        }
        return (-1);
    }
    
    /* ensure that we have opened a really connected keyboard */
    if(kb_check(hKbDevice) < 0){
        etrace("invalid keyboard device %ws",device_name);
        winx_printf("\nInvalid keyboard device %ws!\n",device_name);
        NtCloseSafe(hKbDevice);
        return (-1);
    }
    
    /* create an event for the kb_wait_for_input procedure */
    (void)_snwprintf(event_name,32,L"\\kb_event%u",device_number);
    event_name[31] = 0;
    if(winx_create_event(event_name,SynchronizationEvent,&hKbEvent) < 0){
        winx_printf("\nCannot create %ws event!\n",event_name);
        NtCloseSafe(hKbDevice);
        return (-1);
    }
    (void)NtClearEvent(hKbEvent);
    
    /* add information to the kb array */
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

    /* this case is very extraordinary */
    winx_printf("\nkb array is full!\n");
    winx_destroy_event(hKbEvent);
    NtCloseSafe(hKbDevice);
    return (-1);
}

/**
 * @internal
 * @brief Opens all initialized keyboards.
 * @return Zero for success, negative value otherwise.
 */
static int kb_open(void)
{
    wchar_t event_name[64];
    int i;

    /* initialize kb array */
    memset((void *)kb,0,sizeof(kb));
    number_of_keyboards = 0;
 
    /* create synchronization event for safe access to kids array */
    _snwprintf(event_name,64,L"\\winx_kb_synch_event_%u",
        (unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess));
    event_name[63] = 0;
    if(winx_create_event(event_name,SynchronizationEvent,&hKbSynchEvent) < 0){
        winx_printf("\nCannot create %ws event!\n\n",event_name);
        return (-1);
    }
    (void)NtSetEvent(hKbSynchEvent,NULL);

    /* open all initialized keyboards */
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++) kb_open_device(i);
    
    /* start threads waiting for user input */
    stop_kb_wait_for_input = 0;
    kb_wait_for_input_threads = 0;
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
        if(kb[i].hKbDevice == NULL) break;
        if(winx_create_thread(kb_wait_for_input,(LPVOID)&kb[i]) < 0){
            winx_printf("\nCannot create thread gathering "
                "input from \\Device\\KeyboardClass%u\n\n",
                kb[i].device_number);
            /* stop all threads */
            kb_close();
            return (-1);
        }
    }
    
    /* return zero if at least one keyboard found */
    return (kb[0].hKbDevice) ? 0 : (-1);
}

/**
 * @internal
 * @brief Closes all opened keyboards.
 */
void kb_close(void)
{
    int i;
    
    /* stop threads waiting for user input */
    stop_kb_wait_for_input = 1; i = 0;
    /* 3 sec should be enough; otherwise sometimes it hangs */
    while(kb_wait_for_input_threads && i < 30){
        winx_sleep(STOP_KB_WAIT_INTERVAL);
        i++;
    }
    if(kb_wait_for_input_threads){
        winx_printf("Keyboards polling terminated forcibly...\n");
        winx_sleep(2000);
    }
    
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
        if(kb[i].hKbDevice == NULL) break;
        NtCloseSafe(kb[i].hKbDevice);
        NtCloseSafe(kb[i].hKbEvent);
        /* don't reset device_number member here */
        number_of_keyboards --;
    }
    
    /* destroy synchronization event */
    winx_destroy_event(hKbSynchEvent);
    hKbSynchEvent = NULL;
}

/*
**************************************************************
*                   interface functions                      *
**************************************************************
*/

/**
 * @brief Prepares all existing keyboards
 * for work with user input related procedures.
 * @return Zero for success, negative value otherwise.
 * @note This routine does not intended to be called
 * more than once.
 */
int winx_kb_init(void)
{
    KBD_RECORD kbd_rec;
    int i;
    
    /*
    * Open all PS/2 keyboards - 
    * they're ready immediately.
    */
    (void)kb_open();
    
    /*
    * Give USB keyboards time
    * for initialization.
    */
    winx_printf("\nWait for keyboard initialization (hit Esc to skip) ");
    for(i = 0; i < KB_INIT_DELAY; i++){
        kb_read_time_elapsed = 0;
        if(winx_kb_read(&kbd_rec,1000) == 0){
            if(kbd_rec.wVirtualScanCode == 0x1) break;
        } else if(!kb_read_time_elapsed){
            winx_sleep(1000);
        }
        winx_printf(".");
    }
    winx_printf(" [Done]\n\n");
    
    /*
    * Reset keyboard queues; open 
    * all the connected keyboards.
    */
    kb_close();
    return kb_open();
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
    
    DbgCheck1(pKID,-1);
    
    if(msec_timeout != INFINITE){
        attempts = msec_timeout / MAX_TYPING_DELAY + 1;
        xtime = winx_xtime();
    }
    
    while(number_of_keyboards){
        /* synchronize with other threads */
        synch_interval.QuadPart = MAX_WAIT_INTERVAL;
        Status = NtWaitForSingleObject(hKbSynchEvent,FALSE,&synch_interval);
        if(Status != WAIT_OBJECT_0){
            winx_printf("\nkb_read: synchronization failed: 0x%x\n",(UINT)Status);
            winx_printf("%s\n\n",winx_get_status_description((ULONG)Status));
            return (-1);
        }

        /* pop item from the keyboard queue */
        if(n_written > 0){
            memcpy(pKID,&kids[start_index],sizeof(KEYBOARD_INPUT_DATA));
            start_index ++;
            if(start_index >= KB_QUEUE_LENGTH)
                start_index = 0;
            n_written --;
            (void)NtSetEvent(hKbSynchEvent,NULL);
            return 0;
        }

        /* release synchronization event */
        (void)NtSetEvent(hKbSynchEvent,NULL);

        winx_sleep(MAX_TYPING_DELAY);
        if(msec_timeout != INFINITE){
            attempts --;
            if(attempts == 0){
                kb_read_time_elapsed = 1;
                break;
            }
            if(xtime && (winx_xtime() - xtime >= msec_timeout)){
                kb_read_time_elapsed = 1; break;
            }
        }
    }
    return (-1);
}

/** @} */
