/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file reg.c
 * @brief System registry.
 * @addtogroup Registry
 * @{
 */

/*
* Use standard RtlXxx functions whenever
* possible - they're much easier for use.
*/

#include "prec.h"
#include "zenwinx.h"

/**
 * @internal
 */
struct cmd {
    struct cmd *next;  /* pointer to the next command */
    struct cmd *prev;  /* pointer to the previous command */
    wchar_t *cmd;      /* the command name */
};

/*
**************************************************************
*                   auxiliary functions                      *
**************************************************************
*/

/**
 * @internal
 * @brief get_boot_exec_list helper.
 */
NTSTATUS NTAPI query_routine(PWSTR ValueName,
  ULONG ValueType,PVOID ValueData,ULONG ValueLength,
  PVOID Context,PVOID EntryContext)
{
    struct cmd **list = (struct cmd **)Context;
    struct cmd *command, *prev_command = NULL;
    wchar_t *cmd;
    
    if(list == NULL){
        etrace("the list is equal to NULL");
        return STATUS_UNSUCCESSFUL;
    }
    
    if(ValueType != REG_SZ){
        etrace("invalid %ws value type: 0x%x",
            ValueName, (UINT)ValueType);
        return STATUS_UNSUCCESSFUL;
    }

    cmd = winx_tmalloc(ValueLength + sizeof(wchar_t));
    if(cmd == NULL){
        mtrace();
        return STATUS_NO_MEMORY;
    }
    memset(cmd,0,ValueLength + sizeof(wchar_t));
    memcpy(cmd,ValueData,ValueLength);

    if(*list) prev_command = (*list)->prev;
    command = (struct cmd *)winx_list_insert(
        (list_entry **)(void *)list,
        (list_entry *)prev_command,
        sizeof(struct cmd));
    command->cmd = cmd;
    return STATUS_SUCCESS;
}

/**
 * @internal
 * @brief Retrieves the list of commands
 * registered to be executed at Windows boot.
 * @return Zero for success, a negative value
 * otherwise.
 */
static int get_boot_exec_list(struct cmd **list)
{
    NTSTATUS status;
    RTL_QUERY_REGISTRY_TABLE qt[] = {
        {query_routine, 0, L"BootExecute", NULL, REG_SZ, L"",  0},
        {NULL,          0, NULL,           NULL, 0,      NULL, 0}
    };
    
    status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
        L"Session Manager",qt,(PVOID)list,NULL);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot get list of boot execute commands");
        return (-1);
    }
    return 0;
}

/**
 * @internal
 * @brief Compares two commands intended to be executed at Windows boot.
 * @details Treats 'command' and 'autocheck command' as the same.
 * @param[in] reg_cmd the command read from the registry.
 * @param[in] cmd the command to be searched for.
 * @return A positive value if the commands are equal,
 * zero if they're different, a negative value in case
 * of comparison failure.
 */
static int cmd_compare(wchar_t *reg_cmd,const wchar_t *cmd)
{
    wchar_t *long_cmd;
    int result;
    
    /* do we have the command registered as it is? */
    if(!winx_wcsicmp(cmd,reg_cmd))
        return 1;
        
    /* compare reg_cmd with 'autocheck {cmd}' */
    long_cmd = winx_swprintf(L"autocheck %ws",cmd);
    if(long_cmd == NULL){
        mtrace();
        return (-1);
    }
        
    result = winx_wcsicmp(long_cmd,reg_cmd);
    winx_free(long_cmd);
    return (result == 0) ? 1 : 0;
}

/**
 * @internal
 * @brief Saves a list of commands intended to 
 * be executed at Windows boot to the registry.
 * @return Zero for success, a negative value otherwise.
 */
static int save_boot_exec_list(struct cmd *list)
{
    struct cmd *c;
    int length = 1;
    wchar_t *commands, *p;
    NTSTATUS status;
    
    for(c = list; c; c = c->next){
        if(c->cmd[0]) length += (int)wcslen(c->cmd) + 1;
        if(c->next == list) break;
    }
    commands = winx_malloc(length * sizeof(wchar_t));
    memset(commands,0,length * sizeof(wchar_t));
    for(c = list, p = commands; c; c = c->next){
        if(c->cmd[0]){
            wcscpy(p,c->cmd);
            p += wcslen(c->cmd) + 1;
        }
        if(c->next == list) break;
    }
    
    status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
        L"Session Manager",L"BootExecute",REG_MULTI_SZ,
        commands,length * sizeof(wchar_t));
    winx_free(commands);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot save list of boot execute commands");
        return (-1);
    }
    return 0;
}

/**
 * @internal
 * @brief Destroys a list of commands
 * intended to be executed at Windows boot.
 */
static void destroy_boot_exec_list(struct cmd *list)
{
    struct cmd *c;
    
    for(c = list; c; c = c->next){
        winx_free(c->cmd);
        if(c->next == list) break;
    }
    winx_list_destroy((list_entry **)(void *)&list);
}

/*
**************************************************************
*                   interface functions                      *
**************************************************************
*/

/**
 * @brief Checks wheter a command is registered for
 * being executed during the Windows boot process or not.
 * @param[in] command the name of the command's
 * executable, without extension.
 * @return A positive value if the command
 * is registered, zero if it isn't,
 * a negative value in case of check failure.
 */
int winx_bootex_check(const wchar_t *command)
{
    struct cmd *c, *list = NULL;
    int result = (-1);

    DbgCheck1(command,-1);
    
    if(!wcscmp(command,L""))
        return 0;
    
    /* get the list of the registered commands */
    if(get_boot_exec_list(&list) < 0) goto done;
    
    /* check for the specified command presence */
    result = 0;
    for(c = list; c; c = c->next){
        if(cmd_compare(c->cmd,command) > 0){
            result = 1;
            break;
        }
        if(c->next == list) break;
    }

done:
    destroy_boot_exec_list(list);
    return result;
}

/**
 * @brief Registers a command to be executed
 * during the Windows boot process.
 * @param[in] command the name of the command's
 * executable, without extension.
 * @return Zero for success, a negative value otherwise.
 * @note The command's executable must be placed inside 
 * the system32 directory to be executed successfully.
 */
int winx_bootex_register(const wchar_t *command)
{
    struct cmd *c, *list = NULL;
    struct cmd *prev_command = NULL;
    int cmd_found = 0;
    wchar_t *cmd_copy;
    int result = (-1);
    
    DbgCheck1(command,-1);

    if(!wcscmp(command,L""))
        return 0;
    
    /* get the list of the registered commands */
    if(get_boot_exec_list(&list) < 0) goto done;
    
    /* append the specified command whenever necessary */
    for(c = list; c; c = c->next){
        if(cmd_compare(c->cmd,command) > 0){
            cmd_found = 1;
            break;
        }
        if(c->next == list) break;
    }
    if(cmd_found){
        result = 0;
        goto done;
    }
    cmd_copy = winx_wcsdup(command);
    if(cmd_copy == NULL){
        mtrace();
        goto done;
    }
    if(list) prev_command = list->prev;
    c = (struct cmd *)winx_list_insert(
        (list_entry **)(void *)&list,
        (list_entry *)prev_command,
        sizeof(struct cmd));
    c->cmd = cmd_copy;
    
    /* save the list */
    result = save_boot_exec_list(list);

done:
    destroy_boot_exec_list(list);
    return result;
}

/**
 * @brief Deregisters a command from being
 * executed during the Windows boot process.
 * @param[in] command the name of the
 * command's executable, without extension.
 * @return Zero for success, a negative value otherwise.
 */
int winx_bootex_unregister(const wchar_t *command)
{
    struct cmd *c, *list = NULL;
    struct cmd *head, *next = NULL;
    int result = (-1);
    
    DbgCheck1(command,-1);

    if(!wcscmp(command,L""))
        return 0;
    
    /* get the list of the registered commands */
    if(get_boot_exec_list(&list) < 0) goto done;
    
    /* remove the command */
    for(c = list; c; c = next){
        head = list;
        next = c->next;
        if(cmd_compare(c->cmd,command) > 0){
            winx_list_remove((list_entry **)(void *)&list,
                (list_entry *)c);
        }
        if(list == NULL) break;
        if(next == head) break;
    }
    
    /* save the list */
    result = save_boot_exec_list(list);

done:
    destroy_boot_exec_list(list);
    return result;
}

/** @} */
