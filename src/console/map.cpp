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
 * @file map.cpp
 * @brief Cluster map.
 * @addtogroup Map
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                              Constants
// =======================================================================

#define BLOCKS_PER_HLINE  68
#define BLOCKS_PER_VLINE  10
#define N_BLOCKS          (BLOCKS_PER_HLINE * BLOCKS_PER_VLINE)
#define MAP_SYMBOL        '%'
#define BORDER_COLOR      (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)

// =======================================================================
//                          Global variables
// =======================================================================

char *g_map = NULL;

short g_map_border_color     = BORDER_COLOR;
char  g_map_symbol           = MAP_SYMBOL;
int   g_map_rows             = BLOCKS_PER_VLINE;
int   g_map_symbols_per_line = BLOCKS_PER_HLINE;

int   g_extra_lines = 0;

short g_colors[SPACE_STATES] = {
    0,
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    FOREGROUND_GREEN,
    FOREGROUND_RED | FOREGROUND_INTENSITY,
    FOREGROUND_RED,
    FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    FOREGROUND_BLUE,
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    FOREGROUND_RED | FOREGROUND_GREEN,
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    FOREGROUND_RED | FOREGROUND_GREEN,
    FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    FOREGROUND_RED | FOREGROUND_BLUE,
};

// =======================================================================
//                            Cluster map
// =======================================================================

void init_map(char letter)
{
    if(!g_show_map)
        return;

    g_map = new char[g_map_rows * g_map_symbols_per_line];
    memset(g_map,0,g_map_rows * g_map_symbols_per_line);

    clear_line();
    printf("\r%c: %s%6.2lf%% complete, fragmented/total = %u/%u",
        letter,"analyze:  ",0.00,0,0);

    redraw_map(NULL);

    if(g_use_entire_window){
        /* reserve a single line for the next command prompt */
        printf("\n");
        for(int i = 0; g_show_vol_info && i < g_extra_lines; i++) printf("\n");
        /* move cursor back to the previous line */
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if(!GetConsoleScreenBufferInfo(g_out,&csbi)){
            letrace("cannot get cursor position");
        } else {
            COORD cursor_pos;
            cursor_pos.X = 0;
            cursor_pos.Y = csbi.dwCursorPosition.Y - 1;
            if(g_show_vol_info) cursor_pos.Y -= (SHORT)g_extra_lines;
            if(!SetConsoleCursorPosition(g_out,cursor_pos))
                letrace("cannot set cursor position");
        }
    }
}

void redraw_map(udefrag_progress_info *pi)
{
    if(pi){
        if(pi->cluster_map && pi->cluster_map_size == g_map_rows * g_map_symbols_per_line)
            memcpy(g_map,pi->cluster_map,pi->cluster_map_size);
    }

    printf("\n\n");

    // print top line of the map
    force_color(g_map_border_color);
    short prev_color = g_map_border_color;

    rputch(0xC9);

    for(int j = 0; j < g_map_symbols_per_line; j++) rputch(0xCD);

    rputch(0xBB); printf("\n");

    // print the map
    for(int i = 0; i < g_map_rows; i++){
        rputch(0xBA);

        for(int j = 0; j < g_map_symbols_per_line; j++){
            short color = g_colors[(int)g_map[i * g_map_symbols_per_line + j]];

            if(color != prev_color) force_color(color);
            prev_color = color;

            rputch(g_map_symbol);
        }

        if(g_map_border_color != prev_color)
            force_color(g_map_border_color);
        prev_color = g_map_border_color;

        rputch(0xBA); printf("\n");
    }

    // print bottom line of the map
    rputch(0xC8);

    for(int j = 0; j < g_map_symbols_per_line; j++) rputch(0xCD);

    rputch(0xBC); printf("\n\n");

    force_color(g_use_default_colors ? g_default_color :
        FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void destroy_map(void)
{
    delete [] g_map;
}

/** @} */
