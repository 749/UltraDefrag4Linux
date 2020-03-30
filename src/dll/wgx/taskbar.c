/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file taskbar.c
 * @brief Taskbar icon overlays.
 * @addtogroup Taskbar
 * @{
 */

/*
* We use STATUS_WAIT_0...
* #define WIN32_NO_STATUS
*/
#include <windows.h>
#include "wgx.h"
#include "taskbar.h"
#include <objbase.h>

/**
 * @return Nonzero value indicates
 * that the program is running
 * on Windows 7 or more recent
 * Windows edition.
 */
static int AtLeastWin7(void)
{
    OSVERSIONINFO osvi;
    
    memset(&osvi,0,sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    (void)GetVersionEx(&osvi);
    if(osvi.dwMajorVersion < 6)
        return 0; 
    if(osvi.dwMajorVersion == 6 && osvi.dwMinorVersion < 1)
        return 0;
    return 1;
}

/**
 * @brief Sets an overlay icon for
 * the application's taskbar icon.
 * @param[in] hWindow handle to the main
 * application's window.
 * @param[in] hInstance handle to 
 * an instance of the module
 * containing the icon resource.
 * @param[in] resource_id the identifier
 * of the overlay icon resource.
 * Identifier equal to -1 causes icon removal.
 * @param[in] description the description for 
 * the overlay icon, needed for accessibility.
 * @return Boolean value. TRUE indicates success.
 * @note Taskbar icon overlays are supported
 * by Windows 7 and more recent Windows editions.
 * On older Windows editions this routine does nothing.
 */
BOOL WgxSetTaskbarIconOverlay(HWND hWindow,HINSTANCE hInstance,int resource_id, wchar_t *description)
{
    GUID clsid = CLSID_TaskbarList;
    GUID iid = IID_ITaskbarList3;
    ITaskbarList3 *iTBL;
    HRESULT hr;
    HICON hIcon;
    UINT icon_size;
    BOOL result = FALSE;

    /* Windows 7 is required for icon overlays */
    if(!AtLeastWin7()) return TRUE;

    CoInitialize(NULL);
    hr = CoCreateInstance(&clsid,NULL,
        CLSCTX_INPROC_SERVER,&iid,
        (void **)(void *)&iTBL);
    if(SUCCEEDED(hr) && iTBL != NULL){
        /* set icon */
        if(resource_id == -1){
            hIcon = NULL, description = NULL;
        } else {
            icon_size = GetSystemMetrics(SM_CXICON);
            icon_size = icon_size ? icon_size / 2 : 16;
            (void)WgxLoadIcon(hInstance,resource_id,icon_size,&hIcon);
        }
        hr = ITaskbarList3_SetOverlayIcon(iTBL,hWindow,hIcon,description);
        if(SUCCEEDED(hr)){
            result = TRUE;
        } else {
            WgxDbgPrint("ITaskbarList3_SetOverlayIcon failed with code 0x%x",(UINT)hr);
        }
        /* cleanup */
        DestroyIcon(hIcon);
        ITaskbarList3_Release(iTBL);
    } else {
        WgxDbgPrint("WgxSetTaskbarIconOverlay failed with code 0x%x",(UINT)hr);
    }
    CoUninitialize();
    return result;
}

/**
 * @brief Removes an overlay icon from
 * the application's taskbar icon.
 * @param[in] hWindow handle to the main
 * application's window.
 * @return Boolean value. TRUE indicates success.
 * @note Taskbar icon overlays are supported
 * by Windows 7 and more recent Windows editions.
 * On older Windows editions this routine does nothing.
 */
BOOL WgxRemoveTaskbarIconOverlay(HWND hWindow)
{
    return WgxSetTaskbarIconOverlay(hWindow,NULL,-1,NULL);
}

/** @} */
