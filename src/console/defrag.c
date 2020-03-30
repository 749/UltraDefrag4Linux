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
* UltraDefrag console interface.
*/

/*
* Revised by Stefan Pendl, 2010, 2011
* <stefanpe@users.sourceforge.net>
*/

#include "udefrag.h"
#ifdef LINUX
#include <fcntl.h>
#include <errno.h>
#endif
#include "extrawin.h"

#ifdef STATIC_LIB
                                     /* JPA should be in an included file */
void __stdcall udefrag_native_init(void);
void winx_init_library(void *peb);
#endif

#ifdef LINUX
HANDLE open_volume(char);
int close_volume(HANDLE);
#endif

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
#ifdef LINUX
utf_t *volumes['A' + MAX_DOS_DRIVES - 1] = {0};
#endif
int wait_flag = 0;
int shellex_flag = 0;
int folder_flag = 0;
int folder_itself_flag = 0;
int quick_optimize_flag = 0;
int optimize_mft_flag = 0;
int repeat_flag = 0;
int trace = 0;
int do_nothing = 0;

int first_progress_update;
int stop_flag = 0;

int web_statistics_completed = 0;

/*
* MSDN states that environment variables
* are limited by 32767 characters,
* including terminal zero.
*/
#if defined(LINUX) && STSC
#define MAX_ENV_VARIABLE_LENGTH 1024
#else
#define MAX_ENV_VARIABLE_LENGTH 32766
#endif
utf_t in_filter[MAX_ENV_VARIABLE_LENGTH + 1];
utf_t new_in_filter[MAX_ENV_VARIABLE_LENGTH + 1];
utf_t aux_buffer[MAX_ENV_VARIABLE_LENGTH + 1];
utf_t env_buffer[MAX_ENV_VARIABLE_LENGTH + 1];

/* forward declarations */
static int show_vollist(void);
static int process_volumes(void);
static void RunScreenSaver(void);
#ifdef LINUX
BOOLEAN CtrlHandlerRoutine(DWORD dwCtrlType);
#else
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType);
#endif

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

/**
 * @brief Prints Unicode string.
 * @note Only characters compatible
 * with the current locale (OEM by default)
 * will be printed correctly. Otherwise
 * the '?' characters will be printed instead.
 */
void print_unicode_string(utf_t *string)
{
#if defined(LINUX) | defined(CURSES)
    if (m_flag)
        set_message(ROW_FILE,0,
                (b_flag ? 0 : FOREGROUND_GREEN | FOREGROUND_INTENSITY),
                printableutf(string));
    else
        printf("%s",string);
#else
    DWORD written;
    
    if(string)
        WriteConsoleW(hStdOut,string,utflen(string),&written,NULL);
#endif
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
    if(!GetEnvironmentVariableW(UTF("UD_DISABLE_USAGE_TRACKING"),env_buffer,MAX_ENV_VARIABLE_LENGTH + 1)){
        if(GetLastError() != ERROR_ENVVAR_NOT_FOUND)
            WgxDbgPrintLastError("UpdateWebStatisticsThreadProc: cannot get %%UD_DISABLE_USAGE_TRACKING%%!");
    } else {
        if(utfcmp(env_buffer,UTF("1")) == 0)
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
    HANDLE h;
    DWORD id;

    h = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)UpdateWebStatisticsThreadProc,NULL,0,&id);
    if(h == NULL){
        if (trace)
            WgxDbgPrintLastError("Cannot run UpdateWebStatisticsThreadProc");
        web_statistics_completed = 1;
    } else {
        CloseHandle(h);
    }
}

/**
 * @brief Waits for web statistics request completion.
 */
void stop_web_statistics()
{
#ifndef UNTHREADED
    while(!web_statistics_completed) Sleep(100);
#endif
}

/**
 * @brief Performs basic console initialization.
 * @details Starts thread delivering usage statistics
 * to the server.
 */
static void init_console(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    /* save default color */
    if(GetConsoleScreenBufferInfo(hStdOut,&csbi))
        default_color = csbi.wAttributes;
    /* set green color */
    if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);

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
    if(!b_flag) settextcolor(default_color);

    /* wait for web statistics request completion */
    stop_web_statistics();
    
#if defined(LINUX) | defined(CURSES)
    if (m_flag)
        close_console(hStdOut);
    /* return to caller */
#else
    if (stop_flag)
        code = 1;
    exit(code);
#endif
}

/**
 * @brief Handles Ctrl+C keys.
 */
#ifdef LINUX
/* Cannot be WINAPI */
BOOLEAN CtrlHandlerRoutine(DWORD dwCtrlType)
#else
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
#endif
{
    stop_flag = 1;
    if (trace)
        safe_fprintf(stderr,"CtrlHandlerRoutine called\n");
#ifdef CURSES
    if (m_flag) {
        if(!b_flag) settextcolor(COMMON_LVB_REVERSE_VIDEO);
        set_message(ROW_END,0,
             (b_flag ? 0 : FOREGROUND_RED | FOREGROUND_INTENSITY),
             "Please wait...\n");
        if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
#endif
    return TRUE;
}

#ifdef LINUX
void stop_there(int n, const char *file, int line)
{
    if (trace)
        safe_fprintf(stderr,"Stop code %d called from %s line %d\n",n,file,line);
fprintf(stderr,"Stop code %d called from %s line %d\n",n,file,line);
    stop_flag = (n ? n : 1);
}
#endif

/**
 * @brief Prints the string in red, than restores green color.
 */
void display_error(char *string)
{
    if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
#if defined(LINUX) | defined(CURSES)
    if (m_flag) {
        set_message(ROW_ERR,0,
            (b_flag ? 0 : FOREGROUND_RED | FOREGROUND_INTENSITY), string);
        if (trace)
            safe_fprintf(stderr,string);
    } else
#endif
        printf("%s",string);
    if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void display_formatted_error(const char *format, ...)
{
    char buf[121];
    va_list ap;

    va_start(ap,format);
    vsnprintf(buf, 121, format, ap);
    buf[120] = 0;
    display_error(buf);
    va_end(ap);
}

/**
 * @brief Displays error message
 * specific for failed disk processing tasks.
 */
static void display_defrag_error(udefrag_job_type job_type, int error_code)
{
    char *operation = "optimization";
    
    if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    
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
    printf("\nDisk %s failed!\n\n",operation);
    
    if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("%s\n\n",udefrag_get_error_description(error_code));
    if(error_code == UDEFRAG_UNKNOWN_ERROR)
        printf("Enable logs or use DbgView program to get more information.\n\n");
    
    if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Displays error message
 * specific for invalid disk volumes.
 */
static void display_invalid_volume_error(int error_code)
{
    if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("The disk cannot be processed.\n\n");
    if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    if(error_code == UDEFRAG_UNKNOWN_ERROR){
        printf("Disk is missing or some unknown error has been encountered.\n");
        printf("Enable logs or use DbgView program to get more information.\n\n");
    } else {
        printf("%s\n\n",udefrag_get_error_description(error_code));
    }

    if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Displays last error message in red.
 */
void display_last_error(char *caption)
{
    LPVOID lpMsgBuf;
    DWORD error = GetLastError();

fprintf(stderr,"display_last_error %s\n",caption);
    if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,0,NULL)){
                if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
                if(error == ERROR_COMMITMENT_LIMIT)
                    printf("\n%s\nNot enough memory.\n\n",caption);
                else
                    printf("\n%s\nError code = 0x%x\n\n",caption,(UINT)error);
                if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    } else {
        if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
#if defined(LINUX) | defined(CURSES)
        if (m_flag)
            set_message(ROW_ERR,0,
                (b_flag ? 0 : FOREGROUND_RED | FOREGROUND_INTENSITY), caption);
        else
#endif
            printf("\n%s\n%s\n",caption,(char *)lpMsgBuf);
        if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        LocalFree(lpMsgBuf);
    }
}

/**
* @brief Synchronizes with other scheduled 
* jobs when running with --wait option.
*/
void begin_synchronization(void)
{
#ifndef UNTHREADED
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
#endif
}

void end_synchronization(void)
{
#ifndef UNTHREADED
    /* release event */
    if(hSynchEvent)
        CloseHandle(hSynchEvent);
#endif
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
#if defined(CURSES) | defined(LINUX)
    char buf[120];
#ifdef LINUX
    char *volume;
#else
    char volume[2];
#endif
#endif
    
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
#if defined(CURSES) | defined(LINUX)
#ifdef LINUX
        volume = volumes[volume_letter];
#else
        volume[0] = volume_letter;
        volume[1] = 0;
#endif
        if(pi->current_operation == VOLUME_OPTIMIZATION && !stop_flag && pi->completion_status == 0){
            if(pi->pass_number > 1)
                sprintf(buf,"%s: %s%6.2lf%% complete, pass %lu, moves total = %" LL64 "u",
                    volume,op_name,pi->percentage,pi->pass_number,(ULONGLONG)pi->total_moves);
            else
                sprintf(buf,"%s: %s%6.2lf%% complete, moves total = %" LL64 "u",
                    volume,op_name,pi->percentage,(ULONGLONG)pi->total_moves);
        } else {
            if(pi->pass_number > 1)
                sprintf(buf,"%s: %s%6.2lf%% complete, pass %lu, fragmented/total = %lu/%lu",
                    volume,op_name,pi->percentage,pi->pass_number,pi->fragmented,pi->files);
            else
                sprintf(buf,"%s: %s%6.2lf%% complete, fragmented/total = %lu/%lu",
                    volume,op_name,pi->percentage,pi->fragmented,pi->files);
        }
        if (m_flag)
            set_message(ROW_PROGRESS, 0,
                (b_flag ? 0 : FOREGROUND_GREEN | FOREGROUND_INTENSITY), buf);
        else
            printf("%s\n",buf);
        if (pi->completion_status != 0 && !stop_flag) {
            /* set progress indicator to 100% state */
            if(pi->pass_number > 1)
                sprintf(buf,"%s: %s100.00%% complete, %lu passes needed, fragmented/total = %lu/%lu",
                    volume,op_name,pi->pass_number,pi->fragmented,pi->files);
            else
                sprintf(buf,"%s: %s100.00%% complete, fragmented/total = %lu/%lu",
                    volume,op_name,pi->fragmented,pi->files);
        if (m_flag)
            set_message(ROW_PROGRESS,0,
                (b_flag ? 0 : FOREGROUND_GREEN | FOREGROUND_INTENSITY), buf);
        else
            printf("%s\n",buf);
        }
#else /* CURSES or LINUX */
        clear_line(stderr);
        if(pi->current_operation == VOLUME_OPTIMIZATION && !stop_flag && pi->completion_status == 0){
            if(pi->pass_number > 1)
                fprintf(stderr,"\r%c: %s%6.2lf%% complete, pass %lu, moves total = %" LL64 "u",
                    volume_letter,op_name,pi->percentage,pi->pass_number,(ULONGLONG)pi->total_moves);
            else
                fprintf(stderr,"\r%c: %s%6.2lf%% complete, moves total = %" LL64 "u",
                    volume_letter,op_name,pi->percentage,(ULONGLONG)pi->total_moves);
        } else {
            if(pi->pass_number > 1)
                fprintf(stderr,"\r%c: %s%6.2lf%% complete, pass %lu, fragmented/total = %lu/%lu",
                    volume_letter,op_name,pi->percentage,pi->pass_number,pi->fragmented,pi->files);
            else
                fprintf(stderr,"\r%c: %s%6.2lf%% complete, fragmented/total = %lu/%lu",
                    volume_letter,op_name,pi->percentage,pi->fragmented,pi->files);
        }
        if(pi->completion_status != 0 && !stop_flag){
            /* set progress indicator to 100% state */
            clear_line(stderr);
            if(pi->pass_number > 1)
                fprintf(stderr,"\r%c: %s100.00%% complete, %lu passes needed, fragmented/total = %lu/%lu",
                    volume_letter,op_name,pi->pass_number,pi->fragmented,pi->files);
            else
                fprintf(stderr,"\r%c: %s100.00%% complete, fragmented/total = %lu/%lu",
                    volume_letter,op_name,pi->fragmented,pi->files);
            if(!m_flag) fprintf(stderr,"\n");
        }
#endif /* CURSES or LINUX */
        
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

void __stdcall progress_feedback(udefrag_progress_info *pi, const char *text)
{
#ifdef LINUXMODE
    if (trace && !isatty(2))
        safe_fprintf(stderr,"* %s\n",text);
#ifdef CURSES
    if (m_flag)
        set_message(ROW_FILE,0,
                (b_flag ? 0 : FOREGROUND_GREEN | FOREGROUND_INTENSITY), text);
    else
        safe_fprintf(stdout,"* %s\n",text);
#else
    safe_fprintf(stdout,"* %s\n",text);
#endif
#endif
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

    if (trace)
        safe_fprintf(stderr,"processing single volume %c\n",volume_letter);
    /* validate volume letter */
    if(volume_letter == 0){
        display_error("Drive letter should be specified!\n");
        return 3;
    }
if (((volume_letter >= 'c') && (volume_letter <= 'e'))
|| ((volume_letter >= 'C') && (volume_letter <= 'E')))
{
safe_fprintf(stderr,"Drives C:, D: and E: are not allowed now for safety\n");
exit(1);
}
    result = udefrag_validate_volume(volume_letter,FALSE);
    if(result < 0){
        display_invalid_volume_error(result);
        return 3;
    }

    /* initialize cluster map */
    if(m_flag){
        if(AllocateClusterMap() < 0){
            /* terminate process */
            (void)SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
            terminate_console(4);
        }
        InitializeMapDisplay(volume_letter);
        map_size = map_rows * map_symbols_per_line;
    }
    
    /* check if we need to run in screensaver mode */
    if(screensaver_mode){
        RunScreenSaver();
        FreeClusterMap();
        return 0;
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
    if (trace)
        safe_fprintf(stderr,"job type %s\n",(a_flag ? "analysis" : (o_flag ? "full optimisation" : "quick optimisation")));
    result = udefrag_start_job(volume_letter,job_type,flags,map_size,
        update_progress,progress_feedback,terminator,
        (void *)(DWORD_PTR)volume_letter);
    if(result < 0){
        display_defrag_error(job_type, result);
        FreeClusterMap();
        return 5;
    }

    FreeClusterMap();
    return 0;
}

/**
 * @brief Processes all volumes specified on the command line.
 * @return Zero for success, nonzero value indicates failure.
 */
static int process_volumes(void)
{
    object_path *path, *another_path;
    int single_path = 0;
    int i, result = 0;
    char letter;
    int n, path_found;
    int first_path = 1;
#ifdef LINUX
    HANDLE current_volume;
    int assigned_letter = 'A';
#else
    volume_info *v;
#endif
    
    /* 1. process paths specified on the command line */
    /* skip invalid paths */
    for(path = paths; path; path = path->next){
#ifdef LINUX
        char buf[512];
        int f;

        if (!ntfs_mounted_device(path->path)) {
#if STSC
            f = open(path->path,O_RDONLY);
#else
            f = open64(path->path,O_RDONLY);
#endif
            if (f > 0) {
                n = read(f,buf,512);
                if (n < 512) {
                    display_formatted_error("Could not read the boot sector of %s\n",path->path);
                    path->processed = 1;
                } else {
                    if (strncmp(&buf[3],"NTFS    ",8)) {
                        display_formatted_error("%s is not an NTFS volume\n",path->path);
                        path->processed = 1;
                    }
                }
                close (f);
            } else {
                display_formatted_error("Could not open %s\n",path->path);
                path->processed = 1;
            }
        } else {
            display_formatted_error("%s is mounted, cannot defragment\n",path->path);
            path->processed = 1;
        }
#else
        if(utflen(path->path) < 2){
            display_formatted_error("incomplete path detected: %" LSTR "\n",path->path);
            path->processed = 1;
        }
        if(path->path[1] != ':'){
            display_formatted_error("incomplete path detected: %" LSTR "\n",path->path);
            path->processed = 1;
        }
#endif
        if(path->next == paths) break;
    }
    /* process valid paths */
    for(path = paths; path; path = path->next){
        if(path->processed == 0){
            if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            if(first_path){
                print_unicode_string(path->path);
                if (!m_flag) printf("\n");
            } else {
                if (!m_flag) printf("\n");
                print_unicode_string(path->path);
                if (!m_flag) printf("\n");
            }
            if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            first_path = 0;
            path->processed = 1;
            path_found = 1;
            
            /* extract drive letter */
#ifdef LINUX
            letter = assigned_letter++;
            volumes[letter] = path->path;
            if (trace)
                safe_fprintf(stderr,"Assigning letter %c to %s\n",
                    letter,volumes[letter]);
#else
            letter = (char)path->path[0];
#endif
            
            /* save %UD_IN_FILTER% */
            in_filter[0] = 0;
            if(!GetEnvironmentVariableW(UTF("UD_IN_FILTER"),in_filter,MAX_ENV_VARIABLE_LENGTH + 1)){
                if(GetLastError() != ERROR_ENVVAR_NOT_FOUND)
                    display_last_error("process_volumes: cannot get %%UD_IN_FILTER%%!");
            }
            
            if(shellex_flag && (folder_flag || folder_itself_flag)){
                if(folder_flag){
                    if(path->path[utflen(path->path) - 1] == '\\'){
                        /* set UD_IN_FILTER=c:\* */
                        n = _snwprintf(new_in_filter,MAX_ENV_VARIABLE_LENGTH + 1,UTF("%" LSTR "*"),path->path);
                    } else {
                        /* set UD_IN_FILTER=c:\test;c:\test\* */
                    n = _snwprintf(new_in_filter,MAX_ENV_VARIABLE_LENGTH + 1,UTF("%" LSTR),path->path);
                    }
                } else {
                    /* set UD_IN_FILTER=c:\test */
                    n = _snwprintf(new_in_filter,MAX_ENV_VARIABLE_LENGTH + 1,UTF("%" LSTR),path->path);
                }
                if(n < 0){
                    display_error("process_volumes: cannot set %%UD_IN_FILTER%% - path is too long!");
                    utfcpy(new_in_filter,in_filter);
                    path_found = 0;
                } else {
                    new_in_filter[MAX_ENV_VARIABLE_LENGTH] = 0;
                }
            } else {
                /* save the current path to %UD_IN_FILTER% */
                n = _snwprintf(new_in_filter,MAX_ENV_VARIABLE_LENGTH + 1,UTF("%" LSTR),path->path);
                if(n < 0){
                    display_error("process_volumes: cannot set %%UD_IN_FILTER%% - path is too long!");
                    utfcpy(new_in_filter,in_filter);
                    path_found = 0;
                } else {
                    new_in_filter[MAX_ENV_VARIABLE_LENGTH] = 0;
                }
                
                /* search for another paths with the same drive letter */
                for(another_path = path->next; another_path; another_path = another_path->next){
                    if(another_path == paths) break;
                    if(udefrag_toupper(letter) == udefrag_toupper((char)another_path->path[0])){
                        /* try to append it to %UD_IN_FILTER% */
                        n = _snwprintf(aux_buffer,MAX_ENV_VARIABLE_LENGTH + 1,UTF("%" LSTR ";%" UTFSTR),new_in_filter,another_path->path);
                        if(n >= 0){
                            aux_buffer[MAX_ENV_VARIABLE_LENGTH] = 0;
                            utfcpy(new_in_filter,aux_buffer);
                            path_found = 1;
                            if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                            print_unicode_string(another_path->path);
                            if (!m_flag) printf("\n");
                            if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                            another_path->processed = 1;
                        }
                    }
                }
            }
            
            /* set %UD_IN_FILTER% */
            if(stop_flag) return 0;
            if(!SetEnvironmentVariableW(UTF("UD_IN_FILTER"),new_in_filter)){
                display_last_error("process_volumes: cannot set %%UD_IN_FILTER%%!");
            }
            
            if (trace)
                safe_fprintf(stderr,"about to process drive %c:\n",letter);
            /* run the job */
#ifdef LINUX
            if (path_found) {
                current_volume = open_volume(letter);
                if (current_volume) {
                    result = process_single_volume(letter);
                    close_volume(current_volume);
                } else
                    display_formatted_error("Cannot process volume %s\n",volumes[letter]);
            }
#else
            if(path_found)
                result = process_single_volume(letter);
#endif
            
            /* restore %UD_IN_FILTER% */
            if(!SetEnvironmentVariableW(UTF("UD_IN_FILTER"),in_filter)){
                display_last_error("process_volumes: cannot restore %%UD_IN_FILTER%%!");
            }
        }
        if(path->next == paths) break;
    }
    
    /* if the job was stopped return success */
    if(stop_flag)
        return 0;
    
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
        if(stop_flag) return 0;
        //printf("%c:\n",letters[i]);
#ifdef LINUX
        current_volume = open_volume(letter);
        if (current_volume) {
            result = process_single_volume(letters[i]);
            close_volume(current_volume);
        } else
            display_formatted_error("Cannot process volume %s\n",volumes[letters[i]]);
#else
        result = process_single_volume(letters[i]);
#endif
    }
    
    /* if the job was stopped return success */
    if(stop_flag)
        return 0;
    
    /* in case of a single volume processing return result */
    if(letters[1] == 0 && !all_flag && !all_fixed_flag)
        return result;
    
    /* 3. handle --all and --all-fixed options */
#ifdef LINUX
        /* not supported */
#else
    if(all_flag || all_fixed_flag){
        v = udefrag_get_vollist(all_fixed_flag ? TRUE : FALSE);
        if(v == NULL)
            return 1;
        for(i = 0; v[i].letter != 0; i++){
            if(stop_flag) break;
            //printf("%c:\n",v[i].letter);
            (void)process_single_volume(v[i].letter);
        }
        udefrag_release_vollist(v);
    }
#endif
    return 0;
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
    ULONGLONG free, total;
    double d = 0;

    if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("Disks available for defragmentation:\n\n");
    printf("Drive     FS     Capacity       Free   Label\n");
    printf("--------------------------------------------\n");

    v = udefrag_get_vollist(la_flag ? FALSE : TRUE);
    if(v == NULL){
        if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        return 2;
    }

    for(i = 0; v[i].letter != 0; i++){
        udefrag_bytes_to_hr((ULONGLONG)(v[i].total_space.QuadPart),2,s,sizeof(s));
        /* conversion to LONGLONG is needed for Win DDK */
        /* so, let's divide both numbers to make safe conversion then */
        total = v[i].total_space.QuadPart / 2;
        free = v[i].free_space.QuadPart / 2;
        if(total > 0)
            d = (double)(LONGLONG)free / (double)(LONGLONG)total;
        percent = (int)(100 * d);
        if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("%c:  %8s %12s %8u %%   %" LSTR "\n",
            v[i].letter,v[i].fsname,s,percent,v[i].label);
    }
    udefrag_release_vollist(v);
    if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    return 0;
}

/**
 * @brief A stub.
 */
static void RunScreenSaver(void)
{
    printf("Hello!\n");
}

/*DWORD WINAPI test_thread(LPVOID p)
{
    udefrag_start_job('c',ANALYSIS_JOB,0,0,NULL,NULL,NULL,NULL);
    return 0;
}

void test(void)
{
    int i;
    DWORD id;
    
    for(i = 0; i < 10; i++)
        CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)test_thread,NULL,0,&id);
}
*/

/**
 * @brief Command line tool entry point.
 */
int __cdecl main(int argc, char **argv)
{
    int result, pause_result;
    
    /*test();
    getch();
    return 0;
    */

#ifdef LINUX
    initwincalls();
#endif
#ifdef STATIC_LIB
    winx_init_library((void*)NULL);
    udefrag_native_init();
#endif

    /* initialize the program */
    parse_cmdline(argc,argv);
    init_console();
    
    /* handle help request */
    if(h_flag){
        show_help();
        terminate_console(0);
        goto out;
    }

#ifdef CURSES
    if (m_flag)
        set_message(ROW_CAPTION,0,
                (b_flag ? 0 : FOREGROUND_GREEN | FOREGROUND_INTENSITY),
                 VERSIONINTITLE ", Copyright (c) UltraDefrag Development Team, 2007-2011.\n");
    else
#endif
    printf(VERSIONINTITLE ", Copyright (c) UltraDefrag Development Team, 2007-2011.\n"
        "UltraDefrag comes with ABSOLUTELY NO WARRANTY. This is free software, \n"
        "and you are welcome to redistribute it under certain conditions.\n\n"
        );
        
    /* handle initialization failure */
    if(udefrag_init_failed()){
        display_error("Initialization failed!\nSend bug report to the authors please.\n");
        terminate_console(1);
    }

    /* handle obsolete options */
    if(obsolete_option){
        display_error("The -i, -e, -s, -d options are obsolete.\n"
            "Use environment variables instead!\n");
        terminate_console(1);
        goto out;
    }

    /* show list of volumes if requested */
    if(l_flag) {
        terminate_console(show_vollist());
        goto out;
    }
    
    /* run disk defragmentation job */
    if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE))
        display_last_error("Cannot set Ctrl + C handler!");
    
    begin_synchronization();
    
    /* uncomment for the --wait option testing */
    //printf("the job gets running\n");
    //_getch();
    result = process_volumes();
    
    end_synchronization();
    
    (void)SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
    
    /* display prompt to hit any key in case of context menu handler */
    if(shellex_flag){
        if(!b_flag) settextcolor(default_color);
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
out :
#ifdef LINUX
    endwincalls();
    winx_unload_library();
    if (stop_flag)
        exit(1);
#endif
    return 0; /* this point will never be reached */
}
