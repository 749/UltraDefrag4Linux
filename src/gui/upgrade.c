/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2010-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
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

/*
* On Windows NT 4.0 and Windows 2000 the program
* will check the version.ini file for the most
* recent UltraDefrag 5.0.x release.
*/
#define VERSION_URL    "http://ultradefrag.sourceforge.net/version.ini"
/*
* On Windows XP and more recent Windows editions
* the program will check the version_xp.ini file
* for the most recent UltraDefrag release
* (starting from 6.0 and so on).
*/
#define VERSION_URL_XP "http://ultradefrag.sourceforge.net/version_xp.ini"
#define MAX_VERSION_FILE_LEN 32
#define MAX_ANNOUNCEMENT_LEN 128

char version_ini_path[MAX_PATH + 1];
char version_number[MAX_VERSION_FILE_LEN + 1];
wchar_t announcement[MAX_ANNOUNCEMENT_LEN];

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
    OSVERSIONINFO osvi;
    BOOL bIsWindowsXPorLater;
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
    
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    bIsWindowsXPorLater = 
       ( (osvi.dwMajorVersion > 5) ||
       ( (osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion >= 1) ));

    /* download a file */
    if(bIsWindowsXPorLater)
        result = pURLDownloadToCacheFile(NULL,VERSION_URL_XP,version_ini_path,MAX_PATH,0,NULL);
    else
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
static wchar_t *GetNewVersionAnnouncement(void)
{
    char *lv;
    char *cv = VERSIONINTITLE;
    char *string;
    int lmj, lmn, li; /* latest version numbers */
    int cmj, cmn, ci; /* current version numbers */
    int current_version, last_version;
    int unstable_version = 0;
    int upgrade_needed = 0;
    int res;
    wchar_t *text, *message;

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
    string = _strdup(cv);
    if(string){
        _strlwr(string);
        if(strstr(string,"alpha")) unstable_version = 1;
        else if(strstr(string,"beta")) unstable_version = 1;
        else if(strstr(string,"rc")) unstable_version = 1;
        free(string);
    }
    
    /* 5.0.0 > 4.99.99 */
    current_version = cmj * 10000 + cmn * 100 + ci;
    last_version = lmj * 10000 + lmn * 100 + li;
    if(last_version > current_version) upgrade_needed = 1;
    else if(last_version == current_version && unstable_version) upgrade_needed = 1;
    if(upgrade_needed){
        text = WgxGetResourceString(i18n_table,"UPGRADE_MESSAGE");
        if(text) message = text; else message = L"release is available for download!";
        _snwprintf(announcement,MAX_ANNOUNCEMENT_LEN,L"%hs %ws",lv,message);
        announcement[MAX_ANNOUNCEMENT_LEN - 1] = 0;
        free(text);
        
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
    if(disable_latest_version_check) return;
    
    if(!WgxCreateThread(CheckForTheNewVersionThreadProc,NULL)){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONWARNING,
            "Cannot create thread checking the latest version of the program!");
    }
}

DWORD WINAPI CheckForTheNewVersionThreadProc(LPVOID lpParameter)
{
    wchar_t *s, *text, *caption;
    
    s = GetNewVersionAnnouncement();
    if(s){
        text = WgxGetResourceString(i18n_table,"UPGRADE_CAPTION");
        if(text) caption = text; else caption = L"You can upgrade me ^-^";
        if(MessageBoxW(hWindow,s,caption,MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
            (void)WgxShellExecuteW(hWindow,L"open",L"http://ultradefrag.sourceforge.net",NULL,NULL,SW_SHOW);
        free(text);
    }
    
    return 0;
}

/** @} */
