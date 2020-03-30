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
* UltraDefrag console interface - cluster map drawing code.
*/

/*
* Ideas by Dmitri Arkhangelski <dmitriar@gmail.com>
* and Stefan Pendl <stefanpe@users.sourceforge.net>.
*/

#include "udefrag.h"

#define BLOCKS_PER_HLINE  68//60//79
#define BLOCKS_PER_VLINE  10//8//16
#define N_BLOCKS          (BLOCKS_PER_HLINE * BLOCKS_PER_VLINE)
#define MAP_SYMBOL        '%'
#define BORDER_COLOR      (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)

/* global variables */
char *cluster_map = NULL;

WORD colors[NUM_OF_SPACE_STATES] = {
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
    FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
};

short map_border_color = BORDER_COLOR;
char map_symbol = MAP_SYMBOL;
int map_rows = BLOCKS_PER_VLINE;
int map_symbols_per_line = BLOCKS_PER_HLINE;
int use_entire_window = 0;
int map_completed = 0;

/**
 * @brief Adjusts map dimensions to fill the entire screen.
 */
void CalculateClusterMapDimensions(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT sr;
    HANDLE h;

    h = GetStdHandle(STD_OUTPUT_HANDLE);
    if(GetConsoleScreenBufferInfo(h,&csbi)){
        map_symbols_per_line = csbi.srWindow.Right - csbi.srWindow.Left - 2;
        if(v_flag == 0)
            map_rows = csbi.srWindow.Bottom - csbi.srWindow.Top - 10;
        else
            map_rows = csbi.srWindow.Bottom - csbi.srWindow.Top - 20;
        /* scroll buffer one line up */
        if(csbi.srWindow.Top > 0){
            sr.Top = sr.Bottom = -1;
            sr.Left = sr.Right = 0;
            (void)SetConsoleWindowInfo(h,FALSE,&sr);
        }
    } else {
        display_error("CalculateClusterMapDimensions() failed!\n\n");
    }
}

/**
 * @brief Allocates cluster map.
 * @return Zero for success,
 * negative value otherwise.
 */
int AllocateClusterMap(void)
{
    cluster_map = malloc(map_rows * map_symbols_per_line);
    if(cluster_map == NULL){
        if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        printf("Cannot allocate %i bytes of memory for cluster map!\n\n",
            map_rows * map_symbols_per_line);
        if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        return (-1);
    }
    memset(cluster_map,0,map_rows * map_symbols_per_line);
    return 0;
}

/**
 * @brief Frees cluster map resources.
 */
void FreeClusterMap(void)
{
    if(cluster_map)
        free(cluster_map);
}

/**
 * @brief Redraws cluster map on the screen.
 */
void RedrawMap(udefrag_progress_info *pi)
{
    WORD border_color = map_border_color;
    WORD color, prev_color = 0x0;
    char c[2];
    int i,j;
    
    if(pi){
        if(pi->cluster_map && pi->cluster_map_size == map_rows * map_symbols_per_line)
            memcpy(cluster_map,pi->cluster_map,pi->cluster_map_size);
    }

    fprintf(stderr,"\n\n");
    
    settextcolor(border_color);
    prev_color = border_color;
    c[0] = 0xC9; c[1] = 0;
    fprintf(stderr,c);
    for(j = 0; j < map_symbols_per_line; j++){
        c[0] = 0xCD; c[1] = 0;
        fprintf(stderr,c);
    }
    c[0] = 0xBB; c[1] = 0;
    fprintf(stderr,c);
    fprintf(stderr,"\n");

    for(i = 0; i < map_rows; i++){
        if(border_color != prev_color) settextcolor(border_color);
        prev_color = border_color;
        c[0] = 0xBA; c[1] = 0;
        fprintf(stderr,c);
        for(j = 0; j < map_symbols_per_line; j++){
            color = colors[(int)cluster_map[i * map_symbols_per_line + j]];
            if(color != prev_color) settextcolor(color);
            prev_color = color;
            c[0] = map_symbol; c[1] = 0;
            fprintf(stderr,"%s",c);
        }
        if(border_color != prev_color) settextcolor(border_color);
        prev_color = border_color;
        c[0] = 0xBA; c[1] = 0;
        fprintf(stderr,c);
        fprintf(stderr,"\n");
    }

    if(border_color != prev_color) settextcolor(border_color);
    prev_color = border_color;
    c[0] = 0xC8; c[1] = 0;
    fprintf(stderr,c);
    for(j = 0; j < map_symbols_per_line; j++){
        c[0] = 0xCD; c[1] = 0;
        fprintf(stderr,c);
    }
    c[0] = 0xBC; c[1] = 0;
    fprintf(stderr,c);
    fprintf(stderr,"\n");

    fprintf(stderr,"\n");
    if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    else settextcolor(default_color);
    map_completed = 1;
}

/**
 * @brief Prepares the screen for the map draw.
 */
void InitializeMapDisplay(char volume_letter)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD cursor_pos;

    clear_line(stderr);
    fprintf(stderr,"\r%c: %s%6.2lf%% complete, fragmented/total = %u/%u",
        volume_letter,"analyze:  ",0.00,0,0);
    RedrawMap(NULL);

    if(use_entire_window){
        /* reserve a single line for the next command prompt */
        if(v_flag == 0)
            printf("\n");
        else
            printf("\n\n\n\n\n\n\n\n\n\n\n");
        /* move cursor back to the previous line */
        if(!GetConsoleScreenBufferInfo(hStdOut,&csbi)){
            display_last_error("Cannot retrieve cursor position!");
            return; /* impossible to determine the current cursor position  */
        }
        cursor_pos.X = 0;
        if(v_flag == 0)
            cursor_pos.Y = csbi.dwCursorPosition.Y - 1;
        else
            cursor_pos.Y = csbi.dwCursorPosition.Y - 11;
        if(!SetConsoleCursorPosition(hStdOut,cursor_pos))
            display_last_error("Cannot set cursor position!");
    }
}
