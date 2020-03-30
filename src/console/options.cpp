//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2015 Dmitri Arkhangelski (dmitriar@gmail.com).
//  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//////////////////////////////////////////////////////////////////////////

/**
 * @file options.cpp
 * @brief Configurable options.
 * @addtogroup Options
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

extern "C" {
#define lua_c
#include "../lua5.1/lua.h"
#include "../lua5.1/lauxlib.h"
#include "../lua5.1/lualib.h"
}

#if !defined(__GNUC__)
#define __STDC__ 1
#endif
#include "getopt.h"

bool set_shellex_options(void);

// =======================================================================
//                         Command line parser
// =======================================================================

static struct option long_options[] = {
    /*
    * Disk defragmentation options.
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
    * Miscellaneous options.
    */
    { "wait",                        no_argument,       0,  0  },
    { "shellex",                     no_argument,       0,  0  },
    { "folder",                      no_argument,       0,  0  },
    { "folder-itself",               no_argument,       0,  0  },

    { 0,                             0,                 0,  0  }
};

char short_options[] = "aoql::rpvmbh?";

/**
 * @brief Parses the command line.
 */
bool parse_cmdline(int argc, char **argv)
{
    /*
    * wxCmdLineParser doesn't accept
    * long options in wxCMD_LINE_OPTION
    * without corresponding short options,
    * so let's use GNU getopt.
    */
    if(argc < 2){
        g_help = true;
        return true;
    }

    const char *long_option_name;
    while(1){
        int option_index = 0;
        int c = getopt_long(argc,argv,short_options,
            long_options,&option_index);
        if(c == -1) break;
        switch(c){
        case 0:
            //printf("option %s", long_options[option_index].name);
            //if(optarg) printf(" with arg %s", optarg);
            //printf("\n");
            long_option_name = long_options[option_index].name;
            if(!strcmp(long_option_name,"defragment")){
                /* do nothing */
            } else if(!strcmp(long_option_name,"optimize-mft")){
                g_optimize_mft = true;
            } else if(!strcmp(long_option_name,"map-border-color")){
                if(!optarg) break;
                if(!strcmp(optarg,"black")){
                    g_map_border_color = 0x0; break;
                }
                if(!strcmp(optarg,"white")){
                    g_map_border_color = FOREGROUND_RED | FOREGROUND_GREEN | \
                        FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
                }
                if(!strcmp(optarg,"gray")){
                    g_map_border_color = FOREGROUND_RED | FOREGROUND_GREEN | \
                        FOREGROUND_BLUE; break;
                }

                bool dark_color_flag = (strstr(optarg,"dark") != NULL);

                if(strstr(optarg,"red")){
                    g_map_border_color = FOREGROUND_RED;
                } else if(strstr(optarg,"green")){
                    g_map_border_color = FOREGROUND_GREEN;
                } else if(strstr(optarg,"blue")){
                    g_map_border_color = FOREGROUND_BLUE;
                } else if(strstr(optarg,"yellow")){
                    g_map_border_color = FOREGROUND_RED | FOREGROUND_GREEN;
                } else if(strstr(optarg,"magenta")){
                    g_map_border_color = FOREGROUND_RED | FOREGROUND_BLUE;
                } else if(strstr(optarg,"cyan")){
                    g_map_border_color = FOREGROUND_GREEN | FOREGROUND_BLUE;
                }

                if(!dark_color_flag) g_map_border_color |= FOREGROUND_INTENSITY;
            } else if(!strcmp(long_option_name,"map-symbol")){
                if(!optarg) break;
                if(strstr(optarg,"0x") == optarg){
                    /* decode hexadecimal number */
                    int map_symbol_number = 0;
                    (void)sscanf(optarg,"%x",&map_symbol_number);
                    if(map_symbol_number > 0 && map_symbol_number < 256)
                        g_map_symbol = (char)map_symbol_number;
                } else {
                    if(optarg[0]) g_map_symbol = optarg[0];
                }
            } else if(!strcmp(long_option_name,"wait")){
                g_wait = true;
            } else if(!strcmp(long_option_name,"shellex")){
                g_shellex = true;
            } else if(!strcmp(long_option_name,"folder")){
                g_folder = true;
            } else if(!strcmp(long_option_name,"folder_itself")){
                g_folder_itself = true;
            } else if(!strcmp(long_option_name,"use-entire-window")){
                g_use_entire_window = true;
            } else if(!strcmp(long_option_name,"map-rows")){
                if(!optarg) break;
                int rows = atoi(optarg);
                if(rows > 0) g_map_rows = rows;
            } else if(!strcmp(long_option_name,"map-symbols-per-line")){
                if(!optarg) break;
                int symbols_per_line = atoi(optarg);
                if(symbols_per_line > 0) g_map_symbols_per_line = symbols_per_line;
            } else if(!strcmp(long_option_name,"all")){
                g_all = true;
            } else if(!strcmp(long_option_name,"all-fixed")){
                g_all_fixed = true;
            }
            break;
        case 'a':
            g_analyze = true;
            break;
        case 'o':
            g_optimize = true;
            break;
        case 'q':
            g_quick_optimization = true;
            break;
        case 'l':
            g_list_volumes = true;
            if(optarg){
                if(!strcmp(optarg,"a")) g_list_all = true;
                if(!strcmp(optarg,"all")) g_list_all = true;
            }
            break;
        case 'r':
            g_repeat = true;
            break;
        case 'p':
            g_no_progress = true;
            break;
        case 'v':
            g_show_vol_info = true;
            break;
        case 'm':
            g_show_map = true;
            break;
        case 'b':
            g_use_default_colors = true;
            break;
        case 'h':
            g_help = true;
            break;
        case '?': /* invalid option or -? option */
            if(optopt == '?') g_help = true;
            break;
        default:
            fprintf(stderr,"?? getopt returned character code 0%o ??\n", c);
        }
    }

    if(g_help) return true;

    /* --all-fixed flag has more precedence */
    if(g_all_fixed) g_all = false;

    /* --quick-optimize flag has more precedence */
    if(g_quick_optimization) g_optimize = false;

    /* -p flag disables cluster map as well */
    if(g_no_progress) g_show_map = false;

    /* search for drive letters and paths */
    wxString cmdline(GetCommandLine());
    cmdline.Replace(wxT("\\\""),wxT("\\\\\""));
    wxArrayString opts = wxCmdLineParser::ConvertStringToArgs(cmdline);
    g_volumes = new wxArrayString(); g_paths = new wxArrayString();
    for(int i = 1; i < (int)opts.GetCount(); i++){
        opts[i].Replace(wxT("\\\\"),wxT("\\"));
        //printf("%ls\n",ws(opts[i]));
        if(opts[i].IsEmpty()) continue;
        if((char)opts[i][0] == '-') continue;
        if(opts[i].Len() == 2 && (char)opts[i][1] == ':'){
            g_volumes->Add(opts[i]);
        } else {
            wxFileName path(opts[i]);
            // normalize path but keep wildcards untouched
            path.Normalize(wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS | \
                wxPATH_NORM_ABSOLUTE | wxPATH_NORM_TILDE);
            g_paths->Add(path.GetFullPath().MakeLower());
        }
    }

    /*for(int i = 0; i < g_volumes->GetCount(); i++)
        printf("%ls\n",ws((*g_volumes)[i]));
    for(int i = 0; i < g_paths->GetCount(); i++)
        printf("%ls\n",ws((*g_paths)[i]));
    */

    if(!g_list_volumes && !g_all && !g_all_fixed){
        if(g_volumes->IsEmpty() && g_paths->IsEmpty()){
            g_help = true; return true;
        }
    }

    if(g_use_entire_window){
        udefrag_progress_info pi;
        memset(&pi,0,sizeof(udefrag_progress_info));

        char *results = udefrag_get_results(&pi);
        if(results){
            g_extra_lines = 1;
            for(int i = 0; results[i]; i++)
                if(results[i] == '\n') g_extra_lines ++;
            udefrag_release_results(results);
        }

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if(GetConsoleScreenBufferInfo(h,&csbi)){
            g_map_symbols_per_line = csbi.srWindow.Right - csbi.srWindow.Left - 2;
            g_map_rows = csbi.srWindow.Bottom - csbi.srWindow.Top - 10;
            if(g_show_vol_info) g_map_rows -= g_extra_lines;
            /* scroll buffer one line up */
            if(csbi.srWindow.Top > 0){
                SMALL_RECT sr;
                sr.Top = sr.Bottom = -1;
                sr.Left = sr.Right = 0;
                (void)SetConsoleWindowInfo(h,false,&sr);
            }
        } else {
            letrace("cannot get console window size");
        }
    }

    if(g_shellex){
        bool result = set_shellex_options();

        // reset debug log
        wxString logpath;
        if(wxGetEnv(wxT("UD_LOG_FILE_PATH"),&logpath)){
            wxFileName file(logpath); file.Normalize();
            wxSetEnv(wxT("UD_LOG_FILE_PATH"),file.GetFullPath());
        }
        udefrag_set_log_file_path();

        if(!result) return result;
    }

    return true;
}

/**
 * @brief Sets options specific for
 * the Explorer's context menu handler.
 */
bool set_shellex_options(void)
{
    /*
    * Explorer's context menu handler should be
    * configurable through the options.lua file only.
    */
    wxUnsetEnv(wxT("UD_IN_FILTER"));
    wxUnsetEnv(wxT("UD_EX_FILTER"));
    wxUnsetEnv(wxT("UD_FRAGMENT_SIZE_THRESHOLD"));
    wxUnsetEnv(wxT("UD_FILE_SIZE_THRESHOLD"));
    wxUnsetEnv(wxT("UD_OPTIMIZER_FILE_SIZE_THRESHOLD"));
    wxUnsetEnv(wxT("UD_FRAGMENTS_THRESHOLD"));
    wxUnsetEnv(wxT("UD_FRAGMENTATION_THRESHOLD"));
    wxUnsetEnv(wxT("UD_REFRESH_INTERVAL"));
    wxUnsetEnv(wxT("UD_DISABLE_REPORTS"));
    wxUnsetEnv(wxT("UD_DBGPRINT_LEVEL"));
    wxUnsetEnv(wxT("UD_LOG_FILE_PATH"));
    wxUnsetEnv(wxT("UD_TIME_LIMIT"));
    wxUnsetEnv(wxT("UD_DRY_RUN"));
    wxUnsetEnv(wxT("UD_SORTING"));
    wxUnsetEnv(wxT("UD_SORTING_ORDER"));

    /* interprete options.lua file */
    wxFileName path(wxT("%UD_INSTALL_DIR%\\options.lua"));
    path.Normalize();
    if(!path.FileExists()){
        etrace("%ls file not found",
            ws(path.GetFullPath()));
        return true;
    }

    lua_State *L = lua_open();
    if(!L){
        etrace("Lua initialization failed");
        fprintf(stderr,"Lua initialization failed!\n");
        return false;
    }

    /* stop collector during initialization */
    lua_gc(L,LUA_GCSTOP,0);
    luaL_openlibs(L);
    lua_gc(L,LUA_GCRESTART,0);

    lua_pushnumber(L,1);
    lua_setglobal(L,"shellex_flag");
    int status = luaL_dofile(L,ansi(path.GetShortPath()));
    if(status != 0){
        etrace("cannot interprete %ls",
            ws(path.GetFullPath()));
        fprintf(stderr,"Cannot interprete %ls!\n",
            ws(path.GetFullPath()));
        if(!lua_isnil(L,-1)){
            const char *msg = lua_tostring(L,-1);
            if(!msg) msg = "(error object is not a string)";
            etrace("%hs",msg);
            fprintf(stderr,"%s\n",msg);
            lua_pop(L, 1);
        }
        lua_close(L);
        return false;
    }

    lua_close(L);
    return true;
}

/** @} */
