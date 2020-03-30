/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2015 Dmitri Arkhangelski (dmitriar@gmail.com).
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
* UltraDefrag console interface.
*/

/*
* Revised by Stefan Pendl, 2010, 2011
* <stefanpe@users.sourceforge.net>
*/

#include "udefrag.h"

/* global variables */
HANDLE hSynchEvent = NULL;
HANDLE hStdOut; /* handle of the standard output */
WORD default_color = 0x7; /* default text color */

int a_flag = 0,o_flag = 0;
int b_flag = 0,h_flag = 0;
int l_flag = 0,la_flag = 0;
int p_flag = 0,v_flag = 0;
int m_flag = 0;
int obsolete_option = 0;
int screensaver_mode = 0;
int all_flag = 0;
int all_fixed_flag = 0;
object_path *paths = NULL;
char letters[MAX_DOS_DRIVES] = {0};
int wait_flag = 0;
int shellex_flag = 0;
int folder_flag = 0;
int folder_itself_flag = 0;
int quick_optimize_flag = 0;
int optimize_mft_flag = 0;
int repeat_flag = 0;

int first_progress_update;
int stop_flag = 0;

int web_statistics_completed = 0;

/*
* MSDN states that environment variables
* are limited by 32767 characters,
* including terminal zero.
*/
#define MAX_ENV_VARIABLE_LENGTH 32766
wchar_t orig_cut_filter[MAX_ENV_VARIABLE_LENGTH + 1];
wchar_t cut_filter[MAX_ENV_VARIABLE_LENGTH + 1];
wchar_t aux_buffer[MAX_ENV_VARIABLE_LENGTH + 1];
wchar_t env_buffer[MAX_ENV_VARIABLE_LENGTH + 1];

/* forward declarations */
static int show_vollist(void);
static int process_volumes(void);
static void RunScreenSaver(void);
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType);

/* -------------------------------------------- */
/*                common routines               */
/* -------------------------------------------- */

/**
 * @brief Fills the current line with spaces.
 */
void clear_line(FILE *f)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD cursor_pos;
    char *line;
    int n;
    
    if(!GetConsoleScreenBufferInfo(hStdOut,&csbi))
        return; /* impossible to determine the screen width */
    n = (int)csbi.dwSize.X;
    line = malloc(n + 1);
    if(line == NULL)
        return;
    
    memset(line,0x20,n);
    line[n] = 0;
    fprintf(f,"\r%s",line);
    free(line);
    
    /* move cursor back to the previous line */
    if(!GetConsoleScreenBufferInfo(hStdOut,&csbi)){
        display_last_error("Cannot retrieve cursor position!");
        return; /* impossible to determine the current cursor position  */
    }
    cursor_pos.X = 0;
    cursor_pos.Y = csbi.dwCursorPosition.Y - 1;
    if(!SetConsoleCursorPosition(hStdOut,cursor_pos))
        display_last_error("Cannot set cursor position!");
}

/* -------------------------------------------- */
/*         UltraDefrag specific routines        */
/* -------------------------------------------- */

/**
 * @brief Updates web statistics of the program use.
 */
DWORD WINAPI UpdateWebStatisticsThreadProc(LPVOID lpParameter)
{
    int tracking_enabled = 1;
    
    /* getenv() may give wrong results as stated in MSDN */
    if(!GetEnvironmentVariableW(L"UD_DISABLE_USAGE_TRACKING",env_buffer,MAX_ENV_VARIABLE_LENGTH + 1)){
        if(GetLastError() != ERROR_ENVVAR_NOT_FOUND)
            letrace("cannot get %%UD_DISABLE_USAGE_TRACKING%%");
    } else {
        if(wcscmp(env_buffer,L"1") == 0)
            tracking_enabled = 0;
    }
    
    if(tracking_enabled){
#ifndef _WIN64
        IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/console-x86.html","UA-15890458-1");
#else
    #if defined(_IA64_)
        IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/console-ia64.html","UA-15890458-1");
    #else
        IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/console-x64.html","UA-15890458-1");
    #endif
#endif
    }

    web_statistics_completed = 1;
    return 0;
}

/**
 * @brief Starts web statistics request delivering.
 */
void start_web_statistics(void)
{
    if(!WgxCreateThread(UpdateWebStatisticsThreadProc,NULL)){
        letrace("cannot run UpdateWebStatisticsThreadProc");
        web_statistics_completed = 1;
    }
}

/**
 * @brief Waits for web statistics request completion.
 */
void stop_web_statistics()
{
    while(!web_statistics_completed) Sleep(100);
}

/**
 * @brief Performs basic console initialization.
 * @details Starts thread delivering usage statistics
 * to the server.
 */
static void init_console(void)
{
    /* set green color */
    settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    /* update web statistics */
    start_web_statistics();
}

/**
 * @brief Terminates the program.
 * @details 
 * - Restores default text color.
 * - Waits until the completion of
 * the usage statistics delivering
 * to prevent crash of application
 * in case of slow internet connection.
 */
static void terminate_console(int code)
{
    /* restore default color */
    settextcolor(default_color);

    /* wait for web statistics request completion */
    stop_web_statistics();

    udefrag_flush_dbg_log(0);
    exit(code);
}

/**
 * @brief Handles Ctrl+C keys.
 */
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
    stop_flag = 1;
    return TRUE;
}

/**
 * @brief Prints the string in red, than restores green color.
 */
void display_error(char *string)
{
    settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    fprintf(stderr,"%s",string);
    settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Displays error message
 * specific for failed disk processing tasks.
 */
static void display_defrag_error(udefrag_job_type job_type, int error_code)
{
    char *operation = "optimization";
    
    settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    
    switch(job_type){
    case ANALYSIS_JOB:
        operation = "analysis";
        break;
    case DEFRAGMENTATION_JOB:
        operation = "defragmentation";
        break;
    default:
        break;
    }
    fprintf(stderr,"\nDisk %s failed!\n\n",operation);
    
    settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    fprintf(stderr,"%s\n\n",udefrag_get_error_description(error_code));
    if(error_code == UDEFRAG_UNKNOWN_ERROR)
        fprintf(stderr,"Enable logs or use DbgView program to get more information.\n\n");
    
    settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Displays error message
 * specific for invalid disk volumes.
 */
static void display_invalid_volume_error(int error_code)
{
    settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    fprintf(stderr,"The disk cannot be processed.\n\n");
    settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    if(error_code == UDEFRAG_UNKNOWN_ERROR){
        fprintf(stderr,"Disk is missing or some unknown error has been encountered.\n");
        fprintf(stderr,"Enable logs or use DbgView program to get more information.\n\n");
    } else {
        fprintf(stderr,"%s\n\n",udefrag_get_error_description(error_code));
    }

    settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Displays last error message in red.
 */
void display_last_error(char *caption)
{
    wchar_t *msg;
    int length;
    DWORD error = GetLastError();

    if(!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)(void *)&msg,0,NULL)){
                settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
                if(error == ERROR_COMMITMENT_LIMIT)
                    fprintf(stderr,"\n%s\nNot enough memory.\n\n",caption);
                else
                    fprintf(stderr,"\n%s\nError code = 0x%x\n\n",caption,(UINT)error);
                settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    } else {
        settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        fprintf(stderr,"\n%s\n",caption);
        /* get rid of trailing new line */
        length = wcslen(msg);
        if(length > 0){
            if(msg[length - 1] == '\n')
                msg[length - 1] = 0;
        }
        if(length > 1){
            if(msg[length - 2] == '\r')
                msg[length - 2] = 0;
        }
        WgxPrintUnicodeString(msg,stderr);
        fprintf(stderr,"\n");
        settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        LocalFree(msg);
    }
}

/**
* @brief Synchronizes with other scheduled 
* jobs when running with --wait option.
*/
void begin_synchronization(void)
{
    /* create event */
    do {
        hSynchEvent = CreateEvent(NULL,FALSE,TRUE,"udefrag-exe-synch-event");
        if(hSynchEvent == NULL){
            display_last_error("Cannot create udefrag-exe-synch-event event!");
            return;
        }
        if(GetLastError() == ERROR_ALREADY_EXISTS && wait_flag){
            CloseHandle(hSynchEvent);
            Sleep(1000);
            continue;
        }
        return;
    } while (1);
}

void end_synchronization(void)
{
    /* release event */
    if(hSynchEvent)
        CloseHandle(hSynchEvent);
}

/**
 * @brief Updates progress information on the screen.
 */
void update_progress(udefrag_progress_info *pi, void *p)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD cursor_pos;
    char volume_letter;
    char *op_name = "";
    char *results;
    
    volume_letter = (char)(DWORD_PTR)p;
    
    if(!p_flag){
        if(first_progress_update){
            /*
            * Ultra fast ntfs analysis contains one piece of code 
            * that heavily loads CPU for one-two seconds.
            */
            (void)SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
            first_progress_update = 0;
        }
        
        if(map_completed){ /* we could use simple check for m_flag here */
            if(!GetConsoleScreenBufferInfo(hStdOut,&csbi)){
                display_last_error("Cannot retrieve cursor position!");
                return;
            }
            cursor_pos.X = 0;
            cursor_pos.Y = csbi.dwCursorPosition.Y - map_rows - 3 - 2;
            if(!SetConsoleCursorPosition(hStdOut,cursor_pos)){
                display_last_error("Cannot set cursor position!");
                return;
            }
        }
        
        switch(pi->current_operation){
        case VOLUME_ANALYSIS:
            op_name = "analyze:  ";
            break;
        case VOLUME_DEFRAGMENTATION:
            op_name = "defrag:   ";
            break;
        default:
            op_name = "optimize: ";
            break;
        }
        clear_line(stdout);
        if(pi->current_operation == VOLUME_OPTIMIZATION && !stop_flag && pi->completion_status == 0){
            if(pi->pass_number > 1)
                printf("\r%c: %s%6.2lf%% complete, pass %lu, moves total = %I64u",
                    volume_letter,op_name,pi->percentage,pi->pass_number,pi->total_moves);
            else
                printf("\r%c: %s%6.2lf%% complete, moves total = %I64u",
                    volume_letter,op_name,pi->percentage,pi->total_moves);
        } else {
            if(pi->pass_number > 1)
                printf("\r%c: %s%6.2lf%% complete, pass %lu, fragmented/total = %lu/%lu",
                    volume_letter,op_name,pi->percentage,pi->pass_number,pi->fragmented,pi->files);
            else
                printf("\r%c: %s%6.2lf%% complete, fragmented/total = %lu/%lu",
                    volume_letter,op_name,pi->percentage,pi->fragmented,pi->files);
        }
        if(pi->completion_status != 0 && !stop_flag){
            /* set progress indicator to 100% state */
            clear_line(stdout);
            if(pi->pass_number > 1)
                printf("\r%c: %s100.00%% complete, %lu passes needed, fragmented/total = %lu/%lu",
                    volume_letter,op_name,pi->pass_number,pi->fragmented,pi->files);
            else
                printf("\r%c: %s100.00%% complete, fragmented/total = %lu/%lu",
                    volume_letter,op_name,pi->fragmented,pi->files);
            if(!m_flag) printf("\n");
        }
        
        if(m_flag)
            RedrawMap(pi);
    }
    
    if(pi->completion_status != 0 && v_flag){
        /* print results of the completed job */
        results = udefrag_get_default_formatted_results(pi);
        if(results){
            printf("\n%s",results);
            udefrag_release_default_formatted_results(results);
        }
    }
}

/**
 * @brief Callback routine
 * calling each time when
 * the volume processing routines
 * would like to know whether they
 * should be terminated or not.
 */
int terminator(void *p)
{
    /* do it as quickly as possible :-) */
    return stop_flag;
}

/**
 * @brief Processes a single volume.
 * @return Zero for success, nonzero value indicates failure.
 */
static int process_single_volume(char volume_letter)
{
    long map_size = 0;
    udefrag_job_type job_type = DEFRAGMENTATION_JOB;
    int flags = 0, result;

    /* validate volume letter */
    if(volume_letter == 0){
        display_error("Drive letter must be specified!\n");
        return EXIT_FAILURE;
    }
    result = udefrag_validate_volume(volume_letter,FALSE);
    if(result < 0){
        display_invalid_volume_error(result);
        return EXIT_FAILURE;
    }

    /* initialize cluster map */
    if(m_flag){
        if(AllocateClusterMap() < 0){
            /* terminate process */
            (void)SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
            terminate_console(EXIT_FAILURE);
        }
        InitializeMapDisplay(volume_letter);
        map_size = map_rows * map_symbols_per_line;
    }
    
    /* check if we need to run in screensaver mode */
    if(screensaver_mode){
        RunScreenSaver();
        FreeClusterMap();
        return EXIT_SUCCESS;
    }

    /* run the job */
    stop_flag = 0;
    if(a_flag) job_type = ANALYSIS_JOB;
    else if(o_flag) job_type = FULL_OPTIMIZATION_JOB;
    else if(quick_optimize_flag) job_type = QUICK_OPTIMIZATION_JOB;
    else if(optimize_mft_flag) job_type = MFT_OPTIMIZATION_JOB;
    if(repeat_flag)
        flags = UD_JOB_REPEAT;
    if(shellex_flag)
        flags |= UD_JOB_CONTEXT_MENU_HANDLER;
    first_progress_update = 1;
    result = udefrag_start_job(volume_letter,job_type,flags,map_size,
        update_progress,terminator,(void *)(DWORD_PTR)volume_letter);
    if(result < 0){
        display_defrag_error(job_type, result);
        FreeClusterMap();
        return EXIT_FAILURE;
    }

    FreeClusterMap();
    return EXIT_SUCCESS;
}

/**
 * @brief Processes all volumes specified on the command line.
 * @return Zero for success, nonzero value indicates failure.
 */
static int process_volumes(void)
{
    object_path *path, *another_path;
    int single_path = 0;
    volume_info *v;
    int i, result = EXIT_SUCCESS;
    char letter;
    int n, path_found;
    int first_path = 1;
    
    /* 1. process paths specified on the command line */
    /* skip invalid paths */
    for(path = paths; path; path = path->next){
        if(wcslen(path->path) < 2){
            fprintf(stderr,"incomplete path detected: %ls\n",path->path);
            path->processed = 1;
        }
        if(path->path[1] != ':'){
            fprintf(stderr,"incomplete path detected: %ls\n",path->path);
            path->processed = 1;
        }
        if(path->next == paths) break;
    }
    /* process valid paths */
    for(path = paths; path; path = path->next){
        if(path->processed == 0){
            if(!(shellex_flag && (folder_flag || folder_itself_flag))){
                settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                if(first_path){
                    WgxPrintUnicodeString(path->path,stdout);
                    printf("\n");
                } else {
                    printf("\n");
                    WgxPrintUnicodeString(path->path,stdout);
                    printf("\n");
                }
                settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            }
            first_path = 0;
            path->processed = 1;
            path_found = 1;
            
            /* extract drive letter */
            letter = (char)path->path[0];
            
            /* save %UD_CUT_FILTER% */
            orig_cut_filter[0] = 0;
            if(!GetEnvironmentVariableW(L"UD_CUT_FILTER",orig_cut_filter,MAX_ENV_VARIABLE_LENGTH + 1)){
                if(GetLastError() != ERROR_ENVVAR_NOT_FOUND)
                    display_last_error("process_volumes: cannot get %%UD_CUT_FILTER%%!");
            }
            
            if(shellex_flag && (folder_flag || folder_itself_flag)){
                if(folder_flag){
                    if(path->path[wcslen(path->path) - 1] == '\\'){
                        /* set UD_CUT_FILTER=c:\* */
                        n = _snwprintf(cut_filter,MAX_ENV_VARIABLE_LENGTH + 1,L"%ls*",path->path);
                    } else {
                        /* set UD_CUT_FILTER=c:\test;c:\test\* */
                        n = _snwprintf(cut_filter,MAX_ENV_VARIABLE_LENGTH + 1,L"%ls;%ls\\*",path->path,path->path);
                    }
                } else {
                    /* set UD_CUT_FILTER=c:\test */
                    n = _snwprintf(cut_filter,MAX_ENV_VARIABLE_LENGTH + 1,L"%ls",path->path);
                }
                if(n < 0){
                    display_error("process_volumes: cannot set %%UD_CUT_FILTER%% - path is too long!");
                    wcscpy(cut_filter,L"");
                    path_found = 0;
                } else {
                    cut_filter[MAX_ENV_VARIABLE_LENGTH] = 0;
                    settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    WgxPrintUnicodeString(cut_filter,stdout);
                    printf("\n");
                    settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                }
            } else {
                /* save the current path to %UD_CUT_FILTER% */
                n = _snwprintf(cut_filter,MAX_ENV_VARIABLE_LENGTH + 1,L"%ls",path->path);
                if(n < 0){
                    display_error("process_volumes: cannot set %%UD_CUT_FILTER%% - path is too long!");
                    wcscpy(cut_filter,L"");
                    path_found = 0;
                } else {
                    cut_filter[MAX_ENV_VARIABLE_LENGTH] = 0;
                }
                
                /* search for another paths with the same drive letter */
                for(another_path = path->next; another_path; another_path = another_path->next){
                    if(another_path == paths) break;
                    if(udefrag_toupper(letter) == udefrag_toupper((char)another_path->path[0])){
                        /* try to append it to %UD_CUT_FILTER% */
                        n = _snwprintf(aux_buffer,MAX_ENV_VARIABLE_LENGTH + 1,L"%ls;%ls",cut_filter,another_path->path);
                        if(n >= 0){
                            aux_buffer[MAX_ENV_VARIABLE_LENGTH] = 0;
                            wcscpy(cut_filter,aux_buffer);
                            path_found = 1;
                            settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                            WgxPrintUnicodeString(another_path->path,stdout);
                            printf("\n");
                            settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                            another_path->processed = 1;
                        }
                    }
                }
            }
            
            /* set %UD_CUT_FILTER% */
            if(stop_flag) return EXIT_SUCCESS;
            if(!SetEnvironmentVariableW(L"UD_CUT_FILTER",cut_filter)){
                display_last_error("process_volumes: cannot set %%UD_CUT_FILTER%%!");
            }
            
            /* run the job */
            if(path_found)
                result = process_single_volume(letter);
            
            /* restore %UD_CUT_FILTER% */
            if(!SetEnvironmentVariableW(L"UD_CUT_FILTER",orig_cut_filter)){
                display_last_error("process_volumes: cannot restore %%UD_CUT_FILTER%%!");
            }
        }
        if(path->next == paths) break;
    }
    
    /* if the job was stopped return success */
    if(stop_flag) return EXIT_SUCCESS;
    
    /* in case of a single path processing return result */
    if(paths){
        if(paths->next == paths)
            single_path = 1;
    }
    if(single_path && letters[0] == 0 && !all_flag && !all_fixed_flag)
        return result;
    
    /* 2. process individual volumes specified on the command line */
    for(i = 0; i < MAX_DOS_DRIVES; i++){
        if(letters[i] == 0) break;
        if(stop_flag) return EXIT_SUCCESS;
        //printf("%c:\n",letters[i]);
        result = process_single_volume(letters[i]);
    }
    
    /* if the job was stopped return success */
    if(stop_flag) return EXIT_SUCCESS;
    
    /* in case of a single volume processing return result */
    if(letters[1] == 0 && !all_flag && !all_fixed_flag)
        return result;
    
    /* 3. handle --all and --all-fixed options */
    if(all_flag || all_fixed_flag){
        v = udefrag_get_vollist(all_fixed_flag ? TRUE : FALSE);
        if(v == NULL) return EXIT_FAILURE;
        for(i = 0; v[i].letter != 0; i++){
            if(stop_flag) break;
            //printf("%c:\n",v[i].letter);
            (void)process_single_volume(v[i].letter);
        }
        udefrag_release_vollist(v);
    }
    return EXIT_SUCCESS;
}

/**
 * @brief Displays list of disk volumes
 * available for defragmentation.
 * @return Zero for success, nonzero
 * value indicates failure.
 */
static int show_vollist(void)
{
    volume_info *v;
    int i, percent;
    char s[32];
    double free, total;
    double d = 0;

    settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("Disks available for defragmentation:\n\n");
    printf("Drive     FS     Capacity       Free   Label\n");
    printf("--------------------------------------------\n");

    v = udefrag_get_vollist(la_flag ? FALSE : TRUE);
    if(v == NULL){
        settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        return EXIT_FAILURE;
    }

    for(i = 0; v[i].letter != 0; i++){
        udefrag_bytes_to_hr((ULONGLONG)(v[i].total_space.QuadPart),2,s,sizeof(s));
        total = (double)v[i].total_space.QuadPart;
        free = (double)v[i].free_space.QuadPart;
        if(total > 0) d = free / total;
        percent = (int)(100 * d);
        settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("%c:  %8s %12s %8u %%   %ls\n",
            v[i].letter,v[i].fsname,s,percent,v[i].label);
    }
    udefrag_release_vollist(v);
    settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    return EXIT_SUCCESS;
}

/**
 * @brief A stub.
 */
static void RunScreenSaver(void)
{
    printf("Hello!\n");
}

/*DWORD WINAPI test(LPVOID p)
{
    udefrag_start_job('c',ANALYSIS_JOB,0,0,NULL,NULL,NULL);
    return 0;
}

void test(void)
{
    int i;
    
    for(i = 0; i < 10; i++)
        WgxCreateThread(test,NULL);
}
*/

static int out_of_memory_handler(size_t n)
{
    settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("\nOut of memory!\n");
    settextcolor(default_color);
    udefrag_flush_dbg_log(FLUSH_IN_OUT_OF_MEMORY);
    exit(3); return 0;
}

/**
 * @brief Command line tool entry point.
 */
int __cdecl main(int argc, char **argv)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int init_result;
    int parse_cmdline_result;
    int pause_result;
    int result = EXIT_FAILURE;
    
    /*test();
    getch();
    return 0;
    */

    /* save default color */
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if(GetConsoleScreenBufferInfo(hStdOut,&csbi))
        default_color = csbi.wAttributes;

    /* initialize the program */
    init_result = udefrag_init_library();
    udefrag_set_killer(out_of_memory_handler);
    WgxSetInternalTraceHandler(udefrag_dbg_print);
    parse_cmdline_result = parse_cmdline(argc,argv);
    init_console();
    
    /* handle help request */
    if(h_flag){
        show_help();
        terminate_console(EXIT_SUCCESS);
    }

    printf(VERSIONINTITLE ", Copyright (c) UltraDefrag Development Team, 2007-2013.\n"
        "UltraDefrag comes with ABSOLUTELY NO WARRANTY. This is free software, \n"
        "and you are welcome to redistribute it under certain conditions.\n\n"
        );
        
    /* check for admin rights - they're strongly required */
    if(!WgxCheckAdminRights()){
        display_error("Administrative rights are needed to run the program!\n");
        terminate_console(EXIT_FAILURE);
    }

    /* handle initialization failure */
    if(init_result < 0){
        display_error("Initialization failed!\nSend bug report to the authors please.\n");
        terminate_console(EXIT_FAILURE);
    }

    /* handle obsolete options */
    if(obsolete_option){
        display_error("The -i, -e, -s, -d options are oblolete.\n"
            "Use environment variables instead!\n");
        terminate_console(EXIT_FAILURE);
    }

    /* show list of volumes if requested */
    if(l_flag) terminate_console(show_vollist());
    
    /* run disk defragmentation job */
    if(parse_cmdline_result == 0){
        if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE))
            display_last_error("Cannot set Ctrl + C handler!");
        
        begin_synchronization();
        
        /* uncomment for the --wait option testing */
        //printf("the job gets running\n");
        //_getch();
        result = process_volumes();
        
        end_synchronization();
        
        (void)SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
    }

    /* display prompt to hit any key in case of context menu handler */
    if(shellex_flag){
        settextcolor(default_color);
        printf("\n");
        pause_result = system("pause");
        if(pause_result > 0){
            /* command not found */
            printf("\n");
        }
        if(pause_result != 0 && pause_result != STATUS_CONTROL_C_EXIT){
            /* command or a command interpreter itself not found */
            printf("Hit any key to continue...");
            _getch();
        }
    }

    destroy_paths();
    terminate_console(result);
    return EXIT_SUCCESS; /* this point will never be reached */
}
