/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file event.c
 * @brief Events.
 * @addtogroup Events
 * @{
 */

#include "prec.h"
#include "zenwinx.h"

/**
 * @brief Creates an event.
 * @param[in] name the name of the event.
 * @param[in] type the type: either
 * SynchronizationEvent or NotificationEvent.
 * @param[out] phandle pointer to variable
 * to store the event's handle into.
 * @return Zero for success, a negative value otherwise.
 * @note
 * - The initial state of the successfully created
 *   event is signaled.
 * - If the event already exists this function returns
 *   STATUS_OBJECT_NAME_COLLISION defined in ntndk.h file.
 */
int winx_create_event(wchar_t *name,int type,HANDLE *phandle)
{
    UNICODE_STRING us;
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;

    DbgCheck3(name,(type == SynchronizationEvent) \
        || (type == NotificationEvent),phandle,-1);
    *phandle = NULL;

    RtlInitUnicodeString(&us,name);
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    status = NtCreateEvent(phandle,STANDARD_RIGHTS_ALL | 0x1ff,&oa,type,1);
    if(status == STATUS_OBJECT_NAME_COLLISION){
        *phandle = NULL;
        dtrace("%ws already exists",name);
        return (int)STATUS_OBJECT_NAME_COLLISION;
    }
    if(!NT_SUCCESS(status)){
        *phandle = NULL;
        strace(status,"cannot create %ws",name);
        return (-1);
    }
    return 0;
}

/**
 * @brief Opens an event.
 * @param[in] name the name of the event.
 * @param[in] flags the same flags as in Win32
 * OpenEvent() call's dwDesiredAccess parameter.
 * @param[out] phandle pointer to variable
 * to store the event's handle into.
 * @return Zero for success, a negative value otherwise.
 */
int winx_open_event(wchar_t *name,int flags,HANDLE *phandle)
{
    UNICODE_STRING us;
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;

    DbgCheck2(name,phandle,-1);
    *phandle = NULL;

    RtlInitUnicodeString(&us,name);
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    status = NtOpenEvent(phandle,flags,&oa);
    if(!NT_SUCCESS(status)){
        *phandle = NULL;
        strace(status,"cannot open %ws",name);
        return (-1);
    }
    return 0;
}

/**
 * @brief Destroys an event.
 * @param[in] h the event's handle.
 */
void winx_destroy_event(HANDLE h)
{
    if(h) NtClose(h);
}

/** @} */
