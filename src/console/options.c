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
* UltraDefrag console interface - command line parsing code.
*/

#include "udefrag.h"
#include "../share/getopt.h"

/* forward declarations */
static int ReadShellExOptions(void);
static void search_for_paths(void);

int printf_stub(const char *format,...)
{
    return 0;
}

void show_help(void)
{
    printf(
        "===============================================================================\n"
        VERSIONINTITLE " - a powerful disk defragmentation tool for Windows NT\n"
        "Copyright (c) UltraDefrag Development Team, 2007-2013.\n"
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
        "Usage: udefrag [command] [options] [driveletter:] [path(s)]\n"
        "\n"
        "  The default action is to display this help screen.\n"
        "\n"
        "Commands:\n"
        "  -a,  --analyze                      analyze specified objects\n"
        "  -o,  --optimize                     perform full optimization\n"
        "  -q,  --quick-optimize               perform quick optimization\n"
        "       --optimize-mft                 optimize master file tables only\n"
        "  -l,  --list-available-volumes       list all fixed disks available\n"
        "                                      for defragmentation\n"
        "  -la, --list-available-volumes=all   list all available disks,\n"
        "                                      including removable\n"
        "  -h,  --help                         show this help screen\n"
        "  -?                                  show this help screen\n"
        "\n"
        "  The commands are exclusive and can't be combined with each other.\n"
        "  If none is specified the program will defragment selected objects.\n"
        "\n"
        "Options:\n"
        "  -r,  --repeat                       repeat the disk processing multiple\n"
        "                                      times whenever it makes sense\n"
        "  -b,  --use-system-color-scheme      disable colorization of output\n"
        "  -p,  --suppress-progress-indicator  hide progress indicator and cluster map\n"
        "  -v,  --show-volume-information      show disk information after the job\n"
        "  -m,  --show-cluster-map             show cluster map\n"
        "       --map-border-color=color       set cluster map border color;\n"
        "                                      available colors: black, white, red,\n"
        "                                      green, blue, yellow, magenta, cyan,\n"
        "                                      darkred, darkgreen, darkblue, darkyellow,\n"
        "                                      darkmagenta, darkcyan, gray;\n"
        "                                      yellow is used by default\n"
        "       --map-symbol=x                 set character do draw cluster map with;\n"
        "                                      type it directly or use its hexadecimal\n"
        "                                      number (in range 0x1 ... 0xFF);\n"
        "                                      \'%%\' symbol is used by default\n"
        "       --map-rows=n                   cluster map height (10 by default)\n"
        "       --map-symbols-per-line=n       cluster map width (68 by default)\n"
        "       --use-entire-window            expand map to use entire window\n"
        "       --wait                         wait for completion of other\n"
        "                                      instances before the job startup\n"
        "                                      (useful for scheduled tasks)\n"
        "       --shellex                      list selected objects and display\n"
        "                                      a prompt to hit any key after the job\n"
        "                                      completion (intended to handle context\n"
        "                                      menu entries in Windows Explorer)\n"
        "\n"
        "Drive letters:\n"
        "  Space separated drive letters or one of the following switches:\n"
        "\n"
        "  --all                               process all available drives\n"
        "  --all-fixed                         process all non-removable drives\n"
        "\n"
        "Paths:\n"
        "  Space separated paths which need to be defragmented.\n"
        "  Both absolute and relative paths are supported, as well as wildcards.\n"
        "  Paths including spaces must be enclosed by double quotes (\").\n"
        "\n"
        "Accepted environment variables:\n"
        "\n"
        "  UD_IN_FILTER                        semicolon separated paths which need\n"
        "                                      to be defragmented; the empty list means\n"
        "                                      that everything needs to be defragmented\n"
        "\n"
        "  UD_EX_FILTER                        semicolon separated paths which need\n"
        "                                      to be skipped, i.e. left untouched\n"
        "\n"
        "  UD_FRAGMENT_SIZE_THRESHOLD          eliminate only fragments smaller than\n"
        "                                      specified; accepted size suffixes:\n"
        "                                      KB, MB, GB, TB, PB, EB\n"
        "\n"
        "  UD_FILE_SIZE_THRESHOLD              exclude all files larger than specified;\n"
        "                                      accepted size suffixes:\n"
        "                                      KB, MB, GB, TB, PB, EB\n"
        "\n"
        "  UD_OPTIMIZER_FILE_SIZE_THRESHOLD    for optimization only, exclude all files\n"
        "                                      larger than specified; accepted size\n"
        "                                      suffixes: KB, MB, GB, TB, PB, EB;\n"
        "                                      the default value is 20MB\n"
        "\n"
        "  UD_FRAGMENTS_THRESHOLD              exclude files having less fragments\n"
        "                                      than specified\n"
        "\n"
        "  UD_SORTING                          set sorting criteria for the disk\n"
        "                                      optimization; PATH is used by default,\n"
        "                                      it forces to sort files by their paths;\n"
        "                                      four more options are available:\n"
        "                                      SIZE (sort by size), C_TIME (sort by\n"
        "                                      creation time), M_TIME (sort by last\n"
        "                                      modification time) and A_TIME (sort by\n"
        "                                      last access time)\n"
        "\n"
        "  UD_SORTING_ORDER                    set sorting order for the disk\n"
        "                                      optimization; ASC (ascending) is used\n"
        "                                      by default, DESC (descending) forces\n"
        "                                      to sort files in reverse order\n"
        "\n"
        "  UD_FRAGMENTATION_THRESHOLD          cancel all tasks except of the MFT\n"
        "                                      optimization when fragmentation level\n"
        "                                      is below than specified\n"
        "\n"
        "  UD_TIME_LIMIT                       terminate the job automatically when\n"
        "                                      the specified time interval elapses;\n"
        "                                      the following time format is accepted:\n"
        "                                      Ay Bd Ch Dm Es; here A,B,C,D,E represent\n"
        "                                      integer numbers while y,d,h,m,s represent\n"
        "                                      years, days, hours, minutes and seconds\n"
        "\n"
        "  UD_REFRESH_INTERVAL                 set the progress refresh interval,\n"
        "                                      in milliseconds; the default value is 100\n"
        "\n"
        "  UD_DISABLE_REPORTS                  set it to 1 (one) to disable generation\n"
        "                                      of the file fragmentation reports\n"
        "\n"
        "  UD_DBGPRINT_LEVEL                   set amount of debugging output;\n"
        "                                      NORMAL is used by default, DETAILED\n"
        "                                      can be used to collect information for\n"
        "                                      a bug report, PARANOID turns on really\n"
        "                                      huge amount of debugging information\n"
        "\n"
        "  UD_LOG_FILE_PATH                    set log file path (including file name)\n"
        "                                      to redirect debugging output to a file\n"
        "\n"
        "  UD_DRY_RUN                          set it to 1 (one) to avoid physical\n"
        "                                      movements of files, i.e. to simulate\n"
        "                                      the disk processing; this allows to\n"
        "                                      check out algorithms quickly\n"
        "\n"
        "Note:\n"
        "  All the environment variables are ignored when the --shellex switch is\n"
        "  on the command line. Instead of taking environment variables into account\n"
        "  the program interpretes the %%UD_INSTALL_DIR%%\\options.lua file.\n"
        "\n"
        "Samples:\n"
        "\n"
        "  set UD_LOG_FILE_PATH=C:\\Windows\\Temp\\udefrag.log\n"
        "  set UD_TIME_LIMIT=6h 30m\n"
        "  set UD_EX_FILTER=*temp*;*tmp*\n"
        "  set UD_FRAGMENT_SIZE_THRESHOLD=20MB\n"
        "  udefrag c: d: e: \"h:\\my documents\\movies\\*\"\n"
        "\n"
        "More information and samples can be found in UltraDefrag Handbook.\n"
        "If you have not received it along with this program go to:\n"
        "\n"
        "http://ultradefrag.sourceforge.net/handbook/\n"
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
    { "quick-optimize",              no_argument,       0, 'q' },
    { "optimize-mft",                no_argument,       0,  0  },
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
    { "show-cluster-map",            no_argument,       0, 'm' },
    
    /*
    * Colors and decoration.
    */
    { "use-system-color-scheme",     no_argument,       0, 'b' },
    { "map-border-color",            required_argument, 0,  0  },
    { "map-symbol",                  required_argument, 0,  0  },
    { "map-rows",                    required_argument, 0,  0  },
    { "map-symbols-per-line",        required_argument, 0,  0  },
    { "use-entire-window",           no_argument,       0,  0  },
    
    /*
    * Help.
    */
    { "help",                        no_argument,       0, 'h' },
    
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
    
    { 0,                             0,                 0,  0  }
};

char short_options_[] = "aoql::rpvmbh?iesd";

/* new code based on GNU getopt() function */
int parse_cmdline(int argc, char **argv)
{
    int c;
    int option_index = 0;
    const char *long_option_name;
    int dark_color_flag = 0;
    int map_symbol_number = 0;
    int rows = 0, symbols_per_line = 0;
    int letter_index;
    char ch;
    int length;
    int result;
    
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
        case 'i':
        case 'e':
        case 's':
        case 'd':
            obsolete_option = 1;
            break;
        case '?': /* invalid option or -? option */
            if(optopt == '?') h_flag = 1;
            break;
        default:
            fprintf(stderr,"?? getopt returned character code 0%o ??\n", c);
        }
    }
    
    dbg_print("command line: %ls\n",GetCommandLineW());
    
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
                        fprintf(stderr,"Too many letters specified on the command line.\n");
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
    
    /* scan for paths of objects to be processed */
    search_for_paths();

    /* --all-fixed flag has more precedence */
    if(all_fixed_flag) all_flag = 0;
    
    if(!l_flag && !all_flag && !all_fixed_flag && !letters[0] && !paths) h_flag = 1;
    
    /* --quick-optimize flag has more precedence */
    if(quick_optimize_flag) o_flag = 0;
    
    /* calculate map dimensions if --use-entire-window flag is set */
    if(use_entire_window) CalculateClusterMapDimensions();

    if(shellex_flag){
        result = ReadShellExOptions();
        (void)udefrag_set_log_file_path();
        if(result < 0)
            return result;
    }
    
    return 0;
}

/**
 * @brief Cleans up the environment
 * by removing all the variables
 * controlling the program behavior.
 */
static void CleanupEnvironment(void)
{
    (void)SetEnvironmentVariable("UD_IN_FILTER",NULL);
    (void)SetEnvironmentVariable("UD_EX_FILTER",NULL);
    (void)SetEnvironmentVariable("UD_FRAGMENT_SIZE_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_FILE_SIZE_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_OPTIMIZER_FILE_SIZE_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_FRAGMENTS_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_FRAGMENTATION_THRESHOLD",NULL);
    (void)SetEnvironmentVariable("UD_REFRESH_INTERVAL",NULL);
    (void)SetEnvironmentVariable("UD_DISABLE_REPORTS",NULL);
    (void)SetEnvironmentVariable("UD_DBGPRINT_LEVEL",NULL);
    (void)SetEnvironmentVariable("UD_LOG_FILE_PATH",NULL);
    (void)SetEnvironmentVariable("UD_TIME_LIMIT",NULL);
    (void)SetEnvironmentVariable("UD_DRY_RUN",NULL);
    (void)SetEnvironmentVariable("UD_SORTING",NULL);
    (void)SetEnvironmentVariable("UD_SORTING_ORDER",NULL);
}

/**
 * @brief Reads options specific for
 * the Explorer's context menu handler.
 * @return Zero for success, negative
 * value otherwise.
 */
static int ReadShellExOptions(void)
{
    char instdir[MAX_PATH + 1];
    char path[MAX_PATH + 1];
    lua_State *L;
    int status;
    const char *msg;
    
    /*
    * Explorer's context menu handler should be
    * configurable through the options.lua file only.
    */
    CleanupEnvironment();
    
    /* interprete options.lua file */
    if(!GetEnvironmentVariable("UD_INSTALL_DIR",instdir,MAX_PATH + 1)){
        display_last_error("Cannot query UD_INSTALL_DIR environment variable!");
        return (-1);
    } else {
        _snprintf(path,MAX_PATH + 1,"%s\\options.lua",instdir);
        path[MAX_PATH] = 0;
    }
    
    L = lua_open();
    if(L == NULL){
        display_error("Cannot initialize Lua library!\n\n");
        return (-1);
    }

    /* stop collector during initialization */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, 0);

    lua_pushnumber(L, 1);
    lua_setglobal(L, "shellex_flag");
    status = luaL_dofile(L,path);
    if(status != 0){
        settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        fprintf(stderr,"Cannot interprete %s\n",path);
        if(!lua_isnil(L, -1)){
            msg = lua_tostring(L, -1);
            if(msg == NULL) msg = "(error object is not a string)";
            fprintf(stderr,"%s\n\n",msg);
            lua_pop(L, 1);
        } else {
            fprintf(stderr,"\n");
        }
        settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        lua_close(L);
        return (-1);
    }
    
    lua_close(L);
    return 0;
}

typedef DWORD (WINAPI *GET_LONG_PATH_NAME_W_PROC)(LPCWSTR,LPWSTR,DWORD);
wchar_t long_path[MAX_LONG_PATH + 1];
wchar_t full_path[MAX_LONG_PATH + 1];

/*
* Paths may be either in short or in long format,
* either ANSI or Unicode, either full or relative.
* This is not safe to assume something concrete.
*/
static void search_for_paths(void)
{
    wchar_t *cmdline, *cmdline_copy;
    wchar_t **xargv;
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
    cmdline_copy = malloc(wcslen(cmdline) * sizeof(wchar_t) * 2 + sizeof(wchar_t));
    if(cmdline_copy == NULL){
        display_error("search_for_paths: not enough memory!");
        return;
    }
    length = wcslen(cmdline);
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
    //printf("command line copy: %ls\n",cmdline_copy);
    
    xargv = CommandLineToArgvW(cmdline_copy,&xargc);
    free(cmdline_copy);
    if(xargv == NULL){
        display_last_error("CommandLineToArgvW failed!");
        return;
    }
    
    hKernel32Dll = LoadLibrary("kernel32.dll");
    if(hKernel32Dll == NULL){
        letrace("cannot load kernel32.dll");
    } else {
        pGetLongPathNameW = (GET_LONG_PATH_NAME_W_PROC)GetProcAddress(hKernel32Dll,"GetLongPathNameW");
        if(pGetLongPathNameW == NULL)
            letrace("GetLongPathNameW not found in kernel32.dll");
    }
    
    for(i = 1; i < xargc; i++){
        if(xargv[i][0] == 0) continue;   /* skip empty strings */
        if(xargv[i][0] == '-') continue; /* skip options */
        if(wcslen(xargv[i]) == 2){       /* skip individual volume letters */
            if(xargv[i][1] == ':')
                continue;
        }
        //printf("path detected: arg[%i] = %ls\n",i,xargv[i]);
        /* convert path to the long file name format (on w2k+) */
        if(pGetLongPathNameW){
            result = pGetLongPathNameW(xargv[i],long_path,MAX_LONG_PATH + 1);
            if(result == 0){
                letrace("GetLongPathNameW failed");
                goto use_short_path;
            } else if(result > MAX_LONG_PATH + 1){
                fprintf(stderr,"search_for_paths: long path of \'%ls\' is too long!",xargv[i]);
                goto use_short_path;
            }
        } else {
use_short_path:
            wcsncpy(long_path,xargv[i],MAX_LONG_PATH);
        }
        long_path[MAX_LONG_PATH] = 0;
        /* convert path to the full path */
        result = GetFullPathNameW(long_path,MAX_LONG_PATH + 1,full_path,NULL);
        if(result == 0){
            letrace("GetFullPathNameW failed");
            wcscpy(full_path,long_path);
        } else if(result > MAX_LONG_PATH + 1){
            fprintf(stderr,"search_for_paths: full path of \'%ls\' is too long!",long_path);
            wcscpy(full_path,long_path);
        }
        full_path[MAX_LONG_PATH] = 0;
        /* add path to the list */
        insert_path(full_path);
    }
    
    GlobalFree(xargv);
}

int insert_path(wchar_t *path)
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
    wcsncpy(new_item->path,path,MAX_LONG_PATH);
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
