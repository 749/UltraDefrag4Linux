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
 * @file privilege.c
 * @brief User privileges.
 * @addtogroup Privileges
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Enables a user privilege for the current process.
 * @param[in] luid the identifier of the requested privilege, 
 * ntndk.h file contains definitions of various privileges.
 * @return Zero for success, negative value otherwise.
 * @par Example:
 * @code
 * winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
 * @endcode
 */
int winx_enable_privilege(unsigned long luid)
{
    NTSTATUS status;
    SIZE_T WasEnabled; /* boolean value receiving the previous state */
    
    status = RtlAdjustPrivilege((SIZE_T)luid, TRUE, FALSE, &WasEnabled);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot enable privilege %x",(UINT)luid);
        return (-1);
    }
    return 0;
}

/** @} */
