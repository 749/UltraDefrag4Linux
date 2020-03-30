/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2012 Stefan Pendl (stefanpe@users.sourceforge.net).
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
* We don't know how to build monolithic app on MinGW,
* but on Windows DDK this works fine.
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

/**
 * @brief Initializes the native application.
 */
static int NativeAppInit(PPEB Peb)
{
    /*
    * Initialize registry in case 
    * of smss.exe replacement.
    */
#ifdef USE_INSTEAD_SMSS
    (void)NtInitializeRegistry(0/*FALSE*/); /* saves boot log etc. */
#endif

    /*
    * Since v4.3.0 this app is monolithic
    * to reach the highest level of reliability.
    * It was not reliable before because crashed
    * the system in case of missing DLL's.
    */
    if(winx_init_library(Peb) < 0)
        return (-1);
    
    /* start initial logging */
    set_dbg_log("startup-phase");
    return 0;
}

/**
 * @brief Handles boot in Windows Safe Mode.
 * @return Nonzero value if safe mode is
 * detected, zero otherwise.
 */
static int HandleSafeModeBoot(void)
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
    init_result = NativeAppInit(Peb);

    /* display copyrights */
    winx_print("\n\n");
    winx_print(VERSIONINTITLE " boot time interface\n"
        "Copyright (c) Dmitri Arkhangelski, 2007-2012.\n"
        "Copyright (c) Stefan Pendl, 2010-2012.\n\n"
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
    if(HandleSafeModeBoot())
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
    winx_printf("\nInteractive mode:\nType 'help' for a list of supported commands.\n");
    winx_printf("\nOnly the English keyboard layout is available.\n\n");
    scripting_mode = 0;
    abort_flag = 0;
    winx_init_history(&history);
    while(winx_prompt_ex("# ",buffer,MAX_LINE_WIDTH,&history) >= 0){
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

    /* break on winx_prompt_ex() errors */
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
    wchar_t instdir[MAX_PATH];
    char *logpath = NULL;
    int length;
    wchar_t *unicode_path = NULL;
    
    if(winx_query_env_variable(L"UD_INSTALL_DIR",instdir,MAX_PATH) < 0){
        winx_printf("\nset_dbg_log: cannot get %%ud_install_dir%% path\n\n");
        return;
    }
    
    logpath = winx_sprintf("%ws\\logs\\boot-%s.log",instdir,name);
    if(logpath == NULL){
        winx_printf("\nset_dbg_log: cannot build log path\n\n");
        return;
    }
    
    length = strlen(logpath);
    unicode_path = winx_heap_alloc((length + 1) * sizeof(wchar_t));
    if(unicode_path == NULL){
        winx_printf("\nset_dbg_log: cannot allocate %u bytes of memory",
            (length + 1) * sizeof(wchar_t));
        goto done;
    }
    
    if(_snwprintf(unicode_path,length + 1,L"%hs",logpath) < 0){
        winx_printf("\nset_dbg_log: cannot convert log path to unicode\n\n");
        goto done;
    }
    
    unicode_path[length] = 0;
    if(winx_set_env_variable(L"UD_LOG_FILE_PATH",unicode_path) < 0){
        winx_printf("\nset_dbg_log: cannot set %%UD_LOG_FILE_PATH%%\n\n");
        goto done;
    }
    
    if(udefrag_set_log_file_path() < 0)
        winx_printf("\nset_dbg_log: udefrag_set_log_file_path failed\n\n");

done:
    if(logpath)
        winx_heap_free(logpath);
    if(unicode_path)
        winx_heap_free(unicode_path);
}
