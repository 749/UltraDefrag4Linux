/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
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
* zenwinx.dll interface definitions.
*/

#ifndef _ZENWINX_H_
#define _ZENWINX_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define DEFAULT_TAB_WIDTH 2
#define DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY "      Hit any key to display next page,\n" \
                                             "          ESC or Break to abort..."

#define NtCloseSafe(h) { if(h) { NtClose(h); h = NULL; } }

/* debugging macros */
#include "../../include/dbg.h"

/* prints whatever specified */
#define trace(format,...)  winx_dbg_print(0,format,## __VA_ARGS__)

/* prints {prefix}{function name}: {specified string} */
#define etrace(format,...) winx_dbg_print(0,E "%s: " format,__FUNCTION__,## __VA_ARGS__)
#define itrace(format,...) winx_dbg_print(0,I "%s: " format,__FUNCTION__,## __VA_ARGS__)
#define dtrace(format,...) winx_dbg_print(0,D "%s: " format,__FUNCTION__,## __VA_ARGS__)

/* prints {error prefix}{function name}: {specified string}: {specified nt status and its description} */
#define strace(status,format,...) { \
    NtCurrentTeb()->LastStatusValue = status; \
    winx_dbg_print(NT_STATUS_FLAG,E "%s: " format,__FUNCTION__,## __VA_ARGS__); \
}

/* prints {error prefix}{function name}: {specified string}: {last error and its description} */
#define letrace(format,...) winx_dbg_print(LAST_ERROR_FLAG,E "%s: " format,__FUNCTION__,## __VA_ARGS__)

/* prints {error prefix}{function name}: not enough memory */
#define mtrace() etrace("not enough memory")

#define TraceEnter  { trace(D"Inside function '%s'...",__FUNCTION__); }
#define TraceExit   { trace(D"Leaving function '%s'...",__FUNCTION__); }
#define TraceSource { trace(D"Source file '%s' at line %d ...",__FILE__,__LINE__); }

/*
* DbgCheckN macro definitions are used
* to simplify debugging of a situation 
* when something is mistyped in sources.
*/
#define DbgCheck1(c,r) { \
    if(!(c)) {           \
        etrace("the first parameter is incorrect"); \
        return (r);      \
    }                    \
}

#define DbgCheck2(c1,c2,r) { \
    DbgCheck1(c1,r)          \
    if(!(c2)) {              \
        etrace("the second parameter is incorrect"); \
        return (r);          \
    }                        \
}

#define DbgCheck3(c1,c2,c3,r) { \
    DbgCheck2(c1,c2,r)       \
    if(!(c3)) {              \
        etrace("the third parameter is incorrect"); \
        return (r);          \
    }                        \
}

/* zenwinx functions prototypes */

/* dbg.c */
#define DEFAULT_DBG_PRINT_DECORATION_CHAR  '-'
#define DEFAULT_DBG_PRINT_HEADER_WIDTH     64

void winx_set_dbg_log(wchar_t *path);
#define winx_enable_dbg_log(path) winx_set_dbg_log(path)
#define winx_disable_dbg_log()    winx_set_dbg_log(NULL)

/* flags for winx_flush_dbg_log */
#define FLUSH_ALREADY_SYNCHRONIZED 0x1 /* for internal use only */
#define FLUSH_IN_OUT_OF_MEMORY     0x2 /* flush in the out of memory condition */

void winx_flush_dbg_log(int flags);

void winx_dbg_print(int flags, const char *format, ...);
void winx_dbg_print_header(char ch, int width, const char *format, ...);

/* env.c */
wchar_t *winx_getenv(wchar_t *name);
int winx_setenv(wchar_t *name, wchar_t *value);

/* event.c */
int winx_create_event(wchar_t *name,int type,HANDLE *phandle);
int winx_open_event(wchar_t *name,int flags,HANDLE *phandle);
void winx_destroy_event(HANDLE h);

/* file.c */
typedef struct _WINX_FILE {
    HANDLE hFile;             /* file handle */
    LARGE_INTEGER roffset;    /* offset for read requests */
    LARGE_INTEGER woffset;    /* offset for write requests */
    void *io_buffer;          /* for buffered i/o */
    size_t io_buffer_size;    /* size of the buffer, in bytes */
    size_t io_buffer_offset;  /* current offset inside io_buffer */
    LARGE_INTEGER wboffset;   /* offset for write requests in buffered mode */
} WINX_FILE, *PWINX_FILE;

#define winx_fileno(f) ((f)->hFile)

WINX_FILE *winx_fopen(const wchar_t *filename,const char *mode);
WINX_FILE *winx_fbopen(const wchar_t *filename,const char *mode,int buffer_size);
size_t winx_fread(void *buffer,size_t size,size_t count,WINX_FILE *f);
size_t winx_fwrite(const void *buffer,size_t size,size_t count,WINX_FILE *f);
ULONGLONG winx_fsize(WINX_FILE *f);
void winx_fclose(WINX_FILE *f);
int winx_ioctl(WINX_FILE *f,
    int code,char *description,
    void *in_buffer,int in_size,
    void *out_buffer,int out_size,
    int *pbytes_returned);
int winx_fflush(WINX_FILE *f);
int winx_create_directory(const wchar_t *path);
int winx_delete_file(const wchar_t *filename);
void *winx_get_file_contents(const wchar_t *filename,size_t *bytes_read);
void winx_release_file_contents(void *contents);

/* ftw.c */
/* winx_ftw flags */
#define WINX_FTW_RECURSIVE              0x1 /* forces to recursively scan all subdirectories */
#define WINX_FTW_DUMP_FILES             0x2 /* forces to fill winx_file_disposition structure */
#define WINX_FTW_ALLOW_PARTIAL_SCAN     0x4 /* allows information to be gathered partially */
#define WINX_FTW_SKIP_RESIDENT_STREAMS  0x8 /* forces to skip files of zero length and files located inside MFT */

#define is_readonly(f)            ((f)->flags & FILE_ATTRIBUTE_READONLY)
#define is_hidden(f)              ((f)->flags & FILE_ATTRIBUTE_HIDDEN)
#define is_system(f)              ((f)->flags & FILE_ATTRIBUTE_SYSTEM)
#define is_directory(f)           ((f)->flags & FILE_ATTRIBUTE_DIRECTORY)
#define is_archive(f)             ((f)->flags & FILE_ATTRIBUTE_ARCHIVE)
#define is_device(f)              ((f)->flags & FILE_ATTRIBUTE_DEVICE)
#define is_normal(f)              ((f)->flags & FILE_ATTRIBUTE_NORMAL)
#define is_temporary(f)           ((f)->flags & FILE_ATTRIBUTE_TEMPORARY)
#define is_sparse(f)              ((f)->flags & FILE_ATTRIBUTE_SPARSE_FILE)
#define is_reparse_point(f)       ((f)->flags & FILE_ATTRIBUTE_REPARSE_POINT)
#define is_compressed(f)          ((f)->flags & FILE_ATTRIBUTE_COMPRESSED)
#define is_offline(f)             ((f)->flags & FILE_ATTRIBUTE_OFFLINE)
#define is_not_content_indexed(f) ((f)->flags & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
#define is_encrypted(f)           ((f)->flags & FILE_ATTRIBUTE_ENCRYPTED)
#define are_valid_flags(f)        ((f)->flags & FILE_ATTRIBUTE_VALID_FLAGS)
#define are_valid_set_flags(f)    ((f)->flags & FILE_ATTRIBUTE_VALID_SET_FLAGS)

#define is_fragmented(f)          ((f)->disp.fragments > 1)

typedef struct _winx_blockmap {
    struct _winx_blockmap *next; /* pointer to the next fragment */
    struct _winx_blockmap *prev; /* pointer to the previous fragment */
    ULONGLONG vcn;               /* virtual cluster number */
    ULONGLONG lcn;               /* logical cluster number */
    ULONGLONG length;            /* size of the fragment, in clusters */
} winx_blockmap;

typedef struct _winx_file_disposition {
    ULONGLONG clusters;                /* total number of clusters belonging to the file */
    ULONGLONG fragments;               /* total number of file fragments, not blocks */
    winx_blockmap *blockmap;           /* map of blocks */
} winx_file_disposition;

typedef struct _winx_file_internal_info {
    ULONGLONG BaseMftId;
    ULONGLONG ParentDirectoryMftId;
} winx_file_internal_info;

/*
* All the file access times are in the standard time format.
* That is the number of 100-nanosecond intervals since January 1, 1601.
*/
typedef struct _winx_file_info {
    struct _winx_file_info *next;      /* pointer to the next item */
    struct _winx_file_info *prev;      /* pointer to the previous item */
    wchar_t *name;                     /* name of the file */
    wchar_t *path;                     /* full native path */
    unsigned long flags;               /* combination of FILE_ATTRIBUTE_xxx flags defined in winnt.h */
    winx_file_disposition disp;        /* information about file fragments and their disposition */
    unsigned long user_defined_flags;  /* combination of flags defined by the caller */
    winx_file_internal_info internal;  /* internal information used by ftw_scan_disk support routines */
    ULONGLONG creation_time;           /* the file creation time */
    ULONGLONG last_modification_time;  /* the time of the last file modification */
    ULONGLONG last_access_time;        /* the time of the last file access */
} winx_file_info;

typedef int  (*ftw_filter_callback)(winx_file_info *f,void *user_defined_data);
typedef void (*ftw_progress_callback)(winx_file_info *f,void *user_defined_data);
typedef int  (*ftw_terminator)(void *user_defined_data);

winx_file_info *winx_ftw(wchar_t *path, int flags,
        ftw_filter_callback fcb, ftw_progress_callback pcb, ftw_terminator t,void *user_defined_data);

winx_file_info *winx_scan_disk(char volume_letter, int flags,
        ftw_filter_callback fcb,ftw_progress_callback pcb, ftw_terminator t,void *user_defined_data);

void winx_ftw_release(winx_file_info *filelist);
#define winx_scan_disk_release(f) winx_ftw_release(f)

int winx_ftw_dump_file(winx_file_info *f,ftw_terminator t,void *user_defined_data);

#define WINX_OPEN_FOR_DUMP       0x1 /* open for FSCTL_GET_RETRIEVAL_POINTERS */
#define WINX_OPEN_FOR_BASIC_INFO 0x2 /* open for NtQueryInformationFile(FILE_BASIC_INFORMATION) */
#define WINX_OPEN_FOR_MOVE       0x4 /* open for FSCTL_MOVE_FILE */

#ifdef _NTNDK_H_
NTSTATUS winx_defrag_fopen(winx_file_info *f,int action,HANDLE *phandle);
void winx_defrag_fclose(HANDLE h);
#endif

/* ftw_ntfs.c */
/* int64.c */
/* keyboard.c */
int winx_kb_init(void);

/* keytrans.c */
/* ldr.c */
void *winx_get_proc_address(wchar_t *libname,char *funcname);

/* list.c */
/**
 * @brief Generic structure describing double linked list entry.
 */
typedef struct _list_entry {
    struct _list_entry *next; /* pointer to next entry */
    struct _list_entry *prev; /* pointer to previous entry */
} list_entry;

list_entry *winx_list_insert(list_entry **phead,list_entry *prev,long size);
void winx_list_remove(list_entry **phead,list_entry *item);
void winx_list_destroy(list_entry **phead);

/* lock.c */
typedef struct _winx_spin_lock {
    HANDLE hEvent;
} winx_spin_lock;

winx_spin_lock *winx_init_spin_lock(char *name);
int winx_acquire_spin_lock(winx_spin_lock *sl,int msec);
int winx_release_spin_lock(winx_spin_lock *sl);
void winx_destroy_spin_lock(winx_spin_lock *sl);

/* mem.c */
void *winx_heap_alloc(size_t size,int flags);
void winx_heap_free(void *addr);

/* flags for winx_heap_alloc */
#define MALLOC_ABORT_ON_FAILURE 0x1

/*
* If a small amount of memory is needed,
* call winx_malloc and don't care on the 
* returned value correctness. In case of
* allocation failure it'll simply abort
* the application.
* On the other hand, if some big amount
* of memory needs to be allocated,
* winx_tmalloc needs to be called and then
* its returned value needs to be checked for
* being equal to zero.
*/

/* aborts the application in case of allocation failure */
#define winx_malloc(n) winx_heap_alloc(n,MALLOC_ABORT_ON_FAILURE)

/* this form is tolerant for allocation failures */
#define winx_tmalloc(n) winx_heap_alloc(n,0)

#define winx_free winx_heap_free

typedef int (*winx_killer)(size_t n);
void winx_set_killer(winx_killer k);

/* misc.c */
void winx_sleep(int msec);

#define WINDOWS_NT     40
#define WINDOWS_2K     50
#define WINDOWS_XP     51
#define WINDOWS_2K3    52 /* and Server 2003 R2 and XP x64 */
#define WINDOWS_VISTA  60 /* and Server 2008 */
#define WINDOWS_7      61 /* and Server 2008 R2 */
#define WINDOWS_8      62 /* and Server 2012 */
int winx_get_os_version(void);

wchar_t *winx_get_windows_directory(void);
int winx_query_symbolic_link(wchar_t *name, wchar_t *buffer, int length);

/* process mode constants */
#define INTERNAL_SEM_FAILCRITICALERRORS 0
int winx_set_system_error_mode(unsigned int mode);

wchar_t *winx_get_windows_boot_options(void);
int winx_windows_in_safe_mode(void);

/* mutex.c */
int winx_create_mutex(wchar_t *name,HANDLE *phandle);
int winx_open_mutex(wchar_t *name,HANDLE *phandle);
int winx_release_mutex(HANDLE h);
void winx_destroy_mutex(HANDLE h);

/* path.c */
void winx_path_remove_extension(wchar_t *path);
void winx_path_remove_filename(wchar_t *path);
void winx_path_extract_filename(wchar_t *path);
wchar_t *winx_get_module_filename(void);
int winx_create_path(wchar_t *path);

/* privilege.c */
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE  0x3
#define SE_AUDIT_PRIVILEGE               0x15
#define SE_BACKUP_PRIVILEGE              0x11
#define SE_CREATE_PAGEFILE_PRIVILEGE     0x0f
#define SE_CREATE_PERMANENT_PRIVILEGE    0x10
#define SE_CREATE_TOKEN_PRIVILEGE        0x2
#define SE_DEBUG_PRIVILEGE               0x14
#define SE_IMPERSONATE_PRIVILEGE
#define SE_INC_BASE_PRIORITY_PRIVILEGE   0x0e
#define SE_INCREASE_QUOTA_PRIVILEGE      0x5
#define SE_LOAD_DRIVER_PRIVILEGE         0x0a
#define SE_LOCK_MEMORY_PRIVILEGE         0x4
#define SE_MANAGE_VOLUME_PRIVILEGE       0x1c
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE 0x0d
#define SE_RESTORE_PRIVILEGE             0x12
#define SE_SECURITY_PRIVILEGE            0x8
#define SE_SHUTDOWN_PRIVILEGE            0x13
#define SE_SYSTEM_PROFILE_PRIVILEGE      0x0b
#define SE_SYSTEMTIME_PRIVILEGE          0x0c
#define SE_TAKE_OWNERSHIP_PRIVILEGE      0x9
#define SE_TCB_PRIVILEGE                 0x7

int winx_enable_privilege(unsigned long luid);

/* reg.c */
int winx_bootex_check(const wchar_t *command);
int winx_bootex_register(const wchar_t *command);
int winx_bootex_unregister(const wchar_t *command);

/* stdio.c */
#ifdef _NTNDK_H_
int winx_putch(int ch);
int winx_puts(const char *string);
void winx_print(char *string);
int winx_printf(const char *format, ...);

/*
* Note: winx_scanf() can not be implemented;
* use winx_gets() and sscanf() instead.
*/
int winx_kbhit(int msec);
int winx_kb_read(KBD_RECORD *kbd_rec,int msec);
int winx_breakhit(int msec);

int winx_getch(void);
int winx_getche(void);
int winx_gets(char *string,int n);

typedef struct _winx_history_entry {
    struct _winx_history_entry *next;
    struct _winx_history_entry *prev;
    char *string;
} winx_history_entry;

typedef struct _winx_history {
    winx_history_entry *head;
    winx_history_entry *current;
    int n_entries;
} winx_history;

void winx_init_history(winx_history *h);
void winx_destroy_history(winx_history *h);

int winx_prompt(char *prompt,char *string,int n,winx_history *h);

int winx_print_strings(char **strings,int line_width,
    int max_rows,char *prompt,int divide_to_pages);
#endif /* _NTNDK_H_ */

/* string.c */
/* reliable _toupper and _tolower analogs */
char winx_toupper(char c);
char winx_tolower(char c);
/* reliable towupper and towlower analogs */
wchar_t winx_towupper(wchar_t c);
wchar_t winx_towlower(wchar_t c);
/* reliable _wcsupr and _wcslwr analogs */
wchar_t *winx_wcsupr(wchar_t *s);
wchar_t *winx_wcslwr(wchar_t *s);

char *winx_strdup(const char *s);
wchar_t *winx_wcsdup(const wchar_t *s);
int winx_wcsicmp(const wchar_t *s1, const wchar_t *s2);
wchar_t *winx_wcsistr(const wchar_t *s1, const wchar_t *s2);
char *winx_stristr(const char *s1, const char *s2);
int winx_wcsmatch(wchar_t *string, wchar_t *mask, int flags);
char *winx_vsprintf(const char *format,va_list arg);
char *winx_sprintf(const char *format, ...);
wchar_t *winx_vswprintf(const wchar_t *format,va_list arg);
wchar_t *winx_swprintf(const wchar_t *format, ...);

#define WINX_PAT_ICASE  0x1 /* compile patterns for case insensitive search */

typedef struct _winx_patlist {
    int count;
    wchar_t **array;
    int flags;
    wchar_t *string;
} winx_patlist;

int winx_patcomp(winx_patlist *patterns,wchar_t *string,wchar_t *delim,int flags);
int winx_patfind(wchar_t *string,winx_patlist *patterns);
int winx_patcmp(wchar_t *string,winx_patlist *patterns);
void winx_patfree(winx_patlist *patterns);

int winx_bytes_to_hr(ULONGLONG bytes, int digits, char *buffer, int length);
ULONGLONG winx_hr_to_bytes(char *string);

void winx_to_utf8(char *dst,int size,wchar_t *src);

/* thread.c */
#ifdef _NTNDK_H_
int winx_create_thread(PTHREAD_START_ROUTINE start_addr,PVOID parameter);
void winx_exit_thread(NTSTATUS status);
#endif

/* time.c */
ULONGLONG winx_str2time(char *string);
int winx_time2str(ULONGLONG time,char *buffer,int size);
ULONGLONG winx_xtime(void);
#define winx_xtime_nsec() (winx_xtime() * 1000 * 1000)

typedef struct _winx_time {
    short year;        // range [1601...]
    short month;       // range [1..12]
    short day;         // range [1..31]
    short hour;        // range [0..23]
    short minute;      // range [0..59]
    short second;      // range [0..59]
    short milliseconds;// range [0..999]
    short weekday;     // range [0..6] == [sunday..saturday]
} winx_time;

int winx_get_system_time(winx_time *t);
int winx_get_local_time(winx_time *t);

/* volume.c */
#define DRIVE_ASSIGNED_BY_SUBST_COMMAND 1200
int winx_get_drive_type(char letter);

/*
* Maximal length of the file system name.
* Specified length is more than enough
* to hold all known names.
*/
#define MAX_FS_NAME_LENGTH 31

#ifndef _NTNDK_H_
#pragma pack(push, 1)
typedef struct _NTFS_DATA {
    LARGE_INTEGER VolumeSerialNumber;
    LARGE_INTEGER NumberSectors;
    LARGE_INTEGER TotalClusters;
    LARGE_INTEGER FreeClusters;
    LARGE_INTEGER TotalReserved;
    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    ULONG BytesPerFileRecordSegment;
    ULONG ClustersPerFileRecordSegment;
    LARGE_INTEGER MftValidDataLength;
    LARGE_INTEGER MftStartLcn;
    LARGE_INTEGER Mft2StartLcn;
    LARGE_INTEGER MftZoneStart;
    LARGE_INTEGER MftZoneEnd;
} NTFS_DATA, *PNTFS_DATA;
#pragma pack(pop)
#endif /* !_NTNDK_H_ */

typedef struct _winx_volume_information {
    char volume_letter;                    /* must be set by caller! */
    char fs_name[MAX_FS_NAME_LENGTH + 1];  /* the name of the file system */
    wchar_t label[MAX_PATH + 1];           /* volume label */
    ULONGLONG total_bytes;                 /* total volume size, in bytes */
    ULONGLONG free_bytes;                  /* amount of free space, in bytes */
    ULONGLONG total_clusters;              /* total number of clusters */
    ULONGLONG bytes_per_cluster;           /* cluster size, in bytes */
    ULONG sectors_per_cluster;             /* number of sectors in each cluster */
    ULONG bytes_per_sector;                /* sector size, in bytes */
    NTFS_DATA ntfs_data;                   /* NTFS data, valid for NTFS formatted volumes only */
    int is_dirty;                          /* nonzero value indicates that volume is dirty and needs to be checked */
    ULONGLONG device_capacity;             /* total device capacity (including all partitions), in bytes */
} winx_volume_information;

int winx_get_volume_information(char volume_letter,winx_volume_information *v);
WINX_FILE *winx_vopen(char volume_letter);
int winx_vflush(char volume_letter);

/* winx_get_free_volume_regions flags */
#define WINX_GVR_ALLOW_PARTIAL_SCAN  0x1

typedef struct _winx_volume_region {
    struct _winx_volume_region *next;  /* pointer to the next region */
    struct _winx_volume_region *prev;  /* pointer to the previous region */
    ULONGLONG lcn;                     /* logical cluster number */
    ULONGLONG length;                  /* size of region, in clusters */
} winx_volume_region;

typedef int (*volume_region_callback)(winx_volume_region *reg,void *user_defined_data);

winx_volume_region *winx_get_free_volume_regions(char volume_letter,
        int flags,volume_region_callback cb,void *user_defined_data);
winx_volume_region *winx_add_volume_region(winx_volume_region *rlist,
        ULONGLONG lcn,ULONGLONG length);
winx_volume_region *winx_sub_volume_region(winx_volume_region *rlist,
        ULONGLONG lcn,ULONGLONG length);
void winx_release_free_volume_regions(winx_volume_region *rlist);

/* zenwinx.c */
int winx_init_library(void);
void winx_unload_library(void);

void winx_exit(int exit_code);
void winx_reboot(void);
void winx_shutdown(void);

/* Red-black trees with parent pointers */
#include "prb.h"

#if defined(__cplusplus)
}
#endif

#endif /* _ZENWINX_H_ */
