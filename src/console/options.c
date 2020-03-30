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
* UltraDefrag console interface - command line parsing code.
*/

#include "udefrag.h"
#include "../share/getopt.h"
#include "extrawin.h"

/* forward declarations */
#ifdef LINUXMODE
void search_for_paths(int, char**);
#else
void search_for_paths(void);
#endif

#if LXSC
#undef sscanf  /* No __isoc99_sscanf in older versions */
#endif

int printf_stub(const char *format,...)
{
    return 0;
}

void show_help(void)
{
    printf(
        "===============================================================================\n"
        VERSIONINTITLE " - a powerful disk defragmentation tool for Windows NT\n"
        "Copyright (c) UltraDefrag Development Team, 2007-2011.\n"
        "\n"
        "===============================================================================\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n"
        "===============================================================================\n"
        "\n"
#ifdef LINUXMODE
        "Usage: udefrag [command] [options] [device-path]\n"
#else
        "Usage: udefrag [command] [options] [driveletter:] [path(s)]\n"
#endif
        "\n"
        "  The default action is to display this help message.\n"
        "\n"
        "Commands:\n"
        "  -a,  --analyze                      Analyze disk\n"
        "       --defragment                   Defragment disk\n"
        "  -o,  --optimize                     Optimize disk space\n"
        "  -q,  --quick-optimize               Perform quick optimization\n"
        "       --optimize-mft                 Concatenate the MFT into one single chunk\n"
#ifndef LINUXMODE /* not available now */
        "  -l,  --list-available-volumes       List disks available\n"
        "                                      for defragmentation,\n"
        "                                      except removable media\n"
        "  -la, --list-available-volumes=all   List all available disks\n"
#else
        "  -n,  --dry-run                      Do not apply modifications\n"
#endif
        "  -h,  --help                         Show this help screen\n"
        "  -?                                  Show this help screen\n"
        "\n"
        "  If no command is specified it will defragment the disk.\n"
        "\n"
        "Options:\n"
#ifndef LINUXMODE /* not available now */
        "  -r,  --repeat                       Repeat action until nothing left to move.\n"
        "  -b,  --use-system-color-scheme      Use system (usually black/white)\n"
        "                                      color scheme instead of the green color.\n"
        "  -p,  --suppress-progress-indicator  Hide progress indicator.\n"
        "  -v,  --show-volume-information      Show disk information after a job.\n"
#endif
        "  -m,  --show-cluster-map             Show map representing clusters\n"
        "                                      on the disk.\n"
#ifndef LINUXMODE /* not available now */
        "       --map-border-color=color       Set cluster map border color.\n"
        "                                      Available color values: black, white,\n"
        "                                      red, green, blue, yellow, magenta, cyan,\n"
        "                                      darkred, darkgreen, darkblue, darkyellow,\n"
        "                                      darkmagenta, darkcyan, gray.\n"
        "                                      Yellow color is used by default.\n"
        "       --map-symbol=x                 Set a character used for the map drawing.\n"
        "                                      There are two accepted formats:\n"
        "                                      a character may be typed directly,\n"
        "                                      or its hexadecimal number may be used.\n"
        "                                      For example, --map-symbol=0x1 forces\n"
        "                                      UltraDefrag to use a smile character\n"
        "                                      for the map drawing.\n"
        "                                      Valid numbers are in range: 0x1 - 0xFF\n"
        "                                      \'%%\' symbol is used by default.\n"
#endif
        "       --map-rows=n                   Number of rows in cluster map.\n"
        "                                      Default value is 10.\n"
        "       --map-symbols-per-line=n       Number of map symbols\n"
        "                                      containing in each row of the map.\n"
        "                                      Default value is 68.\n"
#ifndef LINUXMODE /* not available now */
        "       --use-entire-window            Expand cluster map to use entire\n"
        "                                      console window.\n" 
        "       --wait                         Wait until already running instance\n"
        "                                      of udefrag.exe tool completes before\n"
        "                                      starting the job (useful for\n"
        "                                      the scheduled defragmentation).\n"
        "       --shellex                      This option forces to list objects\n"
        "                                      to be processed and display a prompt\n"
        "                                      to hit any key after a job completion.\n"
        "                                      It is intended for use in Explorer\'s\n"
        "                                      context menu handler.\n"
#endif
        "\n"
#ifdef LINUX
        "    device-path                       path to device to defragment\n"
                "                                      such as \"dev/sdb1\"\n"
#else
        "Drive letter:\n"
        "  It is possible to specify multiple drive letters, like this:\n\n"
        "  udefrag c: d: x:\n\n"
        "  Also the following keys can be used instead of the drive letter:\n\n"
        "  --all                               Process all available disks.\n"
        "  --all-fixed                         Process all disks except removable.\n"
        "\n"
        "Path:\n"
        "  It is possible to specify multiple paths, like this:\n\n"
        "  udefrag \"C:\\Documents and Settings\" C:\\WINDOWS\\WindowsUpdate.log\n\n"
        "  Paths including spaces must be enclosed in double quotes (\").\n"
        "  Relative and absolute paths are supported.\n"
        "  Short paths (like C:\\PROGRA~1\\SOMEFI~1.TXT) aren\'t accepted on NT4.\n"
#endif
        "\n"
        "Accepted environment variables:\n"
        "\n"
#ifndef LINUXMODE /* not available now */
        "  UD_IN_FILTER                        List of files to be included\n"
        "                                      in defragmentation process. Patterns\n"
        "                                      must be separated by semicolons.\n"
        "\n"
        "  UD_EX_FILTER                        List of files to be excluded from\n"
        "                                      defragmentation process. Patterns\n"
        "                                      must be separated by semicolons.\n"
        "\n"
#endif
        "  UD_SIZELIMIT                        Exclude all files larger than specified.\n"
        "                                      The following size suffixes are accepted:\n"
        "                                      Kb, Mb, Gb, Tb, Pb, Eb.\n"
        "\n"
        "  UD_FRAGMENTS_THRESHOLD              Exclude all files which have less number\n"
        "                                      of fragments than specified.\n"
        "\n"
        "  UD_TIME_LIMIT                       When the specified time interval elapses\n"
        "                                      the job will be stopped automatically.\n"
        "                                      The following time format is accepted:\n"
        "                                      Ay Bd Ch Dm Es. Here A,B,C,D,E represent\n"
        "                                      any integer numbers, y,d,h,m,s - suffixes\n"
        "                                      used for years, days, hours, minutes\n"
        "                                      and seconds.\n"
        "\n"
        "  UD_REFRESH_INTERVAL                 Progress refresh interval,\n"
        "                                      in milliseconds.\n"
        "                                      The default value is 100.\n"
        "\n"
#ifndef LINUXMODE /* not available now */
        "  UD_DISABLE_REPORTS                  If this environment variable is set\n"
        "                                      to 1 (one), no file fragmentation\n"
        "                                      reports will be generated.\n"
        "\n"
        "  UD_DBGPRINT_LEVEL                   Control amount of the debugging output.\n"
        "                                      NORMAL is used by default, DETAILED\n"
        "                                      may be used to collect information for\n"
        "                                      the bug report, PARANOID turns on\n"
        "                                      a really huge amount of debugging\n"
        "                                      information.\n"
        "\n"
        "  UD_LOG_FILE_PATH                    If this variable is set, it should\n"
        "                                      contain the path (including filename)\n"
        "                                      to the log file to save debugging output\n"
        "                                      into. If the variable is not set, no\n"
        "                                      logging to the file will be performed.\n"
        "\n"
#endif
        "  UD_DRY_RUN                          If this environment variable is set\n"
        "                                      to 1 (one) the files are not physically\n"
        "                                      moved.\n"
        "                                      This allows testing the algorithm without\n"
        "                                      changing the content of the disk.\n"
        "\n"
        "Examples:\n"
#ifdef LINUX
        "\n"
        "  udefrag -om /dev/sda5               optimize partition /dev/sda5\n"
        "                                      and display its map\n"
        "\n"
        "  udefrag --optimize-mft /dev/sda5    optimize the MFT of /dev/sda5\n"
        "\n"
        "  udefrag -anm /dev/sda5              display the map of partition /dev/sda5\n"
        "                                      without changing anything\n"
        "\n"
#else
        "  set UD_IN_FILTER=*windows*;*winnt*  include only paths, which include either\n"
        "                                      'windows' or 'winnt' words.\n"
        "\n"
        "  set UD_SIZELIMIT=50Mb               process only files below 50 MB in size.\n"
        "\n"
        "  set UD_FRAGMENTS_THRESHOLD=6        process only files with more than\n"
        "                                      '6' fragments.\n"
        "\n"
        "  set UD_TIME_LIMIT=6h 30m            terminate processing after 6 hours\n"
        "                                      and 30 minutes.\n"
        "\n"
        "  set UD_LOG_FILE_PATH=C:\\Windows\\Temp\\defrag_native.log\n"
        "                                      save debugging information to the\n"
        "                                      specified file.\n"
        "\n"
        "  udefrag C:                          defragment drive C:\n"
        "\n"
        "  udefrag -la                         list all available drives.\n"
        "\n"
        "  udefrag -a --all-fixed              analyze all drives excluding\n"
        "                                      removable ones.\n"
        "\n"
        "  udefrag -o E: K:                    optimize drives E: and K:\n"
        "\n"
        "  udefrag --optimize-mft O: Q:        optimize the MFT of drives O: and Q:\n"
        "\n"
        "  udefrag C:\\Windows\\WindowsUpdate.log \"C:\\Program Files\"\n"
        "                                      defragment the specified file and folder.\n"
#endif
        "\n"
        );
}

static struct option long_options_[] = {
    /*
    * Disk defragmenting options.
    */
    { "analyze",                     no_argument,       0, 'a' },
    { "defragment",                  no_argument,       0,  0  },
    { "optimize",                    no_argument,       0, 'o' },
    { "max-size",                    required_argument, 0, 's' },
    { "min-fragments",               required_argument, 0, 'f' },
    { "quick-optimize",              no_argument,       0, 'q' },
    { "optimize-mft",                no_argument,       0,  0  },
#ifndef LINUXMODE /* not available now */
    { "all",                         no_argument,       0,  0  },
    { "all-fixed",                   no_argument,       0,  0  },
    
    /*
    * Volume listing options.
    */
    { "list-available-volumes",      optional_argument, 0, 'l' },
    
    /*
    * Volume processing options.
    */
    { "repeat",                      no_argument,       0, 'r' },
    
    /*
    * Progress indicators options.
    */
    { "suppress-progress-indicator", no_argument,       0, 'p' },
    { "show-volume-information",     no_argument,       0, 'v' },
#endif
    { "show-cluster-map",            no_argument,       0, 'm' },
    
    /*
    * Colors and decoration.
    */
    { "use-system-color-scheme",     no_argument,       0, 'b' },
#ifndef LINUXMODE /* not available now */
    { "map-border-color",            required_argument, 0,  0  },
    { "map-symbol",                  required_argument, 0,  0  },
#endif
    { "map-rows",                    required_argument, 0,  0  },
    { "map-symbols-per-line",        required_argument, 0,  0  },
#ifndef LINUXMODE /* not available now */
    { "use-entire-window",           no_argument,       0,  0  },
#endif
    
    /*
    * Help.
    */
    { "help",                        no_argument,       0, 'h' },
    
#ifndef LINUXMODE /* not available now */
    /*
    * Screensaver options.
    */
    { "screensaver",                 no_argument,       0,  0  },
    
    /*
    * Miscellaneous options.
    */
    { "wait",                        no_argument,       0,  0  },
    { "shellex",                     no_argument,       0,  0  },
    { "folder",                      no_argument,       0,  0  },
    { "folder-itself",               no_argument,       0,  0  },
#endif
    { "trace",                       no_argument,       0, 't' },
    { "do-nothing",                  no_argument,       0, 'n' },
    { "dry-run",                     no_argument,       0, 'n' },
    
    { 0,                             0,                 0,  0  }
};

#ifdef LINUXMODE
char short_options_[] = "aos:f:mbh?iednt";
#else
char short_options_[] = "aoql::rpvmbh?iesd";
#endif

/* new code based on GNU getopt() function */
void parse_cmdline(int argc, char **argv)
{
    int c;
    int option_index = 0;
    const char *long_option_name;
    int dark_color_flag = 0;
    int map_symbol_number = 0;
    int rows = 0, symbols_per_line = 0;
    int letter_index;
#ifndef LINUXMODE
    char ch;
    int length;
#endif
    
    memset(&letters,0,sizeof(letters));
    letter_index = 0;
    
    if(argc < 2) h_flag = 1;
    while(1){
        option_index = 0;
        c = getopt_long(argc,argv,short_options_,
            long_options_,&option_index);
        if(c == -1) break;
        switch(c){
        case 0:
            //printf("option %s", long_options_[option_index].name);
            //if(optarg) printf(" with arg %s", optarg);
            //printf("\n");
            long_option_name = long_options_[option_index].name;
            if(!strcmp(long_option_name,"defragment")){
                /* do nothing here */
            } else if(!strcmp(long_option_name,"optimize-mft")){
                optimize_mft_flag = 1;
            } else if(!strcmp(long_option_name,"map-border-color")){
                if(!optarg) break;
                if(!strcmp(optarg,"black")){
                    map_border_color = 0x0; break;
                }
                if(!strcmp(optarg,"white")){
                    map_border_color = FOREGROUND_RED | FOREGROUND_GREEN | \
                        FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
                }
                if(!strcmp(optarg,"gray")){
                    map_border_color = FOREGROUND_RED | FOREGROUND_GREEN | \
                        FOREGROUND_BLUE; break;
                }
                if(strstr(optarg,"dark")) dark_color_flag = 1;

                if(strstr(optarg,"red")){
                    map_border_color = FOREGROUND_RED;
                } else if(strstr(optarg,"green")){
                    map_border_color = FOREGROUND_GREEN;
                } else if(strstr(optarg,"blue")){
                    map_border_color = FOREGROUND_BLUE;
                } else if(strstr(optarg,"yellow")){
                    map_border_color = FOREGROUND_RED | FOREGROUND_GREEN;
                } else if(strstr(optarg,"magenta")){
                    map_border_color = FOREGROUND_RED | FOREGROUND_BLUE;
                } else if(strstr(optarg,"cyan")){
                    map_border_color = FOREGROUND_GREEN | FOREGROUND_BLUE;
                }
                
                if(!dark_color_flag) map_border_color |= FOREGROUND_INTENSITY;
            } else if(!strcmp(long_option_name,"map-symbol")){
                if(!optarg) break;
                if(strstr(optarg,"0x") == optarg){
                    /* decode hexadecimal number */
                    (void)sscanf(optarg,"%x",&map_symbol_number);
                    if(map_symbol_number > 0 && map_symbol_number < 256)
                        map_symbol = (char)map_symbol_number;
                } else {
                    if(optarg[0]) map_symbol = optarg[0];
                }
            } else if(!strcmp(long_option_name,"screensaver")){
                screensaver_mode = 1;
            } else if(!strcmp(long_option_name,"wait")){
                wait_flag = 1;
            } else if(!strcmp(long_option_name,"shellex")){
                shellex_flag = 1;
            } else if(!strcmp(long_option_name,"folder")){
                folder_flag = 1;
            } else if(!strcmp(long_option_name,"folder_itself")){
                folder_itself_flag = 1;
            } else if(!strcmp(long_option_name,"use-entire-window")){
                use_entire_window = 1;
            } else if(!strcmp(long_option_name,"map-rows")){
                if(!optarg) break;
                rows = atoi(optarg);
                if(rows > 0) map_rows = rows;
            } else if(!strcmp(long_option_name,"map-symbols-per-line")){
                if(!optarg) break;
                symbols_per_line = atoi(optarg);
                if(symbols_per_line > 0) map_symbols_per_line = symbols_per_line;
            } else if(!strcmp(long_option_name,"all")){
                all_flag = 1;
            } else if(!strcmp(long_option_name,"all-fixed")){
                all_fixed_flag = 1;
            }
            break;
        case 'a':
            a_flag = 1;
            break;
        case 'o':
            o_flag = 1;
            break;
        case 'q':
            quick_optimize_flag = 1;
            break;
        case 'l':
            l_flag = 1;
            if(optarg){
                if(!strcmp(optarg,"a")) la_flag = 1;
                if(!strcmp(optarg,"all")) la_flag = 1;
            }
            break;
        case 'r':
            repeat_flag = 1;
            break;
        case 'p':
            p_flag = 1;
            break;
        case 'v':
            v_flag = 1;
            break;
        case 'm':
            m_flag = 1;
            break;
        case 'b':
            b_flag = 1;
            break;
        case 'h':
            h_flag = 1;
            break;
        case 'd':
        case 'i':
        case 'e':
//        case 's':
            obsolete_option = 1;
            break;
        case 's' :
            if(!SetEnvironmentVariableA("UD_SIZELIMIT",optarg)) {
                printf("Could not set SIZE LIMIT");
                h_flag++;
            }
        case 'f' :
            if(!SetEnvironmentVariableA("UD_FRAGMENTS_THRESHOLD",optarg)) {
                printf("Could not set FRAGMENTS THRESHOLD");
                h_flag++;
            }
#ifdef LINUXMODE
        case 't':
            trace++;
            break;
        case 'n':
            do_nothing++;
            if(!SetEnvironmentVariableW(UTF("UD_DRY_RUN"),UTF("1"))){
                printf("Could not set DRY-RUN mode");
                h_flag++;
            }
            break;
#endif
        case '?': /* invalid option or -? option */
            if(optopt == '?') h_flag = 1;
            break;
        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }
    
#ifdef LINUXMODE
    if (trace) {
        int i;

        fprintf(stderr,"command line:");
        for (i=0; i<argc; i++)
            fprintf(stderr," %s",argv[i]);
        fprintf(stderr,"\n");
    }
#else
    dbg_print("command line: %" LSTR "\n",GetCommandLineW());
    
    /* scan for individual volume letters */
    if(optind < argc){
        dbg_print("non-option ARGV-elements: ");
        while(optind < argc){
            dbg_print("%s ", argv[optind]);
            length = strlen(argv[optind]);
            if(length == 2){
                if(argv[optind][1] == ':'){
                    ch = argv[optind][0];
                    if(letter_index > (MAX_DOS_DRIVES - 1)){
                        printf("Too many letters specified on the command line.\n");
                    } else {
                        letters[letter_index] = ch;
                        letter_index ++;
                    }
                }
            }
            optind++;
            if(optind < argc)
                dbg_print("; ");
        }
        dbg_print("\n");
    }
#endif
    
    /* scan for paths of objects to be processed */
#ifdef LINUXMODE
    search_for_paths(argc,argv);
#else
    search_for_paths();
#endif

    /* --all-fixed flag has more precedence */
    if(all_fixed_flag) all_flag = 0;
    
    if(!l_flag && !all_flag && !all_fixed_flag && !letters[0] && !paths) h_flag = 1;
    
    /* --quick-optimize flag has more precedence */
    if(quick_optimize_flag) o_flag = 0;
    
    /* calculate map dimensions if --use-entire-window flag is set */
    if(use_entire_window) CalculateClusterMapDimensions();
}

typedef DWORD (WINAPI *GET_LONG_PATH_NAME_W_PROC)(LPCWSTR,LPWSTR,DWORD);
#ifndef LINUX
utf_t long_path[MAX_LONG_PATH + 1];
utf_t full_path[MAX_LONG_PATH + 1];
#endif

/*
* Paths may be either in short or in long format,
* either ANSI or Unicode, either full or relative.
* This is not safe to assume something concrete.
*/
#ifdef LINUXMODE
void search_for_paths(int argc, char **argv)
{
    int xarg;
    const char *parg;

    for (xarg=1; xarg<argc; xarg++) {
        parg = argv[xarg];
        if (*parg && (*parg != '-')) {
#ifndef LINUX
            utf16_t *wpath;

            wpath = (utf_t*)malloc((strlen(parg)+1)*sizeof(utf_t));
            if (wpath) {
                ntfs_toutf16(wpath,parg,strlen(parg)+1);
                insert_path(wpath);
                free(wpath);
            }
#else
            insert_path(parg);
#endif
        }
    }
}

#else /* LINUXMODE */

void search_for_paths(void)
{
    utf_t *cmdline, *cmdline_copy;
    utf_t **xargv;
    int i, j, xargc;
    int length;
    DWORD result;
    HMODULE hKernel32Dll = NULL;
    GET_LONG_PATH_NAME_W_PROC pGetLongPathNameW = NULL;
    
    cmdline = GetCommandLineW();
    
    /*
    * CommandLineToArgvW has one documented bug -
    * it doesn't accept backslash + quotation mark sequence.
    * So, we're adding a dot after each trailing backslash
    * in quoted paths. This dot will be removed by GetFullPathName.
    * http://msdn.microsoft.com/en-us/library/bb776391%28VS.85%29.aspx
    */
    cmdline_copy = malloc(utflen(cmdline) * sizeof(utf_t) * 2 + sizeof(utf_t));
    if(cmdline_copy == NULL){
        display_error("search_for_paths: not enough memory!");
        return;
    }
    length = utflen(cmdline);
    for(i = 0, j = 0; i < length; i++){
        cmdline_copy[j] = cmdline[i];
        j ++;
        if(cmdline[i] == '\\' && i != (length - 1)){
            if(cmdline[i + 1] == '"'){
                /* trailing backslash in a quoted path detected */
                cmdline_copy[j] = '.';
                j ++;
            }
        }
    }
    cmdline_copy[j] = 0;
    //printf("command line copy: %" LSTR "\n",cmdline_copy);
    
    xargv = CommandLineToArgvW(cmdline_copy,&xargc);
    free(cmdline_copy);
    if(xargv == NULL){
        display_last_error("CommandLineToArgvW failed!");
        return;
    }
    
    hKernel32Dll = LoadLibrary("kernel32.dll");
    if(hKernel32Dll == NULL){
        WgxDbgPrintLastError("search_for_paths: cannot load kernel32.dll");
    } else {
        pGetLongPathNameW = (GET_LONG_PATH_NAME_W_PROC)GetProcAddress(hKernel32Dll,"GetLongPathNameW");
        if(pGetLongPathNameW == NULL)
            WgxDbgPrintLastError("search_for_paths: GetLongPathNameW not found in kernel32.dll");
    }
    
    for(i = 1; i < xargc; i++){
        if(xargv[i][0] == 0) continue;   /* skip empty strings */
        if(xargv[i][0] == '-') continue; /* skip options */
        if(utflen(xargv[i]) == 2){       /* skip individual volume letters */
            if(xargv[i][1] == ':')
                continue;
        }
        //printf("path detected: arg[%i] = %" LSTR "\n",i,xargv[i]);
        /* convert path to the long file name format (on w2k+) */
        if(pGetLongPathNameW){
            result = pGetLongPathNameW(xargv[i],long_path,MAX_LONG_PATH + 1);
            if(result == 0){
                WgxDbgPrintLastError("search_for_paths: GetLongPathNameW failed");
                goto use_short_path;
            } else if(result > MAX_LONG_PATH + 1){
                printf("search_for_paths: long path of \'%" LSTR "\' is too long!",xargv[i]);
                goto use_short_path;
            }
        } else {
use_short_path:
            utf16ncpy(long_path,xargv[i],MAX_LONG_PATH);
        }
        long_path[MAX_LONG_PATH] = 0;
        /* convert path to the full path */
        result = GetFullPathNameW(long_path,MAX_LONG_PATH + 1,full_path,NULL);
        if(result == 0){
            WgxDbgPrintLastError("search_for_paths: GetFullPathNameW failed");
            utfcpy(full_path,long_path);
        } else if(result > MAX_LONG_PATH + 1){
            printf("search_for_paths: full path of \'%" LSTR "\' is too long!",long_path);
            utfcpy(full_path,long_path);
        }
        full_path[MAX_LONG_PATH] = 0;
        /* add path to the list */
        insert_path(full_path);
    }
    
    GlobalFree(xargv);
}

#endif /* LINUX */

int insert_path(const utf_t *path)
{
    object_path *new_item, *last_item;
    
    if(path == NULL)
        return (-1);
    
    new_item = malloc(sizeof(object_path));
    if(new_item == NULL){
        display_error("insert_path: not enough memory!");
        return (-1);
    }
    
    if(paths == NULL){
        paths = new_item;
        new_item->prev = new_item->next = new_item;
        goto done;
    }
    
    last_item = paths->prev;
    last_item->next = new_item;
    new_item->prev = last_item;
    new_item->next = paths;
    paths->prev = new_item;
    
done:
    new_item->processed = 0;
    utfncpy(new_item->path,path,MAX_LONG_PATH);
    new_item->path[MAX_LONG_PATH] = 0;
    return 0;
}

void destroy_paths(void)
{
    object_path *item, *next, *head;
    
    if(paths == NULL)
        return;
    
    item = head = paths;
    do {
        next = item->next;
        free(item);
        item = next;
    } while (next != head);
    
    paths = NULL;
}
