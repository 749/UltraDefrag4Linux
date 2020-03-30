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
#if defined(LINUX) | defined(CURSES)
#define BLOCKS_PER_VLINE  15
#else
#define BLOCKS_PER_VLINE  10//8//16
#endif
#define N_BLOCKS          (BLOCKS_PER_HLINE * BLOCKS_PER_VLINE)
#define MAP_SYMBOL        '%'
#define BORDER_COLOR      (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)

/* global variables */
static char *cluster_map = NULL;

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
extern int stop_flag;

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

#if defined(LINUX) | defined(CURSES)

void RedrawMap(udefrag_progress_info *pi)
{
    enum { NW, N, NE, E, SE, S, SW, W };
    static const char glyphs[] = "+-+|+-+|";
    WORD border_color = map_border_color;
    WORD color, prev_color = 0x0;
    char *buf;
    static const char abrt[] = "Only use Ctrl-C to abort";
    static const char key[]
        = "Key : f fragm, u unfrag, d dir, c compr, m MFT, s system, t temp";
    int count;
    int i,j;
    char c;
    
    if(pi){
        if(pi->cluster_map && pi->cluster_map_size == (unsigned int)(map_rows * map_symbols_per_line))
            memcpy(cluster_map,pi->cluster_map,pi->cluster_map_size);
    }

    buf = (char*)malloc(map_symbols_per_line + 3);
    color = 0;
    set_map(0,0,'\n');
    if (trace) safe_fprintf(stderr,"\n");
    settextcolor(border_color);
    prev_color = border_color;
    set_map(0,0,glyphs[NW]);
    if (buf) buf[0] = glyphs[NW];
    for(j = 0; j < map_symbols_per_line; j++){
        set_map(0,j+1,glyphs[N]);
        if (buf) buf[j+1] = glyphs[N];
    }
    set_map(0,j+1,glyphs[NE]);
    if (buf) buf[map_symbols_per_line+1] = glyphs[NE];
    set_map(0,j+2,'\n');
    if (buf) buf[map_symbols_per_line+2] = 0;
    if (trace && buf) safe_fprintf(stderr,"%s\n",buf);

    for(i = 0; i < map_rows; i++){
        if(border_color != prev_color) settextcolor(border_color);
        prev_color = border_color;
        set_map(i+1,0,glyphs[W]);
        if (buf) buf[0] = glyphs[W];
        for(j = 0; j < map_symbols_per_line; j++){
            count = (int)cluster_map[i * map_symbols_per_line + j];
            color = count;
            switch(count) {
            case UNUSED_MAP_SPACE : c = '/'; break;
            case FREE_SPACE : c = '.'; break;
            case SYSTEM_SPACE : c = 's'; break;
            case SYSTEM_OVER_LIMIT_SPACE : c = 'S'; break;
            case FRAGM_SPACE : c = 'f'; break;
            case FRAGM_OVER_LIMIT_SPACE : c = 'F'; break;
            case UNFRAGM_SPACE : c = 'u'; break;
            case UNFRAGM_OVER_LIMIT_SPACE : c = 'U'; break;
            case DIR_SPACE : c = 'd'; break;
            case DIR_OVER_LIMIT_SPACE : c = 'D'; break;
            case COMPRESSED_SPACE : c = 'c'; break;
            case COMPRESSED_OVER_LIMIT_SPACE : c = 'C'; break;
            case MFT_ZONE_SPACE : c = 'm'; break;
            case MFT_SPACE : c = 'M'; break;
            default :
                color = 14;
            case TEMPORARY_SYSTEM_SPACE : c = 't'; break;
            }
#ifdef CURSES
            if(prev_color != colors[color]) {
                settextcolor(colors[color]);
                prev_color = colors[color];
            }
#endif
            set_map(i+1,j+1,c);
            if (buf) buf[j+1] = c;
        }
        if(border_color != prev_color) settextcolor(border_color);
        prev_color = border_color;
        set_map(i+1,j+1,glyphs[E]);
        if (buf) buf[map_symbols_per_line+1] = glyphs[E];
        set_map(i+1,j+2,'\n');
        if (buf) buf[map_symbols_per_line+2] = 0;
        if (trace && buf) safe_fprintf(stderr,"%s\n",buf);
    }

    if(border_color != prev_color) settextcolor(border_color);
    prev_color = border_color;
    set_map(i+1,0,glyphs[SW]);
    if (buf) buf[0] = glyphs[SW];
    for(j = 0; j < map_symbols_per_line; j++){
        set_map(i+1,j+1,glyphs[S]);
        if (buf) buf[j+1] = glyphs[S];
    }
    set_map(i+1,j+1,glyphs[SE]);
    if (buf) buf[map_symbols_per_line+1] = glyphs[SE];
    set_map(i+1,j+2,'\n');
    if (buf) buf[map_symbols_per_line+2] = 0;
    if (trace && buf) safe_fprintf(stderr,"%s\n",buf);

    if(!b_flag) color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    else color = default_color;
    settextcolor(color);
    set_message(ROW_CAPTION,0,color,key);
    if (!stop_flag) {
//        set_message(ROW_END,0," "); /* clear line */
        settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        set_message(ROW_END,(map_symbols_per_line + 2 - sizeof(abrt))/2,
               (b_flag ? 0 : FOREGROUND_RED | FOREGROUND_INTENSITY), abrt);
    }
    settextcolor(color);
    if (buf)
        free(buf);
    map_completed = 1;
}

#else

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

#endif

/**
 * @brief Prepares the screen for the map draw.
 */
void InitializeMapDisplay(char volume_letter)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD cursor_pos;

#if defined(LINUX) | defined(CURSES)
    char buf[120];

#ifdef LINUX
    snprintf(buf,120,"%s: %s%6.2lf%% complete, fragmented/total = %u/%u",
        volumes[volume_letter],"analyze:  ",0.00,0,0);
#else
    snprintf(buf,120,"%c: %s%6.2lf%% complete, fragmented/total = %u/%u",
        volume_letter,"analyze:  ",0.00,0,0);
#endif
    if (m_flag)
        set_message(ROW_PROGRESS,0,
             (b_flag ? 0 : FOREGROUND_GREEN | FOREGROUND_INTENSITY), buf);
    else
        printf("%s\n",buf);
#else
    clear_line(stderr);
    fprintf(stderr,"\r%c: %s%6.2lf%% complete, fragmented/total = %u/%u",
        volume_letter,"analyze:  ",0.00,0,0);
#endif
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
