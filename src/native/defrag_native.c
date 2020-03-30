/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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
* UltraDefrag boot time (native) interface.
*/

#include "defrag_native.h"

/*
* We're releasing this module as a monolithic thing to
* prevent BSOD in case of missing UltraDefrag native libraries.
* Otherwise the following modules becomes critical for the Windows
* boot process: udefrag.dll, zenwinx.dll.
*/

/**
 * @brief Process Environment Block.
 */
PEB *peb = NULL;

/**
 * @brief History of commands
 * typed in interactive mode.
 */
winx_history history = {0};

/**
 * @brief Defines whether the program
 * must exit from interactive mode or not.
 */
int exit_flag = 0;

/* forward declarations */
static void set_dbg_log(char *name);

static int out_of_memory_handler(size_t n)
{
    winx_print("\nOut of memory!\n");
    
    winx_flush_dbg_log(FLUSH_IN_OUT_OF_MEMORY);
    /* terminate process with exit code 3 */
    winx_exit(3); return 0;
}

/**
 * @brief Initializes the native application.
 */
static int native_app_init(void)
{
#ifdef USE_INSTEAD_SMSS
    /* it saves boot log etc. */
    (void)NtInitializeRegistry(0);
#endif

    if(winx_init_library() < 0) return (-1);
    winx_set_killer(out_of_memory_handler);
    
    /* start initial logging */
    set_dbg_log("startup-phase");
    return 0;
}

/**
 * @brief Handles boot in Windows Safe Mode.
 * @return Nonzero value if safe mode is
 * detected, zero otherwise.
 */
static int handle_safe_mode_boot(void)
{
    int safe_mode;
    
    /*
    * In Windows Safe Mode the boot time defragmenter
    * is absolutely useless, because it cannot display
    * text on the screen in this mode.
    */
    safe_mode = winx_windows_in_safe_mode();
    /*
    * In case of errors we'll assume that we're running in Safe Mode.
    * To avoid a very unpleasant situation when ultradefrag works
    * in background and nothing happens on the black screen.
    */
    if(safe_mode > 0 || safe_mode < 0){
        /* display yet a message, for debugging purposes */
        winx_printf("In Windows Safe Mode this program is useless!\n");
        /* if someone will see the message, a little delay will help him to read it */
        short_dbg_delay();
        winx_exit(0);
        return 1;
    }
    return 0;
}

/**
 * @brief Native application entry point.
 */
void __stdcall NtProcessStartup(PPEB Peb)
{
    int init_result;
    /*
    * No longer than MAX_LINE_WIDTH to ensure that escape and backspace
    * keys will work properly with winx_prompt() function.
    */
    char buffer[MAX_LINE_WIDTH + 1];
    wchar_t wbuffer[MAX_LINE_WIDTH + 1];
    int i;
    
    /* initialize the program */
    peb = Peb;
    init_result = native_app_init();

    /* display copyrights */
    winx_print("\n\n");
    winx_print(VERSIONINTITLE " boot time interface\n"
        "Copyright (c) Dmitri Arkhangelski, 2007-2013.\n"
        "Copyright (c) Stefan Pendl, 2010-2013.\n\n"
        "UltraDefrag comes with ABSOLUTELY NO WARRANTY.\n\n"
        "If something is wrong, hit F8 on startup\n"
        "and select 'Last Known Good Configuration'\n"
        "or execute 'CHKDSK {Drive:} /R /F'.\n\n");
        
    /* handle initialization failure */
    if(init_result < 0){
        winx_print("Initialization failed!\n");
        winx_print("Send bug report to the authors please.\n");
        long_dbg_delay();
        winx_exit(1);
    }
        
    /* handle safe mode boot */
    if(handle_safe_mode_boot())
        return;
        
    /* check for the pending boot-off command */
    if(ExecPendingBootOff()){
#ifndef USE_INSTEAD_SMSS
        winx_printf("Good bye ...\n");
        short_dbg_delay();
        winx_exit(0);
#endif
    }

    /* initialize keyboards */
    if(winx_kb_init() < 0){
        winx_printf("Wait 10 seconds ...\n");
        long_dbg_delay();
        winx_exit(1);
    }
    
    /* prompt to exit */
    winx_printf("\nPress any key to exit ");
    for(i = 0; i < 5; i++){
        if(winx_kbhit(1000) >= 0){
            winx_printf("\nGood bye ...\n");
            winx_exit(0);
        }
        //winx_printf("%c ",(char)('0' + 10 - i));
        winx_printf(".");
    }
    winx_printf("\n\n");

    /* process default boot time script */
    ProcessScript(NULL);

    /* start interactive mode */
    winx_printf("\nInteractive mode:\nType 'help' to list available commands.\n");
    winx_printf("\nOnly the English keyboard layout is available.\n\n");
    scripting_mode = 0;
    abort_flag = 0;
    winx_init_history(&history);
    while(winx_prompt("# ",buffer,MAX_LINE_WIDTH,&history) >= 0){
        /* convert command to unicode */
        if(_snwprintf(wbuffer,MAX_LINE_WIDTH,L"%hs",buffer) < 0){
            winx_printf("Command line is too long!\n");
            continue;
        }
        wbuffer[MAX_LINE_WIDTH] = 0;

        /* execute command */
        parse_command(wbuffer);

        /* break on exit */
        if(exit_flag)
            return;
    }

    /* break on winx_prompt() errors */
    exit_handler(0,NULL,NULL);
}

/**
 * @brief Sets debugging log path.
 * @details Intended primarily to log
 * debugging information during the
 * native application startup.
 */
static void set_dbg_log(char *name)
{
    wchar_t *instdir;
    wchar_t *path = NULL;
    int result;

    instdir = winx_getenv(L"UD_INSTALL_DIR");
    if(instdir == NULL){
        winx_printf("\nset_dbg_log: cannot get %%ud_install_dir%% path\n\n");
        return;
    }
    
    path = winx_swprintf(L"%ws\\logs\\boot-%hs.log",instdir,name);
    winx_free(instdir);
    if(path == NULL){
        winx_printf("\nset_dbg_log: cannot build log path\n\n");
        return;
    }

    result = winx_setenv(L"UD_LOG_FILE_PATH",path);
    winx_free(path);
    if(result < 0){
        winx_printf("\nset_dbg_log: cannot set %%UD_LOG_FILE_PATH%%\n\n");
    } else {
        if(udefrag_set_log_file_path() < 0)
            winx_printf("\nset_dbg_log: udefrag_set_log_file_path failed\n\n");
    }
}
