/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

#ifndef LINUX_H
#define LINUX_H 1

#include <string.h>
#include <stdarg.h>

#ifndef STATIC_LIB
#define STATIC_LIB 1
#endif

#if STSC
#define MAX_PATH 1024
#else
//#define MAX_PATH 32700
#define MAX_PATH 8192
#endif

#define STD_OUTPUT_HANDLE -11

#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define COMMON_LVB_REVERSE_VIDEO 0x4000 // reverse video

#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200

#define LANG_NEUTRAL                     0x00
#define SUBLANG_DEFAULT                  0x01    /* user default */

#define ERROR_ALREADY_EXISTS             EEXIST /* 183L */
#define ERROR_ENVVAR_NOT_FOUND           ENODATA /* 203L */
#define ERROR_COMMITMENT_LIMIT           ENOMEM /* 1455L */

#define STATUS_CONTROL_C_EXIT            ((DWORD   )0xC000013AL)

#define THREAD_BASE_PRIORITY_MAX    2   // maximum thread base priority boost
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)

#define FILE_ATTRIBUTE_READONLY         0x00000001
#define FILE_ATTRIBUTE_HIDDEN           0x00000002
#define FILE_ATTRIBUTE_SYSTEM           0x00000004
#define FILE_ATTRIBUTE_DIRECTORY        0x00000010
#define FILE_ATTRIBUTE_ARCHIVE          0x00000020
#define FILE_ATTRIBUTE_NORMAL           0x00000080
#define FILE_ATTRIBUTE_TEMPORARY        0x00000100
// ? #define FILE_ATTRIBUTE_ATOMIC_WRITE     0x00000200
#define FILE_ATTRIBUTE_SPARSE_FILE      0x00000200
// ?#define FILE_ATTRIBUTE_XACTION_WRITE    0x00000400
#define FILE_ATTRIBUTE_REPARSE_POINT    0x00000400
#define FILE_ATTRIBUTE_COMPRESSED       0x00000800
#define FILE_ATTRIBUTE_ENCRYPTED        0x00004000

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

#define FILE_READ_DATA                  0x00000001
#define FILE_LIST_DIRECTORY             0x00000001
#define FILE_WRITE_DATA                 0x00000002
#define FILE_ADD_FILE                   0x00000002
#define FILE_APPEND_DATA                0x00000004
#define FILE_ADD_SUBDIRECTORY           0x00000004
#define FILE_READ_ATTRIBUTES            0x00000080
#define SYNCHRONIZE                     0x00100000
#define FILE_GENERIC_EXECUTE            0x20000000
#define FILE_GENERIC_WRITE              0x40000000
#define FILE_GENERIC_READ               0x80000000
#define STANDARD_RIGHTS_ALL             0x001f0000
#define GENERIC_READ                    0x80000000

#define FILE_SHARE_READ                 0x00000001
#define FILE_SHARE_WRITE                0x00000002
#define FILE_SHARE_DELETE               0x00000004

#define DLL_PROCESS_ATTACH 1
#define DLL_PROCESS_DETACH 0

#define DRIVE_UNKNOWN    0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE  2
#define DRIVE_FIXED      3
#define DRIVE_REMOTE     4
#define DRIVE_CDROM      5

#define RIGHT_ALT_PRESSED     0x0001 // the right alt key is pressed.
#define LEFT_ALT_PRESSED      0x0002 // the left alt key is pressed.
#define RIGHT_CTRL_PRESSED    0x0004 // the right ctrl key is pressed.
#define LEFT_CTRL_PRESSED     0x0008 // the left ctrl key is pressed.
#define SHIFT_PRESSED         0x0010 // the shift key is pressed.
#define NUMLOCK_ON            0x0020 // the numlock light is on.
#define SCROLLLOCK_ON         0x0040 // the scrolllock light is on.
#define CAPSLOCK_ON           0x0080 // the capslock light is on.
#define ENHANCED_KEY          0x0100 // the key is enhanced.

#define WAIT_OBJECT_0    0
#define WAIT_ABANDONED_0 0x00000080L

#define PAGE_READWRITE         0x04

#define MEM_COMMIT           0x1000
#define MEM_RESERVE          0x2000
#define MEM_DECOMMIT         0x4000
#define MEM_RELEASE          0x8000
#define MEM_FREE            0x10000

#define HEAP_GROWABLE                   0x00000002

#define KEY_QUERY_VALUE     0x0001
#define KEY_SET_VALUE       0x0002

#define EVENT_MODIFY_STATE      0x0002

#define SECTION_QUERY       0x0001
#define SECTION_MAP_WRITE   0x0002
#define SECTION_MAP_READ    0x0004 
#define SECTION_MAP_EXECUTE 0x0008 
#define SECTION_EXTEND_SIZE 0x0010 

#define SECTION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SECTION_QUERY|\
                            SECTION_MAP_WRITE |      \
                            SECTION_MAP_READ |       \
                            SECTION_MAP_EXECUTE |    \
                            SECTION_EXTEND_SIZE)

#define REG_MULTI_SZ        7

#define INFINITE            0xFFFFFFFF  // Infinite timeout

typedef LONG ACCESS_MASK;

typedef struct {
	WORD X;
	WORD Y;
} COORD;

typedef struct _SMALL_RECT {
  SHORT Left;
  SHORT Top;
  SHORT Right;
  SHORT Bottom;
} SMALL_RECT;

typedef struct {
	COORD dwSize;
	COORD dwCursorPosition;
	WORD wAttributes;
	SMALL_RECT srWindow;
} CONSOLE_SCREEN_BUFFER_INFO;

typedef struct _LIST_ENTRY { 
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _RTL_CRITICAL_SECTION_DEBUG {
    WORD   Type;
    WORD   CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    DWORD EntryCount;
    DWORD ContentionCount;
    DWORD Depth;
    PVOID OwnerBackTrace[ 5 ];
} RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG;

typedef struct _RTL_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    
    //
    //  The following three fields control entering and exiting the critical
    //  section for the resource
    //
    
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;        // from the thread's ClientId->UniqueThread
    HANDLE LockSemaphore;
    DWORD Reserved;
} RTL_CRITICAL_SECTION, CRITICAL_SECTION,
    *PRTL_CRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef struct _OSVERSIONINFOW {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  WCHAR szCSDVersion[128];
} OSVERSIONINFOW, *LPOSVERSIONINFOW;


typedef ULONG (WINAPI *LPTHREAD_START_ROUTINE)(void*);
typedef BOOLEAN (*PHANDLER_ROUTINE)(DWORD);

/*
 *      Types to be defined
 */

typedef int *NT_TIB;
typedef int *PTOKEN_PRIVILEGES;
typedef int *POWER_ACTION;
typedef int *SYSTEM_POWER_STATE;
typedef int **PSECURITY_DESCRIPTOR;
typedef int *LPSECURITY_ATTRIBUTES;

/*
 *	Forward struct declarations
 */

struct _UNICODE_STRING;

/*
 *       Variables
 */

extern utf_t *volumes[];

/*
 *       Prototypes
 */

HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE,
			LPVOID, DWORD, LPDWORD);
HANDLE WINAPI CreateEvent(LPSECURITY_ATTRIBUTES, BOOLEAN, BOOLEAN, LPCTSTR);
HANDLE WINAPI GetStdHandle(DWORD);
BOOLEAN WINAPI CloseHandle(HANDLE);
VOID WINAPI Sleep(DWORD);
VOID WINAPI RtlInitUnicodeString(struct _UNICODE_STRING*, const utf_t*);
HANDLE WINAPI GetCurrentThread(void);
BOOLEAN WINAPI SetThreadPriority(HANDLE, int);
struct _TEB * WINAPI NtCurrentTeb(void);
utf16_t * WINAPI GetCommandLineW(void);
utf16_t ** WINAPI CommandLineToArgvW(const utf16_t*, const int*);
HANDLE WINAPI LoadLibrary(const char*);
DWORD WINAPI GetEnvironmentVariableW(const utf_t*, utf_t*, DWORD);
BOOLEAN WINAPI SetEnvironmentVariableA(const char*, const char*);
BOOLEAN WINAPI SetEnvironmentVariableW(const utf_t*, const utf_t*);
DWORD WINAPI GetLastError(void);
VOID WINAPI RtlZeroMemory(VOID*, SIZE_T); /* not WINAPI according to msdn */
BOOLEAN WINAPI SetConsoleCursorPosition(HANDLE, COORD);
BOOLEAN WINAPI SetConsoleTextAttribute(HANDLE, WORD);
BOOLEAN WINAPI GetConsoleScreenBufferInfo(HANDLE, CONSOLE_SCREEN_BUFFER_INFO*);
BOOLEAN WINAPI SetConsoleWindowInfo(HANDLE, BOOLEAN, const SMALL_RECT*);
DWORD MAKELANGID(DWORD, DWORD);
/*
 *             Extended C library for processing utf16le strings
 */

#define wchar_t WCHAR /* do not reference the standard wchar_t */
//typedef WCHAR wchar_t;

utf16_t *utf16rchr(const utf16_t*, utf16_t);
utf16_t *utf16str(const utf16_t*, const utf16_t*);

int toutf16(utf16_t*, const char*, int);
int toutf8(char*, const utf16_t*, int);

		/* for debug messages, associated with formats LSTR or WSTR */
const char *printablepath(const utf_t*, int);
const char *printableutf(const utf_t*);
const char *printableutf16(const utf16_t*);

utf_t utflower(utf_t);
char *_strupr(char*);
char *_strlwr(char*);

int _vsnprintf(char*, size_t sz, const char*, va_list);
int _snwprintf(utf_t*, size_t, const utf_t*, ...);
#if WNSC | STSC | SPGC
int safe_fprintf(struct _iobuf*, const char*, ...);
void safe_dump(struct _iobuf*, const char*, const char*, int);
#else
int safe_fprintf(struct _IO_FILE*, const char*, ...);
void safe_dump(struct _IO_FILE*, const char*, const char*, int);
#endif
const char *calledfrom(void*);

/*
 *		Miscellaneous
 */

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

void stop_there(int, const char*, int);
#define get_out(n) stop_there(n,__FILE__,__LINE__)
void initwincalls(void);

/*
 *           Memory allocation through libntfs-3g
 *           (useful for debugging)
 */

#endif /* LINUX_H */
