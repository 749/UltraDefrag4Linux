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

/*
* To include CheckTokenMembership definition 
* on mingw the _WIN32_WINNT constant must be
* set at least to 0x500.
*/
#if defined(__GNUC__)
#define _WIN32_WINNT 0x500
#endif
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <commctrl.h>

#include "../dll/zenwinx/zenwinx.h"

#define error(msg) { \
    if(!silent) MessageBox(NULL,msg, \
        "BootExecute Control", \
        MB_OK | MB_ICONHAND); \
}

char cmd[32768] = {0}; /* should be enough as MSDN states */
int silent = 0;

/**
 * @brief Defines whether the user
 * has administrative rights or not.
 * @details Based on the UserInfo
 * NSIS plug-in.
 */
static int check_admin_rights(void)
{
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = {SECURITY_NT_AUTHORITY};
    PSID psid = NULL;
    BOOL IsMember = FALSE;
    
    if(!AllocateAndInitializeSid(&SystemSidAuthority,2,
      SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,
      0,0,0,0,0,0,&psid)){
        letrace("cannot create the security identifier");
        return 0;
    }
      
    if(!CheckTokenMembership(NULL,psid,&IsMember)){
        letrace("cannot check token membership");
        if(psid) FreeSid(psid);
        return 0;
    }

    if(!IsMember) itrace("the user is not a member of administrators group");
    if(psid) FreeSid(psid);
    return IsMember;
}

int __cdecl main(int argc,char **argv)
{
    int h_flag = 0, r_flag = 0, u_flag = 0;
    int i, result;
    wchar_t *ucmd;

    /*
    * This call is mandatory for all applications
    * depending on comctl32 library, even through
    * the application manifest.
    */
    InitCommonControls();

    /* parse command line */
    for(i = 1; i < argc; i++){
        if(!_stricmp(argv[i],"/h")){
            h_flag = 1;
        } else if(!_stricmp(argv[i],"/r")){
            r_flag = 1;
        } else if(!_stricmp(argv[i],"/s")){
            silent = 1;
        } else if(!_stricmp(argv[i],"/u")){
            u_flag = 1;
        } else if(!_stricmp(argv[i],"/?")){
            h_flag = 1;
        } else {
            strncpy(cmd,argv[i],sizeof(cmd));
            cmd[sizeof(cmd) - 1] = 0;
        }
    }

    /* handle help requests */
    if(h_flag || !cmd[0] || (!r_flag && !u_flag)){
        if(!silent) MessageBox(NULL,
            "Usage:\n"
            "bootexctrl /r [/s] command - register command\n"
            "bootexctrl /u [/s] command - unregister command\n"
            "Specify /s option to run the program in silent mode.",
            "BootExecute Control",
            MB_OK
        );
        return EXIT_SUCCESS;
    }
    
    if(winx_init_library() < 0){
        error("Initialization failed!");
        return EXIT_FAILURE;
    }

    /*
    * Check for admin rights - 
    * they're strongly required.
    */
    if(!check_admin_rights()){
        error("Administrative rights are "
            "needed to run the program!");
        return EXIT_FAILURE;
    }

    /* convert command to Unicode */
    ucmd = winx_swprintf(L"%hs",cmd);
    if(!ucmd){
        error("Not enough memory!");
        return EXIT_FAILURE;
    }

    if(r_flag){
        result = winx_bootex_register(ucmd);
        if(result < 0){
            error("Cannot register the command!\n"
                "Use DbgView program to get more information.");
        }
    } else {
        result = winx_bootex_unregister(ucmd);
        if(result < 0){
            error("Cannot unregister the command!\n"
                "Use DbgView program to get more information.");
        }
    }
    winx_free(ucmd);
    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
