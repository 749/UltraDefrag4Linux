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
 * @file env.c
 * @brief Process environment.
 * @addtogroup Environment
 * @{
 */

#include "prec.h"
#include "zenwinx.h"

/*
* MSDN states that environment variables
* are limited by 32767 characters,
* including terminal zero.
*/
#define MAX_ENV_VALUE_LENGTH 32767

/**
 * @brief Queries an environment variable.
 * @param[in] name the name of the variable.
 * @return The value. NULL indicates failure.
 * @note The returned string should be released
 * by the winx_free call after its use.
 */
wchar_t *winx_getenv(wchar_t *name)
{
    wchar_t *value;
    UNICODE_STRING n, v;
    NTSTATUS status;
    
    DbgCheck1(name,NULL);
    
    value = winx_malloc(MAX_ENV_VALUE_LENGTH * sizeof(wchar_t));

    RtlInitUnicodeString(&n,name);
    v.Buffer = value;
    v.Length = 0;
    v.MaximumLength = MAX_ENV_VALUE_LENGTH * sizeof(wchar_t);
    status = RtlQueryEnvironmentVariable_U(NULL,&n,&v);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot query %ws",name);
        winx_free(value);
        return NULL;
    }
    if(value[0] == 0){
        winx_free(value);
        return NULL;
    }
    return value;
}

/**
 * @brief Sets an environment variable.
 * @param[in] name the name of the variable.
 * @param[in] value the value; NULL causes deletion of the variable.
 * @return Zero for success, a negative value otherwise.
 * @note The value must not exceed 32767 characters,
 * including terminal zero, as mentioned in MSDN.
 */
int winx_setenv(wchar_t *name, wchar_t *value)
{
    UNICODE_STRING n, v;
    NTSTATUS status;

    DbgCheck1(name,-1);

    RtlInitUnicodeString(&n,name);
    if(value){
        if(value[0]){
            RtlInitUnicodeString(&v,value);
            status = RtlSetEnvironmentVariable(NULL,&n,&v);
        } else {
            status = RtlSetEnvironmentVariable(NULL,&n,NULL);
        }
    } else {
        status = RtlSetEnvironmentVariable(NULL,&n,NULL);
    }
    if(!NT_SUCCESS(status)){
        strace(status,"cannot set %ws",name);
        return (-1);
    }
    return 0;
}

/** @} */
