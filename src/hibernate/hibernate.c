/*
 *  Hibernate for Windows - a command line tool for Windows hibernation.
 *  Copyright (c) 2009-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../dll/wgx/wgx.h"

static void show_help(void)
{
    printf(
        "===============================================================================\n"
        "Hibernate for Windows - a command line tool for Windows hibernation.\n"
        "Copyright (c) UltraDefrag Development Team, 2009-2012.\n"
        "\n"
        "===============================================================================\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n"
        "===============================================================================\n"
        "\n"
        "Usage: \n"
        "  hibernate now - hibernates PC\n"
        "  hibernate /?  - displays this help\n"
        );
}

static void dbg_print(char *format, ...)
{
    va_list arg;
    char *buffer;
    int size, result;
    
    if(format){
        va_start(arg,format);
        size = 128; /* set initial buffer size */
        do {
            buffer = malloc(size);
            if(buffer == NULL) break;
            memset(buffer,0,size); /* needed for _vsnprintf */
            result = _vsnprintf(buffer,size,format,arg);
            if(result != -1 && result != size){
                OutputDebugString(buffer);
                OutputDebugString("\n");
                free(buffer);
                break;
            }
            /* buffer is too small; try to allocate two times larger */
            free(buffer);
            size <<= 1;
            if(size <= 0) break;
        } while(1);
        va_end(arg);
    }
}

static void handle_error(char *msg)
{
    DWORD error;
    wchar_t *error_message;
    int length;

    error = GetLastError();
    /* format message and display it on the screen */
    if(FormatMessageW( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR)(void *)&error_message,
        0,
        NULL)){
            fprintf(stderr,"%s: ",msg);
            /* get rid of trailing dot and new line */
            length = wcslen(error_message);
            if(length > 0){
                if(error_message[length - 1] == '\n')
                    error_message[length - 1] = 0;
            }
            if(length > 1){
                if(error_message[length - 2] == '\r')
                    error_message[length - 2] = 0;
            }
            if(length > 2){
                if(error_message[length - 3] == '.')
                    error_message[length - 3] = 0;
            }
            WgxPrintUnicodeString(error_message,stderr);
            fprintf(stderr,"!\n");
            LocalFree(error_message);
    } else {
        if(error == ERROR_COMMITMENT_LIMIT)
            fprintf(stderr,"%s: not enough memory!\n",msg);
        else
            fprintf(stderr,"%s: unknown reason!\n",msg);
    }
}

int __cdecl main(int argc, char **argv)
{
    int i, now = 0;
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp;

    for(i = 0; i < argc; i++){
        if(!strcmp(argv[i],"now")) now = 1;
        if(!strcmp(argv[i],"NOW")) now = 1;
    }

    if(now == 0){
        show_help();
        return 0;
    }

    printf("Hibernate for Windows - a command line tool for Windows hibernation.\n");
    printf("Copyright (c) UltraDefrag Development Team, 2009-2012.\n\n");
    
    WgxSetDbgPrintHandler(dbg_print);

    /* enable shutdown privilege */
    if(!OpenProcessToken(GetCurrentProcess(), 
    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)){
        handle_error("Cannot open process token");
        return 1;
    }
    
    LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
    AdjustTokenPrivileges(hToken,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);         
    if(GetLastError() != ERROR_SUCCESS){
        handle_error("Cannot set shutdown privilege");
        return 1;
    }
    
    /* hibernate, request permission from apps and drivers */
    /* the second parameter must be FALSE, dmitriar's windows xp hangs otherwise */
    if(!SetSystemPowerState(FALSE,FALSE)){
        handle_error("Cannot hibernate the computer");
        return 1;
    }
    return 0;
}
