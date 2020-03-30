/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2011 by Stefan Pendl (stefanpe@users.sourceforge.net).
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
* UltraDefrag boot time (native) interface - native commands implementation.
*/

#include "defrag_native.h"

/**
 * @brief Defines whether @echo is on or not.
 */
int echo_flag = DEFAULT_ECHO_FLAG;

/*
* boot-off command modifies registry, 
* but shutdown\reboot commands releases its effect,
* because all registry modifications becomes lost.
* Therefore we track pending boot-off request through
* a special pending-boot-off file in windows directory.
*/
int pending_boot_off = 0;

/**
 * @brief Defines whether command is executed
 * in scripting mode or in interactive mode.
 */
int scripting_mode = 1;

/* help screen */
char *help_message[] = {
    "Interactive mode commands:",
    "",
    "  boot-off      - disable boot time defragger",
    "  boot-on       - enable boot time defragger",
    "  call          - execute the boot script or the specified",
    "                  command script",
    "  echo          - enable/disable command line display,",
    "                  display a message or the display status",
    "  exit          - continue Windows boot",
    "  help          - display this help screen",
    "  hexview       - display the contents of the specified",
    "                  file in a HEX viewer layout",
    "  history       - display the list of typed commands",
    "  man           - list or display available manual pages",
    "  pause         - halt the execution for the specified timeout",
    "                  or till a key is pressed",
    "  reboot        - reboot the PC",
    "  set           - list, set or clear environment variables",
    "  shutdown      - halt the PC",
    "  type          - display the contents of the boot script or",
    "                  the specified file",
    "  udefrag       - list, analyze, defrag or optimize disks",
    "",
    "    For detailed information execute 'man {command}'.",
    "",
    NULL
};

typedef int (*cmd_handler_proc)(int argc,short **argv,short **envp);
typedef struct {
    short *cmd_name;
    cmd_handler_proc cmd_handler;
} cmd_table_entry;

/* forward declarations */
extern cmd_table_entry cmd_table[];
static int boot_off_handler(int argc,short **argv,short **envp);
static int type_handler(int argc,short **argv,short **envp);

static int man_listing_terminator(void *user_defined_parameter)
{
    KBD_RECORD kbd_rec;

    /* give user a chance to break a likely hanged man listing */
    if(winx_kb_read(&kbd_rec,0) == 0){
        /* check for escape key hits */
        if(kbd_rec.wVirtualScanCode == 0x1)
            return 1;
    }
    return 0;
}

static int list_installed_man_pages(int argc,short **argv,short **envp)
{
    char windir[MAX_PATH + 1];
    char path[MAX_PATH + 1];
    short wpath[MAX_PATH + 1];
    winx_file_info *file, *filelist;
    WINX_FILE *f;
    int i, column;
    int max_columns = (MAX_LINE_WIDTH - 1) / 15;
    
    if(argc < 1)
        return (-1);

    /* get %winir% path */
    if(winx_get_windows_directory(windir,MAX_PATH) < 0){
        winx_printf("\n%ws: cannot get %%windir%% path\n\n",argv[0]);
        return (-1);
    }

    /* try to get list of installed man pages through winx_ftw call */
    _snwprintf(wpath,MAX_PATH,L"%hs\\UltraDefrag\\man",windir);
    wpath[MAX_PATH] = 0;
    filelist = winx_ftw(wpath,0,NULL,NULL,man_listing_terminator,NULL);
    if(filelist){
        winx_printf("Available Manual Pages:\n");
        for(file = filelist->prev, column = 0; file; file = file->prev){
            /* display man files only */
            _wcslwr(file->name);
            if(wcsstr(file->name,L".man")){
                winx_printf("%-15ws",file->name);
                column ++;
                if(column >= max_columns){
                    winx_printf("\n");
                    column = 0;
                }
            }
            if(file->prev == filelist->prev) break;
        }
        winx_printf("\n");
        winx_ftw_release(filelist);
    } else {
        winx_printf("Available Command Manuals:\n");
        /* cycle through names of existing commands */
        for(i = 0, column = 0; cmd_table[i].cmd_handler != NULL; i++){
            /* build path to the manual page */
            _snprintf(path,MAX_PATH,"%s\\UltraDefrag\\man\\%ws.man",windir,cmd_table[i].cmd_name);
            path[MAX_PATH] = 0;
            /* check for the page existence */
            f = winx_fopen(path,"r");
            if(f != NULL){
                winx_fclose(f);
                /* display man page filename on the screen */
                _snprintf(path,MAX_PATH,"%ws.man",cmd_table[i].cmd_name);
                path[MAX_PATH] = 0;
                winx_printf("%-15s",path);
                column ++;
                if(column >= max_columns){
                    winx_printf("\n");
                    column = 0;
                }
            }
        }
        winx_printf("%-15s\n","variables.man");
    }

    return 0;
}

/**
 * @brief man command handler.
 */
static int man_handler(int argc,short **argv,short **envp)
{
    short *type_argv[2];
    char path[MAX_PATH + 1];
    short wpath[MAX_PATH + 1];
    size_t native_prefix_length;
    
    if(argc < 1)
        return (-1);
    
    if(argc < 2){
        /* installed man pages listing is requested */
        return list_installed_man_pages(argc,argv,envp);
    }
    
    /* build path to requested manual page */
    if(winx_get_windows_directory(path,MAX_PATH) < 0){
        winx_printf("\n%ws: cannot get %%windir%% path\n\n",argv[0]);
        return (-1);
    }
    _snwprintf(wpath,MAX_PATH,L"%hs\\UltraDefrag\\man\\%ws.man",path,argv[1]);
    wpath[MAX_PATH] = 0;

    /* build argv for type command handler */
    type_argv[0] = L"man";
    /* skip native prefix in path */
    native_prefix_length = wcslen(L"\\??\\");
    if(wcslen(wpath) >= native_prefix_length)
        type_argv[1] = wpath + native_prefix_length;
    else
        type_argv[1] = wpath;
    return type_handler(2,type_argv,envp);
}

/**
 * @brief help command handler.
 */
static int help_handler(int argc,short **argv,short **envp)
{
    if(argc < 1)
        return (-1);

    /* check for individual command's help */
    if(argc > 1)
        return man_handler(argc,argv,envp);
    
    return winx_print_array_of_strings(help_message,
        MAX_LINE_WIDTH,MAX_DISPLAY_ROWS,
        DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
        scripting_mode ? 0 : 1);
}

/**
 * @brief history command handler.
 */
static int history_handler(int argc,short **argv,short **envp)
{
    winx_history_entry *entry;
    char **strings;
    int i, result;

    if(argc < 1)
        return (-1);

    if(scripting_mode || history.head == NULL)
        return 0;
    
    /* convert list of strings to array */
    strings = winx_heap_alloc((history.n_entries + 3) * sizeof(char *));
    if(strings == NULL){
        winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
            argv[0],(history.n_entries + 3) * sizeof(char *));
        return (-1);
    }
    strings[0] = "Typed commands history:";
    strings[1] = "";
    i = 2;
    for(entry = history.head; i < history.n_entries; entry = entry->next){
        if(entry->string){
            strings[i] = entry->string;
            i++;
        }
        if(entry->next == history.head) break;
    }
    strings[i] = NULL;
    
    winx_printf("\n");

    result = winx_print_array_of_strings(strings,
        MAX_LINE_WIDTH,MAX_DISPLAY_ROWS,
        DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,1);

    winx_heap_free(strings);
    return result;
}

/**
 * @brief echo command handler.
 */
static int echo_handler(int argc,short **argv,short **envp)
{
    int i;
    
    if(argc < 1)
        return (-1);

    /* check whether an empty line is requested */
    if(!wcscmp(argv[0],L"echo.")){
        winx_printf("\n");
        return 0;
    }
    
    /* check whether echo status is requested */
    if(argc < 2){
        if(echo_flag)
            winx_printf("echo is on\n");
        else
            winx_printf("echo is off\n");
        return 0;
    }
    
    /* handle on and off keys */
    if(argc == 2){
        if(!wcscmp(argv[1],L"on")){
            echo_flag = 1;
            return 0;
        } else if(!wcscmp(argv[1],L"off")){
            echo_flag = 0;
            return 0;
        }
    }
    
    /* handle echo command */
    for(i = 1; i < argc; i++){
        winx_printf("%ws",argv[i]);
        if(i != argc - 1)
            winx_printf(" ");
    }
    winx_printf("\n");
    return 0;
}

/**
 * @brief type command handler.
 */
static int type_handler(int argc,short **argv,short **envp)
{
    char path[MAX_PATH];
    short *filename;
    int i, length;
    size_t filesize;
    unsigned char *buffer, *second_buffer;
    int unicode_detected;
    char *strings[] = { NULL, NULL };
    int result;
    
    if(argc < 1)
        return (-1);

    /* display boot time script if filename is missing */
    if(argc < 2){
        if(winx_get_windows_directory(path,MAX_PATH) < 0){
            winx_printf("\n%ws: cannot get %%windir%% path\n\n",argv[0]);
            return (-1);
        }
        (void)strncat(path,"\\system32\\ud-boot-time.cmd",
                MAX_PATH - strlen(path) - 1);
    } else {
        length = 0;
        for(i = 1; i < argc; i++)
            length += wcslen(argv[i]) + 1;
        filename = winx_heap_alloc(length * sizeof(short));
        if(filename == NULL){
            winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
                argv[0],length * sizeof(short));
            return (-1);
        }
        filename[0] = 0;
        for(i = 1; i < argc; i++){
            wcscat(filename,argv[i]);
            if(i != argc - 1)
                wcscat(filename,L" ");
        }
        (void)_snprintf(path,MAX_PATH - 1,"\\??\\%ws",filename);
        path[MAX_PATH - 1] = 0;
        winx_heap_free(filename);
    }
    (void)filename;

    /* read file contents entirely */
    buffer = winx_get_file_contents(path,&filesize);
    if(buffer == NULL)
        return 0; /* file is empty or some error */
    
    /* terminate buffer by two zeros */
    buffer[filesize] = buffer[filesize + 1] = 0;

    /* check for UTF-16 signature which exists in files edited in Notepad */
    unicode_detected = 0;
    if(filesize >= sizeof(short)){
        if(buffer[0] == 0xFF && buffer[1] == 0xFE)
            unicode_detected = 1;
    }

    /* print file contents */
    if(unicode_detected){
        second_buffer = winx_heap_alloc(filesize + 1);
        if(second_buffer == NULL){
            winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
                argv[0],filesize + 1);
            winx_release_file_contents(buffer);
            return (-1);
        }
        (void)_snprintf(second_buffer,filesize + 1,"%ws",(short *)(buffer + 2));
        second_buffer[filesize] = 0;
        strings[0] = second_buffer;
        result = winx_print_array_of_strings(strings,MAX_LINE_WIDTH,
            MAX_DISPLAY_ROWS,DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
            scripting_mode ? 0 : 1);
        winx_heap_free(second_buffer);
    } else {
        strings[0] = buffer;
        result = winx_print_array_of_strings(strings,MAX_LINE_WIDTH,
            MAX_DISPLAY_ROWS,DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
            scripting_mode ? 0 : 1);
    }
    
    /* cleanup */
    winx_release_file_contents(buffer);
    return result;
}

/**
 * @brief hexview command handler.
 */
static int hexview_handler(int argc,short **argv,short **envp)
{
    char path[MAX_PATH];
    short *filename;
    int i, length;
    WINX_FILE *f;
    ULONGLONG size;
    size_t filesize;
    size_t bytes_to_read, bytes_to_print, n, j, k, m;
    #define SCREEN_BUFFER_SIZE (8 * (MAX_DISPLAY_ROWS - 5))
    unsigned char buffer[SCREEN_BUFFER_SIZE];
    int result;
    char *offset;
    KBD_RECORD kbd_rec;
    int escape_detected = 0;
    int break_detected = 0;
    char esq[] = {'\a', '\b', '\f', '\n', '\r', '\t', '\v', 0};
    int esq_found = 0;
    
    if(argc < 1)
        return (-1);

    if(argc < 2){
        winx_printf("\n%ws: file name must be specified\n\n",argv[0]);
        return (-1);
    }

    /* build the native path to the file */
    length = 0;
    for(i = 1; i < argc; i++)
        length += wcslen(argv[i]) + 1;
    filename = winx_heap_alloc(length * sizeof(short));
    if(filename == NULL){
        winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
            argv[0],length * sizeof(short));
        return (-1);
    }
    filename[0] = 0;
    for(i = 1; i < argc; i++){
        wcscat(filename,argv[i]);
        if(i != argc - 1)
            wcscat(filename,L" ");
    }
    (void)_snprintf(path,MAX_PATH - 1,"\\??\\%ws",filename);
    path[MAX_PATH - 1] = 0;
    winx_heap_free(filename);
    
    /* open the file */
    f = winx_fopen(path,"r");
    if(f == NULL){
        winx_printf("\n%ws: cannot open %s\n\n",argv[0],path);
        return (-1);
    }
    size = winx_fsize(f);
    if(size == 0){
        winx_fclose(f);
        return 0; /* nothing to display */
    }
#ifndef _WIN64
    if(size > 0xFFFFFFFF){
        winx_printf("\n%ws: files larger than ~4Gb aren\'t supported\n\n",argv[0]);
        winx_fclose(f);
        return (-1);
    }
#endif
    filesize = (size_t)size;
    
    /* read the file by portions needed to fill a single screen */
    offset = 0x0;
    while(filesize){
        bytes_to_read = min(SCREEN_BUFFER_SIZE,filesize);
        result = winx_fread(buffer,sizeof(char),bytes_to_read,f);
        if(result != bytes_to_read && winx_fsize(f) == size){
            winx_printf("\n%ws: cannot read %s\n\n",argv[0],path);
            winx_fclose(f);
            return (-1);
        }
        /* fill a screen */
        bytes_to_print = bytes_to_read;
        j = 0;
        while(bytes_to_print){
            n = min(8, bytes_to_print);
            winx_printf("%p: ",offset);
            for(k = 0; k < n; k++)
                winx_printf("%02x ",(UINT)buffer[j+k]);
            for(; k < 8; k++)
                winx_printf("   ");
            winx_printf("| ");
            for(k = 0; k < n; k++){
                /* replace escape sequences and 0x0 codes by spaces */
                esq_found = 0;
                for(m = 0; esq[m]; m++){
                    if(esq[m] == buffer[j+k]){
                        esq_found = 1;
                        break;
                    }
                }
                if(esq_found || buffer[j+k] == 0x0)
                    winx_printf(" ");
                else
                    winx_printf("%c",buffer[j+k]);
            }
            for(; k < 8; k++)
                winx_printf(" ");
            winx_printf("\n");
            offset += n;
            j += n;
            bytes_to_print -= n;
        }
        /* go to the next portion of data */
        filesize -= bytes_to_read;
        if(filesize && !scripting_mode){
            /* display prompt to hit any key in interactive mode */
            winx_printf("\n%s\n\n",DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY);
            /* wait for any key */
            if(winx_kb_read(&kbd_rec,INFINITE) < 0){
                break; /* break in case of errors */
            }
            /* check for escape */
            if(kbd_rec.wVirtualScanCode == 0x1){
                escape_detected = 1;
            } else if(kbd_rec.wVirtualScanCode == 0x1d){
                /* distinguish between control keys and break key */
                if(!(kbd_rec.dwControlKeyState & LEFT_CTRL_PRESSED) && \
                  !(kbd_rec.dwControlKeyState & RIGHT_CTRL_PRESSED)){
                    break_detected = 1;
                }
            }
            if(escape_detected || break_detected)
                break;
        }
    }

    /* cleanup */
    winx_fclose(f);
    return 0;
}

/**
 * @brief Displays list of all environment variables.
 */
static int list_environment_variables(int argc,short **argv,short **envp)
{
    char **strings;
    int i, j, n, length;
    int result;
    int filter_strings = 0;
    
    if(argc < 1 || envp == NULL)
        return (-1);
    
    if(envp[0] == NULL)
        return 0; /* nothing to print */
    
    if(argc > 1)
        filter_strings = 1;
    
    /* convert envp to array of ANSI strings */
    for(n = 0; envp[n] != NULL; n++) {}
    strings = winx_heap_alloc((n + 1) * sizeof(char *));
    if(strings == NULL){
        winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
            argv[0],(n + 1) * sizeof(char *));
        return (-1);
    }
    RtlZeroMemory((void *)strings,(n + 1) * sizeof(char *));
    for(i = 0, j = 0; i < n; i++){
        if(filter_strings && winx_wcsistr(envp[i],argv[1]) != (wchar_t *)envp[i])
            continue;
        length = wcslen(envp[i]);
        strings[j] = winx_heap_alloc((length + 1) * sizeof(char));
        if(strings[j] == NULL){
            winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
                argv[0],(length + 1) * sizeof(char));
            goto fail;
        }
        (void)_snprintf(strings[j],length + 1,"%ws",envp[i]);
        strings[j][length] = 0;
        j++;
    }
    /* print strings */
    result = winx_print_array_of_strings(strings,MAX_LINE_WIDTH,
        MAX_DISPLAY_ROWS,DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
        scripting_mode ? 0 : 1);
    /* cleanup */
    for(i = 0; i < n; i++){
        if(strings[i])
            winx_heap_free(strings[i]);
    }
    winx_heap_free(strings);
    return result;

fail:
    for(i = 0; i < n; i++){
        if(strings[i])
            winx_heap_free(strings[i]);
    }
    winx_heap_free(strings);
    return (-1);
}

/**
 * @brief set command handler.
 */
static int set_handler(int argc,short **argv,short **envp)
{
    int name_length = 0, value_length = 0;
    short *name = NULL, *value = NULL;
    int i, j, n, result;
    
    if(argc < 1)
        return (-1);

    /*
    * Check whether environment variables
    * listing is requested or not.
    */
    if(argc < 2){
        /* list all environment variables */
        return list_environment_variables(argc,argv,envp);
    } else {
        /* check whether the first parameter contains '=' character */
        if(!wcschr(argv[1],'=')){
            /*
            * List variables containing argv[1] string
            * in the beginning of their names.
            */
            return list_environment_variables(argc,argv,envp);
        }
        /* calculate name and value lengths */
        n = wcslen(argv[1]);
        for(i = 0; i < n; i++){
            if(argv[1][i] == '='){
                name_length = i;
                value_length = n - i - 1;
                break;
            }
        }
        /* validate '=' character position */
        if(name_length == 0 || (value_length == 0 && argc >= 3)){
            winx_printf("\n%ws: invalid syntax\n\n",argv[0]);
            return (-1);
        }
        /* append all remaining parts of the value string */
        for(i = 2; i < argc; i++)
            value_length += 1 + wcslen(argv[i]);
        /* allocate memory */
        name = winx_heap_alloc((name_length + 1) * sizeof(short));
        if(name == NULL){
            winx_printf("\n%ws: cannot allocate %u bytes of memory\n",
                argv[0],(name_length + 1) * sizeof(short));
            return (-1);
        }
        if(value_length){
            value = winx_heap_alloc((value_length + 1) * sizeof(short));
            if(value == NULL){
                winx_printf("\n%ws: cannot allocate %u bytes of memory\n",
                    argv[0],(value_length + 1) * sizeof(short));
                winx_heap_free(name);
                return (-1);
            }
        }
        /* extract name and value */
        n = wcslen(argv[1]);
        for(i = 0; i < n; i++){
            if(argv[1][i] == '=') break;
            name[i] = argv[1][i];
        }
        name[i] = 0;
        if(value_length){
            for(i++, j = 0; i < n; i++){
                value[j] = argv[1][i];
                j++;
            }
            value[j] = 0;
            for(i = 2; i < argc; i++){
                wcscat(value,L" ");
                wcscat(value,argv[i]);
            }
        }
        if(value_length){
            /* set environment variable */
            result = winx_set_env_variable(name,value);
            winx_heap_free(value);
        } else {
            /* clear environment variable */
            result = winx_set_env_variable(name,NULL);
        }
        /* handle a special case of %UD_LOG_FILE_PATH% */
        if(wcscmp(_wcsupr(name),L"UD_LOG_FILE_PATH") == 0){
            if(udefrag_set_log_file_path() < 0)
                winx_printf("\n%ws: udefrag_set_log_file_path failed\n");
        }
        winx_heap_free(name);
        return result;
    }
    
    return 0; /* this point will never be reached */
}

/**
 * @brief pause command handler.
 */
static int pause_handler(int argc,short **argv,short **envp)
{
    int msec;

    if(argc < 1)
        return (-1);

    /* check whether "Hit any key to continue..." prompt is requested */
    if(argc < 2){
        winx_printf("%s",PAUSE_MESSAGE);
        (void)winx_kbhit(INFINITE);
        winx_printf("\n\n");
        return 0;
    }
    
    /* pause execution for specified time interval */
    msec = (int)_wtol(argv[1]);
    winx_sleep(msec);
    return 0;
}

/**
 * @brief Executes pending boot-off command.
 * @return Boolean value indicating whether
 * pending boot-off command was detected or not.
 */
int ExecPendingBootOff(void)
{
    char path[MAX_PATH];
    WINX_FILE *f;

    if(winx_get_windows_directory(path,MAX_PATH) < 0){
        DebugPrint("ExecPendingBootOff: cannot get %%windir%% path");
        winx_printf("\nExecPendingBootOff: cannot get %%windir%% path\n\n");
        short_dbg_delay();
        return 0;
    }
    (void)strncat(path,"\\pending-boot-off",
            MAX_PATH - strlen(path) - 1);

    f = winx_fopen(path,"r");
    if(f == NULL) return 0;

    winx_fclose(f);
    if(boot_off_handler(0,NULL,NULL) < 0){
        short_dbg_delay();
    }
    if(winx_delete_file(path) < 0){
        DebugPrint("ExecPendingBootOff: cannot delete %%windir%%\\pending-boot-off file");
        winx_printf("\nExecPendingBootOff: cannot delete %%windir%%\\pending-boot-off file\n\n");
        short_dbg_delay();
    }
    winx_printf("\nPending boot-off command execution completed.\n");
    return 1;
}

static void SavePendingBootOffState(void)
{
    char path[MAX_PATH];
    WINX_FILE *f;
    char *comment = "UltraDefrag boot-off command is pending.";
    
    if(!pending_boot_off) return;

    if(winx_get_windows_directory(path,MAX_PATH) < 0){
        DebugPrint("SavePendingBootOffState: cannot get %%windir%% path");
        winx_printf("\nSavePendingBootOffState: cannot get %%windir%% path\n\n");
        short_dbg_delay();
        return;
    }
    (void)strncat(path,"\\pending-boot-off",
            MAX_PATH - strlen(path) - 1);
    f = winx_fopen(path,"w");
    if(f == NULL){
        DebugPrint("%%windir%%\\pending-boot-off file creation failed");
        winx_printf("\n%%windir%%\\pending-boot-off file creation failed\n\n");
        short_dbg_delay();
        return;
    }
    (void)winx_fwrite(comment,sizeof(char),sizeof(comment)/sizeof(char) - 1,f);
    winx_fclose(f);
}

/**
 * @brief boot-on command handler.
 */
static int boot_on_handler(int argc,short **argv,short **envp)
{
    pending_boot_off = 0;
    
    if(winx_register_boot_exec_command(L"defrag_native") < 0){
        winx_printf("\nCannot enable the boot time defragmenter.\n\n");
        return (-1);
    }
    return 0;
}

/**
 * @brief boot-off command handler.
 */
int boot_off_handler(int argc,short **argv,short **envp)
{
    pending_boot_off = 1;
    
    if(winx_unregister_boot_exec_command(L"defrag_native") < 0){
        winx_printf("\nCannot disable the boot time defragmenter.\n\n");
        return (-1);
    }
    return 0;
}

/**
 * @brief shutdown command handler.
 */
static int shutdown_handler(int argc,short **argv,short **envp)
{
    winx_printf("Shutdown ...\n");
    SavePendingBootOffState();
    winx_shutdown();
    winx_printf("\nShutdown your computer manually.\n");
    return 0;
}

/**
 * @brief reboot command handler.
 */
static int reboot_handler(int argc,short **argv,short **envp)
{
    winx_printf("Reboot ...\n");
    SavePendingBootOffState();
    winx_reboot();
    winx_printf("\nReboot your computer manually.\n");
    return 0;
}

/**
 * @brief exit command handler.
 */
int exit_handler(int argc,short **argv,short **envp)
{
    int exit_code = 0;
    
    if(!scripting_mode)
        exit_flag = 1;
    
    if(argc > 1)
        exit_code = _wtoi(argv[1]);
    
    winx_printf("Good bye ...\n");
    winx_destroy_history(&history);
    winx_exit(exit_code);
    return 0;
}

/**
 * @brief call command handler.
 */
static int call_handler(int argc,short **argv,short **envp)
{
    short *filename;
    int i, length;
    int result;
    int old_scripting_mode;
    
    if(argc < 1)
        return (-1);
        
    old_scripting_mode = scripting_mode;
    
    if(argc < 2){
        result = ProcessScript(NULL);
    } else {
        length = 0;
        for(i = 1; i < argc; i++)
            length += wcslen(argv[i]) + 1;
        filename = winx_heap_alloc(length * sizeof(short));
        if(filename == NULL){
            winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
                argv[0],length * sizeof(short));
            return (-1);
        }
        filename[0] = 0;
        for(i = 1; i < argc; i++){
            wcscat(filename,argv[i]);
            if(i != argc - 1)
                wcscat(filename,L" ");
        }
        result = ProcessScript(filename);
        winx_heap_free(filename);
    }

    scripting_mode = old_scripting_mode;
    return result;
}

/**
 * @brief test command handler.
 * @details Paste here any code
 * which needs to be tested.
 */
static int test_handler(int argc,short **argv,short **envp)
{
    winx_printf("Hi, I'm here ;-)\n");
    return 0;
}

/**
 * @brief List of supported commands.
 */
cmd_table_entry cmd_table[] = {
    { L"boot-off",  boot_off_handler },
    { L"boot-on",   boot_on_handler },
    { L"call",      call_handler },
    { L"echo",      echo_handler },
    { L"echo.",     echo_handler },
    { L"exit",      exit_handler },
    { L"help",      help_handler },
    { L"hexview",   hexview_handler },
    { L"history",   history_handler },
    { L"man",       man_handler },
    { L"pause",     pause_handler },
    { L"reboot",    reboot_handler },
    { L"set",       set_handler },
    { L"shutdown",  shutdown_handler },
    { L"test",      test_handler },
    { L"type",      type_handler },
    { L"udefrag",   udefrag_handler },
    { L"",          NULL }
};

int first_command = 1;

/**
 * @brief Executes the command.
 * @param[in] cmdline the command line.
 * @return Zero for success, negative 
 * value otherwise.
 */
int parse_command(short *cmdline)
{
    int i, j, n, argc;
    int at_detected = 0;
    int arg_detected;
    short *cmdline_copy;
    short **argv;
    short **envp;
    short *string;
    int length;
    int result;
    
    /* disable default logging before the first command parsing */
    if(first_command){
        winx_disable_dbg_log();
        (void)winx_set_env_variable(L"UD_LOG_FILE_PATH",NULL);
        first_command = 0;
    }
    
    /*
    * Cleanup the command line by removing
    * spaces and newlines from the beginning
    * and the end of the string.
    */
    while(*cmdline == 0x20 || *cmdline == '\t')
        cmdline ++; /* skip leading spaces */
    n = wcslen(cmdline);
    for(i = n - 1; i >= 0; i--){
        if(cmdline[i] != 0x20 && cmdline[i] != '\t' && \
            cmdline[i] != '\n' && cmdline[i] != '\r') break;
        cmdline[i] = 0; /* remove trailing spaces and newlines */
    }
    
    /*
    * Skip @ in the beginning of the line.
    */
    if(cmdline[0] == '@'){
        at_detected = 1;
        cmdline ++;
    }
    
    /*
    * Handle empty lines and comments.
    */
    if(cmdline[0] == 0 || cmdline[0] == ';' || cmdline[0] == '#'){
        if(echo_flag && !at_detected)
            winx_printf("%ws\n",cmdline);
        return 0;
    }
    
    /*
    * Prepare argc, argv, envp variables.
    * Return immediately if argc == 0.
    */
    /* a. make a copy of command line */
    n = wcslen(cmdline);
    cmdline_copy = winx_heap_alloc((n + 1) * sizeof(short));
    if(cmdline_copy == NULL){
        winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
            cmdline,(n + 1) * sizeof(short));
        return (-1);
    }
    wcscpy(cmdline_copy,cmdline);
    /* b. replace all spaces by zeros */
    argc = 1;
    for(i = 0; i < n; i++){
        if(cmdline_copy[i] == 0x20 || cmdline_copy[i] == '\t'){
            cmdline_copy[i] = 0;
            if(cmdline_copy[i+1] != 0x20 && cmdline_copy[i+1] != '\t')
                argc ++;
        }
    }
    /* c. allocate memory for argv array */
    argv = winx_heap_alloc(sizeof(short *) * argc);
    if(argv == NULL){
        winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
            cmdline,sizeof(short *) * argc);
        winx_heap_free(cmdline_copy);
        return (-1);
    }
    /* d. fill argv array */
    j = 0; arg_detected = 0;
    for(i = 0; i < n; i++){
        if(cmdline_copy[i]){
            if(!arg_detected){
                argv[j] = cmdline_copy + i;
                j ++;
                arg_detected = 1;
            }
        } else {
            arg_detected = 0;
        }
    }
    /* e. build environment */
    envp = NULL;
    if(peb){
        if(peb->ProcessParameters){
            if(peb->ProcessParameters->Environment){
                /* build array of unicode strings */
                string = peb->ProcessParameters->Environment;
                for(n = 0; ; n++){
                    /* empty line indicates the end of environment */
                    if(string[0] == 0) break;
                    length = wcslen(string);
                    string += length + 1;
                }
                if(n > 0){
                    envp = winx_heap_alloc((n + 1) * sizeof(short *));
                    if(envp == NULL){
                        winx_printf("\n%ws: cannot allocate %u bytes of memory\n\n",
                            cmdline,(n + 1) * sizeof(short *));
                    } else {
                        RtlZeroMemory((void *)envp,(n + 1) * sizeof(short *));
                        string = peb->ProcessParameters->Environment;
                        for(i = 0; i < n; i++){
                            /* empty line indicates the end of environment */
                            if(string[0] == 0) break;
                            envp[i] = string;
                            length = wcslen(string);
                            string += length + 1;
                        }
                    }
                }
            }
        }
    }
    
    /*
    * Print command line if echo is on
    * and the name of the command is not
    * preceeding by the at sign.
    * Don't print also if echo command
    * is executed.
    */
    if(echo_flag && !at_detected && wcscmp(argv[0],L"echo") && \
        wcscmp(argv[0],L"echo.")) winx_printf("%ws\n",cmdline);
    
    /*
    * Check whether the command 
    * is supported or not.
    */
    for(i = 0; cmd_table[i].cmd_handler != NULL; i++)
        if(!wcscmp(argv[0],cmd_table[i].cmd_name)) break;
    
    /*
    * Handle unknown commands.
    */
    if(cmd_table[i].cmd_handler == NULL){
        winx_printf("\nUnknown command %ws!\n\n",argv[0]);
        winx_heap_free(argv);
        if(envp)
            winx_heap_free(envp);
        winx_heap_free(cmdline_copy);
        return 0;
    }
    
    /*
    * Handle the command.
    */
    result = cmd_table[i].cmd_handler(argc,argv,envp);
    winx_heap_free(argv);
    if(envp)
        winx_heap_free(envp);
    winx_heap_free(cmdline_copy);
    return result;
}
