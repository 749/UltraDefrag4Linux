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

#endif /* CURSES */
