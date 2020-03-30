/*
 *  WGX - Windows GUI Extended Library.
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
 * @file web-analytics.c
 * @brief Web Analytics.
 * @addtogroup WebAnalytics
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "wgx.h"

#define MAX_GA_REQUEST_LENGTH 1024

typedef HRESULT (__stdcall *URLMON_PROCEDURE)(
    /* LPUNKNOWN */ void *lpUnkcaller,
    LPCSTR szURL,
    LPTSTR szFileName,
    DWORD cchFileName,
    DWORD dwReserved,
    /*IBindStatusCallback*/ void *pBSC
);

/* forward declaration */
DWORD WINAPI SendWebAnalyticsRequestThreadProc(LPVOID lpParameter);

/**
 * @internal
 * @brief Sends web analytics request.
 * @note Passed url must be allocated by malloc.
 */
static BOOL SendWebAnalyticsRequest(char *url)
{
    URLMON_PROCEDURE pURLDownloadToCacheFile;
    HMODULE hUrlmonDLL = NULL;
    HRESULT result;
    char path[MAX_PATH + 1] = {0};
    
    if(url == NULL){
        WgxDbgPrint("SendWebAnalyticsRequest: URL = NULL\n");
        goto fail;
    }
    
    WgxDbgPrint("SendWebAnalyticsRequest: URL = %s\n",url);
    
    /* load urlmon.dll library */
    hUrlmonDLL = LoadLibrary("urlmon.dll");
    if(hUrlmonDLL == NULL){
        WgxDbgPrintLastError("SendWebAnalyticsRequest: LoadLibrary(urlmon.dll) failed");
        goto fail;
    }
    
    /* get an address of procedure downloading a file */
    pURLDownloadToCacheFile = (URLMON_PROCEDURE)GetProcAddress(hUrlmonDLL,"URLDownloadToCacheFileA");
    if(pURLDownloadToCacheFile == NULL){
        WgxDbgPrintLastError("SendWebAnalyticsRequest: URLDownloadToCacheFile not found in urlmon.dll");
        goto fail;
    }
    
    /* download a file */
    result = pURLDownloadToCacheFile(NULL,url,path,MAX_PATH,0,NULL);
    path[MAX_PATH] = 0;
    if(result != S_OK){
        if(result == E_OUTOFMEMORY)
            WgxDbgPrint("SendWebAnalyticsRequest: not enough memory\n");
        else
            WgxDbgPrint("SendWebAnalyticsRequest: URLDownloadToCacheFile failed\n");
        goto fail;
    }
    
    /* remove cached data, otherwise it may not be loaded next time */
    if (path[0] != 0) (void)remove(path);
    
    /* free previously allocated url */
    free(url);
    return TRUE;
    
fail:
    free(url);
    return FALSE;
}

/**
 * @internal
 */
DWORD WINAPI SendWebAnalyticsRequestThreadProc(LPVOID lpParameter)
{
    (void)SendWebAnalyticsRequest((char *)lpParameter);
    return 0;
}

/**
 * @internal
 * @note Based on http://www.vdgraaf.info/google-analytics-without-javascript.html
 * and http://code.google.com/apis/analytics/docs/tracking/gaTrackingTroubleshooting.html
 */
static char *build_ga_request(char *hostname,char *path,char *account)
{
    int utmn, utmhid, cookie, random;
    __int64 today;
    char *ga_request;
    
    ga_request = malloc(MAX_GA_REQUEST_LENGTH);
    if(ga_request == NULL){
        WgxDbgPrint("build_ga_request: cannot allocate %u bytes of memory\n",
            MAX_GA_REQUEST_LENGTH);
        return NULL;
    }
    
    srand((unsigned int)time(NULL));
    utmn = (rand() << 16) + rand();
    utmhid = (rand() << 16) + rand();
    cookie = (rand() << 16) + rand();
    random = (rand() << 16) + rand();
    today = (__int64)time(NULL);
    
    (void)_snprintf(ga_request,MAX_GA_REQUEST_LENGTH,
        "http://www.google-analytics.com/__utm.gif?utmwv=4.6.5"
        "&utmn=%u"
        "&utmhn=%s"
        "&utmhid=%u"
        "&utmr=-"
        "&utmp=%s"
        "&utmac=%s"
        "&utmcc=__utma%%3D%u.%u.%I64u.%I64u.%I64u.50%%3B%%2B__utmz%%3D%u.%I64u.27.2.utmcsr%%3Dgoogle.com%%7Cutmccn%%3D(referral)%%7Cutmcmd%%3Dreferral%%7Cutmcct%%3D%%2F%3B",
        utmn,hostname,utmhid,path,account,
        cookie,random,today,today,today,cookie,today
        );
    ga_request[MAX_GA_REQUEST_LENGTH - 1] = 0;
    return ga_request;
}

/**
 * @brief Increases a Google Analytics counter
 * by sending the request directly to the service.
 * @param[in] hostname the name of the host.
 * @param[in] path the relative path of the page.
 * @param[in] account the account string.
 * @return Boolean value indicating the status of
 * the operation. TRUE means success, FALSE - failure.
 * @note This function is designed especially to
 * collect statistics about the use of the program,
 * or about the frequency of some error, or about
 * similar things through Google Analytics service.
 *
 * How it works. It sends a request to Google 
 * Analytics service as described here: 
 * http://code.google.com/apis/analytics/docs/
 * (see Troubleshooting section).
 * Thus it translates the frequency of some event
 * inside your application to the frequency of an 
 * access to some predefined webpage. Therefore
 * you will have a handy nice looking report of
 * application events frequencies. And moreover,
 * it will represent statistics globally, collected
 * from all instances of the program.
 *
 * It is not supposed to increase counters of your
 * website though! If you'll try to use it for 
 * illegal purposes, remember - you are forewarned
 * and we have no responsibility for your illegal 
 * actions.
 * @par Example 1:
 * @code
 * // collect statistics about the UltraDefrag GUI client use
 * IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/gui.html","UA-15890458-1");
 * @endcode
 * @par Example 2:
 * @code
 * if(cannot_define_windows_version){
 *     // collect statistics about this error frequency
 *     IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/errors/winver.html","UA-15890458-1");
 * }
 * @endcode
 */
BOOL IncreaseGoogleAnalyticsCounter(char *hostname,char *path,char *account)
{
    char *url;
    
    url = build_ga_request(hostname,path,account);
    if(url == NULL)
        return FALSE;
    
    return SendWebAnalyticsRequest(url);
}

/**
 * @internal
 * @brief An asynchronous equivalent 
 * of IncreaseGoogleAnalyticsCounter.
 * @note Runs in a separate thread.
 * @bug This routine is not safe.
 * If the calling process terminates before
 * the request completion, the program
 * crashes.
 */
void IncreaseGoogleAnalyticsCounterAsynch(char *hostname,char *path,char *account)
{
    char *url;
    HANDLE h;
    DWORD id;
    
    url = build_ga_request(hostname,path,account);
    if(url == NULL)
        return;
    
    h = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)SendWebAnalyticsRequestThreadProc,(void *)url,0,&id);
    if(h == NULL){
        WgxDbgPrintLastError("SendWebAnalyticsRequestAsynch: cannot create thread");
        free(url);
        return;
    }
    
    CloseHandle(h);
}

/** @} */
