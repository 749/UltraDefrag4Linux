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

/*
 *		identify the OS and compiler used
 *	and define integer, pointer and string types accordingly
 *
 *      Only these types can be used when interfacing with ntfs-3g
 */

#ifndef COMPILER_H
#define COMPILER_H 1

#ifndef LXSC            /* Linux X86-32 with Sozobon compiler */
#define LXSC 0
#endif

#ifndef L8SC            /* Linux X86-64 with Sozobon compiler */
#define L8SC 0
#endif

#ifndef STSC            /* M68k Linux-like with Sozobon compiler */
#define STSC 0
#endif

#ifndef LXGC            /* Linux X86-32 or X86-64 with GNU compiler */
#define LXGC 0
#endif

#ifndef SPGC            /* Linux Sparc with GNU compiler */
#define SPGC 0
#endif

#ifndef PPGC            /* Linux PowerPc with GNU compiler */
#define PPGC 0
#endif

#ifndef ARGC            /* Linux ARM (armv7 or aarch64) with GNU compiler */
#define ARGC 0
#endif

#ifndef WNSC            /* Windows X86-32 with Sozobon compiler */
#define WNSC 0
#endif

/*
 *		Specific configuration parameters
 *	Mostly to select a code variant
 */

/* truncate, instead of delete a region after a cluster move failed */
#define TRUNCREGION 1

#undef USE_MSVC

/*
 *            Compiler dependent keywords and types definitions
 */

#if LXSC | L8SC

#define LINUX 1
#define LINUXMODE 1
// #define UNTHREADED 1 /* unthreaded is slower (more screen displaying) */
#define CURSES 1
#define STATIC_LIB 1   /* should not be here */
                                         /* 64 bits */
typedef long long LONGLONG;
typedef long long SLONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long __int64;
typedef unsigned long long LELONGLONG;
                                         /* 32 bits */
typedef int LONG;
typedef unsigned int ULONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int LELONG;
                                         /* 16 bits */
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned short LESHORT;
typedef unsigned short WCHAR; /* little endian ! */
                                         /* 8 bits */
typedef signed char SCHAR;
typedef unsigned char UCHAR;

typedef long DWORD_PTR; /* integer with same size as a pointer */

typedef unsigned long SIZE_T;  /* size not important */

//typedef int BOOL;  // already in ntndk.h
typedef char BOOLEAN; /* has to be a char for data in ntfs.h */

typedef void VOID;

#define WINAPI __stdcall
#define NTAPI __stdcall
#define signed

#endif /* LXSC | L8SC */

#if LXGC

#define LINUX 1
#define LINUXMODE 1
//#define UNTHREADED 1
#define CURSES 1
#define STATIC_LIB 1   /* should not be here */

                                         /* 64 bits */
typedef long long LONGLONG;
typedef long long SLONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long __int64;
typedef unsigned long long LELONGLONG;
                                         /* 32 bits */
typedef int LONG;
typedef unsigned int ULONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int LELONG;
                                         /* 16 bits */
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned short LESHORT;
typedef unsigned short WCHAR; /* little endian ! */
#undef __WCHAR_TYPE__
#define __WCHAR_TYPE__ WCHAR

                                         /* 8 bits */
typedef signed char SCHAR;
typedef unsigned char UCHAR;

typedef long DWORD_PTR; /* integer with same size as a pointer */

typedef unsigned long SIZE_T;  /* size not important */

//typedef int BOOL;  // already in ntndk.h
typedef char BOOLEAN; /* has to be a char for data in ntfs.h */

typedef void VOID;

#define WINAPI
#define NTAPI
#define __stdcall
#define __cdecl
#define signed

#endif /* LXGC */

#if SPGC | PPGC

#define LINUX 1
#define LINUXMODE 1
#define UNTHREADED 1
#if PPGC
#define CURSES 1
#else
#undef CURSES
#endif
#define BIGENDIAN 1
#define STATIC_LIB 1   /* should not be here */

                                         /* 64 bits */
typedef long long LONGLONG;
typedef long long SLONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long __int64;
typedef unsigned long long LELONGLONG;
                                         /* 32 bits */
typedef int LONG;
typedef unsigned int ULONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int LELONG;

                                         /* 16 bits */
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned short LESHORT;
typedef unsigned short WCHAR; /* little endian ! */
#undef __WCHAR_TYPE__
#define __WCHAR_TYPE__ WCHAR

                                         /* 8 bits */
typedef signed char SCHAR;
typedef unsigned char UCHAR;

typedef long DWORD_PTR; /* integer with same size as a pointer */

typedef unsigned long SIZE_T;  /* size not important */

//typedef int BOOL;  // already in ntndk.h
typedef char BOOLEAN; /* has to be a char for data in ntfs.h */

typedef void VOID;

#define WINAPI
#define NTAPI
#define __stdcall
#define __cdecl
#define signed

#endif /* SPGC | PPGC */

#if ARGC

#define LINUX 1
#define LINUXMODE 1
#undef UNTHREADED
#define CURSES 1
#define STATIC_LIB 1   /* should not be here */

                                         /* 64 bits */
typedef long long LONGLONG;
typedef long long SLONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long __int64;
typedef unsigned long long LELONGLONG;
                                         /* 32 bits */
typedef int LONG;
typedef unsigned int ULONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int LELONG;

                                         /* 16 bits */
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned short LESHORT;
typedef unsigned short WCHAR; /* little endian ! */
#undef __WCHAR_TYPE__
#define __WCHAR_TYPE__ WCHAR

                                         /* 8 bits */
typedef signed char SCHAR;
typedef unsigned char UCHAR;

typedef long DWORD_PTR; /* integer with same size as a pointer */

typedef unsigned long SIZE_T;  /* size not important */

//typedef int BOOL;  // already in ntndk.h
typedef char BOOLEAN; /* has to be a char for data in ntfs.h */

typedef void VOID;

#define WINAPI
#define NTAPI
#define __stdcall
#define __cdecl

#endif /* ARGC */

#if STSC

#define LINUX 1
#define LINUXMODE 1
#define UNTHREADED 1
#define BIGENDIAN 1
#define STATIC_LIB 1   /* should not be here */
                                         /* 64 bits */
typedef long long LONGLONG;
typedef long long SLONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long __int64;
typedef unsigned long long LELONGLONG;
                                         /* 32 bits */
typedef long LONG;
typedef unsigned long ULONG;
typedef long INT;
typedef unsigned long UINT;
typedef unsigned long LELONG;
                                         /* 16 bits */
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned short LESHORT;
typedef unsigned short WCHAR; /* little endian ! */
                                         /* 8 bits */
typedef signed char SCHAR;
typedef unsigned char UCHAR;

typedef long DWORD_PTR; /* integer with same size as a pointer */

typedef unsigned long SIZE_T;  /* size not important */

//typedef int BOOL;  // already in ntndk.h
typedef char BOOLEAN;

typedef void VOID;

#define WINAPI __stdcall
#define NTAPI __stdcall
#define signed

//typedef WCHAR wchar_t;
typedef unsigned long size_t;
#define _SIZE_T

#endif /* STSC */

#if WNSC

#define LINUXMODE 1
//#define UNTHREADED 1
#define CURSES 1
#define NODOUBLE 1  /* do not define LONGLONG as "double" ! */
#define STATIC_LIB 1   /* should not be here */
                                         /* 64 bits */
   /* LONGLONG is dangerous, may be defined as double */
typedef unsigned long long ULONGLONG;
typedef long long SLONGLONG;
typedef long long __int64;
typedef unsigned long long LELONGLONG;
typedef unsigned long LELONG;
typedef unsigned short LESHORT;

typedef long DWORD_PTR; /* integer with same size as a pointer */

#define __stdcall __stdcall
#define inline

#endif

/*
 *            Types definitions relying on types defined above
 */

#ifdef LINUX

#define FALSE 0
#define TRUE (!FALSE)

typedef union {
	LONGLONG QuadPart;
} LARGE_INTEGER;

typedef ULONGLONG USN;

typedef UINT DWORD;

typedef USHORT WORD;

typedef WCHAR utf16_t;

typedef SCHAR CHAR;
typedef SCHAR CCHAR; /* to be clarified (not const !) */
typedef UCHAR BYTE;

typedef ULONG *PULONG;
typedef DWORD *PDWORD;
typedef DWORD *LPDWORD;

typedef WCHAR *PWSTR;
typedef WCHAR *LPWSTR;
typedef const WCHAR *LPCWSTR;
typedef const WCHAR *PCWSTR;

typedef SCHAR *LPTSTR;
typedef SCHAR *LPSTR;
typedef SCHAR *PCHAR;
typedef UCHAR *PUCHAR;
typedef const SCHAR *LPCTSTR;

typedef LARGE_INTEGER *PLARGE_INTEGER;

typedef VOID *HANDLE;
typedef VOID *HMODULE;
typedef HANDLE *PHANDLE;

typedef VOID *PVOID;
typedef VOID *LPVOID;
typedef const VOID *LPCVOID;

PCWSTR UTF16(const char*); /* must return permanent storage */

/*
 *           Misc keywords
 */

#define IN
#define OUT
#define INOUT

#define ANYSIZE_ARRAY 1

#if STSC
#define ENODATA 61
#endif

#else /* LINUX */

typedef unsigned short utf16_t;

#endif /* LINUX */

/*
 *                       String definitions
 */

#ifdef LINUX

typedef char utf_t;     /* usual strings are utf-8 encoded */
#define utflen(s) strlen(s)
#define utfcpy(d,s) strcpy(d,s)
#define utfcmp(d,s) strcmp(d,s)
#define utfcat(d,s) strcat(d,s)
#define utfncpy(d,s,n) strncpy(d,s,n)
#define utfchr(s,c) strchr(s,c)
#define utfrchr(s,c) strrchr(s,c)
#define utfstr(s,t) strstr(s,t)
#define winx_utfdup(s) winx_strdup(s)
#define utftol(s) atol(s)
#define utftoi(s) atoi(s)

int utf16len(const utf16_t*);
utf16_t *utf16ncpy(utf16_t*, const utf16_t*, int size);
utf16_t *utf16cpy(utf16_t*, const utf16_t*);
int utf16cmp(const utf16_t*, const utf16_t*);

int utfmixcmp(const utf16_t*, const utf_t*);

#define UTF(s) s  /* no () ! */
#define UTFSTR "s"
#define LSTR "s"
#define WSTR "s"
#define LL64 "ll"

#else /* LINUX */

typedef unsigned short utf_t;     /* usual strings are utf-16 LE encoded */
//typedef wchar_t utf_t; /* usual strings are utf-16 LE encoded */
#define utflen(s) wcslen(s)
#define utfcpy(d,s) wcscpy(d,s)
#define utfcmp(d,s) wcscmp(d,s)
#define utfcat(d,s) wcscat(d,s)
#define utfncpy(d,s,n) wcsncpy(d,s,n)
#define utfchr(s,c) wcschr(s,c)
#define utfrchr(s,c) wcsrchr(s,c)
#define utfstr(s,t) wcsstr(s,t)
#define winx_utfdup(s) winx_wcsdup(s)
#define utftol(s) _wtol(s)
#define utftoi(s) _wtoi(s)

#define utf16len(s) wcslen(s)
#define utf16ncpy(d,s,n) wcsncpy(d,s,n)
#define utf16cpy(d,s) wcscpy(d,s)
#define utf16cmp(l,r) wcscmp(l,r)

#define utfmixcmp(l,r) wcscmp(l,r)

#if defined(LINUXMODE) & !defined(LINUX)
					/* temporary, for debug messages */
const char *printablepath(const utf_t*, int);
const char *printableutf(const utf_t*);
const char *printableutf16(const utf16_t*);
#define LSTR "s"
#define WSTR "s"
#else
		/* for debug messages, associated with formats LSTR or WSTR */
#define printablepath(p,l) (p)
#define printableutf16(p) (p)
#define printableutf(p) (p)
#define LSTR "ls"
#define WSTR "ws"
#endif

#define UTF(s) L##s
#define UTFSTR "ls"
#define LL64 "I64"

#endif /* LINUX */

#ifdef BIGENDIAN

#define bswap_16(x) ((((x) & 255) << 8) + (((x) >> 8) & 255))
#define bswap_32(x) ((((((((x) & 255L) << 8) + (((x) >> 8) & 255L)) << 8) \
                    + (((x) >> 16) & 255)) << 8) + (((x) >> 24) & 255))
extern ULONGLONG bswap_64(ULONGLONG);

#define GET_LESHORT(x) ((USHORT)bswap_16(x))
#define GET_LELONG(x) ((ULONG)bswap_32(x))
#define GET_LELONGLONG(x) bswap_64(x)
#define SET_LESHORT(x) ((LESHORT)bswap_16(x))
#define SET_LELONG(x) ((LELONG)bswap_32(x))
#define SET_LELONGLONG(x) bswap_64(x)

#else /* BIGENDIAN */

#ifndef _MYENDIANS_H   /* endians checking mode (C++) */

#define GET_LESHORT(x) ((USHORT)(x))
#define GET_LELONG(x) ((ULONG)(x))
#define GET_LELONGLONG(x) ((ULONGLONG)(x))
#define SET_LESHORT(x) ((LESHORT)(x))
#define SET_LELONG(x) ((LELONG)(x))
#define SET_LELONGLONG(x) ((LELONGLONG)(x))

#endif /* _MYENDIANS_H  */

#endif /* BIGENDIAN */

#ifdef LINUXMODE
#if WNSC | STSC | SPGC
struct _iobuf;
int safe_fprintf(struct _iobuf*, const char*, ...);
void safe_dump(struct _iobuf*, const char*, const char*, int);
#else
struct _IO_FILE;
int safe_fprintf(struct _IO_FILE*, const char*, ...);
void safe_dump(struct _IO_FILE*, const char*, const char*, int);
#endif
const char *calledfrom(void*);
#endif /* LINUXMODE */

#endif /* COMPILER_H */
