//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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

#include "prec.h"
#include "main.h"

extern "C" {
#define lua_c
#include "../lua5.1/lua.h"
#include "../lua5.1/lauxlib.h"
#include "../lua5.1/lualib.h"
}

struct _color {
    const char *name;
    short value;
};

bool set_shellex_options(void);

// =======================================================================
//                         Command line parser
// =======================================================================

static const wxCmdLineEntryDesc opts[] = {
    // action switches
    {wxCMD_LINE_SWITCH, "a",  "analyze"},
    {wxCMD_LINE_SWITCH, "o",  "optimize"},
    {wxCMD_LINE_SWITCH, "q",  "quick-optimization"},
    {wxCMD_LINE_SWITCH, NULL, "optimize-mft"},

    // drives selection switches
    {wxCMD_LINE_SWITCH, NULL, "all"},
    {wxCMD_LINE_SWITCH, NULL, "all-fixed"},

    // drives listing switches
    {wxCMD_LINE_SWITCH, "l",  "list-available-volumes"},
    {wxCMD_LINE_SWITCH, "la"},

    // progress indication switches
    {wxCMD_LINE_SWITCH, "p", "suppress-progress-indicator"},
    {wxCMD_LINE_SWITCH, "v", "show-volume-information"},
    {wxCMD_LINE_SWITCH, "m", "show-cluster-map"},

    // colors and decoration
    {wxCMD_LINE_SWITCH, "b",  "use-system-color-scheme"},
    {
        wxCMD_LINE_OPTION, NULL, "map-border-color", NULL,
        wxCMD_LINE_VAL_STRING, wxCMD_LINE_NEEDS_SEPARATOR
    },
    {
        wxCMD_LINE_OPTION, NULL, "map-symbol", NULL,
        wxCMD_LINE_VAL_STRING, wxCMD_LINE_NEEDS_SEPARATOR
    },
    {
        wxCMD_LINE_OPTION, NULL, "map-rows", NULL,
        wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_NEEDS_SEPARATOR
    },
    {
        wxCMD_LINE_OPTION, NULL, "map-symbols-per-line", NULL,
        wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_NEEDS_SEPARATOR
    },
    {wxCMD_LINE_SWITCH, NULL, "use-entire-window"},

    // miscellaneous switches
    {wxCMD_LINE_SWITCH, NULL, "wait"},
    {wxCMD_LINE_SWITCH, NULL, "shellex"},
    {wxCMD_LINE_SWITCH, NULL, "folder"},
    {wxCMD_LINE_SWITCH, NULL, "folder-itself"},

    // help switches
    {wxCMD_LINE_SWITCH, "h", "help"},
    {wxCMD_LINE_SWITCH, "?"},

    // obsolete switches
    {wxCMD_LINE_SWITCH, NULL, "defragment"},
    {wxCMD_LINE_SWITCH, NULL, "quick-optimize"},
    {wxCMD_LINE_SWITCH, "r",  "repeat"},

    // drive letters and paths
    {
        wxCMD_LINE_PARAM,
        NULL, NULL, NULL,
        wxCMD_LINE_VAL_STRING,
        wxCMD_LINE_PARAM_OPTIONAL | \
        wxCMD_LINE_PARAM_MULTIPLE
    },

    // the end of the list
    {wxCMD_LINE_NONE}
};

static const struct _color colors[] = {
    {"black",       0                                                                         },
    {"darkred",     FOREGROUND_RED                                                            },
    {"darkgreen",   FOREGROUND_GREEN                                                          },
    {"darkblue",    FOREGROUND_BLUE                                                           },
    {"darkyellow",  FOREGROUND_RED | FOREGROUND_GREEN                                         },
    {"darkmagenta", FOREGROUND_RED | FOREGROUND_BLUE                                          },
    {"darkcyan",    FOREGROUND_GREEN | FOREGROUND_BLUE                                        },
    {"gray",        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE                       },
    {"red",         FOREGROUND_RED | FOREGROUND_INTENSITY                                     },
    {"green",       FOREGROUND_GREEN | FOREGROUND_INTENSITY                                   },
    {"blue",        FOREGROUND_BLUE | FOREGROUND_INTENSITY                                    },
    {"yellow",      FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY                  },
    {"magenta",     FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY                   },
    {"cyan",        FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY                 },
    {"white",       FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY}
};

/**
 * @brief Parses the command line.
 */
bool parse_cmdline(int argc, char **argv)
{
    if(argc < 2){
        g_help = true;
        return true;
    }

    /*
    * If --list-available-volumes is defined as a switch
    * it cannot take a value, if it's defined as an option
    * it requires a value, so we need a little trick here.
    */
    wxString cmdline(GetCommandLine());
    cmdline.Replace(wxT("--list-available-volumes=all"),wxT("-la"));

    wxCmdLineParser parser(opts,cmdline);
    if(parser.Parse(false) > 0) return false;

    g_analyze = parser.Found(wxT("a"));
    g_optimize = parser.Found(wxT("o"));
    g_quick_optimization = parser.Found(wxT("q"));
    g_optimize_mft = parser.Found(wxT("optimize-mft"));

    // support obsolete --quick-optimize option
    if(parser.Found(wxT("quick-optimize")))
        g_quick_optimization = true;

    g_all = parser.Found(wxT("all"));
    g_all_fixed = parser.Found(wxT("all-fixed"));

    g_list_volumes = parser.Found(wxT("l"));
    g_list_all = parser.Found(wxT("la"));
    if(g_list_all) g_list_volumes = true;

    g_no_progress = parser.Found(wxT("p"));
    g_show_vol_info = parser.Found(wxT("v"));
    g_show_map = parser.Found(wxT("m"));

    g_use_default_colors = parser.Found(wxT("b"));

    wxString color;
    if(parser.Found(wxT("map-border-color"),&color)){
        for(int i = 0; i < sizeof(colors) / sizeof(colors[0]); i++){
            if(color == colors[i].name){
                g_map_border_color = colors[i].value;
                break;
            }
        }
    }

    wxString map_symbol;
    if(parser.Found(wxT("map-symbol"),&map_symbol)){
        long value = 0;
        if(map_symbol.Find(wxT("0x")) == 0){
            if(!map_symbol.ToLong(&value,16)) value = 0;
        } else {
            value = (long)map_symbol[0].GetValue();
        }
        if(value > 0 && value < 256)
            g_map_symbol = (char)value;
    }

    long rows = 0, columns = 0;
    if(parser.Found(wxT("map-rows"),&rows)){
        if(rows > 0) g_map_rows = rows;
    }
    if(parser.Found(wxT("map-symbols-per-line"),&columns)){
        if(columns > 0) g_map_symbols_per_line = columns;
    }

    g_use_entire_window = parser.Found(wxT("use-entire-window"));

    g_wait = parser.Found(wxT("wait"));
    g_shellex = parser.Found(wxT("shellex"));
    g_folder = parser.Found(wxT("folder"));
    g_folder_itself = parser.Found(wxT("folder-itself"));

    g_help = parser.Found(wxT("h")) || parser.Found(wxT("?"));

    if(g_help) return true;

    /* --all-fixed flag has more precedence */
    if(g_all_fixed) g_all = false;

    /* --quick-optimization flag has more precedence */
    if(g_quick_optimization) g_optimize = false;

    /* -p flag disables cluster map as well */
    if(g_no_progress) g_show_map = false;

    /* search for drive letters and paths */
    cmdline = GetCommandLine();
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
            g_map_symbols_per_line = csbi.srWindow.Right - csbi.srWindow.Left;
            if(g_map_symbols_per_line > 2) g_map_symbols_per_line -= 2;
            else g_map_symbols_per_line = 1;

            g_map_rows = csbi.srWindow.Bottom - csbi.srWindow.Top;
            if(g_map_rows > 10) g_map_rows -= 10; else g_map_rows = 1;
            if(g_show_vol_info){
                if(g_map_rows > g_extra_lines) g_map_rows -= g_extra_lines;
                else g_map_rows = 1;
            }

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
    wxFileName path(wxT("%UD_INSTALL_DIR%\\conf\\options.lua"));
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
