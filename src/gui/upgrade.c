/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2010-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file upgrade.c
 * @brief Upgrade.
 * @addtogroup Upgrade
 * @{
 */

/*
* Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
* and Dmitri Arkhangelski <dmitriar@gmail.com>.
*/

#include "main.h"

#define VERSION_URL "http://ultradefrag.sourceforge.net/version.ini"
#define MAX_VERSION_FILE_LEN 32
#define MAX_ANNOUNCEMENT_LEN 128

char version_ini_path[MAX_PATH + 1];
char version_number[MAX_VERSION_FILE_LEN + 1];
short announcement[MAX_ANNOUNCEMENT_LEN];

typedef HRESULT (__stdcall *URLMON_PROCEDURE)(
    /* LPUNKNOWN */ void *lpUnkcaller,
    LPCSTR szURL,
    LPTSTR szFileName,
    DWORD cchFileName,
    DWORD dwReserved,
    /*IBindStatusCallback*/ void *pBSC
);

/* forward declaration */
DWORD WINAPI CheckForTheNewVersionThreadProc(LPVOID lpParameter);

/**
* @brief Retrieves the latest version number from the project's website.
* @return Version number string. NULL indicates failure.
* @note Based on http://msdn.microsoft.com/en-us/library/ms775122(VS.85).aspx
*/
static char *GetLatestVersion(void)
{
    URLMON_PROCEDURE pURLDownloadToCacheFile;
    HMODULE hUrlmonDLL = NULL;
    HRESULT result;
    FILE *f;
    int res;
    int i;
    
    /* load urlmon.dll library */
    hUrlmonDLL = LoadLibrary("urlmon.dll");
    if(hUrlmonDLL == NULL){
        WgxDbgPrintLastError("GetLatestVersion: LoadLibrary(urlmon.dll) failed");
        return NULL;
    }
    
    /* get an address of procedure downloading a file */
    pURLDownloadToCacheFile = (URLMON_PROCEDURE)GetProcAddress(hUrlmonDLL,"URLDownloadToCacheFileA");
    if(pURLDownloadToCacheFile == NULL){
        WgxDbgPrintLastError("GetLatestVersion: URLDownloadToCacheFile not found in urlmon.dll");
        return NULL;
    }
    
    /* download a file */
    result = pURLDownloadToCacheFile(NULL,VERSION_URL,version_ini_path,MAX_PATH,0,NULL);
    version_ini_path[MAX_PATH] = 0;
    if(result != S_OK){
        if(result == E_OUTOFMEMORY)
            WgxDbgPrint("GetLatestVersion: not enough memory for URLDownloadToCacheFile\n");
        else
            WgxDbgPrint("GetLatestVersion: URLDownloadToCacheFile failed\n");
        return NULL;
    }
    
    /* open the file */
    f = fopen(version_ini_path,"rb");
    if(f == NULL){
        WgxDbgPrint("GetLatestVersion: cannot open %s: %s\n",
            version_ini_path,_strerror(NULL));
        return NULL;
    }
    
    /* read version string */
    res = fread(version_number,1,MAX_VERSION_FILE_LEN,f);
    (void)fclose(f);
    /* remove cached data, otherwise it may not be loaded next time */
    (void)remove(version_ini_path);
    if(res == 0){
        WgxDbgPrint("GetLatestVersion: cannot read %s\n",version_ini_path);
        if(feof(f))
            WgxDbgPrint("File seems to be empty\n");
        return NULL;
    }
    
    /* remove trailing \r \n characters if they exists */
    version_number[res] = 0;
    for(i = res - 1; i >= 0; i--){
        if(version_number[i] != '\r' && version_number[i] != '\n') break;
        version_number[i] = 0;
    }
    
    return version_number;
}

/**
 * @brief Defines whether a new version is available for download or not.
 * @return A string containing an announcement. NULL indicates that
 * there is no new version available.
 */
static short *GetNewVersionAnnouncement(void)
{
    char *lv;
    char *cv = VERSIONINTITLE;
    int lmj, lmn, li; /* latest version numbers */
    int cmj, cmn, ci; /* current version numbers */
    int res;

    lv = GetLatestVersion();
    if(lv == NULL) return NULL;
    
    /*lv[2] = '4';*/
    res = sscanf(lv,"%u.%u.%u",&lmj,&lmn,&li);
    if(res != 3){
        WgxDbgPrint("GetNewVersionAnnouncement: the first sscanf call returned %u\n",res);
        return NULL;
    }
    res = sscanf(cv,"UltraDefrag %u.%u.%u",&cmj,&cmn,&ci);
    if(res != 3){
        WgxDbgPrint("GetNewVersionAnnouncement: the second sscanf call returned %u\n",res);
        return NULL;
    }
    
    /* 5.0.0 > 4.99.99 */
    if(lmj * 10000 + lmn * 100 + li > cmj * 10000 + cmn * 100 + ci){
        _snwprintf(announcement,MAX_ANNOUNCEMENT_LEN,L"%hs%ws",
            lv,L" release is available for download!");
        announcement[MAX_ANNOUNCEMENT_LEN - 1] = 0;
        
        WgxDbgPrint("GetNewVersionAnnouncement: upgrade to %s\n",lv);
        return announcement;
    }
    
    return NULL;
}

/**
 * @brief Checks for the new version available for download.
 * @note Runs in a separate thread to speedup the GUI window displaying.
 */
void CheckForTheNewVersion(void)
{
    HANDLE h;
    DWORD id;
    
    if(disable_latest_version_check) return;
    
    h = create_thread(CheckForTheNewVersionThreadProc,NULL,&id);
    if(h == NULL){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONWARNING,
            "Cannot create thread checking the latest version of the program!");
    } else {
        CloseHandle(h);
    }
}

DWORD WINAPI CheckForTheNewVersionThreadProc(LPVOID lpParameter)
{
    short *s;
    
    s = GetNewVersionAnnouncement();
    if(s){
        if(MessageBoxW(hWindow,s,L"You can upgrade me ^-^",MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
            (void)WgxShellExecuteW(hWindow,L"open",L"http://ultradefrag.sourceforge.net",NULL,NULL,SW_SHOW);
    }
    
    return 0;
}

/** @} */
