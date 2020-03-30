/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
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

/*
* BootExecute Control program.
*/

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <commctrl.h>

#include "../include/ultradfgver.h"

int h_flag = 0;
int r_flag = 0;
int u_flag = 0;
int silent = 0;
int invalid_opts = 0;
char cmd[MAX_PATH + 1] = "";

void show_help(void)
{
    if(silent)
        return;
    
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

void DisplayLastError(char *caption)
{
    LPVOID lpMsgBuf;
    char buffer[128];
    DWORD error = GetLastError();

    if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,0,NULL)){
                if(error == ERROR_COMMITMENT_LIMIT){
                    (void)_snprintf(buffer,sizeof(buffer),
                            "Not enough memory.");
                } else {
                    (void)_snprintf(buffer,sizeof(buffer),
                            "Error code = 0x%x",(UINT)error);
                }
                buffer[sizeof(buffer) - 1] = 0;
                MessageBox(NULL,buffer,caption,MB_OK | MB_ICONHAND);
                return;
    } else {
        MessageBox(NULL,(LPCTSTR)lpMsgBuf,caption,MB_OK | MB_ICONHAND);
        LocalFree(lpMsgBuf);
    }
}

int open_smss_key(HKEY *phkey)
{
    LONG result;
    
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
          "SYSTEM\\CurrentControlSet\\Control\\Session Manager",
          0,KEY_QUERY_VALUE | KEY_SET_VALUE,phkey);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError("Cannot open SMSS key!");
        }
        return 0;
    }
    return 1;
}

/**
 * @brief Compares two boot execute commands.
 * @details Treats 'command' and 'autocheck command' as the same.
 * @param[in] reg_cmd command read from registry.
 * @param[in] cmd command to be searched for.
 * @return Positive value indicates that commands are equal,
 * zero indicates that they're different, negative value
 * indicates failure of comparison.
 * @note Internal use only.
 */
static int cmd_compare(char *reg_cmd,char *cmd)
{
    char *reg_cmd_copy = NULL;
    char *cmd_copy = NULL;
    char *long_cmd = NULL;
    char autocheck[] = "autocheck ";
    int length;
    int result = (-1);
    
    /* do we have the command registered as it is? */
    if(!strcmp(reg_cmd,cmd))
        return 1;
    
    /* allocate memory */
    reg_cmd_copy = _strdup(reg_cmd);
    if(reg_cmd_copy == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        goto done;
    }
    cmd_copy = _strdup(cmd);
    if(cmd_copy == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        goto done;
    }
    length = (strlen(cmd) + strlen(autocheck) + 1) * sizeof(char);
    long_cmd = malloc(length);
    if(long_cmd == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        goto done;
    }
    strcpy(long_cmd,autocheck);
    strcat(long_cmd,cmd);
    
    /* convert all strings to lowercase */
    _strlwr(reg_cmd_copy);
    _strlwr(cmd_copy);
    _strlwr(long_cmd);

    /* compare */
    if(!strcmp(reg_cmd_copy,cmd_copy) || !strcmp(reg_cmd_copy,long_cmd)){
        result = 1;
        goto done;
    }

    result = 0;
    
done:
    if(reg_cmd_copy) free(reg_cmd_copy);
    if(cmd_copy) free(cmd_copy);
    if(long_cmd) free(long_cmd);
    return result;
}

int register_cmd(void)
{
    HKEY hKey;
    DWORD type, size;
    char *data, *curr_pos;
    DWORD i, length, curr_len;
    LONG result;

    if(!open_smss_key(&hKey))
        return 3;

    type = REG_MULTI_SZ;
    result = RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
    if(result != ERROR_SUCCESS && result != ERROR_MORE_DATA){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError("Cannot query BootExecute value size!");
        }
        return 4;
    }
    
    data = malloc(size + strlen(cmd) + 10);
    if(data == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        (void)RegCloseKey(hKey);
        return 5;
    }

    type = REG_MULTI_SZ;
    result = RegQueryValueEx(hKey,"BootExecute",NULL,&type,data,&size);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError("Cannot query BootExecute value!");
        }
        (void)RegCloseKey(hKey);
        free(data);
        return 6;
    }
    
    if(size < 2){ /* "\0\0" */
        /* empty value detected */
        strcpy(data,cmd);
        data[strlen(data) + 1] = 0;
        length = strlen(data) + 1 + 1;
        goto save_changes;
    }
    
    /* terminate value by two zeros */
    data[size - 2] = 0;
    data[size - 1] = 0;

    length = size - 1;
    for(i = 0; i < length;){
        curr_pos = data + i;
        curr_len = strlen(curr_pos) + 1;
        /* if the command is yet registered then exit */
        if(cmd_compare(curr_pos,cmd) > 0) goto done;
        i += curr_len;
    }
    (void)strcpy(data + i,cmd);
    data[i + strlen(cmd) + 1] = 0;
    length += strlen(cmd) + 1 + 1;

save_changes:
    result = RegSetValueEx(hKey,"BootExecute",0,REG_MULTI_SZ,data,length);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError("Cannot set BootExecute value!");
        }
        (void)RegCloseKey(hKey);
        free(data);
        return 7;
    }

done:
    (void)RegCloseKey(hKey);
    free(data);
    return 0;
}

int unregister_cmd(void)
{
    HKEY hKey;
    DWORD type, size;
    char *data, *new_data = NULL, *curr_pos;
    DWORD i, length, new_length, curr_len;
    LONG result;

    if(!open_smss_key(&hKey))
        return 3;

    type = REG_MULTI_SZ;
    result = RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
    if(result != ERROR_SUCCESS && result != ERROR_MORE_DATA){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError("Cannot query BootExecute value size!");
        }
        return 4;
    }
    
    data = malloc(size);
    if(data == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        (void)RegCloseKey(hKey);
        return 5;
    }

    type = REG_MULTI_SZ;
    result = RegQueryValueEx(hKey,"BootExecute",NULL,&type,data,&size);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError("Cannot query BootExecute value!");
        }
        (void)RegCloseKey(hKey);
        free(data);
        return 6;
    }

    if(size < 2){ /* "\0\0" */
        /* empty value detected */
        goto done;
    }
    
    /* terminate value by two zeros */
    data[size - 2] = 0;
    data[size - 1] = 0;

    new_data = malloc(size);
    if(new_data == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        (void)RegCloseKey(hKey);
        free(data);
        return 5;
    }

    /*
    * Now we should copy all strings except specified command
    * to the destination buffer.
    */
    memset((void *)new_data,0,size);
    length = size - 1;
    new_length = 0;
    for(i = 0; i < length;){
        curr_pos = data + i;
        curr_len = strlen(curr_pos) + 1;
        if(cmd_compare(curr_pos,cmd) <= 0){
            (void)strcpy(new_data + new_length,curr_pos);
            new_length += curr_len;
        }
        i += curr_len;
    }
    new_data[new_length] = 0;

    result = RegSetValueEx(hKey,"BootExecute",0,REG_MULTI_SZ,new_data,new_length + 1);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError("Cannot set BootExecute value!");
        }
        (void)RegCloseKey(hKey);
        free(data);
        free(new_data);
        return 7;
    }

done:
    (void)RegCloseKey(hKey);
    free(data);
    free(new_data);
    return 0;
}

int parse_cmdline(void)
{
    int argc;
    short **argv;
    
    if(wcsstr(_wcslwr(GetCommandLineW()),L"/s"))
        silent = 1;

    argv = (short **)CommandLineToArgvW(_wcslwr(GetCommandLineW()),&argc);
    if(argv == NULL){
        if(!silent)
            DisplayLastError("CommandLineToArgvW failed!");
        exit(1);
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
                invalid_opts = 1;
                if(!silent)
                    MessageBox(NULL,"Command name is too long!","Error",MB_OK | MB_ICONHAND);
                exit(2);
            } else {
                _snprintf(cmd,MAX_PATH,"%ws",argv[argc]);
                cmd[MAX_PATH] = 0;
            }
        }
    }
    
    if(cmd[0] == 0)
        h_flag = 1;
    
    if(r_flag == 0 && u_flag == 0)
        h_flag = 1;
    
    GlobalFree(argv);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    int result = 0;
    
    /* strongly required! to be compatible with manifest */
    InitCommonControls();

    parse_cmdline();

    if(h_flag){
        show_help();
    } else if(r_flag){
        result = register_cmd();
    } else if(u_flag){
        result = unregister_cmd();
    }
    
    return result;
}
