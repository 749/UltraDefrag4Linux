/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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
* UltraDefrag boot time (native) interface header.
*/

#ifndef _DEFRAG_NATIVE_H_
#define _DEFRAG_NATIVE_H_

#include "../dll/zenwinx/zenwinx.h"
#include "../dll/udefrag/udefrag.h"
#include "../include/ultradfgver.h"

/* uncomment it if you want to replace smss.exe by this program */
//#define USE_INSTEAD_SMSS

#define short_dbg_delay() winx_sleep(3000)
#define long_dbg_delay()  winx_sleep(10000)

/* define how many lines to display for each text page,
   smallest boot screen height is 24 rows,
   which must be reduced by one row for the prompt */
#define MAX_DISPLAY_ROWS 23

/* define how many characters may be
   printed on line after a prompt */
#define MAX_LINE_WIDTH 70

/* message to terminate volume processing */
#define BREAK_MESSAGE "Use Pause/Break key to abort the process early.\n\n"

/* define whether @echo is on by default or not */
#define DEFAULT_ECHO_FLAG 1

/* message to be shown when pause command is used without parameters */
#define PAUSE_MESSAGE "Hit any key to continue..."

/* Returns current debug level, declared in udefrag.c */
int GetDebugLevel();

#define MAX_ENV_VARIABLE_LENGTH 32766
#define MAX_LONG_PATH MAX_ENV_VARIABLE_LENGTH /* must be equal */
typedef struct _object_path {
    struct _object_path *next;
    struct _object_path *prev;
    wchar_t path[MAX_LONG_PATH + 1];
    int processed;
} object_path;

/* prototypes */
void NativeAppExit(int exit_code);
int parse_command(wchar_t *cmdline);
int ProcessScript(wchar_t *filename);
int ExecPendingBootOff(void);
int exit_handler(int argc,wchar_t **argv,wchar_t **envp);
int udefrag_handler(int argc,wchar_t **argv,wchar_t **envp);

extern PEB *peb;
extern winx_history history;
extern int scripting_mode;
extern int exit_flag;
extern int abort_flag;
extern int escape_flag;

#endif /* _DEFRAG_NATIVE_H_ */
