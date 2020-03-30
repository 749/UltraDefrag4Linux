/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2016 Dmitri Arkhangelski (dmitriar@gmail.com).
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

/*
* UltraDefrag debugger.
*/

#include "main.h"

udefrag_shared_data *sd = NULL;

wchar_t url[1024];
wchar_t tracking_path[1024];

/**
 * @brief Tries to pick up shared objects.
 * @return Positive value for success,
 * zero otherwise.
 */
static int pick_up_shared_objects(void)
{
    int id; char path[64];
    HANDLE hSynchEvent;
    HANDLE hSharedMemory;

    if(sd) return 1; // already picked up

    id = GetCurrentProcessId();
    _snprintf(path,64,"udefrag-synch-event-%u",id);
    hSynchEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,path);
    if(hSynchEvent == NULL) return 0;

    _snprintf(path,64,"udefrag-shared-mem-%u",id);
    hSharedMemory = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,path);
    if(hSharedMemory == NULL){
        CloseHandle(hSynchEvent);
        return 0;
    }

    sd = (udefrag_shared_data *)MapViewOfFile(hSharedMemory,
        FILE_MAP_ALL_ACCESS,0,0,sizeof(udefrag_shared_data));
    if(sd == NULL){
        CloseHandle(hSharedMemory);
        CloseHandle(hSynchEvent);
        return 0;
    }

    return 1;
}

/**
 * @brief Sends crash report
 * via Google Analytics service.
 */
static void send_crash_report()
{
    wchar_t *v;
    int utmn, utmhid, cookie;
    int random; __int64 today;
    wchar_t *id = TRACKING_ACCOUNT_ID;
    wchar_t path[MAX_PATH + 1];
    HRESULT result;

    if(sd->version != SHARED_DATA_VERSION){
        etrace("version mismatch: %u != %u",
            sd->version,SHARED_DATA_VERSION);
        return;
    }

    itrace("exception code: 0x%x",sd->exception_code);
    itrace("exception address: %p",sd->exception_address);

    // return immediately if usage tracking is disabled
    v = winx_getenv(L"UD_DISABLE_USAGE_TRACKING");
    if(v){
        if(v[0] == '1'){
            itrace("usage tracking is disabled, so "
                "crash reporting is disabled as well");
            winx_free(v);
            return;
        }
        winx_free(v);
    }

    srand((unsigned int)time(NULL));
    utmn = (rand() << 16) + rand();
    utmhid = (rand() << 16) + rand();
    cookie = (rand() << 16) + rand();
    random = (rand() << 16) + rand();
    today = (__int64)time(NULL);

    switch(sd->exception_code){
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        // division by zero errors will likely occur in the main
        // executable, so it's good to record the exception address here
        _snwprintf(tracking_path,sizeof(tracking_path) / sizeof(wchar_t),
            L"/%hs/%ls/0x%x @ %p",wxUD_ABOUT_VERSION,sd->tracking_id,
            sd->exception_code,sd->exception_address
        );
        break;
    default:
        // memory corruption errors might show up anywhere, so the exception
        // address will likely be meaningless without the name of the module
        // and its version; nevertheless, memory corruption debugging is too
        // hard so let's record just the exception code here
        _snwprintf(tracking_path,sizeof(tracking_path) / sizeof(wchar_t),
            L"/%hs/%ls/0x%x",wxUD_ABOUT_VERSION,sd->tracking_id,
            sd->exception_code
        );
        break;
    }

    _snwprintf(url,sizeof(url) / sizeof(wchar_t),
        L"http://www.google-analytics.com/__utm.gif?utmwv=4.6.5"
        L"&utmn=%u&utmhn=ultradefrag.sourceforge.net&utmhid=%u&utmr=-"
        L"&utmp=%s&utmac=%s&utmcc=__utma%%3D%u.%u.%I64u.%I64u.%I64u."
        L"50%%3B%%2B__utmz%%3D%u.%I64u.27.2.utmcsr%%3Dgoogle.com%%7Cutmccn%%3D"
        L"(referral)%%7Cutmcmd%%3Dreferral%%7Cutmcct%%3D%%2F%%3B",
        utmn,utmhid,tracking_path,id,cookie,random,today,today,today,cookie,today
    );
    memset(path,0,sizeof(path));

    itrace("downloading %ls",url);

#ifdef SEND_CRASH_REPORTS
    result = URLDownloadToCacheFileW(NULL,url,path,MAX_PATH,0,NULL);
    if(result != S_OK){
        etrace("failed with code 0x%x",(UINT)result);
        return;
    }
#endif

    itrace("application crash reported successfully");
    (void)DeleteFileW(path);
}

/**
 * @brief Gives helpful
 * advice to the user.
 */
static void show_advice(void)
{
    MessageBox(NULL,
       "Oops, UltraDefrag crashed on your system.\n\n"
       "The program itself is known to be pretty\n"
       "stable, so you probably have either broken\n"
       "hardware or a virus infection. However, if\n"
       "it's not your case, dont' hesitate to report\n"
       "the crash via our bug tracker:\n\n"
       "http://sourceforge.net/p/ultradefrag/bugs/\n\n"
       "Also you might downgrade to UltraDefrag v6\n"
       "which is known to be very stable.",
       "UltraDefrag debugger",
       MB_OK | MB_ICONHAND | MB_SYSTEMMODAL
    );
}

int __cdecl main(int argc,char **argv)
{
    HANDLE hMutex;

    if(init() < 0){
        MessageBox(NULL,
            "Initialization failed!",
            "UltraDefrag debugger",
            MB_OK | MB_ICONHAND
        );
        return 1;
    }

    while(1){
        // check for parent process existence
        hMutex = OpenMutex(MUTEX_ALL_ACCESS,
            FALSE,"ultradefrag_mutex");

        // check for debugging data
        if(pick_up_shared_objects()){
            if(sd->ready){
                send_crash_report();
                show_advice();
                break;
            }
        }

        // run till the caller termination
        if(hMutex == NULL) break;

        // nothing to do at the moment
        CloseHandle(hMutex);
        Sleep(1000);
    }

    return 0;
}
