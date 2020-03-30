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

/*
 *                  Windows calls emulations
 *
 *         Do not import symbols from libntfs-3g, relay to ntfs-3g.c
 *
 *	Trace levels :
 *		1 : debug : only major errors
 *		2 : expert : major library and system calls
 *		3 : forensic : details on system calls
 *	levels 1 and 2 should be much similar to the ones on Windows
 */

#include "compiler.h"
#include "linux.h"
#include <stdio.h>
#include <string.h>
//#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
//#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#ifndef UNTHREADED
#include <pthread.h>
#endif
#include "zenwinx.h"
#include "ntfs.h"
#include "ntfs-3g.h"
#include "extrawin.h"

#if STSC
#undef USETIMEOFDAY
#else
#define USETIMEOFDAY 1
#endif

#define STACKSZ 131072 /* stack size */
#define PBUFSZ MAX_PATH  /* temporary print buffer size */

/*
 *		Declarations which should be shared hackwin/linux
 */

#define TIMEUNIT 10000000 /* number of time units (100ns) in a second */

struct TRANSLATIONS {
	struct TRANSLATIONS *greater;
	struct TRANSLATIONS *less;
	const char *source;
	utf16_t translation[1];
} ;

#ifndef UNTHREADED

struct THREADLIST {
	struct THREADLIST *next;
	PTHREAD_START_ROUTINE start;
	PVOID param;
	ULONG_PTR minstack;
	ULONG_PTR maxstack;
	int tid;
	int state;
	pthread_t thread;
} ;

#define THREADCREATED 1
#define THREADSTARTED 2
#define THREADTERMINATED 4
#define THREADCLOSED 8

struct EVENTLIST {
	struct EVENTLIST *next;
	const char *name;
} ;

#endif

void showstack(const char*);
//void zdump(FILE*, const char*, const char*, int);
void abort(void);

extern int trace;

static char *devicename; /* name of NTFS device */
static HANDLE devicehandle; /* handle to device (ntfs_volume*) */
static int devicecount = 0; /* current count of openings */
static struct TRANSLATIONS *translations = (struct TRANSLATIONS*)NULL;

#if STSC
char *getenv(const char*);
int setenv(const char*, const char*, int);
#endif

#if (LXSC | L8SC | STSC | SPGC | ARGC) & !NOMEMCHK

void *ntfs_mymalloc(size_t, const char*, int); /* extra relay desirable */
void *ntfs_mycalloc(size_t,size_t, const char*, int); /* extra relay desirable */
void *ntfs_myrealloc(void*,size_t, const char*, int); /* extra relay desirable */
void ntfs_myfree(void*, const char*, int); /* extra relay desirable */
#define malloc(sz) ntfs_mymalloc(sz,__FILE__,__LINE__)
#define calloc(sz,cnt) ntfs_mycalloc(sz,cnt,__FILE__,__LINE__)
#define realloc(old,sz) ntfs_myrealloc(old,sz,__FILE__,__LINE__)
#define free(old) ntfs_myfree(old,__FILE__,__LINE__)

#else /*  LXSC | L8SC | STSC | SPGC | ARGC */

void *ntfs_malloc(size_t);
void *ntfs_calloc(size_t);
void *ntfs_realloc(size_t,void*);
#define malloc(sz) ntfs_malloc(sz)
#define calloc(sz,cnt) ntfs_calloc(sz*cnt)
#define realloc(sz,old) ntfs_realloc(sz,old)

#endif /*  LXSC | L8SC | STSC | SPGC | ARGC */

#ifndef UNTHREADED

pthread_mutex_t loglock;
pthread_mutex_t threadlock;
pthread_mutex_t eventlock;
struct THREADLIST *threadlist;
struct EVENTLIST *eventlist;

#endif

static void undefined(const char *name)
{
	fprintf(stderr,"Undefined function %s()\n",name);
#ifdef CURSES
	curs_set(1);
	endwin();
#endif
fflush(stdout);
fflush(stderr);
#if STSC
	exit(1);
#else
	abort();
#endif
}

#define UNDEF(name) int name() { undefined(#name); return (0); }
#define WUNDEF(name) int __stdcall name() { undefined(#name); return (0); }
#define RUNDEF(name) { undefined(#name); return (0); }
#define VUNDEF(name) { undefined(#name); }

void showstack(const char *text)
{
	fprintf(stderr,"Stack at %s : 0x%lx\n",text,&text);
}

/*
 *		Allocate a temporary buffer
 *	to only be used during a printf or system call
 */

static char *nextbuf(void)
{
	static char buf[PBUFSZ*8];
	static int n = 0;

	return (&buf[(++n & 7)*PBUFSZ]);
}


const char *calledfrom(void *firstarg)
{
	static char buf[50];

#if L8SC | PPGC | SPGC | ARGC
		/* computers which do not push the return addr to stack */
	sprintf(buf,"called from undetermined");
#else
	unsigned long *p;

	p = (unsigned long*)firstarg;
	sprintf(buf,"called from 0x%lx",*--p);
#endif
	return (buf);
}

void safe_dump(FILE *f, const char *text, const char *zone, int cnt)
{
	int i,j;

#ifndef UNTHREADED
	pthread_mutex_lock(&loglock);
#endif
	fprintf(stderr,"Dump of %s %d bytes at 0x%lx",text,cnt,(long)zone);
#if STSC | PMSC | PPGC | SPGC
	fprintf(stderr," (big endian)\n");
#else
	fprintf(stderr," (little endian)\n");
#endif
#ifndef UNTHREADED
	pthread_mutex_unlock(&loglock);
#endif
	for (i=0; i<cnt; i+=16) {
#ifndef UNTHREADED
		pthread_mutex_lock(&loglock);
#endif
		fprintf(stderr,"%06lx %04x ",((long)&zone[i]) & 0xffffff,i);
		for (j=0; (j<16) && ((i+j)<cnt); j++)
			fprintf(stderr,(j & 3 ? "%02x" : " %02x"),zone[i+j] & 255);
		while (j < 18)
			fprintf(stderr,(j++ & 3 ? "  " : "   "));
		for (j=0; (j<16) && ((i+j)<cnt); j++)
			fprintf(stderr,"%c",
			    ((zone[i+j] >= ' ') && (zone[i+j] < 0x7f) ? zone[i+j] : '.'));
		fprintf(stderr,"\n");
#ifndef UNTHREADED
		pthread_mutex_unlock(&loglock);
#endif
	}
}

/*
 *		Serialized logging
 */

int safe_fprintf(FILE *f, const char *format, ...)
{
	int r;
	va_list ap;

	va_start(ap,format);
#ifdef UNTHREADED
	r = vfprintf(f,format,ap);
#else
	pthread_mutex_lock(&loglock);
	r = vfprintf(f,format,ap);
	pthread_mutex_unlock(&loglock);
#endif
	va_end(ap);
	return (r);
}

void WgxDbgPrintLastError(const char *format, ...)
{
	va_list ap;

	va_start(ap,format);
#ifdef UNTHREADED
	vfprintf(stderr,format,ap);
	fprintf(stderr,"\n");
#else
	pthread_mutex_lock(&loglock);
	vfprintf(stderr,format,ap);
	fprintf(stderr,"\n");
	pthread_mutex_unlock(&loglock);
#endif
	va_end(ap);
}

/*
 *		Time functions
 */

SLONGLONG currenttime(void)
{
	SLONGLONG units;
	struct timespec now;
#ifdef USETIMEOFDAY
	struct timeval microseconds;
   
	gettimeofday(&microseconds, (struct timezone*)NULL);
	now.tv_sec = microseconds.tv_sec;
	now.tv_nsec = microseconds.tv_usec*1000;
#else
	now.tv_sec = time((time_t*)NULL);
	now.tv_nsec = 0;
#endif
	units = (SLONGLONG)now.tv_sec * TIMEUNIT
		+ ((SLONGLONG)(369 * 365 + 89) * 24 * 3600 * TIMEUNIT)
		+ now.tv_nsec/100;
	return (units);
}

VOID NTAPI RtlTimeToTimeFields(PLARGE_INTEGER time,PTIME_FIELDS fields)
{
	DWORD days;
	DWORD millisecs;
	unsigned int year;
	int weekday;
	int mon;
	int cnt;
      
	days = time->QuadPart/(86400LL*TIMEUNIT);
	millisecs = (time->QuadPart - days*(86400LL*TIMEUNIT))/(TIMEUNIT/1000);
	weekday = (days + 1) % 7;
	year = 1601;
					/* periods of 400 years */
	cnt = days/146097;
	days -= 146097*cnt;
	year += 400*cnt;
					/* periods of 100 years */
	cnt = (3*days + 3)/109573;
	days -= 36524*cnt;
	year += 100*cnt;
					/* periods of 4 years */
	cnt = days/1461;
	days -= 1461*cnt;
	year += 4*cnt;
					/* periods of a single year */
	cnt = (3*days + 3)/1096;
	days -= 365*cnt;
	year += cnt;
      
	if ((!(year % 100) ? (year % 400) : (year % 4))
	    && (days > 58))
		days++;
	if (days > 59) {
		mon = (5*days + 161)/153;
		days -= (153*mon - 162)/5;
	} else {
		mon = days/31 + 1;
		days -= 31*(mon - 1) - 1;
	}
	fields->Year = year;
	fields->Month = mon;
	fields->Day = days;
	fields->Weekday = weekday;
	fields->Hour = millisecs / 3600000;
	fields->Minute = (millisecs / 60000) % 60;
	fields->Second = (millisecs / 1000) % 60;
	fields->Milliseconds = millisecs % 1000;
	if (trace > 2)
		safe_fprintf(stderr,"%d/%d/%d %02d:%02d:%02d wday %d\n",(int)fields->Day,(int)fields->Month,(int)fields->Year,(int)fields->Hour,(int)fields->Minute,(int)fields->Second,(int)fields->Weekday);
}

NTSTATUS NTAPI RtlSystemTimeToLocalTime(const LARGE_INTEGER *SystemTime, PLARGE_INTEGER LocalTime)
{
#if STSC
	LocalTime->QuadPart = SystemTime->QuadPart;
#else
	long days;
	time_t seconds;
	struct tm *ptm;

	LocalTime->QuadPart = SystemTime->QuadPart;
	days = SystemTime->QuadPart/(86400LL*TIMEUNIT) - 134774;
	if ((days > -24850) && (days < 24850)) {
		/* get local time the same day at 12:00 UTC */
		seconds = days*86400 + 43200;
		ptm = localtime(&seconds);
		LocalTime->QuadPart += (ptm->tm_hour - 12)*3600LL*TIMEUNIT;
	}
#endif
	return (STATUS_SUCCESS);
}

/*
 *                Translate from utf16 for outputing text.
 *        It is wise to reserve a target size at least three times
 *        the incnt to avoid overflow when converting to utf8
 */

int ntfs_toutf8(char *out, const utf16_t *in, int incnt)
{
	char *t;
	const char *cin;
	int i, size;
	utf16_t halfpair;

	halfpair = 0;
	t = out;
	cin = (char*)in;
	size = 0;
	for (i = 0; i < incnt; i++) {
	    	unsigned short c = (cin[2*i] & 255) + ((cin[2*i+1] & 255) << 8);

		if (halfpair) {
			if ((c >= 0xdc00) && (c < 0xe000)) {
				*t++ = 0xf0 + (((halfpair + 64) >> 8) & 7);
				*t++ = 0x80 + (((halfpair + 64) >> 2) & 63);
				*t++ = 0x80 + ((c >> 6) & 15) + ((halfpair & 3) << 4);
				*t++ = 0x80 + (c & 63);
				halfpair = 0;
			} else
				goto fail;
		} else if (c < 0x80) {
			*t++ = c;
	    	} else {
			if (c < 0x800) {
			   	*t++ = (0xc0 | ((c >> 6) & 0x3f));
		        	*t++ = 0x80 | (c & 0x3f);
			} else if (c < 0xd800) {
			   	*t++ = 0xe0 | (c >> 12);
			   	*t++ = 0x80 | ((c >> 6) & 0x3f);
		        	*t++ = 0x80 | (c & 0x3f);
			} else if (c < 0xdc00)
				halfpair = c;
			else if (c >= 0xe000) {
				*t++ = 0xe0 | (c >> 12);
				*t++ = 0x80 | ((c >> 6) & 0x3f);
			        *t++ = 0x80 | (c & 0x3f);
			} else goto fail;
	        }
	}
fail :
	return (t - out);
}

/*
 *	       Translate to utf16le
 *
 *   The incnt is the number of bytes (may contain a '\0')
 *   Returns the number of utf16 little endian chars.
 */
int ntfs_toutf16(utf16_t *out, const char *in, int incnt)
{
	int size;
	unsigned int code;
	char *target;
	int c;
	int i;
	enum { BASE, TWO, THREE, THREE2, THREE3, FOUR, FOUR2, FOUR3, ERR } state;

	i = 0;
	target = (char*)out;
	size = 0;
	c = 0;
	state = BASE;
	while (i < incnt) {
		c = in[i++] & 255;
		switch (state) {
		case BASE :
			if (!(c & 0x80)) {
				*target++ = c;
				*target++ = 0;
				size++;
			} else {
				if (c < 0xc2)
					state = ERR;
				else
					if (c < 0xe0) {
						code = c & 31;
						state = TWO;
					} else
						if (c < 0xf0) {
							code = c & 15;
							if (c == 0xe0)
								state = THREE2;
							else
								if (c == 0xed)
									state = THREE3;
								else
									state = THREE;
						} else
							if (c < 0xf8) {
								code = c & 7;
								state = FOUR;
							} else
								state = ERR;
			}
			break;
		case TWO :
			if ((c & 0xc0) != 0x80)
				state = ERR;
			else {
				*target++ = ((code & 3) << 6) + (c & 63);
				*target++ = ((code >> 2) & 255);
				size++;
				state = BASE;
			}
			break;
		case THREE :
			if ((c & 0xc0) != 0x80)
				state = ERR;
			else {
				code = ((code & 15) << 6) + (c & 63);
				state = TWO;
			}
			break;
		case THREE2 :
			if ((c & 0xe0) != 0xa0)
				state = ERR;
			else {
				code = ((code & 15) << 6) + (c & 63);
				state = TWO;
			}
			break;
		case THREE3 :
			if ((c & 0xe0) != 0x80)
				state = ERR;
			else {
				code = ((code & 15) << 6) + (c & 63);
				state = TWO;
			}
			break;
		case FOUR :
			if ((c & 0xc0) != 0x80)
				state = ERR;
			else {
				code = (code << 6) + (c & 63);
				state = FOUR2;
			}
			break;
		case FOUR2 :
			if ((c & 0xc0) != 0x80)
				state = ERR;
			else {
				code = (code << 6) + (c & 63);
				state = FOUR3;
			}
			break;
		case FOUR3 :
			if ((code > 0x43ff)
			 || (code < 0x400)
			 || ((c & 0xc0) != 0x80))
				state = ERR;
			else {
				*target++ = ((code - 0x400) >> 4) & 255;
				*target++ = 0xd8 + (((code - 0x400) >> 12) & 3);
				*target++ = ((code & 3) << 6) + (c & 63);
				*target++ = 0xdc + ((code >> 2) & 3);
				size += 2;
				state = BASE;
			}
			break;
		case ERR :
			break;
		}
	}
	if (state != BASE) { /* error */
		size = 0;
		*target++ = 0;
		*target++ = 0;
	}
	return (size);
}

char *_strlwr(char *strg)
{
	int i;

		/* replaces original */
	for (i=0; strg[i]; i++)
		if ((strg[i] >= 'A') && (strg[i] <= 'Z'))
			strg[i] += 'a' - 'A';
	return (strg);
}

/*
 *		Translate a constant UTF8 string to UTF16
 *
 *	The input MUST be a static string, we compare pointers to
 *	original strings.
 */

const utf16_t *UTF16(const char *in)
{
	utf16_t *out;
	struct TRANSLATIONS *current;
	struct TRANSLATIONS **link;
	static int num = 0;

	if (in) {
		if (trace > 2)
			safe_fprintf(stderr,"UTF16 %s\n",in);
		current = translations;
		link = &translations;
		if (current) {
			do {
				if (in > current->source) {
					link = &current->greater;
					current = current->greater;
				} else
					if (in < current->source) {
						link = &current->less;
						current = current->less;
					}
			} while (current && (in != current->source));
		}
		if (!current) {
			current = (struct TRANSLATIONS*)malloc
				(sizeof(struct TRANSLATIONS)
				+ 2*strlen(in));
			if (current) {
				out = current->translation;
				ntfs_toutf16(out,in,strlen(in) + 1);
				current->source = in;
				current->greater = (struct TRANSLATIONS*)NULL;
				current->less = (struct TRANSLATIONS*)NULL;
				*link = current;
				num++;
				if (trace > 2)
					safe_fprintf(stderr,"allocated num %u string %s\n",num,in);
			} else {
				out = (utf16_t*)NULL;
				safe_fprintf(stderr,"** No more memory in UTF16\n");
				get_out(1);
			}
		} else
			out = current->translation;
	} else {
		if (trace > 2)
			safe_fprintf(stderr,"UTF16 null in\n");
		out = (utf16_t*)NULL;
	}
	return (out);
}

static void freetranslation(struct TRANSLATIONS *tr)
{
	if (tr->greater) freetranslation(tr->greater);
	if (tr->less) freetranslation(tr->less);
	free(tr);
}

/*
 *		Output formatting routines
 */


/*
 *		Build a relative path from a prefix "\??\x:\"
 *	where x matches the current open device
 *	A '/' separates the directory levels, and "//" announces
 *	an inner stream.
 *
 *	result returned in a circular buffer
 */

const utf_t *printablepath(const utf_t *p, int l)
{
	char *buf;
	const char *r;
	int i;

	r = "???"; /* default return, never NULL */
	if (p) {
		if (!l)
			l = strlen(p);
		if ((l >= 7)
		    && !strncmp(p,"\\??\\",4)
		    && (p[4] >= 'A') && (p[4] <= 'Z')
		    && (p[5] == ':') && (p[6] == '\\')) {
					/* prefix : must match open device */
			if ((devicecount > 0)
			    && !strcmp(volumes[p[4]],devicename)
			    && (l < (PBUFSZ + 6))) {
				buf = nextbuf();        
				buf[0] = '/';
				for (i=7; i<l; i++)   
					buf[i-6] = p[i];
				buf[l-6] = 0;
				r = buf;
			}
		} else {
					/* no prefix, just copy */
			if (l < PBUFSZ) {
				buf = nextbuf();
				memcpy(buf,p,l);
				buf[l] = 0;
				r = buf;
			}
		}
	}
	return (r);
}

/*
 *		Translate an utf16_t text for printing (debugging)
 *	result returned in a circular buffer
 */

const char *printableutf16(const utf16_t *in)
{
	static char buf[PBUFSZ*8];
	static int n = 0;
	int sz;
	char *p;

	if (in) {
		sz = utf16len(in);
		p = &buf[(++n & 7)*PBUFSZ];
		ntfs_toutf8(p,in,sz+1);
	} else
		p = "(*null*)";
	return (p);
}

/*
 *		Translate an utf_t text for printing (debugging)
 */

const char *printableutf(const utf_t *p)
{
	return (p ? p : "(*null*)");
}

/* Note : this is not a snwprintf, it is same as snprintf */
int _snwprintf(utf_t *buf, size_t size, const utf_t *format, ...)
{
	va_list ap;
	size_t sz;

	if (strstr(format,"%ws") || strstr(format,"%ls"))
		safe_fprintf(stderr,"** Unsupported format %s %s\n",format,calledfrom(&buf));
	va_start(ap,format);
	sz = vsnprintf(buf, size, format, ap);
	if (trace > 2)
		safe_fprintf(stderr,"Returning buf %s\n",buf);
	va_end(ap);
	return (sz);
}

char *adjformat(char *target, const char *source)
{
	const char *s;
	char *t;
	char c;
	enum { BEGIN, PERCENT, I, I6, L, W } state;

	state = BEGIN;
	s = source;
	t = target;
	while ((c = *s++)) {
		switch (state) {
		case BEGIN :
			if (c == '%')
				state = PERCENT;
			*t++ = c;
			break;
		case PERCENT :
			switch (c) {
			case 'w' :
				state = W;
				break;
			case 'I' :
				state = I;
				break;
			default :
				if (((c < '0') || (c > '9'))
				    && (c != '.') || (c != '*'))
					state = BEGIN;
				*t ++ = c;
				break;
			}
		case I : 
			if (c != '6') {
				*t++ = 'I';
				*t++ = c;
				state = BEGIN;
			} else
				state = I6;
			break;
		case I6 : 
			if (c != '6') {
				*t++ = 'I';
				*t++ = '6';
				*t++ = c;
			} else {
				*t++ = 'l';
				*t++ = 'l';
			}
			state = BEGIN;
			break;
		case L :
			if (c != 's')
				*t++ = 'l';
			*t++ = c;
			state = BEGIN;
			break;
		case W :
			if (c != 's')
				*t++ = 'w';
			*t++ = c;
			state = BEGIN;
			break;
		}
	}
	return (target);
}


int _snprintf(utf_t *buf, int size, const utf_t *format, ...)
{
	va_list ap;
	int sz;
	const utf16_t *p;

	if (trace > 2)
		safe_fprintf(stderr,"_snprintf format %s allowed size %d\n",format,(int)size);
	va_start(ap,format);
	if (!strcmp(format,"%ws")) { /* just translating to utf8 */
                      /* not used any more */
		p = va_arg(ap,utf16_t*);
		sz = utf16len(p) + 1;
		if (size < 3*sz) {
			safe_fprintf(stderr,"** Unsafe translation sz %d size %d\n",(int)sz,(int)size);
		}
		sz = ntfs_toutf8(buf,p,sz);
	} else {
		if (strstr(format,"%ws") || strstr(format,"%ls"))
			safe_fprintf(stderr,"** Unsupported format %s\n",format);
		sz = vsnprintf(buf, size, format, ap);
	}
	va_end(ap);
	if (trace > 2)
		safe_fprintf(stderr,"_snprintf buf %s\n",buf);
	return (sz);
}

int _vsnprintf(char *buf, size_t size, const char * format, va_list args)
{
	size_t sz;

	if (trace > 2) {
#if ARGC
		safe_fprintf(stderr,"_vsnprintf size 0x%lx format %s args 0x%lx %s\n",size,format, &args, calledfrom(&buf));
#else
		safe_fprintf(stderr,"_vsnprintf size 0x%lx format %s args 0x%lx %s\n",size,format,(args ? *(char**)args : (char*)NULL),calledfrom(&buf));
#endif
		safe_fprintf(stderr,"%s\n",calledfrom(&buf));
	}
	if (strstr(format,"%ws") || strstr(format,"%ls")) {
		safe_fprintf(stderr,"** Undesired format %s\n",format);
		get_out(1);
	}
	sz = vsnprintf(buf,size,format,args);
	if (trace > 2)
		safe_fprintf(stderr,"returning buf %s\n",buf);
	return (sz);
}

void winx_print(char *string)
VUNDEF(winx_print)

int __cdecl winx_printf(const char *format, ...)
RUNDEF(winx_printf)

long long _atoi64(const char *s)
{
	long long v;
	BOOLEAN neg;

	v = 0;
	neg = FALSE;
	while (*s == ' ') s++;
	if (*s == '-') {
		neg = TRUE;
		s++;
	}
	while ((*s >= '0') && (*s <= '9'))
		v = v*10 + (*s++) - '0';
	if (neg) v = -v;
	return (v);
}

/*
 *		       Operations on utf16le strings
 */

/*
 *		Translate to lower case
 *	Only needed for comparing against plain ascii chars,
 *	no need to assemble a full Unicode point
 */

utf_t utflower(utf_t wc)
{
	return ((wc < 'A') || (wc > 'Z') ? wc : wc + 'a' - 'A');
}

int utf16len(const utf16_t *in)
{
	int i;

	i = 0;
	while (in[i]) i++;
	return (i);
}

utf16_t *utf16cpy(utf16_t *dst, const utf16_t *src)
{
	int i;

	i = 0;
	do {
	} while ((dst[i] = src[i]) && ++i);
	return (dst);
}

utf16_t *utf16ncpy(utf16_t *dst, const utf16_t *src, int size)
{
	int i;

	i = 0;
	if (size)
		do {
		} while ((dst[i] = src[i]) && (++i < size));
	return (dst);
}

int utf16cmp(const utf16_t *left, const utf16_t *right)
{
	unsigned int lc,rc;

	if (trace > 2) {
		safe_fprintf(stderr,"utf16cmp l %s",printableutf16(left));
		safe_fprintf(stderr," -- r %s",printableutf16(right));
	}

	do { } while (((rc = *right++) == (lc = *left++)) && rc);
#ifdef BIGENDIAN
	if (lc && rc) {
		rc = ((rc & 255) << 8) | ((rc >> 8) & 255);
		lc = ((lc & 255) << 8) | ((lc >> 8) & 255);
	}
#endif
	if (trace > 2) {
		safe_fprintf(stderr," -- lc 0x%x rc 0x%x : %d\n",lc,rc,lc-rc);
		safe_fprintf(stderr,"%s\n",calledfrom(&left));
	}
	return (lc - rc);
}

int utfmixcmp(const utf16_t *left, const utf_t *right)
{
	unsigned int lc,rc;

	if (trace > 2) {
		safe_fprintf(stderr,"utfmixcmp l %s",printableutf16(left));
		safe_fprintf(stderr," -- r %s",right);
	}
	do {
		lc = *left++;
#ifdef BIGENDIAN
		lc = ((lc & 255) << 8) | ((lc >> 8) & 255);
#endif
		rc = *right++ & 255;
		if (rc > 127) {
// TODO make it safer : first 11xxxxxx next 10xxxxxx
			if (rc < 224)
				rc = ((rc & 31) << 6) + (*right++ & 63);
			else {
				rc = ((rc & 15) << 6) + (*right++ & 63);
				rc = (rc << 6) + (*right++ & 63);
			}
		}
	} while ((rc == lc) && rc);
	if (trace > 2)
		safe_fprintf(stderr," -- lc 0x%x rc 0x%x : %d %s\n",lc,rc,lc-rc,calledfrom(&left));
	return (lc - rc);
}

/*
 *	      Temporary redefinition of standard functions
 */

char *_strupr(char *strg)
{
	int i;

		/* replaces original */
	for (i=0; strg[i]; i++)
		if ((strg[i] >= 'a') && (strg[i] <= 'z'))
			strg[i] += 'A' - 'a';
	return (strg);
}

int winx_enable_privilege(unsigned long luid)
{
	return (0);
}

#ifndef CURSES

/*
 *			Minimal console output, not using curses
 */

HANDLE WINAPI GetStdHandle(DWORD num)
{
	return (~num == 10 ? (HANDLE)stdout : (HANDLE)NULL);
}

BOOLEAN WINAPI GetConsoleScreenBufferInfo(HANDLE h, CONSOLE_SCREEN_BUFFER_INFO *csbi)
{
	csbi->dwSize.X = 80;
	csbi->dwSize.Y = 24;
	csbi->dwCursorPosition.X = 0;
	csbi->dwCursorPosition.Y = 0;
	csbi->wAttributes = 0;
	csbi->srWindow.Left = 0;
	csbi->srWindow.Top = 0;
	csbi->srWindow.Right = csbi->dwSize.X - 1;
	csbi->srWindow.Left = csbi->dwSize.Y - 1;
	return (TRUE);
}

void WINAPI close_console(HANDLE h)
{
}

BOOLEAN WINAPI SetConsoleCursorPosition(HANDLE h, COORD p)
{
//safe_fprintf(stderr,"moving to y %d x %d %s\n",p.Y,p.X,calledfrom(&h));
	return (TRUE);
}

BOOLEAN WINAPI SetConsoleTextAttribute(HANDLE h, WORD attr)
{
	return (TRUE);
}

void set_map(int i, int j, char c)
{
//	putc(c,stderr);
}

void set_message(int i, int j, int attr, const char *text)
{
//	safe_fprintf(stderr,"%s\n",text);
}

#endif

/*
 *		      Substitutes for Windows calls
 */

VOID NTAPI RtlFreeAnsiString(PANSI_STRING in)
{
		/* String allocated by RtlUnicodeStringToAnsiString() */
	if (in) {
		if (in->Buffer)
			free(in->Buffer);
		in->Buffer = (PCHAR)NULL;
		in->Length = 0;
		in->MaximumLength = 0;
	}
}

VOID NTAPI RtlInitAnsiString(PANSI_STRING out, const utf_t *in)
{
	if (out) {
		if (in) {
			out->Length = strlen(in);
				/* danger : dropping the const attribute */
			out->Buffer = (PCHAR)strchr(in,in[0]);
			out->MaximumLength = out->Length + 1; /* termination byte included */
		} else {
			out->Buffer = (PCHAR)NULL;
			out->Length = 0;
			out->MaximumLength = 0;
		}
	} else {
		safe_fprintf(stderr,"Bad args to RtlInitAnsiString\n");
		get_out(1);
	}
	if (trace > 2)
		safe_fprintf(stderr,"RtlInitAnsiString out 0x%lx buf 0x%lx mxl %d\n",(long)out,(long)out->Buffer,(int)out->MaximumLength);
}

VOID NTAPI RtlFreeUnicodeString(PUNICODE_STRING in)
{
		/* string allocated by RtlAnsiStringToUnicodeString() */
	if (in) {
		if (in->Buffer)
			free(in->Buffer);
		in->Buffer = (utf_t*)NULL;
		in->Length = 0;
		in->MaximumLength = 0;
	}
}

VOID WINAPI RtlInitUnicodeString(struct _UNICODE_STRING *out, const utf_t *in)
{
	if (out) {
		if (in) {
			out->Length = strlen(in);
				/* danger : dropping the const attribute */
			out->Buffer = (utf_t*)strchr(in,in[0]);
			out->MaximumLength = out->Length + 1; /* termination byte included */
		} else {
			out->Buffer = (utf_t*)NULL;
			out->Length = 0;
			out->MaximumLength = 0;
		}
	} else {
		safe_fprintf(stderr,"Bad args to RtlInitUnicodeString\n");
		get_out(1);
	}
	if (trace > 2)
		safe_fprintf(stderr,"RtlInitUnicodeString out 0x%lx buf 0x%lx mxl %d\n",(long)out,(long)out->Buffer,(int)out->MaximumLength);
}

NTSTATUS NTAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING out, PANSI_STRING in, SIZE_T newout)
{
	NTSTATUS r;

	r = STATUS_SUCCESS;
	if (in && out) {
		if (newout) {
			out->MaximumLength = in->Length + 1;
			out->Buffer = (utf_t*)malloc(out->MaximumLength*sizeof(utf_t));
		}
		if (out->Buffer && (out->MaximumLength >= in->Length)) {
			out->Length = in->Length;
			memcpy(out->Buffer,in->Buffer,in->Length);
		} else {
			r = STATUS_UNSUCCESSFUL;
		}
	} else {
		safe_fprintf(stderr,"Bad args to RtlAnsiStringToUnicodeString\n");
		r = STATUS_UNSUCCESSFUL;
		get_out(1);
	}
	if (trace > 2)
		safe_fprintf(stderr,"RtlAnsiStringToUnicodeString out 0x%lx buf 0x%lx mxl %d\n",(long)out,(long)out->Buffer,(int)out->MaximumLength);
	return (r);
}

NTSTATUS NTAPI RtlUnicodeStringToAnsiString(PANSI_STRING a, PUNICODE_STRING b, SIZE_T c)
RUNDEF(RtlUnicodeStringToAnsiString)

/*
 *		Memory allocation
 */

PVOID NTAPI RtlAllocateHeap(HANDLE h, SIZE_T flags, SIZE_T size)
{
	PVOID p;

	if (h) {
		p = (PVOID)malloc(size);
		if (!p)
			safe_fprintf(stderr,"** Memory allocation failed\n");
			/* proceed in order to get details */
	} else {
fprintf(stderr,"** Heap was not created\n");
		p = (PVOID)NULL;
		get_out(1);
	}
	if (trace > 2)
		safe_fprintf(stderr,"%ld bytes allocated at 0x%lx %s\n",(long)size,(long)p,calledfrom(&h));
	return (p);
}

HANDLE NTAPI RtlCreateHeap(SIZE_T a, PVOID b, SIZE_T c, SIZE_T d, PVOID e, PRTL_HEAP_DEFINITION f)
{
   	return ((HANDLE)RtlCreateHeap);
}

HANDLE NTAPI RtlDestroyHeap(HANDLE a)
{
#if (LXSC | L8SC | STSC | SPGC | ARGC) & !NOMEMCHK
	showlist(999); /* check memory leaks */
#endif
   	return ((HANDLE)NULL);
}

/* 2nd arg is ULONG on msdn */
BOOLEAN NTAPI RtlFreeHeap(HANDLE h, SIZE_T b, PVOID p)
{
	BOOLEAN r;

	r = TRUE;
	if (h) {
		if (trace > 2)
			safe_fprintf(stderr,"freeing 0x%lx\n",(long)p);
		free(p);
//domemcheck();
	} else {
fprintf(stderr,"** Heap was not allocated\n");
		r = FALSE;
		get_out(1);
	}
	return (r);
}

NTSTATUS NTAPI NtAllocateVirtualMemory(HANDLE a, PVOID *b, SIZE_T c, SIZE_T *d, SIZE_T e, SIZE_T f)
RUNDEF(NtAllocateVirtualMemory)

UNDEF(LocalFree)

NTSTATUS NTAPI NtFreeVirtualMemory(HANDLE a, PVOID *b, SIZE_T *c, SIZE_T d)
RUNDEF(NtFreeVirtualMemory)

/*
 *		Processes, threads and synchronization
 */

HANDLE WINAPI GetCurrentThread(void)
{
#ifndef UNTHREADED
	HANDLE h;
	struct THREADLIST *item;

	h = (HANDLE)NULL;
	pthread_mutex_lock(&threadlock);
	item = threadlist;
	while (item
	    && (((ULONG_PTR)&h < item->minstack)
			|| ((ULONG_PTR)&h > item->maxstack)))
		item = item->next;
	if (item)
		h = (HANDLE)item->thread;
	pthread_mutex_unlock(&threadlock);
	return (h); /* NULL in main thread */
#else
	return ((HANDLE)(DWORD_PTR)getpid());
#endif
}

BOOLEAN WINAPI SetThreadPriority(HANDLE h, int p)
{
	return (TRUE);
}

NTSTATUS NTAPI ZwTerminateThread(HANDLE h, NTSTATUS b)
{
#ifndef UNTHREADED
	NTSTATUS r;
	HANDLE th;
	HANDLE curr;
	struct THREADLIST *item;
	struct THREADLIST *previous;

	if (trace > 1)
		safe_fprintf(stderr,"ZwTerminateThread 0x%lx\n",(long)h);
	r = STATUS_UNSUCCESSFUL;
	previous = (struct THREADLIST*)NULL;
	curr = GetCurrentThread();
	if (h == (HANDLE)-2)
		th = curr;
	else
		th = h;
	if (th) {
		pthread_mutex_lock(&threadlock);
		item = threadlist;
		while (item && ((HANDLE)item->thread != th)) {
			previous = item;
			item = item->next;
		}

		if (item && ((HANDLE)item->thread == th)) {
			if (trace > 1)
				safe_fprintf(stderr,"terminating thread 0x%lx item 0x%lx\n",
						(long)th,(long)item);
			/* terminated and closed : remove from list */
			if (item->state & THREADCLOSED) {
				if (trace > 1)
					safe_fprintf(stderr,"removing thread 0x%lx\n",(long)th);
				if (previous)
					threadlist = previous->next;
				else
					threadlist = item->next;
				free(item);
				if (th == curr) {
					pthread_mutex_unlock(&threadlock);
					pthread_exit((void*)NULL);
					get_out(1); /* should not happen */
				}
			} else
				item->state |= THREADTERMINATED;
			r = STATUS_SUCCESS;
			pthread_mutex_unlock(&threadlock);
		} else {
			pthread_mutex_unlock(&threadlock);
			if (trace)
				safe_fprintf(stderr,"** Failed to terminate thread\n");
			get_out(1);
		}
	} else {
		safe_fprintf(stderr,"** Cannot terminate main thread\n");
		get_out(1);
	}
	return (r);
#else
	return (STATUS_UNSUCCESSFUL);
#endif
}

#ifndef UNTHREADED

/*
 *		Effective start of a thread
 *
 *	Collect starting parameters
 */

void *threadstart(void *arg)
{
	struct THREADLIST *item;
	static int tid = 0;

	item = (struct THREADLIST*)arg;
	if (trace > 1)
		safe_fprintf(stderr,"Thread starting item 0x%lx start 0x%lx\n",
				(long)item,(long)item->start);
	pthread_mutex_lock(&threadlock);
	item->tid = ++tid;
	item->maxstack = (ULONG_PTR)&arg; /* assume stack grows down */
	item->minstack = item->maxstack - STACKSZ;
	item->state |= THREADSTARTED;
	pthread_mutex_unlock(&threadlock);
	(*item->start)(item->param);
	ZwTerminateThread((HANDLE)item->thread,STATUS_SUCCESS);
		/*
		 * Getting here means the handle was not (yet) closed by
		 * the creating thread. So we close it, hoping it is not
		 * used any more by the creating thread.
		 * Of course this can lead to a double closing.
		 */
	if (trace > 1)
		safe_fprintf(stderr,"Thread handle was not closed\n");
	CloseHandle((HANDLE)item->thread);
	get_out(1); /* should not happen */
	return ((void*)NULL);
}

#endif

NTSTATUS NTAPI RtlCreateUserThread(HANDLE parent, PSECURITY_DESCRIPTOR psec,
				SIZE_T susp, SIZE_T zero,
				SIZE_T reserv, SIZE_T comm,
				PTHREAD_START_ROUTINE start, PVOID param,
				PHANDLE ph, PCLIENT_ID pcid)
{
#ifndef UNTHREADED
	NTSTATUS r;
	struct THREADLIST *newitem;

	r = STATUS_UNSUCCESSFUL;
	newitem = (struct THREADLIST*)malloc(sizeof(struct THREADLIST));
	if (newitem) {
		newitem->state = THREADCREATED;
		newitem->start = start;
		newitem->param = param;
		newitem->tid = 0;
		newitem->maxstack = 0;
		newitem->thread = (pthread_t)NULL;
		pthread_mutex_lock(&threadlock);
		newitem->next = threadlist;
		threadlist = newitem;
		pthread_mutex_unlock(&threadlock);
			/* start through an auxiliary routine */
		if (!pthread_create(&newitem->thread,(pthread_attr_t*)NULL,
					threadstart,newitem)) {
			if (trace > 1)
				safe_fprintf(stderr,"Thread created, handle 0x%lx item 0x%lx start 0x%lx\n",
						(long)newitem->thread,(long)newitem,(long)start);
			*ph = (PHANDLE)newitem->thread;
			if (pcid) {
				pcid->UniqueProcess = (HANDLE)NULL;
				pcid->UniqueThread = (HANDLE)newitem->thread;
			}
			r = STATUS_SUCCESS;
		}
	}
	return (r);
#else
	return (STATUS_UNSUCCESSFUL);
#endif
}

HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES psec, SIZE_T size,
			LPTHREAD_START_ROUTINE start,
                        LPVOID param, DWORD flags, LPDWORD pid)
{
#ifndef UNTHREADED
	HANDLE h;
	NTSTATUS r;
	CLIENT_ID cid;

	r = RtlCreateUserThread((HANDLE)NULL, (PSECURITY_DESCRIPTOR)NULL,
				0, 0, size, size, start, param, &h, &cid);
	if (pid && (r == STATUS_SUCCESS))
		*pid = (DWORD)(ULONG_PTR)cid.UniqueThread;
	return (r == STATUS_SUCCESS ? h : (HANDLE)NULL);
#else
	return ((HANDLE)NULL);
#endif
}

NTSTATUS NTAPI NtCreateEvent(PHANDLE ph, ACCESS_MASK b, const OBJECT_ATTRIBUTES *c, SIZE_T d, SIZE_T e)
{
if (trace > 1) fprintf(stderr,"** Skipping NtCreateEvent\n");
	*ph = (void*)NULL;
	return (STATUS_SUCCESS);
}

HANDLE WINAPI CreateEvent(LPSECURITY_ATTRIBUTES pattr, BOOLEAN reset,
				BOOLEAN initial, LPCTSTR name)
{
#ifndef UNTHREADED
	HANDLE h;
	struct EVENTLIST *event;
	struct EVENTLIST *newevent;

	h = (HANDLE)NULL;
	newevent = (struct EVENTLIST*)malloc(sizeof(struct EVENTLIST));
	if (newevent && !pattr && !reset && initial && name) {
		pthread_mutex_lock(&eventlock);
		event = eventlist;
		while (event && strcmp(event->name,name))
			event = event->next;
		if (event) {
			h = (HANDLE)event;
			errno = ERROR_ALREADY_EXISTS;
			free(newevent);
		} else {
			h = (HANDLE)newevent;
			newevent->name = name;
			newevent->next = eventlist;
			eventlist = newevent;
		}
		pthread_mutex_unlock(&eventlock);
	if (trace > 1)
		safe_fprintf(stderr,"creating event %s h 0x%lx errno %d\n",
				name,(long)h,errno);
	}
	return ((HANDLE)h);
#else
	return ((HANDLE)NULL);
#endif
}

UNDEF(_getch)

BOOLEAN __stdcall IncreaseGoogleAnalyticsCounter(const char *hostname,
                      const char *path, const char *account)
{
	return (TRUE);
}

NTSTATUS NTAPI RtlGetVersion(PRTL_OSVERSIONINFOW pinf)
{
	const char *s;
	NTSTATUS r;
	
	if (trace > 2)
		safe_fprintf(stderr,"RtlGetVersion\n");
	if (pinf) {
						/* data for Vista */
		pinf->dwOSVersionInfoSize = sizeof(PRTL_OSVERSIONINFOW);
		pinf->dwMajorVersion = 6;
		pinf->dwMinorVersion = 0;
		pinf->dwBuildNumber = 0x1772;
		pinf->dwPlatformId = 2;
		s = "Service Pack 2";
		ntfs_toutf16(pinf->szCSDVersion, s, strlen(s) + 1);
		r = STATUS_SUCCESS;
	} else {
		safe_fprintf(stderr,"Bad RtlGetVersion parameter\n");
		r = STATUS_UNSUCCESSFUL;
		get_out(1);
	}
	return (r);
}

NTSTATUS NTAPI LdrGetDllHandle(SIZE_T a,SIZE_T b, const UNICODE_STRING *c, HMODULE *d)
{
	if (trace > 2)
		safe_fprintf(stderr,"Ignoring LdrGetDllHandle\n");
	return (STATUS_SUCCESS);
}

NTSTATUS NTAPI LdrGetProcedureAddress(PVOID a, PANSI_STRING name, SIZE_T c, PVOID *pfunc)
{
	PVOID func;

	func = (PVOID)NULL;
	if (!strncmp((const char*)name->Buffer,"RtlGetVersion",12))
		func = (PVOID)RtlGetVersion;
	if (!func) {
		safe_fprintf(stderr,"** Cannot get addr of %s\n",name->Buffer);
		get_out(1);
	}
		/*
		 * Danger : returned functions are __stdcall,
		 * caller has to be aware of signature
		 */
	*pfunc = func;
	return (func ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

NTSTATUS NTAPI NtClose(HANDLE h)
{
	int r;
#ifndef UNTHREADED
	struct THREADLIST *item;
	struct THREADLIST *previous;
	struct EVENTLIST *event;
	struct EVENTLIST *previousevent;
	HANDLE current;
#endif

	if (trace > 1)
		safe_fprintf(stderr,"NtClose h 0x%lx devicecount %d\n",(long)h,devicecount);
	if (devicecount && (h == devicehandle)) {
		if (!--devicecount) {
			r = ntfs_close(h);
			free(devicename);
			devicename = (char*)NULL;
			devicehandle = (HANDLE)NULL;
		} else
			r = 0;
	} else {
#ifndef UNTHREADED
		current = (HANDLE)GetCurrentThread();
		pthread_mutex_lock(&threadlock);
		item = threadlist;
		previous = (struct THREADLIST*)NULL;
		while (item && ((HANDLE)item->thread != h)) {
			previous = item;
			item = item->next;
		}
		if (item && ((HANDLE)item->thread == h)) {
			if (trace > 1)
				safe_fprintf(stderr,"closing thread 0x%lx state 0x%lx\n",
						(long)h,(long)item->state);
			/* terminated and closed : remove from list */
			if (item->state & THREADTERMINATED) {
				if (trace > 1)
					safe_fprintf(stderr,"removing thread 0x%lx\n",(long)h);
				if (previous)
					threadlist = previous->next;
				else
					threadlist = item->next;
				free(item);
				if (h == current) {
					pthread_mutex_unlock(&threadlock);
					if (trace > 1)
						safe_fprintf(stderr,"terminating current thread 0x%lx\n",
							(long)h);
					pthread_exit((void*)NULL);
					get_out(1); /* should not happen */
				}
			}else
				item->state |= THREADCLOSED;
			r = 0;
			pthread_mutex_unlock(&threadlock);
		}else {
			pthread_mutex_unlock(&threadlock);

			pthread_mutex_lock(&eventlock);
			event = eventlist;
			previousevent = (struct EVENTLIST*)NULL;
			while (event && (event != (HANDLE)h)) {
				previousevent = event;
				event = event->next;
			}
			if (event == (HANDLE)h) {
				if (trace > 1)
					safe_fprintf(stderr,"removing event %s\n",event->name);
				if (previousevent)
					previousevent->next = event->next;
				else
					eventlist = event->next;
				free(event);
				r = 0;
				pthread_mutex_unlock(&eventlock);
			} else {
				pthread_mutex_unlock(&eventlock);

#else
		{
			{
#endif
				if (h && devicecount) {
					r = ntfs_close_file(h);
				} else {
					safe_fprintf(stderr,"** Closing closed device\n");
					get_out(1);
					r = -1;
				}
			}
		}
	}
	return (r ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS); /* return some error */
}

BOOLEAN WINAPI CloseHandle(HANDLE h)
{
	NTSTATUS r;

	r = NtClose(h);
	return (r == STATUS_SUCCESS);
}

NTSTATUS NTAPI NtDelayExecution(SIZE_T a, const LARGE_INTEGER *b)
{
	unsigned long delay;
	if (trace > 2)
		safe_fprintf(stderr,"** sleeping %lld msec\n",(long long)b->QuadPart/10000);
	if (b->QuadPart < TIMEUNIT)
		delay = 1;
	else
		delay = b->QuadPart/TIMEUNIT;
	sleep(delay);
	return (STATUS_SUCCESS);
}

VOID WINAPI Sleep(DWORD msecs)
{
	if (msecs >= 1000)
		sleep(msecs/1000);
}

NTSTATUS NTAPI NtDeleteFile(POBJECT_ATTRIBUTES in)
{
	const char *path;
	int r;

	r = -1;
	if (in) {
		path = printablepath(in->ObjectName->Buffer,in->ObjectName->Length);
		if (path) {
			if (trace > 2)
				safe_fprintf(stderr,"deleting %s\n",path);
			r = ntfs_unlink(devicehandle,path);
		} else {
			safe_fprintf(stderr,"Bad NtDeleteFile path %s\n",path);
			get_out(1);
		}
	} else {
		safe_fprintf(stderr,"Bad NtDeleteFile argument\n");
		get_out(1);
	}
	if (trace > 2)
		safe_fprintf(stderr,"NtDeleteFile %s : %s\n",
			printablepath(in->ObjectName->Buffer,in->ObjectName->Length),
			(r ? "not deleted" : "deleted"));
	return (r ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS); /* return some error */
}

NTSTATUS NTAPI NtDeviceIoControlFile(HANDLE h, HANDLE he, PIO_APC_ROUTINE pr,
                            PVOID pc, PIO_STATUS_BLOCK ps, SIZE_T cc,
                            PVOID in, SIZE_T inl, PVOID out, SIZE_T outl)
{
	NTSTATUS r;
	struct _NTFS_DATA *pdata;

	if (trace > 2) {
		safe_fprintf(stderr,"NtDeviceIoControlFile h 0x%lx he 0x%lx pr 0x%lx\n",(long)h,(long)he,(long)pr);
		safe_fprintf(stderr,"        pc 0x%lx ps 0x%lx cc 0x%lx\n",(long)pc,(long)ps,(long)cc);
		safe_fprintf(stderr,"        in 0x%lx inl %ld out 0x%lx outl %ld\n",(long)in,(long)inl,(long)out,(long)outl);
	}
	r = STATUS_UNSUCCESSFUL;
	switch (cc) {
	case 0x64 : /* FSCTL_GET_NTFS_VOLUME_DATA */
		if (devicecount && !he && !pr && !pc && !in
                     && h && ps && out && (outl == sizeof(struct _NTFS_DATA))) {
			pdata = (struct _NTFS_DATA*)out;
			if (!ntfs_fill_ntfs_data(h,
					&pdata->VolumeSerialNumber,
					&pdata->BytesPerSector,
					&pdata->MftValidDataLength)) {
				ps->Information = (ULONG_PTR)sizeof(struct _NTFS_DATA);
				r = STATUS_SUCCESS;
				}
		} else {
			safe_fprintf(stderr,"Bad NtDeviceIoControlFile parameter for controlcode 0x%ld\n",(long)cc);
			get_out(1);
		}
		break;
	case 0x78 : /* FSCTL_IS_VOLUME_DIRTY : get the volume flags */
		if (devicecount && !he && !pr && !pc && !in
		    && h && ps && out && (outl == sizeof(ULONG))
		    && !ntfs_fill_vol_flags(h,(ULONG*)out)) {
			ps->Information = (ULONG_PTR)sizeof(ULONG);
			r = STATUS_SUCCESS;
		} else {
			safe_fprintf(stderr,"Bad NtDeviceIoControlFile parameter for controlcode 0x%ld\n",(long)cc);
			get_out(1);
		}
		break;
	default :
		safe_fprintf(stderr,"Bad NtDeviceIoControlFile controlcode 0x%lx\n",(long)cc);
		r = STATUS_UNSUCCESSFUL;
		get_out(1);
	}
	ps->Status = r;
	return (r);
}

NTSTATUS NTAPI NtFlushBuffersFile(HANDLE h, PIO_STATUS_BLOCK pb)
{
	NTSTATUS r;

	r = STATUS_UNSUCCESSFUL;
	if (h && pb) {
		if (!ntfs_sync(h))
			r = STATUS_SUCCESS;
	} else {
		safe_fprintf(stderr,"Bad args to NtFlushBuffersFile\n");
		get_out(1);
	}
	pb->Information = (ULONG_PTR)0;
	pb->Status = r;
	if (trace > 2)
		safe_fprintf(stderr,"NtFlushBuffersFile : 0x%lx\n",(long)r);
	return (r);
}

/* need win data */
NTSTATUS NTAPI NtFsControlFile(HANDLE h, HANDLE he, PIO_APC_ROUTINE pr,
                            PVOID pc, PIO_STATUS_BLOCK ps, SIZE_T cc,
                            PVOID in, SIZE_T inl, PVOID out, SIZE_T outl)
// in is "device-specific information to be given to the target driver"
// out is "information returned from the target driver"
{
	NTSTATUS r;
	ULONGLONG pos;
	ULONGLONG fullsize;
	ULONGLONG inode;
	ULONG retlen;
	BOOLEAN bad = FALSE;
	int s;
	NTFS_FILE_RECORD_OUTPUT_BUFFER *pinode;
	BITMAP_DESCRIPTOR *pbitmap;
	const MOVEFILE_DESCRIPTOR *pmove;
	GET_RETRIEVAL_DESCRIPTOR *prun;
	struct _NTFS_DATA *pdata;
	int headsz;

	if (trace > 2) {
		safe_fprintf(stderr,"NtFsControlFile h 0x%lx he 0x%lx pr 0x%lx\n",(long)h,(long)he,(long)pr);
		safe_fprintf(stderr,"        pc 0x%lx ps 0x%lx cc 0x%lx\n",(long)pc,(long)ps,(long)cc);
		safe_fprintf(stderr,"        in 0x%lx inl %ld out 0x%lx outl %ld\n",(long)in,(long)inl,(long)out,(long)outl);
	}
	r = STATUS_UNSUCCESSFUL;
	bad = TRUE;
	switch (cc - 0x90000) {
	case 0x64 : /* FSCTL_GET_NTFS_VOLUME_DATA, on X64 only */
		if (devicecount && !he && !pr && !pc && !in
                     && h && ps && out && (outl == sizeof(struct _NTFS_DATA))) {
			pdata = (struct _NTFS_DATA*)out;
			if (!ntfs_fill_ntfs_data(h,
					&pdata->VolumeSerialNumber,
					&pdata->BytesPerSector,
					&pdata->MftValidDataLength)) {
				ps->Information = (ULONG_PTR)sizeof(struct _NTFS_DATA);
				r = STATUS_SUCCESS;
				}
			bad = FALSE;
		}
		ps->Status = r;
		break;
	case 0x68 :
					/* Get a file record */
		if (devicecount && !he && !pr && !pc
		     && h && ps && in && (inl == 8) && out && outl) {
			pinode = (NTFS_FILE_RECORD_OUTPUT_BUFFER*)out;
			memcpy(&inode,in,8);
                        if (trace > 2)
				safe_fprintf(stderr,"get file record for inode %lld\n",(long long)inode);
			headsz = (char*)pinode->FileRecordBuffer
				- (char*)pinode;
			if (!ntfs_fill_inode_data(h,
					pinode->FileRecordBuffer,
// danger : unaligned buffer
					outl - headsz,
					inode, &retlen)) {
//safe_dump(stderr,"inode",pinode->FileRecordBuffer,retlen);
				pinode->FileReferenceNumber = inode;
				ps->Information = retlen + headsz;
// might want to return 0xc000000d for an unused record.
				r = STATUS_SUCCESS;
			} else {
				display_error("Failed to get inode data");
				if (trace > 2)
					safe_fprintf(stderr,"** Failed to get inode %lld\n",(long long)inode);
			}
			bad = FALSE;
		}
		ps->Status = r;
		break;
	case 0x6f :
					/* Get part of bitmap */
		if (devicecount && !he && !pr && !pc
                     && h && ps && in && (inl == 8) && out && outl) {
			memcpy(&pos,in,8);
			pos = (pos >> 3); /* align to multiple of 8 */
			pbitmap = (BITMAP_DESCRIPTOR*)out;
			headsz = (char*)pbitmap->Map - (char*)pbitmap;
			if (!ntfs_fill_bitmap(h, pbitmap->Map, outl - headsz,
					pos, &fullsize, &retlen)) {
				pbitmap->ClustersToEndOfVol = fullsize - (pos << 3);
				pbitmap->StartLcn = pos << 3;
				ps->Information = (ULONG_PTR)(retlen + headsz);
				if ((retlen << 3) < pbitmap->ClustersToEndOfVol)
					r = STATUS_BUFFER_OVERFLOW;
				else
					r = STATUS_SUCCESS;
				if (trace > 2)
					safe_fprintf(stderr,"bitmap size returned %ld\n",(long)ps->Information);
				bad = FALSE;
			} else {
				display_error("Failed to get a bitmap");
				if (trace > 2)
					safe_fprintf(stderr,"** Failed to get a bitmap");
			}
		}
		ps->Status = r;
		break;
	case 0x73 :
					/* get the runlist (vcn and lcn) */
/*
 *          Format of the returned data (CPU endian)
 *                  cnt        vcn[0]       cnt is the number of runs returned
 *        0        vcn[1]      lcn[0]       size needed : 16*(cnt + 1)
 *        1        vcn[2]      lcn[1]       sparse entries have lcn = -1
 *        2        vcn[3]      lcn[2]
 *       ...       ......      ......
 *      cnt-2     vcn[cnt-1]  lcn[cnt-2]
 *      cnt-1     1st free    lcn[cnt-1]
 */
		if (devicecount && !he && !pr && !pc
                     && h && ps && in && (inl == 8) && out && outl) {
			memcpy(&pos,in,8);
			prun = (GET_RETRIEVAL_DESCRIPTOR*)out;
			headsz = (char*)prun->Pair - (char*)prun;
			s = ntfs_fill_runlist(h,pos,
					&prun->NumberOfPairs,
					prun->Pair, outl - headsz);
			if (s >= 0) {
				ps->Information =
					(ULONG_PTR)(prun->NumberOfPairs*16 + headsz);
				prun->StartVcn = pos;
				if (s)
					r = STATUS_BUFFER_OVERFLOW;
				else
					r = STATUS_SUCCESS;
				if (trace > 2) {
					safe_fprintf(stderr,"returned size %ld pairs %ld headsz %ld\n",
						(long)ps->Information,(long)prun->NumberOfPairs,(long)headsz);
					safe_dump(stderr,"runlist",(char*)out,ps->Information);
				}
			} else {
				display_error("Failed to get a runlist");
				if (trace > 2)
					safe_fprintf(stderr,"** Failed to get runlist\n");
			}
			bad = FALSE;
// apparently the size is the only returned information
// check returned size
		}
		ps->Status = r;
		break;
	case 0x74 :
					/* relocate cluster */
/*
NtDeviceIoControlFile move.c code 0x90074
Dump of NtDeviceIoControlFile input 32 bytes at 0xb3f2d8 (Windows endian)
0000  9c000000 2d000000 5e080000 00000000     ....-...^.......
0010  873e0000 00000000 c2000000 03000000     .>..............
svcn at 8, tlcn at 16, count at 24
*/
		if (devicecount && !he && !pr && !pc
                    && h && ps && in && (inl == sizeof(MOVEFILE_DESCRIPTOR))
		    && !out && !outl) {
			pmove = (const MOVEFILE_DESCRIPTOR*)in;
			if (!ntfs_move_clusters(pmove->FileHandle,
					pmove->StartVcn.QuadPart,
					pmove->TargetLcn.QuadPart,
					pmove->NumVcns)) {
				ps->Information = (ULONG_PTR)inl;
				r = STATUS_SUCCESS;
			} else {
				display_error("Failed to move clusters");
				if (trace > 2)
					safe_fprintf(stderr,"** Failed to move clusters vcn 0x%llx cnt %ld to lcn 0x%llx\n",
						(long long)pmove->StartVcn.QuadPart,
						(long)pmove->NumVcns,
						(long long)pmove->TargetLcn.QuadPart);
			}
			bad = FALSE;
		}
		ps->Status = r;
		break;
	case 0x78 : /* FSCTL_IS_VOLUME_DIRTY : get the volume flags */
		if (devicecount && !he && !pr && !pc && !in
		    && h && ps && out && (outl == sizeof(ULONG))
		    && !ntfs_fill_vol_flags(h,(ULONG*)out)) {
			ps->Information = (ULONG_PTR)sizeof(ULONG);
			r = STATUS_SUCCESS;
			bad = FALSE;
		}
		ps->Status = r;
		break;
	default :
		bad = TRUE;
		break;
	}
		/* Only wrong calls, other errors to be processed by caller */
	if (bad) {
		safe_fprintf(stderr,"Bad NtFsControlFile parameter for controlcode 0x%lx\n",(long)cc);
		r = STATUS_UNSUCCESSFUL;
		ps->Status = r;
		get_out(1);
	}
	return (r);
}


NTSTATUS NTAPI NtLoadDriver(PUNICODE_STRING a)
RUNDEF(NtLoadDriver)

NTSTATUS NTAPI NtOpenEvent(PHANDLE ph, ACCESS_MASK msk, const OBJECT_ATTRIBUTES *pattr)
{
	/* Only used to trigger an external debugger */
	*ph = (PHANDLE)NULL;
	return (STATUS_UNSUCCESSFUL);
}

NTSTATUS NTAPI NtOpenKey(PHANDLE a, ACCESS_MASK b, POBJECT_ATTRIBUTES c)
RUNDEF(NtOpenKey)

NTSTATUS NTAPI NtQueryDirectoryFile(HANDLE a, HANDLE b, PIO_APC_ROUTINE c, PVOID d, PIO_STATUS_BLOCK e, PVOID f, SIZE_T g, FILE_INFORMATION_CLASS h, SIZE_T i, PUNICODE_STRING j, SIZE_T k)
RUNDEF(NtQueryDirectoryFile)

NTSTATUS NTAPI NtQueryInformationFile(HANDLE a, PIO_STATUS_BLOCK b, PVOID c, SIZE_T d, FILE_INFORMATION_CLASS f)
RUNDEF(NtQueryInformationFile)

NTSTATUS NTAPI NtQueryInformationProcess(HANDLE a, PROCESSINFOCLASS b, PVOID c, SIZE_T d, PULONG e)
RUNDEF(NtQueryInformationProcess)

NTSTATUS NTAPI NtQuerySystemTime(PLARGE_INTEGER ptime)
{
	ptime->QuadPart = currenttime();
	return (STATUS_SUCCESS);
}

NTSTATUS NTAPI NtQueryValueKey(HANDLE a, PUNICODE_STRING b, KEY_VALUE_INFORMATION_CLASS c, PVOID d, SIZE_T e, PULONG f)
RUNDEF(NtQueryValueKey)

struct _TEB * WINAPI NtCurrentTeb(void)
{
   	static struct _TEB teb;
	static int pid;

   	pid = getpid();
	teb.ClientId.UniqueProcess = (HANDLE*)&pid;
   	return (&teb);
}

NTSTATUS NTAPI NtQueryVolumeInformationFile(HANDLE h, PIO_STATUS_BLOCK pb,
	  PVOID buf, SIZE_T sz, FS_INFORMATION_CLASS class)
{
	static char ntfs[] = { 'N', 0, 'T', 0, 'F', 0, 'S', 0 } ;
	FILE_FS_VOLUME_INFORMATION *pvi;
	FILE_FS_DEVICE_INFORMATION *pdi;
	FILE_FS_SIZE_INFORMATION *psz;
	FILE_FS_ATTRIBUTE_INFORMATION *pai;
	NTSTATUS r;

	if (trace > 2)
		safe_fprintf(stderr,"NtQueryVolumeInformationFile buf 0x%lx sz %d class %d\n",
			(long)buf,(int)sz,(int)class);
	r = STATUS_UNSUCCESSFUL;
	switch (class) {
        case FileFsVolumeInformation :
/*
Dump of volinfo4 540 bytes at 0x2507c8
0000  80ca3404 cd92ca01 56553d49 06000000     ..4.....VU=I....
0010  01007000 71003100 00000000 00000000     ..p.q.1.........
*/
		pvi = (FILE_FS_VOLUME_INFORMATION*)buf;
		if (!ntfs_fill_vol_info(h,
				&pvi->VolumeCreationTime,
			/* bytes 0x48-0x4b in cpu endianness */
				&pvi->VolumeSerialNumber,
			/* number of bytes expected */
				&pvi->VolumeLabelLength,
			/* encoded on Windows on 1 byte + alignment */
				&pvi->Unknown, /* BOOLEAN SupportsObjects, */
				pvi->VolumeLabel))
			r = STATUS_SUCCESS;
		break;
	case FileFsSizeInformation :
/*
Dump of volinfo2 24 bytes at 0x2507c8
0000  f9ef0e00 00000000 a1ae0300 00000000
0010  08000000 00020000
*/
		psz = (FILE_FS_SIZE_INFORMATION*)buf;
		if (!ntfs_fill_fs_sizes(h,
				&psz->TotalAllocationUnits,
				&psz->AvailableAllocationUnits,
				&psz->SectorsPerAllocationUnit,
				&psz->BytesPerSector))
			r = STATUS_SUCCESS;
		break;
	case FileFsAttributeInformation :
/*
Dump of volinfo3 534 bytes at 0x2507c8
0000  ff002700 ff000000 08000000 4e005400     ..'.........N.T.
0010  46005300 00000000 00000000 00000000     F.S.............
*/
		pai = (FILE_FS_ATTRIBUTE_INFORMATION*)buf;
                  /* oid, reparse, sparse, compress, acls, unicode, etc. */
		pai->FileSystemAttributes = 0x002700ff;
		pai->MaximumComponentNameLength = 255;
		pai->FileSystemNameLength = 8;
		memcpy(pai->FileSystemName,ntfs,8);
		r = STATUS_SUCCESS;
		break;
	case FileFsDeviceInformation :
		pdi = (FILE_FS_DEVICE_INFORMATION*)buf;
fprintf(stderr,"** Unsupported FileFsDeviceInformation\n");
get_out(1);
		break;
	default :
		safe_fprintf(stderr,"** Unsupported NtQueryVolumeInformationFile class %d\n",
			   (int)class);
		get_out(1);
	}
	return (r);
}

NTSTATUS NTAPI NtReadFile(HANDLE h, HANDLE he, PIO_APC_ROUTINE pr,
			PVOID pc, PIO_STATUS_BLOCK ps, PVOID buf,
			SIZE_T sz, PLARGE_INTEGER ppos, PULONG pi)
{
	NTSTATUS r;
	LONGLONG pos;

	if (trace > 2) {
		safe_fprintf(stderr,"NtReadFile h 0x%lx he 0x%lx pr 0x%lx\n",(long)h,(long)he,(long)pr);
		safe_fprintf(stderr,"        pc 0x%lx ps 0x%lx buf 0x%lx\n",(long)pc,(long)ps,(long)buf);
		safe_fprintf(stderr,"        sz 0x%lx ppos 0x%lx pi 0x%lx\n",(long)sz,(long)ppos,(long)pi);
	}
	r = STATUS_UNSUCCESSFUL;
	if (!he && !pr && !pc && !pi
	    && h && buf && sz && ppos && devicecount) {
		memcpy(&pos,&ppos->QuadPart,8); /* why is this wrongly aligned ? */
		if (!ntfs_read(h,buf,sz,pos)) {
			r = STATUS_SUCCESS;
			ps->Information = (ULONG_PTR)sz;
		if (trace > 2)
			safe_dump(stderr,"read",buf,sz);
		}
		ps->Status = r;
	} else {
		safe_fprintf(stderr,"Bad args to NtReadFile\n");
		get_out(1);
	}
	return (r);
}


NTSTATUS NTAPI NtSetEvent(HANDLE a, PULONG b)
RUNDEF(NtSetEvent)


NTSTATUS NTAPI NtUnloadDriver(PUNICODE_STRING a)
RUNDEF(NtUnloadDriver)

NTSTATUS NTAPI NtWaitForSingleObject(HANDLE h, SIZE_T sz, const LARGE_INTEGER *pdelay)
{
	if (pdelay && (trace > 1))
		fprintf(stderr,"** Skipping NtWaitForSingleObject h 0x%lx sz %ld delay %lld\n",(long)h,(long)sz,(long long)pdelay->QuadPart);
	return (STATUS_SUCCESS);
}

NTSTATUS NTAPI NtWriteFile(HANDLE h, HANDLE he, PIO_APC_ROUTINE pr,
			PVOID pc, PIO_STATUS_BLOCK ps, PVOID buf,
			SIZE_T sz, PLARGE_INTEGER ppos, PULONG pi)
{
	NTSTATUS r;
	ULONGLONG pos;

	if (trace > 2) {
		safe_fprintf(stderr,"NtWriteFile h 0x%lx he 0x%lx pr 0x%lx\n",(long)h,(long)he,(long)pr);
		safe_fprintf(stderr,"        pc 0x%lx ps 0x%lx buf 0x%lx\n",(long)pc,(long)ps,(long)buf);
		safe_fprintf(stderr,"        sz 0x%lx ppos 0x%lx pi 0x%lx\n",(long)sz,(long)ppos,(long)pi);
	}
	r = STATUS_UNSUCCESSFUL;
	if (!he && !pr && !pc && !pi
//	    && h && buf && sz && ppos && devicecount) {
	    && h && buf && ppos && devicecount) {
		memcpy(&pos,&ppos->QuadPart,8);
		if (h == devicehandle) {
safe_dump(stderr,"writing to device",buf,sz);
get_out(1);
			if (!ntfs_write(h,buf,sz,pos)) {
				r = STATUS_SUCCESS;
				ps->Information = (ULONG_PTR)sz;
			}
		} else {
			if (trace > 2)
				safe_dump(stderr,"writing to file",buf,sz);
//			if (!ntfs_write_file(h,buf,sz,pos)) {
			if (!sz || !ntfs_write_file(h,buf,sz,pos)) {
				r = STATUS_SUCCESS;
				ps->Information = (ULONG_PTR)sz;
			}
		}
		ps->Status = r;
	} else {
		safe_fprintf(stderr,"Bad args to NtWriteFile\n");
		get_out(1);
	}
	return (r);
}


VOID WINAPI RtlZeroMemory(VOID *Destination, SIZE_T Length)
{
	if (trace > 2)
		safe_fprintf(stderr,"memset 0x%lx lth %ld %s\n",(long)Destination,(long)Length,calledfrom(&Destination));
   	memset(Destination, 0, Length);
}

BOOLEAN WINAPI SetConsoleWindowInfo(HANDLE h, BOOLEAN abs, const SMALL_RECT*pr)
RUNDEF(SetConsoleWindowInfo)

UNDEF(udefrag_fbsize)
UNDEF(udefrag_toupper)

UNDEF(_wcsupr)

utf16_t * WINAPI GetCommandLineW(void)
   {
   undefined("GetCommandLineW");
   return (NULL);
   }

NTSTATUS NTAPI NtOpenSection(HANDLE *a, ACCESS_MASK b, const OBJECT_ATTRIBUTES *c)
RUNDEF(NtOpenSection)

NTSTATUS NTAPI NtUnmapViewOfSection(HANDLE h, PVOID p)
RUNDEF(NtUnmapViewOfSection)

NTSTATUS NTAPI NtMapViewOfSection(HANDLE a, HANDLE b, PVOID *c, SIZE_T d, SIZE_T e, const LARGE_INTEGER *f, SIZE_T *g, SECTION_INHERIT h, SIZE_T i, SIZE_T j)
RUNDEF(NtMapViewOfSection)

BOOLEAN WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE routine, BOOLEAN add)
{
#if STSC
	if (trace > 2)
		fprintf(stderr,"Skipping SetConsoleCtrlHandler\n");
	return (TRUE);
#else
	typedef void (*signalroutine)(int);
	signalroutine oldroutine;

	if (add)
		oldroutine = signal(SIGINT, (signalroutine)routine);
	else
		oldroutine = signal(SIGINT, SIG_DFL);
	if (trace > 2) {
		if (add)
			safe_fprintf(stderr,"set ctrl-c to 0x%lx old 0x%lx\n",(long)routine,(long)oldroutine);
		else
			safe_fprintf(stderr,"removed ctrl-c 0x%lx\n",(long)oldroutine);
	}
	return (oldroutine != SIG_ERR);
#endif
}

DWORD WINAPI GetEnvironmentVariableW(const utf_t *name, utf_t *buf, DWORD size)
{
	char *p;
	DWORD sz;

	sz = 0;
	if (name && buf && size) {
		p = getenv(name);
		if (p) {
			sz = strlen(p);
			if (sz < size) {
				strcpy(buf,p);
				errno = 0;
			} else
				sz = 0;
		} else {
			errno = ERROR_ENVVAR_NOT_FOUND;
			sz = 0;
		}
		buf[sz] = 0;
	}
	return (sz);
}

BOOLEAN WINAPI SetEnvironmentVariableW(const utf_t *name, const utf_t *value)
{
	int r;

	r = setenv(name,value,1);
	if (trace > 2)
		safe_fprintf(stderr,"setenv name [%s] value [%s] : %d\n",
			name,value,r);
	return (!r);
}

BOOLEAN WINAPI SetEnvironmentVariableA(const char *name, const char *value)
{
	int r;

	r = setenv(name,value,1);
	if (trace > 2)
		safe_fprintf(stderr,"setenv name [%s] value [%s] : %d\n",
			name,value,r);
	return (!r);
}

NTSTATUS NTAPI RtlQueryEnvironmentVariable_U(PWSTR a, PUNICODE_STRING in, PUNICODE_STRING out)
{
	char *name;
	const char *p;
	NTSTATUS r;

	r = STATUS_UNSUCCESSFUL;
	if (in && out && (in->Length < PBUFSZ)) {
		name = nextbuf();
		memcpy(name,in->Buffer,in->Length);
		name[in->Length] = 0;
		p = getenv(name);
		if (p) {
			if (trace > 2)
				safe_fprintf(stderr,"querying %s, got %s\n",name,p);
			if (strlen(p) <= out->MaximumLength) {
				out->Length = strlen(p);
				memcpy(out->Buffer,p,out->Length);
				r = STATUS_SUCCESS;
			} else {
fprintf(stderr,"** Env too long\n");
				get_out(1);
			}
		} else {
			errno = ERROR_ENVVAR_NOT_FOUND;
			out->Length = 0;
			if (trace > 2)
				safe_fprintf(stderr,"querying %s, was not set\n",name);
		}
	} else {
		safe_fprintf(stderr,"Bad args to RtlQueryEnvironmentVariable_U\n");
		get_out(1);
	}
	return (r);
}

NTSTATUS NTAPI RtlSetEnvironmentVariable(PWSTR env,
				PUNICODE_STRING name, PUNICODE_STRING value)
{
	char *n;
	char *v;
	NTSTATUS r;

	r = STATUS_UNSUCCESSFUL;
	if (name && value
	    && (name->Length < PBUFSZ) && (value->Length < PBUFSZ)) {
		n = nextbuf();
		memcpy(n,name->Buffer,name->Length);
		n[name->Length] = 0;
		v = nextbuf();
		memcpy(v,value->Buffer,value->Length);
		v[value->Length] = 0;

		if (setenv(n,v,1))
			r = STATUS_SUCCESS;
		if (trace > 2)
			safe_fprintf(stderr,"setenv name [%s] value [%s] : 0x%lx\n",
				n,v,r);
	}
	return (r);
}

DWORD WINAPI GetLastError(void)
{
	int old = errno;

		/* using errno, thread safe */
	errno = 0;
	return (old);
}

DWORD WINAPI FormatMessageA(DWORD flg, LPCVOID src, DWORD idmess,
                 DWORD idlang, LPSTR buf, DWORD sz, va_list *args)
//DWORD WINAPI FormatMessageA(long, char*, long, long, char*, long, va_list*)
{
	return (-1);  /* return an error */
}

DWORD WINAPI FormatMessage(DWORD flg, LPCVOID src, DWORD idmess,
                 DWORD idlang, LPSTR buf, DWORD sz, va_list *args)
//DWORD WINAPI FormatMessage(long, char*, long, long, char*, long, va_list*)
{
	return (-1);  /* return an error */
}

NTSTATUS NTAPI xNtCreateFile(PHANDLE ph, ACCESS_MASK acc, POBJECT_ATTRIBUTES p,
			PIO_STATUS_BLOCK s, PLARGE_INTEGER i, SIZE_T attr,
			SIZE_T share, SIZE_T d, SIZE_T e, PVOID q, SIZE_T f,
const char *file, int line)
{
	const char *filename;

// TODO check all arguments
	if (trace > 1) {
		safe_fprintf(stderr,"NtCreateFile phandle 0x%lx acc 0x%lx attr 0x%lx from %s line %d\n",(long)ph,(long)acc,(long)attr,file,line);
		safe_fprintf(stderr,"     name %s devicecount %d\n",printablepath(p->ObjectName->Buffer,p->ObjectName->Length),devicecount);
	}
	if (devicecount) {
				/* device already open */
		if ((strlen(devicename) == p->ObjectName->Length)
		   && !memcmp(devicename,p->ObjectName->Buffer,p->ObjectName->Length)) {
			devicecount++;
			*ph = devicehandle;
			if (trace > 2)
				safe_fprintf(stderr,"Reopening %s handle 0x%lx\n",
						devicename,devicehandle);
		} else {
			filename = printablepath(p->ObjectName->Buffer,p->ObjectName->Length);
// TODO allow any length (remove prefix, insert terminator)
			if (filename) {
				if (acc & (FILE_GENERIC_WRITE | FILE_APPEND_DATA))
					*ph = ntfs_create_file(devicehandle,filename);
				else
					*ph = ntfs_open_file(devicehandle,filename);
			} else {
				safe_fprintf(stderr,"** Opening several devices\n");
				*ph = (HANDLE)NULL;
				get_out(1);
			}
		}
	} else {
		devicename = (char*)malloc(p->ObjectName->Length + 1);
		if (devicename) {
			memcpy(devicename,p->ObjectName->Buffer,p->ObjectName->Length);
			devicename[p->ObjectName->Length] = 0;
			*ph = (HANDLE)ntfs_open(devicename);
			if (*ph) {
				if (trace > 2)
					safe_fprintf(stderr,"NTFS device opened, handle 0x%lx\n",(long)*ph);
				devicecount++;
				devicehandle = *ph;
			}
		} else {
			safe_fprintf(stderr,"** Failed to allocate %u bytes for device name\n",
				(unsigned int)p->ObjectName->Length);
			*ph = (HANDLE)NULL;
			get_out(1);
		}
	}
	return (*ph ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL); /* return some error */
}

DWORD MAKELANGID(DWORD lang, DWORD sublang)
{
	return (0);
}

NTSTATUS NTAPI NtSetInformationProcess(HANDLE a, PROCESS_INFORMATION_CLASS b,PVOID c, SIZE_T d)
{
	return (STATUS_SUCCESS);
}

VOID NTAPI DbgBreakPoint(VOID)
VUNDEF(DbgBreakPoint)

PRTL_USER_PROCESS_PARAMETERS NTAPI RtlNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS *pp)
{
	return (pp);
}

void initwincalls(void)
{
#ifndef UNTHREADED
	threadlist = (struct THREADLIST*)NULL;
	eventlist = (struct EVENTLIST*)NULL;
	pthread_mutex_init(&loglock,(pthread_mutexattr_t*)NULL);
	pthread_mutex_init(&threadlock,(pthread_mutexattr_t*)NULL);
	pthread_mutex_init(&eventlock,(pthread_mutexattr_t*)NULL);
#endif
}

void endwincalls(void)
{
	if (translations)
		freetranslation(translations);
	translations = (struct TRANSLATIONS*)NULL;
}

#if STSC | SPGC

UNDEF(_wcslwr)
UNDEF(wcscat)
UNDEF(wcschr)

#endif

#if 0

/*
 *		Functions which are probably not needed any more
 *	(kept here, just in case)
 */

utf16_t WINAPI **CommandLineToArgvW(const utf16_t *xargv, const int *xargc)
   {
   undefined("CommandLineToArgvW");
   return (NULL);
   }

WUNDEF(GetFullPathNameW)
WUNDEF(GetProcAddress)
WUNDEF(GlobalFree)

HANDLE WINAPI LoadLibrary(const char *name)
   {
   undefined("LoadLibrary");
   return (NULL);
   }

NTSTATUS NTAPI NtCancelIoFile(HANDLE a, PIO_STATUS_BLOCK b)
RUNDEF(NtCancelIoFile)

NTSTATUS NTAPI NtDisplayString(PUNICODE_STRING in)
{
	char buf[256];
	NTSTATUS r;

	r = STATUS_UNSUCCESSFUL;
	if (in && (in->Length < 256)) {
		memcpy(buf,in->Buffer,in->Length);
		buf[in->Length] = 0;
		safe_fprintf(stderr,"console : %s",buf);
		r = STATUS_SUCCESS;
	} else {
		safe_fprintf(stderr,"Bad args to NtDisplayString\n");
		get_out(1);
	}
	return (r);
}

NTSTATUS NTAPI NtFlushKey(HANDLE KeyHandle)
RUNDEF(NtFlushKey)


NTSTATUS NTAPI NtOpenSymbolicLinkObject(PHANDLE a, ACCESS_MASK b, POBJECT_ATTRIBUTES d)
RUNDEF(NtOpenSymbolicLinkObject)

NTSTATUS NTAPI NtQueryPerformanceCounter(PLARGE_INTEGER pval, PLARGE_INTEGER pfreq)
/* danger : not documented as NTAPI on msdn */
{
	if (pval)
		pval->QuadPart = 0;
	if (pfreq)
		pfreq->QuadPart = 0;
	return (STATUS_SUCCESS);
}

NTSTATUS NTAPI NtQuerySymbolicLinkObject(HANDLE a, PUNICODE_STRING b, PULONG c)
RUNDEF(NtQuerySymbolicLinkObject)

NTSTATUS NTAPI NtSetValueKey(HANDLE a, PUNICODE_STRING b, SIZE_T c, SIZE_T d, PVOID e, SIZE_T f)
RUNDEF(NtSetValueKey)

NTSTATUS NTAPI NtShutdownSystem(SHUTDOWN_ACTION a)
RUNDEF(NtShutdownSystem)

NTSTATUS NTAPI NtTerminateProcess(HANDLE a, SIZE_T b)
RUNDEF(NtTerminateProcess)

NTSTATUS NTAPI RtlAdjustPrivilege(SIZE_T Id, SIZE_T Enable, SIZE_T ForCurrentThread, SIZE_T *WasEnabled)
{
	if (trace > 2)
		fprintf(stderr,"** Skipping RtlAdjustPrivilege\n");
	return (STATUS_SUCCESS);
}

PRTL_USER_PROCESS_PARAMETERS NTAPI RtlNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS *a)
RUNDEF(RtlNormalizeProcessParams)

WUNDEF(SetConsoleTextAttribute)
WUNDEF(Sleep)

UNDEF(WriteConsoleW)

#endif /* not needed any more */
