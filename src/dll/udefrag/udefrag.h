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
* Udefrag.dll interface header.
*/

#ifndef _UDEFRAG_H_
#define _UDEFRAG_H_

/* debug print levels */
#define DBG_NORMAL     0
#define DBG_DETAILED   1
#define DBG_PARANOID   2

/* UltraDefrag error codes */
#define UDEFRAG_UNKNOWN_ERROR     (-1)
#define UDEFRAG_FAT_OPTIMIZATION  (-2)
#define UDEFRAG_W2K_4KB_CLUSTERS  (-3)
#define UDEFRAG_NO_MEM            (-4)
#define UDEFRAG_CDROM             (-5)
#define UDEFRAG_REMOTE            (-6)
#define UDEFRAG_ASSIGNED_BY_SUBST (-7)
#define UDEFRAG_REMOVABLE         (-8)
#define UDEFRAG_UDF_DEFRAG        (-9)
#define UDEFRAG_NO_MFT            (-10)
#define UDEFRAG_UNMOVABLE_MFT     (-11)
#define UDEFRAG_DIRTY_VOLUME      (-12)

#define DEFAULT_REFRESH_INTERVAL 100

#define MAX_DOS_DRIVES 26
#define MAXFSNAME      32  /* I think, that's enough */

typedef struct _volume_info {
    char letter;
    char fsname[MAXFSNAME];
    wchar_t label[MAX_PATH + 1];
    LARGE_INTEGER total_space;
    LARGE_INTEGER free_space;
    int is_removable;
    int is_dirty;
} volume_info;

volume_info *udefrag_get_vollist(int skip_removable);
void udefrag_release_vollist(volume_info *v);
int udefrag_validate_volume(char volume_letter,int skip_removable);
int udefrag_get_volume_information(char volume_letter,volume_info *v);

int udefrag_bytes_to_hr(ULONGLONG bytes, int digits, char *buffer, int length);
#define udefrag_bytes_to_hr(b,d,buf,lth) winx_bytes_to_hr(b,d,buf,lth)
ULONGLONG udefrag_hr_to_bytes(char *string);

typedef enum {
    ANALYSIS_JOB = 0,
    DEFRAGMENTATION_JOB,
    FULL_OPTIMIZATION_JOB,
    QUICK_OPTIMIZATION_JOB,
    MFT_OPTIMIZATION_JOB
} udefrag_job_type;

typedef enum {
    VOLUME_ANALYSIS = 0,     /* should be zero */
    VOLUME_DEFRAGMENTATION,
    VOLUME_OPTIMIZATION
} udefrag_operation_type;

/* flags triggering algorithm features */
#define UD_JOB_REPEAT               0x1
#define UD_PREVIEW_MATCHING         0x2
// #define UD_PREVIEW_MOVE_FRONT       0x4
// #define UD_PREVIEW_SKIP_PARTIAL     0x8
#define UD_JOB_CONTEXT_MENU_HANDLER 0x10

/*
* MFT_ZONE_SPACE has special meaning - 
* it is used as a marker for MFT Zone space.
*/
enum {
    UNUSED_MAP_SPACE = 0,        /* other colors have more precedence */
    FREE_SPACE,                  /* has lowest precedence */
    SYSTEM_SPACE,
    SYSTEM_OVER_LIMIT_SPACE,
    FRAGM_SPACE,
    FRAGM_OVER_LIMIT_SPACE,
    UNFRAGM_SPACE,
    UNFRAGM_OVER_LIMIT_SPACE,
    DIR_SPACE,
    DIR_OVER_LIMIT_SPACE,
    COMPRESSED_SPACE,
    COMPRESSED_OVER_LIMIT_SPACE,
    MFT_ZONE_SPACE,
    MFT_SPACE,
    TEMPORARY_SYSTEM_SPACE,      /* has highest precedence */
    NUM_OF_SPACE_STATES          /* this must always be the last */
};

#define UNKNOWN_SPACE FRAGM_SPACE

typedef struct _udefrag_progress_info {
    unsigned long files;              /* number of files */
    unsigned long directories;        /* number of directories */
    unsigned long compressed;         /* number of compressed files */
    unsigned long fragmented;         /* number of fragmented files */
    ULONGLONG fragments;              /* number of fragments */
    ULONGLONG total_space;            /* volume size, in bytes */
    ULONGLONG free_space;             /* free space amount, in bytes */
    ULONGLONG mft_size;               /* mft size, in bytes */
    udefrag_operation_type current_operation;  /* identifies currently running operation */
    unsigned long pass_number;        /* the current volume optimizer pass */
    ULONGLONG clusters_to_process;    /* number of clusters to process */
    ULONGLONG processed_clusters;     /* number of already processed clusters */
    double percentage;                /* used to deliver a job completion percentage to the caller */
    int completion_status;            /* zero for running job, positive value for succeeded, negative for failed */
    char *cluster_map;                /* pointer to the cluster map buffer */
    unsigned int cluster_map_size;    /* size of the cluster map buffer, in bytes */
    ULONGLONG moved_clusters;         /* number of moved clusters */
    ULONGLONG total_moves;            /* number of moves by move_files_to_front/back functions */
    ULONGLONG current_inode;      /* inode being processed */
    ULONG step;              /* number of steps on this inode */
    ULONGLONG feedback_time;      /* JPA time of last feedback */
} udefrag_progress_info;

typedef void  (*udefrag_progress_callback)(udefrag_progress_info *pi, void *p);
// remove the stdcall ?
typedef void  (__stdcall *progress_feedback_callback)(udefrag_progress_info *pi, const char *p);
typedef int   (*udefrag_terminator)(void *p);

int udefrag_start_job(char volume_letter,udefrag_job_type job_type,
    int flags,unsigned int cluster_map_size,udefrag_progress_callback cb,
    progress_feedback_callback pf,udefrag_terminator t,void *p);

char *udefrag_get_default_formatted_results(udefrag_progress_info *pi);
void udefrag_release_default_formatted_results(char *results);

char *udefrag_get_error_description(int error_code);

/* reliable _toupper and _tolower analogs */
char udefrag_toupper(char c);
char udefrag_tolower(char c);

int udefrag_set_log_file_path(void);
int udefrag_init_failed(void);

#endif /* _UDEFRAG_H_ */
