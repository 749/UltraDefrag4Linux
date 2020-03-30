/*
* Hibernate for Windows - a command line tool for Windows hibernation.
*
* Programmer:    Dmitri Arkhangelski (dmitriar@gmail.com)
* Creation date: October 2009
* License:       Public Domain
*/

/* Revised by Dmitri Arkhangelski, 2011 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void show_help(void)
{
    printf(
        "Hibernate for Windows - command line tool for Windows hibernation.\n\n"
        "Usage: \n"
        "  hibernate now - hibernates PC\n"
        "  hibernate /?  - displays this help\n"
        );
}

void HandleError(char *msg)
{
    DWORD error;
    LPVOID error_message;

    error = GetLastError();
    /* format message and display it on the screen */
    if(FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &error_message,
        0,
        NULL )){
            printf(msg,(LPCTSTR)error_message);
            LocalFree(error_message);
    } else {
        if(error == ERROR_COMMITMENT_LIMIT)
            printf(msg,"not enough memory");
        else
            printf(msg,"unknown reason");
    }
}

int __cdecl main(int argc, char **argv)
{
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 
    int i;
    int now = 0;

    for(i = 0; i < argc; i++){
        if(!strcmp(argv[i],"now")) now = 1;
        if(!strcmp(argv[i],"NOW")) now = 1;
    }

    if(now == 0){
        show_help();
    } else {
        /* enable shutdown privilege */
        if(!OpenProcessToken(GetCurrentProcess(), 
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)){
            HandleError("Cannot open process token: %s!\n");
            return 1;
        }
        
        LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
        tkp.PrivilegeCount = 1;  // one privilege to set    
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
        AdjustTokenPrivileges(hToken,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);         
        if(GetLastError() != ERROR_SUCCESS){
            HandleError("Cannot set shutdown privilege: %s!\n");
            return 1;
        }
        
        /* the second parameter must be FALSE, dmitriar's windows xp hangs otherwise */
        if(!SetSystemPowerState(FALSE,FALSE)){ /* hibernate, request permission from apps and drivers */
            HandleError("Cannot hibernate PC: %s!\n");
            return 1;
        }
    }
    return 0;
}
