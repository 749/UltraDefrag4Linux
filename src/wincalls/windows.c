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
 *	Extra code for Windows running Posix threads and curses
 *
 *	This is not needed when using Windows native threads and display
 */

#include "compiler.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include "extrawin.h"

/*
 *  Currently    CURSES implies linemode
 *               threaded + linemode implies POSIX
 *  So force POSIX if CURSES and not UNTHREADED
 */

#if defined(CURSES) & !defined(UNTHREADED)
#define POSIX 1 /* use POSIX-type synchronizations instead of the native ones */
#else
#undef POSIX
#endif

#if defined(POSIX) & !defined(UNTHREADED)
#include <pthread.h>
#endif

#define UNDEF(name) int name() { undefined(#name); return (0); }
#define WUNDEF(name) int __stdcall name() { undefined(#name); return (0); }
#define RUNDEF(name) { undefined(#name); return (0); }
#define VUNDEF(name) { undefined(#name); }

#define get_out(n) exit(n)
#define pthread_exit(p) pthread_mutex_lock(&mainlock); /* stop there ! */

#define PBUFSZ MAX_PATH  /* temporary print buffer size */
#define STACKSZ 131072

/*
 *                   A few defines for some special situations
 *
 *            probably better include ntndk.h
 */

typedef long NTSTATUS;
typedef int EVENT_TYPE;
#define STATUS_SUCCESS 0
#define STATUS_UNSUCCESSFUL           ((NTSTATUS)0xC0000001)
#define NTAPI __stdcall

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
  ULONG           Length;
  HANDLE          RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG           Attributes;
  PVOID           SecurityDescriptor;
  PVOID           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;


/*
 *		Declarations which should be shared winhack/linux
 */

struct TRANSLATIONS {
	struct TRANSLATIONS *greater;
	struct TRANSLATIONS *less;
	const char *source;
	utf16_t translation[1];
} ;

void safe_dump(FILE*, const char*, const char*, int);
void showstack(const char*);

extern int trace;

#if defined(POSIX) & !defined(UNTHREADED)

struct THREADLIST {
	struct THREADLIST *next;
	PTHREAD_START_ROUTINE start;
	PVOID param;
	ULONG_PTR minstack;
	ULONG_PTR maxstack;
	int state;
	int tid;
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

pthread_mutex_t loglock;
pthread_mutex_t threadlock;
pthread_mutex_t eventlock;
pthread_mutex_t mainlock;
struct THREADLIST *threadlist;
struct EVENTLIST *eventlist;
#endif

static struct TRANSLATIONS *translations = (struct TRANSLATIONS*)NULL;

static void undefined(const char *name)
   {
   fprintf(stderr,"Undefined function %s()\n",name);
   exit(1);
   }

void showstack(const char *text)
{
	fprintf(stderr,"Stack at %s : 0x%lx\n",text,&text);
}

static void initwincalls(void)
{
	static int inited = 0;

#if defined(POSIX) & !defined(UNTHREADED)
	if (!inited++) {
		threadlist = (struct THREADLIST*)NULL;
		eventlist = (struct EVENTLIST*)NULL;
		pthread_mutex_init(&loglock,(pthread_mutexattr_t*)NULL);
		pthread_mutex_init(&threadlock,(pthread_mutexattr_t*)NULL);
		pthread_mutex_init(&eventlock,(pthread_mutexattr_t*)NULL);
			/* prevent other threads to exit */
		pthread_mutex_init(&mainlock,(pthread_mutexattr_t*)NULL);
		pthread_mutex_lock(&mainlock);
	}
#endif
}

void safe_dump(FILE *f, const char *text, const char *zone, int cnt)
{
	int i,j;

#if defined(POSIX) & !defined(UNTHREADED)
	pthread_mutex_lock(&loglock);
#endif
	fprintf(stderr,"Dump of %s %d bytes at 0x%lx",text,cnt,(long)zone);
	fprintf(stderr," (Windows endian)\n");
#if defined(POSIX) & !defined(UNTHREADED)
	pthread_mutex_unlock(&loglock);
#endif
	for (i=0; i<cnt; i+=16) {
#if defined(POSIX) & !defined(UNTHREADED)
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
#if defined(POSIX) & !defined(UNTHREADED)
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
#if defined(POSIX) & !defined(UNTHREADED)
	pthread_mutex_lock(&loglock);
	r = vfprintf(f,format,ap);
	pthread_mutex_unlock(&loglock);
#else
	r = vfprintf(f,format,ap);
#endif
	va_end(ap);
	return (r);
}

void WgxDbgPrintLastError(const char *format, ...)
{
	va_list ap;

	va_start(ap,format);
#if defined(POSIX) & !defined(UNTHREADED)
	pthread_mutex_lock(&loglock);
	vfprintf(stderr,format,ap);
	fprintf(stderr,"\n");
	pthread_mutex_unlock(&loglock);
#else
	vfprintf(stderr,format,ap);
	fprintf(stderr,"\n");
#endif
	va_end(ap);
}

UNDEF(_cputs)

int optopt;
char *optarg;
int optind;

int getopt_long(int argc, char * const *argv, const char *sopt,
               const struct option *lopt, int *v)
   {
   int opt;
   const char *ptr;
   int sz;
   const struct option *pt;
   const char *ptr2;
   const char *ptxt;
   static int xargc = 0;
   static int iarg = 0;

   if (!xargc) initwincalls(); /* use this opportunity to initialize */
   if (!iarg) xargc++;
   if (xargc < argc)
      {
      optarg = argv[xargc];
      ptr = &argv[xargc][iarg];
      if (!iarg && (ptr[0] == '-') && (ptr[1] == '-') && lopt)
         {
         pt = lopt;
         ptxt = pt->name; // pt->lopt;
         while (ptxt
              && strncmp(ptxt,&ptr[2],strlen(ptxt)))
            {
            pt++;
            ptxt = pt->name; // pt->lopt;
            }
         opt = -1; /* default return */
         if (ptxt)
            {
            sz = strlen(ptxt);
            if (pt->has_arg) // pt->args)
               {  /* argument expected */
               if (ptr[sz + 2] == '=')
                  {
                  optarg = &argv[xargc][sz + 3];
                  }
               else
                  if ((xargc + 1) < argc) /* isolated argument */
                     {
                     ptr = argv[xargc + 1];
                     if (*ptr != '-')
                        {
                        xargc++;
                        optarg = argv[xargc];
                        }
                     }
               opt = pt->val; // pt->sopt;
               }
            else
               if (!ptr[sz + 2] && !pt->has_arg) // !pt->args)
                  {  /* no arg and none expected */
                  opt = pt->val; // pt->sopt;
                  }
            }
         iarg = 0;
         }
      else
         if ((iarg || (ptr[0] == '-'))
           && (((ptr[1] >= 'a') && (ptr[1] <= 'z'))
             || ((ptr[1] >= 'A') && (ptr[1] <= 'Z'))
             || ((ptr[1] >= '0') && (ptr[1] <= '9')))
           && sopt)
            {
            opt = ptr[1];
            ptr2 = strchr(sopt,ptr[1]);
            if (ptr2 && (ptr2[1] == ':'))
               {
               iarg = 0;
               if (ptr[2])  /* argument appended */
                  {
                  optarg = strchr(&ptr[2],ptr[2]); /* this is &ptr[2]; */
                  }
               else
                  if ((xargc + 1) < argc) /* isolated argument */
                     {
                     ptr = argv[xargc + 1];
                     if (*ptr != '-')
                        {
                        xargc++;
                        optarg = argv[xargc];
                        }
                     }
               }
            else /* no argument expected, another option may follow */
               if (ptr[2]) iarg++;
               else iarg = 0;
            }
         else
            opt = -1; /* this is not an option */
      optind = xargc + 1;
      }
   else opt = -1;
   optopt = 0; /* todo... */
   return (opt);
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
 *             Translate to utf16le
 *
 *   The incnt is the number of bytes (may contain a '\0')
 *   Returns the number of utf16 little endian chars.
 */
int ntfs_toutf16(utf16_t *out, const char *in, int incnt)
   {
   int i;
   int outcnt;
   unsigned int c;
   char *p;

   i = 0;
   outcnt = 0;
   p = (char*)out;
   while (i < incnt)
      {
      c = in[i++] & 255;
      if (c > 127)
         {
         if (c < 224)
            c = ((c & 31) << 6) + (in[i++] & 63);
         else
            {
            c = ((c & 15) << 6) + (in[i++] & 63);
            c = (c << 6) + (in[i++] & 63);
            }
         }
      *p++ = c & 255;
      *p++ = (c >> 8) & 255;
      outcnt++;
      }
   return (outcnt);
   }

const char *printableutf16(const utf16_t *in)
{
	static char buf[PBUFSZ*8];
	static int n = 0;
	int sz;
	char *p;

	if (in) {
		sz = utflen(in);
		p = &buf[(++n & 7)*PBUFSZ];
		ntfs_toutf8(p,in,sz+1);
	} else
		p = "(*null*)";
	return (p);
}

const char *printableutf(const utf16_t *in)
{
	return (printableutf16(in));
}

const char *printablepath(const utf16_t *path, int lth)
{
	return (printableutf16(path));
}

BOOLEAN WINAPI IncreaseGoogleAnalyticsCounter(const char *hostname,
                      const char *path, const char *account)
{
	return (TRUE);
}

wchar_t *_wcsupr(wchar_t *p)
RUNDEF(_wcsupr)

int _snwprintf(wchar_t *buf, size_t sz, const wchar_t *format, ...)
{
	va_list ap;
	int r;

	va_start(ap,format);
	r = vsnwprintf(buf,sz,format,ap);
	va_end(ap);
	return (r);
}

UNDEF(_kbhit)
UNDEF(udefrag_toupper)
UNDEF(_inp)
UNDEF(_outpw)

utf16_t ** WINAPI CommandLineToArgvW(const utf16_t *a, const int *b)
RUNDEF(CommandLineToArgvW)

UNDEF(_getch)
WUNDEF(udefrag_fbsize)
UNDEF(_cgets)
UNDEF(_putch)
UNDEF(_outp)
UNDEF(_getche)

int _snprintf(char *buf, size_t sz, const char *format, ...)
{
	va_list ap;
	int r;

	va_start(ap,format);
	r = vsnprintf(buf,sz,format,ap);
	va_end(ap);
	return (r);
}

UNDEF(_inpw)
UNDEF(_ungetch)

int _vsnprintf(char *buf, size_t sz, const char * format, va_list args)
{
	int r;

	vsnprintf(buf,sz,format,args);
	return (r);
}

char *_strupr(char *strg)
{
	int i;

		/* replaces original */
	for (i=0; strg[i]; i++)
		if ((strg[i] >= 'a') && (strg[i] <= 'z'))
			strg[i] += 'A' - 'a';
	return (strg);
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

long _wtol(const utf16_t *s)
{
	long v;
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

int _wtoi(const utf16_t *s)
{
	return (_wtol(s));
}

#ifdef POSIX

/*
 *		Processes, threads and synchronization, the Posix way
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
	return ((HANDLE)getpid());
#endif
}

BOOL WINAPI SetThreadPriority(HANDLE h, int p)
{
	return (TRUE);
}

#ifndef UNTHREADED

NTSTATUS NTAPI ZwTerminateThread(HANDLE h, NTSTATUS b)
{
#ifndef UNTHREADED
	NTSTATUS r;
	HANDLE th;
	HANDLE curr;
	struct THREADLIST *item;
	struct THREADLIST *previous;

	if (trace > 2)
		safe_fprintf(stderr,"ZwTerminateThread 0x%lx pid %d\n",(long)h);
	r = STATUS_UNSUCCESSFUL;
	previous = (struct THREADLIST*)NULL;
	curr = GetCurrentThread();
	if (h = (HANDLE)-2)
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
			if (trace > 2)
				safe_fprintf(stderr,"terminating thread 0x%lx item 0x%lx\n",
					(long)h,(long)item);
		/* terminated and closed : remove from list */
			if (item->state & THREADCLOSED) {
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
			if (trace > 2)
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
	if (trace > 2)
		safe_fprintf(stderr,"Thread starting item 0x%lx start 0x%lx\n",
				(long)item,(long)item->start);
	pthread_mutex_lock(&threadlock);
	item->tid = ++tid;
	item->maxstack = (ULONG_PTR)&item; /* assume stack grows down */
	item->minstack = item->maxstack - STACKSZ; /* assume stack grows down */
	item->state |= THREADSTARTED;
	pthread_mutex_unlock(&threadlock);
	(*item->start)(item->param);
	if (trace > 2)
		safe_fprintf(stderr,"thread 0x%lx %d done\n",(long)item->thread,item->tid);
	ZwTerminateThread((HANDLE)item->thread,STATUS_SUCCESS);
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
	pthread_t thread;
	struct THREADLIST *newitem;
	typedef void *(*start_routine)(void*);

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
		if (!pthread_create(&thread,(pthread_attr_t*)NULL,
				(start_routine)threadstart,newitem)) {
			if (trace > 2)
				safe_fprintf(stderr,"Thread created, handle 0x%lx item 0x%lx\n",
						(long)thread,(long)newitem);
			newitem->thread = thread;
			*ph = (PHANDLE)thread;
			if (pcid) {
				pcid->UniqueProcess = (HANDLE)NULL;
				pcid->UniqueThread = (HANDLE)thread;
			}
			r = STATUS_SUCCESS;
		}
	}
	return (r);
#else
	return (STATUS_UNSUCCESSFUL);
#endif
}

HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES psec, DWORD size,
			LPTHREAD_START_ROUTINE start,
                        LPVOID param, DWORD flags, LPDWORD pid)
{
#ifndef UNTHREADED
	HANDLE h;
	NTSTATUS r;
	CLIENT_ID,cid;

	r = RtlCreateUserThread((HANDLE)NULL, (PSECURITY_DESCRIPTOR)NULL,
				0, 0, size, size, start, param, &h, &cid);
	if (pid && (r == STATUS_SUCCESS))
		*pid = (DWORD)(ULONG_PTR)cid.UniqueThread;
	return (r == STATUS_SUCCESS ? h : (HANDLE)NULL);
#else
	return ((HANDLE)NULL);
#endif
}

HANDLE WINAPI CreateEvent(LPSECURITY_ATTRIBUTES pattr, BOOL reset,
				BOOL initial, LPCTSTR name)
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
	}
	return ((HANDLE)h);
#else
	return ((HANDLE)NULL);
#endif
}

NTSTATUS NTAPI NtCreateEvent(PHANDLE ph, ACCESS_MASK b, const OBJECT_ATTRIBUTES *c, SIZE_T d, SIZE_T e)
{
	initwincalls();
	if (trace > 2)
		safe_fprintf(stderr,"** Skipping NtCreateEvent\n");
	*ph = (void*)NULL;
	return (STATUS_SUCCESS);
}


NTSTATUS NTAPI NtClose(HANDLE h)
{
	NTSTATUS r;
	typedef NTSTATUS NTAPI (*tNtClose)(HANDLE);
	tNtClose pNtClose = (tNtClose)NULL;
	HANDLE ntdll;
#ifndef UNTHREADED
	struct THREADLIST *item;
	struct THREADLIST *previous;
	struct EVENTLIST *event;
	struct EVENTLIST *previousevent;
#endif

	if (trace > 2)
		safe_fprintf(stderr,"NtClose h 0x%lx\n",(long)h);
	r = STATUS_UNSUCCESSFUL;
	if (!pNtClose) {
		ntdll = LoadLibrary("ntdll.dll");
		pNtClose = (tNtClose)GetProcAddress(ntdll,"NtClose");
	}
#ifndef UNTHREADED
	pthread_mutex_lock(&threadlock);
	item = threadlist;
	previous = (struct THREADLIST*)NULL;
	while (item && ((HANDLE)item->thread != h)) {
		previous = item;
		item = item->next;
	}
	if (item && ((HANDLE)item->thread == h)) {
		/* terminated and closed : remove from list */
		if (item->state & THREADTERMINATED) {
			if (previous)
				threadlist = previous->next;
			else
				threadlist = item->next;
			free(item);
		}else
			item->state |= THREADCLOSED;
		r = STATUS_SUCCESS;
		pthread_mutex_unlock(&threadlock);
	} else {
		pthread_mutex_unlock(&threadlock);

		pthread_mutex_lock(&eventlock);
		event = eventlist;
		previousevent = (struct EVENTLIST*)NULL;
		while (event && (event != (HANDLE)h)) {
			previousevent = event;
			event = event->next;
		}
		if (event == (HANDLE)h) {
			if (previousevent)
				previousevent->next = event->next;
			else
				eventlist = event->next;
			free(event);
			r = STATUS_SUCCESS;
			pthread_mutex_unlock(&eventlock);
		} else {
			pthread_mutex_unlock(&eventlock);
			if (pNtClose)
				r = (*pNtClose)(h);
		}
	}
#else
	if (pNtClose)
		r = (*pNtClose)(h);
#endif
	if (trace > 2)
		safe_fprintf(stderr,"NtClose 0x%lx : 0x%lx\n",(long)h,(long)r);
	return (r);
}

BOOL WINAPI CloseHandle(HANDLE h)
{
	NTSTATUS r;

	if (trace > 2)
		safe_fprintf(stderr,"CloseHandle 0x%lx\n",(long)h);
	r = NtClose(h);
	return (r == STATUS_SUCCESS);
}


#else /* POSIX */

/*
 *		We have a problem with NtCreateEvent in native mode
 */


NTSTATUS NTAPI NtCreateEvent(PHANDLE EventHandle, ACCESS_MASK DesiredAccess,
                       POBJECT_ATTRIBUTES ObjectAttributes,
                       EVENT_TYPE EventType, BOOLEAN InitialState)
{
	if (trace > 2)
		safe_fprintf(stderr,"** Skipping NtCreateEvent\n");
	*EventHandle = (void*)NULL;
	return (STATUS_SUCCESS);
}

#endif

long long _atoi64(const char *s)
{
	long long r;

	r = atoi64(s);
	return (r);
}

NTSTATUS NTAPI NtDeleteFile(const POBJECT_ATTRIBUTES ObjectAttributes)
{
	int r;
	int cnt;
	PWSTR p;

	r = 0;
	cnt = ObjectAttributes->ObjectName->Length;
	p = (PWSTR)malloc((cnt + 1)*sizeof(*p));
	if (p) {
		memcpy(p,ObjectAttributes->ObjectName->Buffer,cnt*sizeof(*p));
		p[cnt] = 0;
		r = DeleteFileW(p);
		if (trace > 2) {
			fprintf(stderr,"deleting %s : r 0x%lx\n",
                                  printableutf(p),(long)r);
		}
		free(p);
	}
	return (r ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

DWORD WINAPI FormatMessageA(DWORD flg, LPCVOID src, DWORD idmess,
                 DWORD idlang, LPSTR buf, DWORD sz, va_list *args)
{
	return (-1); /* return an error */
}

