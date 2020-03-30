/*
 *  UltraDefrag debugger.
 *  Copyright (c) 2015, 2016 Dmitri Arkhangelski (dmitriar@gmail.com).
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

#ifndef _UDEFRAG_DBG_MAIN_H_
#define _UDEFRAG_DBG_MAIN_H_

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>

#if defined(__GNUC__)
HRESULT WINAPI URLDownloadToCacheFileW(
    /* LPUNKNOWN */ void *lpUnkcaller,
    LPCWSTR szURL,
    LPWSTR szFileName,
    DWORD cchFileName,
    DWORD dwReserved,
    /*IBindStatusCallback*/ void *pBSC
);
#endif

#include "../include/dbg.h"
#include "../include/version.h"
#include "../dll/zenwinx/zenwinx.h"

#define APP_ICON     100
#define DUMMY_DIALOG 101

#define TRACKING_ACCOUNT_ID L"UA-70148850-1"

int init(void);

#endif /* _UDEFRAG_DBG_MAIN_H_ */
