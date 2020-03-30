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

#ifndef _CMD_UDEFRAG_H_
#define _CMD_UDEFRAG_H_

#include "compiler.h"
#ifdef LINUX
#include "linux.h"
#else /* LINUX */
#ifdef NODOUBLE
#define double long long /* LONGLONG must NOT be declared double ! */
#include <windows.h>
#undef double
#else /* NODOUBLE */
#include <windows.h>
#endif /* NODOUBLE */
#ifndef COMMON_LVB_REVERSE_VIDEO
#define COMMON_LVB_REVERSE_VIDEO 0x4000 // reverse video, expected in wincon.h
#endif /* COMMON_LVB_REVERSE_VIDEO */
#ifndef ERROR_COMMITMENT_LIMIT
#define ERROR_COMMITMENT_LIMIT 1455L // expected in winerror.h
#endif /* ERROR_COMMITMENT_LIMIT */
#endif /* LINUX */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef LINUX
#else /* LINUX */
#include <process.h>
#include <conio.h>
#include <shellapi.h>
#endif /* LINUX */

#ifdef USE_MSVC
#define DWORD_PTR DWORD
#endif

#ifdef LINUXMODE
#else
#include "../dll/wgx/wgx.h"
#endif
#include "../dll/udefrag/udefrag.h"
#include "../include/ultradfgver.h"

#if defined(LINUX) | defined(CURSES)
#define settextcolor(c) do { if (m_flag) SetConsoleTextAttribute(hStdOut,c); } while (0)
#else
#define settextcolor(c) (void)SetConsoleTextAttribute(hStdOut,c)
#endif

enum { ROW_CAPTION, ROW_END, ROW_FILE, ROW_ERR, ROW_PROGRESS } ;

void set_map(int, int, char);
void set_message(int, int, int, const char*);
void WINAPI close_console(HANDLE);

/* uncomment to test command line parser */
//#define CMDLINE_PARSER_TEST

#ifdef CMDLINE_PARSER_TEST
#define dbg_print printf
#else
#define dbg_print printf_stub
#endif
int printf_stub(const char *format,...);

#if defined(LINUX) && STSC
#define MAX_LONG_PATH 4095
#else
#define MAX_LONG_PATH 32767
#endif

typedef struct _object_path {
    struct _object_path *next;
    struct _object_path *prev;
    utf_t path[MAX_LONG_PATH + 1];
    int processed;
} object_path;

extern HANDLE hStdOut;
extern WORD default_color;
extern int map_rows;
extern int map_symbols_per_line;
extern short map_border_color;
extern char map_symbol;
extern int use_entire_window;
extern int screensaver_mode;
extern int map_completed;

extern int a_flag;
extern int o_flag;
extern int b_flag;
extern int h_flag;
extern int l_flag;
extern int la_flag;
extern int p_flag;
extern int v_flag;
extern int m_flag;
extern int obsolete_option;
extern int all_flag;
extern int all_fixed_flag;
extern object_path *paths;
extern char letters[MAX_DOS_DRIVES];
#ifdef LINUX
extern utf_t *volumes[];
#endif
extern int wait_flag;
extern int shellex_flag;
extern int folder_flag;
extern int folder_itself_flag;
extern int quick_optimize_flag;
extern int optimize_mft_flag;
extern int repeat_flag;
#ifdef LINUXMODE
extern int trace;
extern int do_nothing;
#endif

/* prototypes */
void parse_cmdline(int argc, char **argv);
void show_help(void);
int  AllocateClusterMap(void);
void InitializeMapDisplay(char volume_letter);
void CalculateClusterMapDimensions(void);
void RedrawMap(udefrag_progress_info *pi);
void FreeClusterMap(void);
void display_error(char *string);
void display_last_error(char *caption);
void clear_line(FILE *f);
int  insert_path(const utf_t *path);
void destroy_paths(void);

#endif /* _CMD_UDEFRAG_H_ */
