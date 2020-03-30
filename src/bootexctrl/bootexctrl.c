/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
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

/*
* BootExecute Control program.
*/

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <commctrl.h>

#define WgxTraceHandler udefrag_dbg_print
#include "../dll/wgx/wgx.h"

#include "../dll/udefrag/udefrag.h"
#include "../include/ultradfgver.h"

#define DisplayError(msg) { \
    if(!silent) \
        MessageBox(NULL,msg, \
            "BootExecute Control", \
            MB_OK | MB_ICONHAND); \
}

int h_flag = 0;
int r_flag = 0;
int u_flag = 0;
int silent = 0;
wchar_t cmd[MAX_PATH + 1] = L"";

void show_help(void)
{
    if(!silent){
        MessageBox(NULL,
            "Usage:\n"
            "bootexctrl /r [/s] command - register command\n"
            "bootexctrl /u [/s] command - unregister command\n"
            "Specify /s option to run the program in silent mode."
            ,
            "BootExecute Control",
            MB_OK
        );
    }
}

int parse_cmdline(wchar_t *cmdline)
{
    wchar_t **argv;
    int argc;
    
    if(cmdline == NULL){
        h_flag = 1;
        return EXIT_SUCCESS;
    }
    
    argv = (wchar_t **)CommandLineToArgvW(cmdline,&argc);
    if(argv == NULL){
        if(!silent)
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
                L"BootExecute Control: CommandLineToArgvW failed!");
        return EXIT_FAILURE;
    }

    for(argc--; argc >= 1; argc--){
        if(!wcscmp(argv[argc],L"/r")){
            r_flag = 1;
        } else if(!wcscmp(argv[argc],L"/u")){
            u_flag = 1;
        } else if(!wcscmp(argv[argc],L"/h")){
            h_flag = 1;
        } else if(!wcscmp(argv[argc],L"/?")){
            h_flag = 1;
        } else if(!wcscmp(argv[argc],L"/s")){
            silent = 1;
        } else {
            if(wcslen(argv[argc]) > MAX_PATH){
                DisplayError("Command name is too long!");
                return EXIT_FAILURE;
            } else {
                wcscpy(cmd,argv[argc]);
            }
        }
    }
    
    if(!cmd[0] || (!r_flag && !u_flag))
        h_flag = 1;
    
    GlobalFree(argv);
    return EXIT_SUCCESS;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    wchar_t *cmdline = GetCommandLineW();
    int result;

    /* check for /s option */
    if(cmdline){
        _wcslwr(cmdline);
        if(wcsstr(cmdline,L"/s"))
            silent = 1;
    }

    /* strongly required! to be compatible with manifest */
    InitCommonControls();

    if(udefrag_init_library() < 0){
        DisplayError("Initialization failed!");
        return EXIT_FAILURE;
    }

    WgxSetInternalTraceHandler(udefrag_dbg_print);

    result = parse_cmdline(cmdline);
    if(result != EXIT_SUCCESS)
        return EXIT_FAILURE;

    if(h_flag){
        show_help();
        return EXIT_SUCCESS;
    }
    
    /* check for admin rights - they're strongly required */
    if(!WgxCheckAdminRights()){
        DisplayError("Administrative rights"
            " are needed to run the program!");
        return EXIT_FAILURE;
    }

    if(r_flag){
        result = udefrag_bootex_register(cmd);
        if(result < 0){
            DisplayError("Cannot register the command!\n"
                "Use DbgView program to get more information.");
        }
    } else {
        result = udefrag_bootex_unregister(cmd);
        if(result < 0){
            DisplayError("Cannot unregister the command!\n"
                "Use DbgView program to get more information.");
        }
    }
    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
