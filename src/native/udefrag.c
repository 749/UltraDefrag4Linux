/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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
* UltraDefrag boot time (native) interface - native udefrag command implementation.
*/

#include "defrag_native.h"

/**
 * @brief The current job type.
 */
udefrag_job_type current_job;

/**
 * @brief The current job flags.
 */
int current_job_flags;

/**
 * @brief Indicates whether 
 * defragmentation must be
 * aborted or not.
 */
int abort_flag = 0;

/* for the progress draw speedup */
int progress_line_length = 0;

object_path *paths = NULL;

wchar_t orig_cut_filter[MAX_ENV_VARIABLE_LENGTH + 1];
wchar_t cut_filter[MAX_ENV_VARIABLE_LENGTH + 1];
wchar_t aux_buffer[MAX_ENV_VARIABLE_LENGTH + 1];
wchar_t aux_buffer2[MAX_ENV_VARIABLE_LENGTH + 1];

/* forward declarations */
static void search_for_paths(int argc,wchar_t **argv,wchar_t **envp);
static void add_path(wchar_t *buffer);

/**
 * @brief Returns the current debugging level.
 */
int GetDebugLevel()
{
    int result = DBG_NORMAL;
    wchar_t *buffer;
    
    buffer = winx_getenv(L"UD_DBGPRINT_LEVEL");
    if(buffer){
        (void)_wcsupr(buffer);
        if(!wcscmp(buffer,L"DETAILED"))
            result = DBG_DETAILED;
        else if(!wcscmp(buffer,L"PARANOID"))
            result = DBG_PARANOID;
        winx_free(buffer);
    }
    return result;
}

/**
 * @brief Redraws the progress information.
 */
void RedrawProgress(udefrag_progress_info *pi)
{
    int p1, p2;
    char *op_name = "Optimization";
    char s[MAX_LINE_WIDTH + 1];
    char format[16];
    char *results;

    if(pi->completion_status == 0 || abort_flag){
        /*
        * if the job is still running or aborted
        * display progress of the current operation
        */
        if(pi->current_operation == VOLUME_ANALYSIS)
            op_name = "Analysis";
        if(pi->current_operation == VOLUME_DEFRAGMENTATION)
            op_name = "Defragmentation";

        p1 = (int)(__int64)(pi->percentage * 100.00);
        p2 = p1 % 100;
        p1 = p1 / 100;

        if(pi->current_operation == VOLUME_OPTIMIZATION && pi->completion_status == 0 && !abort_flag){
            if(pi->pass_number > 1)
                _snprintf(s,sizeof(s),"%s: %u.%02u%%, pass %lu, moves total = %I64u",
                    op_name,p1,p2,pi->pass_number,pi->total_moves);
            else
                _snprintf(s,sizeof(s),"%s: %u.%02u%%, moves total = %I64u",
                    op_name,p1,p2,pi->total_moves);
        } else {
            if(pi->pass_number > 1)
                _snprintf(s,sizeof(s),"%s: %u.%02u%%, pass %lu, fragmented/total = %lu/%lu",
                    op_name,p1,p2,pi->pass_number,pi->fragmented,pi->files);
            else
                _snprintf(s,sizeof(s),"%s: %u.%02u%%, fragmented/total = %lu/%lu",
                    op_name,p1,p2,pi->fragmented,pi->files);
        }
    } else {
        /*
        * if the job is completed display
        * progress of the entire job
        */
        if(current_job == ANALYSIS_JOB)
            op_name = "Analysis";
        if(current_job == DEFRAGMENTATION_JOB)
            op_name = "Defragmentation";

        if(pi->pass_number > 1)
            _snprintf(s,sizeof(s),"%s: 100.00%%, %lu passes, fragmented/total = %lu/%lu",
                op_name,pi->pass_number,pi->fragmented,pi->files);
        else
            _snprintf(s,sizeof(s),"%s: 100.00%%, fragmented/total = %lu/%lu",
                op_name,pi->fragmented,pi->files);
    }

    s[sizeof(s) - 1] = 0;
    _snprintf(format,sizeof(format),"\r%%-%us",progress_line_length);
    format[sizeof(format) - 1] = 0;
    winx_printf(format,s);
    progress_line_length = (int)strlen(s);

    if(pi->completion_status != 0){
        /* print results of the completed job */
        results = udefrag_get_results(pi);
        if(results){
            if(abort_flag) winx_printf("\n\naborted...");
            winx_printf("\n\n%s\n",results);
            udefrag_release_results(results);
        } else {
            if(abort_flag) winx_printf("\n\naborted...\n");
        }
    }
}

/**
 * @brief Updates the progress information
 * on the screen and terminates the job
 * if the user hit either Escape or Break.
 */
void update_progress(udefrag_progress_info *pi, void *p)
{
    KBD_RECORD kbd_rec;
    int escape_detected = 0;
    int break_detected = 0;
    
    /* check for escape and break key hits */
    if(winx_kb_read(&kbd_rec,100) >= 0){
        /* check for escape */
        if(kbd_rec.wVirtualScanCode == 0x1){
            escape_detected = 1;
            escape_flag = 1;
        } else if(kbd_rec.wVirtualScanCode == 0x1d){
            /* distinguish between control keys and the break key */
            if(!(kbd_rec.dwControlKeyState & LEFT_CTRL_PRESSED) && \
              !(kbd_rec.dwControlKeyState & RIGHT_CTRL_PRESSED)){
                break_detected = 1;
            }
        }
        if(escape_detected || break_detected)
            abort_flag = 1;
    }
    RedrawProgress(pi);
}

int terminator(void *p)
{
    /* do it as quickly as possible :-) */
    return abort_flag;
}

/**
 * @brief Processes a single volume.
 */
void ProcessVolume(char letter)
{
    int status;
    char *message = "";
    wchar_t *buffer;

    /* validate the volume before any processing */
    status = udefrag_validate_volume(letter,FALSE);
    if(status < 0){
        winx_printf("\nThe disk %c: cannot be processed!\n",letter);
        if(status == UDEFRAG_UNKNOWN_ERROR)
            winx_printf("Disk is missing or some unknown error has been encountered.\n");
        else
            winx_printf("%s\n",udefrag_get_error_description(status));
        return;
    }
    
    progress_line_length = 0;
    winx_printf("\nPreparing to ");
    switch(current_job){
    case ANALYSIS_JOB:
        winx_printf("analyze %c: ...\n",letter);
        message = "Analysis";
        break;
    case DEFRAGMENTATION_JOB:
        winx_printf("defragment %c: ...\n",letter);
        message = "Defragmentation";
        break;
    case FULL_OPTIMIZATION_JOB:
        winx_printf("optimize %c: ...\n",letter);
        message = "Optimization";
        break;
    case QUICK_OPTIMIZATION_JOB:
        winx_printf("quick optimization of %c: ...\n",letter);
        message = "Quick optimization";
        break;
    case MFT_OPTIMIZATION_JOB:
        winx_printf("optimize mft on %c: ...\n",letter);
        message = "MFT optimization";
        break;
    }
    /* display the time limit whenever it's set */
    buffer = winx_getenv(L"UD_TIME_LIMIT");
    if(buffer){
        winx_printf("\nProcess will be terminated in %ws automatically.\n",buffer);
        winx_free(buffer);
    }
    
    winx_printf(BREAK_MESSAGE);
    status = udefrag_start_job(letter,current_job,current_job_flags,0,update_progress,terminator,NULL);
    if(status < 0){
        winx_printf("\n%s failed!\n",message);
        winx_printf("%s\n",udefrag_get_error_description(status));
        return;
    }
}

/**
 * @brief Enumerates volumes
 * available for defragmentation.
 * @param[in] skip_removable defines
 * whether to skip removable media or not.
 * @return Zero for success, negative
 * value otherwise.
 */
static int DisplayAvailableVolumes(int skip_removable)
{
    volume_info *v;
    int i;

    v = udefrag_get_vollist(skip_removable);
    if(v){
        winx_printf("\nAvailable drive letters:   ");
        for(i = 0; v[i].letter != 0; i++)
            winx_printf("%c   ",v[i].letter);
        udefrag_release_vollist(v);
        winx_printf("\n\n");
        return 0;
    }
    winx_printf("\n\n");
    return (-1);
}

/**
 * @brief udefrag command handler.
 */
int udefrag_handler(int argc,wchar_t **argv,wchar_t **envp)
{
    int l_flag = 0, la_flag = 0;
    int a_flag = 0, o_flag = 0;
    int quick_optimization_flag = 0;
    int optimize_mft_flag = 0;
    int all_flag = 0, all_fixed_flag = 0;
    char letters[MAX_DOS_DRIVES];
    int i, n_letters = 0;
    char letter;
    volume_info *v;
    int debug_level;
    object_path *path, *another_path;
    int n, path_found;
    int result;
    wchar_t *cf;
    
    if(argc < 2){
        winx_printf("\nNo drive letter specified!\n\n");
        return (-1);
    }
    
    /* parse command line */
    for(i = 1; i < argc; i++){
        /* handle flags */
        if(!wcscmp(argv[i],L"-l")){
            l_flag = 1;
            continue;
        } else if(!wcscmp(argv[i],L"-la")){
            la_flag = 1;
            continue;
        } else if(!wcscmp(argv[i],L"-a")){
            a_flag = 1;
            continue;
        } else if(!wcscmp(argv[i],L"-o")){
            o_flag = 1;
            continue;
        } else if(!wcscmp(argv[i],L"-q")){
            quick_optimization_flag = 1;
            continue;
        } else if(wcsstr(argv[i],L"--quick-opt") == argv[i]){
            /* support --quick-optimization as well as obsolete --quick-optimize options */
            quick_optimization_flag = 1;
            continue;
        } else if(!wcscmp(argv[i],L"--optimize-mft")){
            optimize_mft_flag = 1;
            continue;
        } else if(!wcscmp(argv[i],L"--all")){
            all_flag = 1;
            continue;
        } else if(!wcscmp(argv[i],L"--all-fixed")){
            all_fixed_flag = 1;
            continue;
        } else if(!wcscmp(argv[i],L"-r")){
            /* it's safe to just ignore it */
            continue;
        } else if(!wcscmp(argv[i],L"--repeat")){
            /* it's safe to just ignore it */
            continue;
        }
        /* handle individual drive letters */
        if(wcslen(argv[i]) == 2){
            if(argv[i][1] == ':'){
                if(n_letters > (MAX_DOS_DRIVES - 1)){
                    winx_printf("\n%ws: too many letters specified on the command line\n\n",
                        argv[0]);
                } else {
                    letters[n_letters] = (char)argv[i][0];
                    n_letters ++;
                }
                continue;
            }
        }
        /* handle unknown options */
        if(wcsstr(argv[i],L"-") == argv[i])
            winx_printf("\n%ws: unknown option \'%ws\' found\n",argv[0],argv[i]);
        continue;
    }
    
    /* handle the volumes listing request */
    if(la_flag) return DisplayAvailableVolumes(FALSE);
    if(l_flag) return DisplayAvailableVolumes(TRUE);
    
    /* scan for paths of objects to be processed */
    search_for_paths(argc,argv,envp);
    
    /* check whether volume letters are specified or not */
    if(!n_letters && !all_flag && !all_fixed_flag && !paths){
        winx_printf("\nNo drive letter specified!\n\n");
        return (-1);
    }
    
    /* --quick-optimization flag has more precedence */
    if(quick_optimization_flag) o_flag = 0;
    
    /* set the current_job global variable */
    if(a_flag) current_job = ANALYSIS_JOB;
    else if(o_flag) current_job = FULL_OPTIMIZATION_JOB;
    else if(quick_optimization_flag) current_job = QUICK_OPTIMIZATION_JOB;
    else if(optimize_mft_flag) current_job = MFT_OPTIMIZATION_JOB;
    else current_job = DEFRAGMENTATION_JOB;
    
    current_job_flags = 0;
    
    /*
    * In the interactive mode let the job run
    * regardless of whether the previous job
    * has been aborted or not.
    */
    if(!scripting_mode) abort_flag = 0;

    debug_level = GetDebugLevel();
    
    /* process paths specified on the command line */
    /* skip invalid paths */
    for(path = paths; path; path = path->next){
        if(wcslen(path->path) < 2){
            winx_printf("incomplete path detected: %ls\n",path->path);
            path->processed = 1;
        }
        if(path->path[1] != ':'){
            winx_printf("incomplete path detected: %ls\n",path->path);
            path->processed = 1;
        }
        if(path->next == paths) break;
    }
    /* process valid paths */
    for(path = paths; path; path = path->next){
        if(path->processed == 0){
            winx_printf("\n%ls\n",path->path);
            path->processed = 1;
            path_found = 1;
            
            /* extract drive letter */
            letter = (char)path->path[0];
            
            /* save %UD_CUT_FILTER% */
            orig_cut_filter[0] = 0;
            cf = winx_getenv(L"UD_CUT_FILTER");
            if(cf){
                wcsncpy(orig_cut_filter,cf,MAX_ENV_VARIABLE_LENGTH + 1);
                orig_cut_filter[MAX_ENV_VARIABLE_LENGTH] = 0;
                winx_free(cf);
            }
            
            /* save the current path to %UD_CUT_FILTER% */
            n = _snwprintf(cut_filter,MAX_ENV_VARIABLE_LENGTH + 1,L"%ls",path->path);
            if(n < 0){
                winx_printf("Cannot set %%UD_CUT_FILTER%% - path is too long!\n");
                wcscpy(cut_filter,L"");
                path_found = 0;
            } else {
                cut_filter[MAX_ENV_VARIABLE_LENGTH] = 0;
            }
            
            /* search for other paths with the same drive letter */
            for(another_path = path->next; another_path; another_path = another_path->next){
                if(another_path == paths) break;
                if(winx_toupper(letter) == winx_toupper((char)another_path->path[0])){
                    /* try to append it to %UD_CUT_FILTER% */
                    n = _snwprintf(aux_buffer,MAX_ENV_VARIABLE_LENGTH + 1,L"%ls;%ls",cut_filter,another_path->path);
                    if(n >= 0){
                        aux_buffer[MAX_ENV_VARIABLE_LENGTH] = 0;
                        wcscpy(cut_filter,aux_buffer);
                        path_found = 1;
                        winx_printf("%ls\n",another_path->path);
                        another_path->processed = 1;
                    }
                }
            }
            
            /* set %UD_CUT_FILTER% */
            if(abort_flag) goto done;
            if(winx_setenv(L"UD_CUT_FILTER",cut_filter) < 0){
                winx_printf("Cannot set %%UD_CUT_FILTER%%!\n");
            }
            
            /* run the job */
            if(path_found){
                ProcessVolume(letter);
                if(debug_level > DBG_NORMAL) short_dbg_delay();
            }
            
            /* restore %UD_CUT_FILTER% */
            result = winx_setenv(L"UD_CUT_FILTER",orig_cut_filter);
            if(result < 0){
                winx_printf("Cannot restore %%UD_CUT_FILTER%%!\n");
            }
        }
        if(path->next == paths) break;
    }
    
    if(abort_flag)
        goto done;
    
    /* process volumes specified on the command line */
    for(i = 0; i < n_letters; i++){
        if(abort_flag) break;
        letter = letters[i];
        ProcessVolume(letter);
        if(debug_level > DBG_NORMAL) short_dbg_delay();
    }

    if(abort_flag)
        goto done;
    
    /* process all volumes if requested */
    if(all_flag || all_fixed_flag){
        v = udefrag_get_vollist(all_fixed_flag ? TRUE : FALSE);
        if(v == NULL){
            winx_printf("\n%ws: udefrag_get_vollist failed\n\n",argv[0]);
            goto fail;
        }
        for(i = 0; v[i].letter != 0; i++){
            if(abort_flag) break;
            letter = v[i].letter;
            ProcessVolume(letter);
            if(debug_level > DBG_NORMAL) short_dbg_delay();
        }
        udefrag_release_vollist(v);
    }

done:    
    winx_list_destroy((list_entry **)(void *)&paths);
    return 0;

fail:
    winx_list_destroy((list_entry **)(void *)&paths);
    return (-1);
}

static void search_for_paths(int argc,wchar_t **argv,wchar_t **envp)
{
    int leading_quote_found = 0;
    int i, n;
    
    aux_buffer[0] = 0;  /* reset the main buffer */
    aux_buffer2[0] = 0; /* reset an auxiliary buffer */
    for(i = 1; i < argc; i++){
        if(argv[i][0] == 0) continue;   /* skip empty strings */
        if(argv[i][0] == '-') continue; /* skip options */
        if(wcslen(argv[i]) == 2){       /* skip individual volume letters */
            if(argv[i][1] == ':')
                continue;
        }
        //winx_printf("part of path detected: arg[%i] = %ls\n",i,argv[i]);
        if(argv[i][0] == '"'){
            /* a leading quote found */
            add_path(aux_buffer);
            wcsncpy(aux_buffer,argv[i] + 1,MAX_LONG_PATH);
            aux_buffer[MAX_LONG_PATH] = 0;
            /* check for a trailing quote */
            if(argv[i][wcslen(argv[i]) - 1] == '"'){
                /* remove the trailing quote */
                n = (int)wcslen(aux_buffer);
                if(n > 0) aux_buffer[n - 1] = 0;
                add_path(aux_buffer);
                aux_buffer[0] = 0;
            } else {
                leading_quote_found = 1;
            }
        } else if(argv[i][wcslen(argv[i]) - 1] == '"'){
            /* a trailing quote found */
            if(aux_buffer[0])
                n = _snwprintf(aux_buffer2,MAX_LONG_PATH + 1,L"%ls %ls",aux_buffer,argv[i]);
            else
                n = _snwprintf(aux_buffer2,MAX_LONG_PATH + 1,L"%ls",argv[i]);
            if(n < 0){
                winx_printf("search_for_path: path is too long!\n");
            } else {
                wcsncpy(aux_buffer,aux_buffer2,MAX_LONG_PATH);
                aux_buffer[MAX_LONG_PATH] = 0;
                /* remove the trailing quote */
                n = (int)wcslen(aux_buffer);
                if(n > 0) aux_buffer[n - 1] = 0;
            }
            add_path(aux_buffer);
            aux_buffer[0] = 0;
            leading_quote_found = 0;
        } else {
            if(leading_quote_found){
                if(aux_buffer[0])
                    n = _snwprintf(aux_buffer2,MAX_LONG_PATH + 1,L"%ls %ls",aux_buffer,argv[i]);
                else
                    n = _snwprintf(aux_buffer2,MAX_LONG_PATH + 1,L"%ls",argv[i]);
                if(n < 0){
                    winx_printf("search_for_path: path is too long!\n");
                } else {
                    wcsncpy(aux_buffer,aux_buffer2,MAX_LONG_PATH);
                    aux_buffer[MAX_LONG_PATH] = 0;
                }
            } else {
                add_path(aux_buffer);
                wcsncpy(aux_buffer,argv[i],MAX_LONG_PATH);
                aux_buffer[MAX_LONG_PATH] = 0;
            }
        }
    }
    add_path(aux_buffer);
}

/* size of the buffer must be equal to MAX_LONG_PATH */
static void add_path(wchar_t *buffer)
{
    object_path *new_item, *last_item = NULL;

    if(buffer == NULL) return;

    if(buffer[0]){
        if(paths) last_item = paths->prev;
        new_item = (object_path *)winx_list_insert((list_entry **)(void *)&paths,
            (list_entry *)last_item,sizeof(object_path));
        new_item->processed = 0;
        wcsncpy(new_item->path,buffer,MAX_LONG_PATH);
        new_item->path[MAX_LONG_PATH] = 0;
    }
}
