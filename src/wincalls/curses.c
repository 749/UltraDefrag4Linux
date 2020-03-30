/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2011      by Jean-Pierre Andre for the Linux version
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

#include "compiler.h"

#ifdef CURSES

#define getch wingetch /* symbol conflict with curses */

#ifdef LINUX
#include "linux.h"
#else
#include <windows.h>
#endif

#include <stdio.h>
#include "../console/udefrag.h"

#undef getch

#define trace stdtrace /* symbol conflict with curses */
#include <curses.h>
#undef trace

#ifndef UNTHREADED
#include <pthread.h>
#endif

extern int map_rows;
extern int map_symbols_per_line;
extern int m_flag;
extern int trace;

#ifndef UNTHREADED 
pthread_mutex_t curseslock;
#endif

void init_colorpairs(void);
int colortrans(WORD input);
int colornum(int fg, int bg);
short curs_color(int fg);

/*
 *			Console output
 */

HANDLE WINAPI GetStdHandle(DWORD num)
{
	HANDLE h;

	if (trace)
		fprintf(stderr,"GetStdHandle -%ld\n",(long)-num);
	if (m_flag) {
		setterm(0);
		initscr();
		keypad(stdscr,TRUE);
		noecho();
		nonl();
		cbreak();
		curs_set(0);
    	if (has_colors()) {
			start_color();
	    	use_default_colors();
			init_colorpairs();
		}
		h = (HANDLE)stdscr;
#ifndef UNTHREADED
	pthread_mutex_init(&curseslock,(pthread_mutexattr_t*)NULL);
#endif
	} else
		h = (HANDLE)stdout;
	return (h);
}

#ifdef LINUX
BOOLEAN WINAPI GetConsoleScreenBufferInfo(HANDLE h, CONSOLE_SCREEN_BUFFER_INFO *csbi)
#else
BOOL WINAPI GetConsoleScreenBufferInfo(HANDLE h, CONSOLE_SCREEN_BUFFER_INFO *csbi)
#endif
{
	csbi->dwSize.X = COLS;
	csbi->dwSize.Y = LINES;
	csbi->dwCursorPosition.X = 0;
	csbi->dwCursorPosition.Y = 0;
	csbi->wAttributes = 0;
	csbi->srWindow.Left = 0;
	csbi->srWindow.Top = 0;
	csbi->srWindow.Right = csbi->dwSize.X - 1;
	csbi->srWindow.Left = csbi->dwSize.Y - 1;
	return (TRUE);
}

#ifdef LINUX
BOOLEAN WINAPI SetConsoleCursorPosition(HANDLE h, COORD p)
#else
BOOL WINAPI SetConsoleCursorPosition(HANDLE h, COORD p)
#endif
{
		/* the position is designed for scrolling screens */
	return (TRUE);
}

#ifdef LINUX
BOOLEAN WINAPI SetConsoleTextAttribute(HANDLE h, WORD attr)
#else
BOOL WINAPI SetConsoleTextAttribute(HANDLE h, WORD attr)
#endif
{
	/* do not use : multhreading might lead to wrong association to text */
#ifndef UNTHREADED
    pthread_mutex_lock(&curseslock);
#endif
    attron(COLOR_PAIR(colortrans(attr)));
#ifndef UNTHREADED
    pthread_mutex_unlock(&curseslock);
#endif
	return (TRUE);
}

void set_map(int i, int j, char c)
{
#ifndef UNTHREADED
        pthread_mutex_lock(&curseslock);
#endif
	move(i,(COLS - map_symbols_per_line)/2 + j - 1);
	if (c == '\n') {
		clrtoeol();
		touchwin(stdscr);
		refresh();
	} else
		addch(c);
#ifndef UNTHREADED
        pthread_mutex_unlock(&curseslock);
#endif
}

void set_message(int i, int j, int attr, const char *text)
{
	int final;

#ifndef UNTHREADED
        pthread_mutex_lock(&curseslock);
#endif
	move(map_rows+2+i,(COLS - map_symbols_per_line)/2 - 1 + j);
	if (attr & FOREGROUND_RED)
		standout();
	final = (COLS - map_symbols_per_line)/2 - 1 + j + strlen(text);
	if (final >= COLS)
		addstr(&text[final - COLS + 1]);
	else
		addstr(text);
	if (attr & FOREGROUND_RED)
		standend();
	if (!strchr(text,'\n')) /* do not clear next line ! */
		clrtoeol();
	touchwin(stdscr);
	refresh();
#ifndef UNTHREADED
        pthread_mutex_unlock(&curseslock);
#endif
}

void WINAPI close_console(HANDLE h)
{
	if (trace)
		fprintf(stderr,"endwin\n");
#ifndef UNTHREADED
        pthread_mutex_lock(&curseslock);
#endif
	refresh();
#ifndef UNTHREADED
        pthread_mutex_unlock(&curseslock);
#endif
	SetConsoleTextAttribute(NULL,FOREGROUND_RED | FOREGROUND_INTENSITY);
	set_message(ROW_END,0,
              (b_flag ? 0 : FOREGROUND_RED | FOREGROUND_INTENSITY),
              "Press any key to close screen");
	SetConsoleTextAttribute(NULL,0);
	curs_set(1);
	wgetch(stdscr);
	endwin();
}

void init_colorpairs(void)
{
    int fg, bg;
    int colorpair;

    for (bg = 0; bg <= 7; bg++) {
        for (fg = 0; fg <= 7; fg++) {
            colorpair = colornum(fg, bg);
            init_pair(colorpair, curs_color(fg), curs_color(bg));
        }
    }
}

int colortrans(WORD input)
{
	return (1 << 7) | (input & 7);
}

int colornum(int fg, int bg)
{
    int B, bbb, ffff;

    B = 1 << 7;
    bbb = (7 & bg) << 4;
    ffff = 7 & fg;

    return (B | bbb | ffff);
}

short curs_color(int fg)
{
    switch (7 & fg) {           /* RGB */
    case 0:                     /* 000 */
        return (COLOR_BLACK);
    case 1:                     /* 001 */
        return (COLOR_BLUE);
    case 2:                     /* 010 */
        return (COLOR_GREEN);
    case 3:                     /* 011 */
        return (COLOR_CYAN);
    case 4:                     /* 100 */
        return (COLOR_RED);
    case 5:                     /* 101 */
        return (COLOR_MAGENTA);
    case 6:                     /* 110 */
        return (COLOR_YELLOW);
    case 7:                     /* 111 */
        return (COLOR_WHITE);
    }
}

#endif /* CURSES */
