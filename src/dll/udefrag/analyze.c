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

/**
 * @file analyze.c
 * @brief Volume analysis.
 * @addtogroup Analysis
 * @{
 */

/*
* Ideas by Dmitri Arkhangelski <dmitriar@gmail.com>
* and Stefan Pendl <stefanpe@users.sourceforge.net>.
*/

/*
* NTFS formatted volumes cannot be safely accessed
* in NT4, as mentioned in program's handbook.
* On our test machines NTFS access caused blue
* screens regardless of whether we accessed it
* from kernel mode or user mode.
*/

#include "compiler.h"
#include "udefrag-internals.h"

extern int progress_trigger;

static void update_progress_counters(winx_file_info *f,udefrag_job_parameters *jp);

/**
 * @brief Retrieves all information about the volume.
 * @return Zero for success, negative value otherwise.
 * @note Resets statistics and cluster map.
 */
int get_volume_information(udefrag_job_parameters *jp)
{
    /* reset coordinates of mft zones */
    memset(&jp->mft_zones,0,sizeof(struct _mft_zones));
    
    /* reset v_info structure */
    memset(&jp->v_info,0,sizeof(winx_volume_information));
    
    /* reset statistics */
    jp->pi.files = 0;
    jp->pi.directories = 0;
    jp->pi.compressed = 0;
    jp->pi.fragmented = 0;
    jp->pi.fragments = 0;
    jp->pi.total_space = 0;
    jp->pi.free_space = 0;
    jp->pi.mft_size = 0;
    jp->pi.clusters_to_process = 0;
    jp->pi.processed_clusters = 0;
    jp->pi.step = 0;
    jp->pi.feedback_time = winx_xtime();
    
    jp->fs_type = FS_UNKNOWN;
    
    /* reset file lists */
    destroy_lists(jp);
    
    /* update global variables holding drive geometry */
    if(winx_get_volume_information(jp->volume_letter,&jp->v_info) < 0){
        return (-1);
    } else {
        jp->pi.total_space = jp->v_info.total_bytes;
        jp->pi.free_space = jp->v_info.free_bytes;
        if(jp->v_info.bytes_per_cluster) jp->clusters_per_256k = _256K / jp->v_info.bytes_per_cluster;
        else jp->clusters_per_256k = 0;
        DebugPrint("total clusters: %" LL64 "u",(ULONGLONG)jp->v_info.total_clusters);
        DebugPrint("cluster size: %" LL64 "u",(ULONGLONG)jp->v_info.bytes_per_cluster);
        if(!jp->clusters_per_256k){
            DebugPrint("clusters are larger than 256 kbytes");
            jp->clusters_per_256k ++;
        }
        /* validate geometry */
        ExpertPrint("clusters %ld bytes per cluster %ld",(long)jp->v_info.total_clusters,(long)jp->v_info.bytes_per_cluster);
        if(!jp->v_info.total_clusters || !jp->v_info.bytes_per_cluster){
            DebugPrint("wrong volume geometry detected");
            return (-1);
        }
        /* check partition type */
        DebugPrint("%s partition detected",jp->v_info.fs_name);
        if(!strcmp(jp->v_info.fs_name,"NTFS")){
            jp->fs_type = FS_NTFS;
        } else if(!strcmp(jp->v_info.fs_name,"FAT32")){
            jp->fs_type = FS_FAT32;
        } else if(strstr(jp->v_info.fs_name,"FAT")){
            /* no need to distinguish better */
            jp->fs_type = FS_FAT16;
        } else if(!strcmp(jp->v_info.fs_name,"UDF")){
            jp->fs_type = FS_UDF;
        } else {
//        } else if(!strcmp(jp->v_info.fs_name,"FAT12")){
//            jp->fs_type = FS_FAT12;
//        } else if(!strcmp(jp->v_info.fs_name,"FAT16")){
//            jp->fs_type = FS_FAT16;
//        } else if(!strcmp(jp->v_info.fs_name,"FAT32")){
//            /* check FAT32 version */
//            if(jp->v_info.fat32_mj_version > 0 || jp->v_info.fat32_mn_version > 0){
//                DebugPrint("cannot recognize FAT32 version %u.%u",
//                    jp->v_info.fat32_mj_version,jp->v_info.fat32_mn_version);
//                /* for safe low level access in future releases */
//                jp->fs_type = FS_FAT32_UNRECOGNIZED;
//            } else {
//                jp->fs_type = FS_FAT32;
//            }
//        } else {
            DebugPrint("file system type is not recognized");
            DebugPrint("type independent routines will be used to defragment it");
            jp->fs_type = FS_UNKNOWN;
        }
    }

    jp->pi.clusters_to_process = jp->v_info.total_clusters;
    jp->pi.processed_clusters = 0;

    /* reset cluster map */
    reset_cluster_map(jp);
    if(jp->progress_router)
        jp->progress_router(jp); /* redraw progress */
    return 0;
}

/**
 * @brief get_free_space_layout helper.
 */
static int process_free_region(winx_volume_region *rgn,void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;
    
    if(jp->udo.dbgprint_level >= DBG_PARANOID)
        DebugPrint("Free block start: %" LL64 "u len: %" LL64 "u",(ULONGLONG)rgn->lcn,(ULONGLONG)rgn->length);
    colorize_map_region(jp,rgn->lcn,rgn->length,FREE_SPACE,SYSTEM_SPACE);
    jp->pi.processed_clusters += rgn->length;
    jp->free_regions_count ++;
    if(jp->progress_router)
        jp->progress_router(jp); /* redraw progress */
    return 0; /* continue scan */
}

/**
 * @brief Retrieves free space layout.
 * @return Zero for success, negative value otherwise.
 */
static int get_free_space_layout(udefrag_job_parameters *jp)
{
    jp->free_regions = winx_get_free_volume_regions(jp->volume_letter,
        WINX_GVR_ALLOW_PARTIAL_SCAN,process_free_region,(void *)jp);
    
    DebugPrint("free regions count: %u",jp->free_regions_count);
    
    /* let full disks to pass the analysis successfully */
    if(jp->free_regions == NULL || jp->free_regions_count == 0)
        DebugPrint("get_free_space_layout: disk is full or some error has been encountered");
    return 0;
}

/**
 * @brief Checks whether specified 
 * region is inside processed volume.
 */
int check_region(udefrag_job_parameters *jp,ULONGLONG lcn,ULONGLONG length)
{
    if(lcn < jp->v_info.total_clusters \
      && (lcn + length) <= jp->v_info.total_clusters)
        return 1;
    
    return 0;
}

/**
 * @brief Retrieves mft zones layout.
 * @note MFT zone space becomes excluded from the free space list.
 * Because Windows 2000 disallows to move files there. And because on 
 * other systems this dirty technique may increase MFT fragmentation.
 */
static void get_mft_zones_layout(udefrag_job_parameters *jp)
{
    ULONGLONG start,length,mirror_size;
    ULONGLONG mft_length = 0;

    if(jp->fs_type != FS_NTFS)
        return;
    
    /* 
    * Don't increment progress counters,
    * because mft zones are partially
    * inside already counted free space pool.
    */
    DebugPrint("%-12s: %-20s: %-20s", "mft section", "start", "length");

    /* $MFT */
    start = jp->v_info.ntfs_data.MftStartLcn.QuadPart;
    if(jp->v_info.ntfs_data.BytesPerCluster)
        length = jp->v_info.ntfs_data.MftValidDataLength.QuadPart / jp->v_info.ntfs_data.BytesPerCluster;
    else
        length = 0;
    DebugPrint("%-12s: %-20" LL64 "u: %-20" LL64 "u", "mft", (ULONGLONG)start, (ULONGLONG)length);
    if(check_region(jp,start,length)){
        /* to be honest, this is not MFT zone */
        /*colorize_map_region(jp,start,length,MFT_ZONE_SPACE,0);
        jp->free_regions = winx_sub_volume_region(jp->free_regions,start,length);*/
        jp->mft_zones.mft_start = start; jp->mft_zones.mft_end = start + length - 1;
        mft_length += length;
        ExpertPrint("new actual mft start 0x%" LL64 "x end 0x%" LL64 "x lth 0x%lx",(long long)jp->mft_zones.mft_start,(long long)jp->mft_zones.mft_end,(long)length);
    }

    /* MFT Zone */
    start = jp->v_info.ntfs_data.MftZoneStart.QuadPart;
    ExpertPrint("returned start MFT zone 0x%" LL64 "x end 0x%" LL64 "x MFT 0x%" LL64 "x",(long long)start,(long long)jp->v_info.ntfs_data.MftZoneEnd.QuadPart,(long long)jp->v_info.ntfs_data.MftStartLcn.QuadPart);
    length = jp->v_info.ntfs_data.MftZoneEnd.QuadPart - start;
    DebugPrint("%-12s: %-20" LL64 "x: %-20" LL64 "x", "mft zone", (ULONGLONG)start, (ULONGLONG)length);
    if(check_region(jp,start,length)){
        /* remark space as reserved */
        colorize_map_region(jp,start,length,MFT_ZONE_SPACE,0);
        jp->free_regions = winx_sub_volume_region(jp->free_regions,start,length);
        jp->mft_zones.mftzone_start = start; jp->mft_zones.mftzone_end = start + length - 1;
    }

    /* $MFT Mirror */
    start = jp->v_info.ntfs_data.Mft2StartLcn.QuadPart;
    length = 1;
    mirror_size = jp->v_info.ntfs_data.BytesPerFileRecordSegment * 4;
    if(jp->v_info.ntfs_data.BytesPerCluster && mirror_size > jp->v_info.ntfs_data.BytesPerCluster){
        length = mirror_size / jp->v_info.ntfs_data.BytesPerCluster;
        if(mirror_size - length * jp->v_info.ntfs_data.BytesPerCluster)
            length ++;
    }
    DebugPrint("%-12s: %-20" LL64 "u: %-20" LL64 "u", "mft mirror", (ULONGLONG)start, (ULONGLONG)length);
    if(check_region(jp,start,length)){
        /* to be honest, this is not MFT zone */
        /*colorize_map_region(jp,start,length,MFT_ZONE_SPACE,0);
        jp->free_regions = winx_sub_volume_region(jp->free_regions,start,length);*/
        jp->mft_zones.mftmirr_start = start; jp->mft_zones.mftmirr_end = start + length - 1;
    }
    
    jp->pi.mft_size = mft_length * jp->v_info.bytes_per_cluster;
    DebugPrint("mft size = %" LL64 "u bytes",(ULONGLONG)jp->pi.mft_size);
    if(jp->progress_router)
        jp->progress_router(jp); /* redraw progress */
}

/**
 * @brief Defines whether the file
 * must be excluded from the volume
 * processing or not.
 * @return Nonzero value indicates
 * that the file must be excluded.
 */
int exclude_by_path(winx_file_info *f,udefrag_job_parameters *jp)
{
#ifndef LINUX /* TODO */
    /* note that paths have \??\ internal prefix while patterns haven't */
    if(utflen(f->path) < 0x4)
        return 1; /* path is invalid */
    
    /* ignore ex_filter in context menu handler */
    if(!(jp->udo.job_flags & UD_JOB_CONTEXT_MENU_HANDLER)){
        if(jp->udo.ex_filter.count){
            if(winx_patcmp(f->path + 0x4,&jp->udo.ex_filter))
                return 1;
        }
    }
    
    if(jp->udo.in_filter.count == 0) return 0;
    return !winx_patcmp(f->path + 0x4,&jp->udo.in_filter);
#else
        /* JPA taken from old version for now */
    return winx_patfind(f->path,&jp->udo.ex_filter);
#endif
}

/**
 * @brief find_files helper.
 * @note Optimized for speed.
 */
static int filter(winx_file_info *f,void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;
    int length;
    ULONGLONG filesize;
    
    /* skip entries with empty path, as well as their children */
    if(f->path == NULL) goto skip_file_and_children;
    if(f->path[0] == 0) goto skip_file_and_children;
    
    /* skip resident streams, but not in context menu handler */
    if(f->disp.fragments == 0 && !(jp->udo.job_flags & UD_JOB_CONTEXT_MENU_HANDLER))
        return 0;
    
    /*
    * Remove trailing dot from the root
    * directory path, otherwise we'll not
    * be able to defragment it.
    */
    length = utflen(f->path);
    if(length >= 2){
        if(f->path[length - 1] == '.' && f->path[length - 2] == '\\'){
            DebugPrint("root directory detected, its trailing dot will be removed");
            f->path[length - 1] = 0;
        }
    }
    
    /* skip resident streams in context menu handler, but count them */
    if(f->disp.fragments == 0){
        if(!exclude_by_path(f,jp))
            update_progress_counters(f,jp);
        return 0;
    }
    
    /* show debugging information about interesting cases */
    if(is_sparse(f))
        DebugPrint("sparse file found: %" WSTR,printableutf(f->path));
    if(is_reparse_point(f))
        DebugPrint("reparse point found: %" WSTR,printableutf(f->path));
    /* comment it out after testing to speed things up */
    /*if(winx_utfistr(f->path,UTF("$BITMAP")))
        DebugPrint("bitmap found: %" WSTR,printableutf(f->path));
    if(winx_utfistr(f->path,UTF("$ATTRIBUTE_LIST")))
        DebugPrint("attribute list found: %" WSTR,printableutf(f->path));
    */

    /*
    * No files can be filtered out when
    * volume optimization job is requested.
    */
    if(jp->job_type == FULL_OPTIMIZATION_JOB \
      || jp->job_type == QUICK_OPTIMIZATION_JOB \
      || jp->job_type == MFT_OPTIMIZATION_JOB)
        return 0;

    /* skip temporary files, as well as their children */
    if(is_temporary(f)) goto skip_file_and_children;

    /* filter files by their sizes */
    filesize = f->disp.clusters * jp->v_info.bytes_per_cluster;
    if(jp->udo.size_limit && filesize > jp->udo.size_limit){
        f->user_defined_flags |= UD_FILE_OVER_LIMIT;
        goto skip_file;
    }

    /* filter files by their number of fragments */
    if(jp->udo.fragments_limit && f->disp.fragments < jp->udo.fragments_limit)
        goto skip_file;

    /* filter files by their paths */
    if(exclude_by_path(f,jp)){
        f->user_defined_flags |= UD_FILE_EXCLUDED;
        f->user_defined_flags |= UD_FILE_EXCLUDED_BY_PATH;
    }
    /* don't skip children since their paths may match patterns */
    return 0;

skip_file:
    f->user_defined_flags |= UD_FILE_EXCLUDED;
    return 0;

skip_file_and_children:
    f->user_defined_flags |= UD_FILE_EXCLUDED;
    return 1;
}

/**
 * @internal
 */
static void update_progress_counters(winx_file_info *f,udefrag_job_parameters *jp)
{
    ULONGLONG filesize;

    jp->pi.files ++;
    if(is_directory(f)) jp->pi.directories ++;
    if(is_compressed(f)) jp->pi.compressed ++;
    jp->pi.processed_clusters += f->disp.clusters;

    filesize = f->disp.clusters * jp->v_info.bytes_per_cluster;
    if(filesize >= GIANT_FILE_SIZE)
        jp->f_counters.giant_files ++;
    else if(filesize >= HUGE_FILE_SIZE)
        jp->f_counters.huge_files ++;
    else if(filesize >= BIG_FILE_SIZE)
        jp->f_counters.big_files ++;
    else if(filesize >= AVERAGE_FILE_SIZE)
        jp->f_counters.average_files ++;
    else if(filesize >= SMALL_FILE_SIZE)
        jp->f_counters.small_files ++;
    else
        jp->f_counters.tiny_files ++;

    if(jp->progress_router)
        jp->progress_router(jp); /* redraw progress */
}

/**
 * @brief find_files helper.
 */
static void progress_callback(winx_file_info *f,void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;
    winx_blockmap *block;
    
    /* don't count excluded files in context menu handler */
    if(!(jp->udo.job_flags & UD_JOB_CONTEXT_MENU_HANDLER))
        update_progress_counters(f,jp);

    /* add file blocks to the binary search tree */
    for(block = f->disp.blockmap; block; block = block->next){
        if(add_block_to_file_blocks_tree(jp,f,block) < 0) break;
        if(block->next == f->disp.blockmap) break;
    }
}

/**
 * @brief find_files helper.
 */
static int terminator(void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;

    return jp->termination_router((void *)jp);
}

/**
 * @brief Displays file counters.
 */
void dbg_print_file_counters(udefrag_job_parameters *jp)
{
    DebugPrint("folders total:    %u",jp->pi.directories);
    DebugPrint("files total:      %u",jp->pi.files);
    DebugPrint("fragmented files: %u",jp->pi.fragmented);
    DebugPrint("compressed files: %u",jp->pi.compressed);
    DebugPrint("tiny ...... <  10 kB: %u",jp->f_counters.tiny_files);
    DebugPrint("small ..... < 100 kB: %u",jp->f_counters.small_files);
    DebugPrint("average ... <   1 MB: %u",jp->f_counters.average_files);
    DebugPrint("big ....... <  16 MB: %u",jp->f_counters.big_files);
    DebugPrint("huge ...... < 128 MB: %u",jp->f_counters.huge_files);
    DebugPrint("giant ..............: %u",jp->f_counters.giant_files);
}

/**
 * @brief Searches for all files on disk.
 * @return Zero for success, negative value otherwise.
 */
static int find_files(udefrag_job_parameters *jp)
{
    int context_menu_handler = 0;
    utf_t parent_directory[MAX_PATH + 1];
    utf_t *p;
    utf_t c;
    int flags = 0;
    winx_file_info *f;
    
    /* check for context menu handler */
    if(jp->udo.job_flags & UD_JOB_CONTEXT_MENU_HANDLER){
        if(jp->udo.in_filter.count > 0){
            if(utflen(jp->udo.in_filter.array[0]) >= utflen(UTF("C:\\")))
                context_menu_handler = 1;
        }
    }

    /* speed up the context menu handler */
    if(jp->fs_type != FS_NTFS && context_menu_handler){
        /* in case of c:\* or c:\ scan entire disk */
        c = jp->udo.in_filter.array[0][3];
        if(c == 0 || c == '*')
            goto scan_entire_disk;
        /* in case of c:\test;c:\test\* scan parent directory recursively */
        if(jp->udo.in_filter.count > 1)
            flags = WINX_FTW_RECURSIVE;
        /* in case of c:\test scan parent directory, not recursively */
        _snwprintf(parent_directory, MAX_PATH, UTF("\\??\\%" WSTR), jp->udo.in_filter.array[0]);
        parent_directory[MAX_PATH] = 0;
        p = utfrchr(parent_directory,'\\');
        if(p) *p = 0;
        if(utflen(parent_directory) <= utflen(UTF("\\??\\C:\\")))
            goto scan_entire_disk;
        jp->filelist = winx_ftw(parent_directory,
            flags | WINX_FTW_DUMP_FILES | \
            WINX_FTW_ALLOW_PARTIAL_SCAN | WINX_FTW_SKIP_RESIDENT_STREAMS,
            filter,progress_callback,terminator,(void *)jp);
    } else {
    scan_entire_disk:
        jp->filelist = winx_scan_disk(jp->volume_letter,
            WINX_FTW_DUMP_FILES | WINX_FTW_ALLOW_PARTIAL_SCAN | \
            WINX_FTW_SKIP_RESIDENT_STREAMS,
            filter,progress_callback,terminator,(void *)jp);
    }
    if(jp->filelist == NULL && !jp->termination_router((void *)jp))
        return (-1);
    
    /* calculate number of fragmented files; redraw map */
    for(f = jp->filelist; f; f = f->next){
        /* skip excluded files */
        if(!is_fragmented(f) || is_excluded(f)){
            jp->pi.fragments ++;
        } else {
            jp->pi.fragmented ++;
            jp->pi.fragments += f->disp.fragments;
        }

        /* count nonresident files included by context menu handler */
        if(jp->udo.job_flags & UD_JOB_CONTEXT_MENU_HANDLER){
            if(!is_excluded_by_path(f))
                update_progress_counters(f,jp);
        }
    
        /* redraw cluster map */
        colorize_file(jp,f,SYSTEM_SPACE);

        //DebugPrint("%" WSTR,printableutf(f->path));
        if(jp->progress_router) /* need speedup? */
            jp->progress_router(jp); /* redraw progress */
        
        if(f->next == jp->filelist) break;
    }

    dbg_print_file_counters(jp);
    return 0;
}

/**
 * @brief Defines whether the file is from
 * well known locked files list or not.
 * @note Skips $Mft file, because it is
 * intended to be drawn in magenta color.
 */
static int is_well_known_locked_file(winx_file_info *f,udefrag_job_parameters *jp)
{
    int length;
    const utf_t mft_name[] = UTF("$Mft");
    const utf_t config_sign[] = UTF("\\system32\\config\\");
    
    length = utflen(f->path);

    /* search for NTFS internal files */
    if(length >= 9){ /* to ensure that we have at least \??\X:\$x */
        if(f->path[7] == '$'){
            if(length == 11){
                if(winx_utfistr(f->name,mft_name))
                    return 0; /* skip $mft */
            }
            return 1;
        }
    }
    
    if(winx_utfistr(f->name,UTF("pagefile.sys")))
        return 1;
    if(winx_utfistr(f->name,UTF("hiberfil.sys")))
        return 1;
    if(winx_utfistr(f->name,UTF("ntuser.dat")))
        return 1;

    if(winx_utfistr(f->path,config_sign)){
        if(winx_utfistr(f->name,UTF("sam")))
            return 1;
        if(winx_utfistr(f->name,UTF("system")))
            return 1;
        if(winx_utfistr(f->name,UTF("software")))
            return 1;
        if(winx_utfistr(f->name,UTF("security")))
            return 1;
    }
    
    return 0;
}

/**
 * @brief Defines whether the file
 * is locked by system or not.
 * @return Nonzero value indicates
 * that the file is locked.
 */
int is_file_locked(winx_file_info *f,udefrag_job_parameters *jp)
{
    NTSTATUS status;
    HANDLE hFile;
    
    /* check whether the file has been passed the check already */
    if(f->user_defined_flags & UD_FILE_NOT_LOCKED)
        return 0;
    if(f->user_defined_flags & UD_FILE_LOCKED)
        return 1;

    /* file status is undefined, so let's try to open it */
    status = winx_defrag_fopen(f,WINX_OPEN_FOR_MOVE,&hFile);
    if(status == STATUS_SUCCESS){
        winx_defrag_fclose(hFile);
        f->user_defined_flags |= UD_FILE_NOT_LOCKED;
        return 0;
    }

    /*DebugPrintEx(status,"cannot open %" WSTR,printableutf(f->path));*/
    /* redraw space */
    colorize_file_as_system(jp,f);
    /* file is locked by other application, so its state is unknown */
    /* however, don't reset file information here */
    f->user_defined_flags |= UD_FILE_LOCKED;
    if(jp->progress_router)
        jp->progress_router(jp); /* redraw progress */
    return 1;
}

/**
 * @brief Searches for well known locked files
 * and applies their dispositions to the map.
 * @details Resets f->disp structure of locked files.
 * @todo Speed up.
 */
static void redraw_well_known_locked_files(udefrag_job_parameters *jp)
{
    winx_file_info *f;
    ULONGLONG time;

    winx_dbg_print_header(0,0,"search for well known locked files...");
    time = winx_xtime();
    
    for(f = jp->filelist; f; f = f->next){
        if(f->disp.blockmap && f->path && f->name){ /* otherwise nothing to redraw */
            if(is_well_known_locked_file(f,jp)){
                if(!is_file_locked(f,jp)){
                    /* possibility of this case must be reduced */
                    DebugPrint("false detection: %" WSTR,printablepath(f->path,0));
                }
            }
        }
        if(f->next == jp->filelist) break;
    }

    winx_dbg_print_header(0,0,"well known locked files search completed in %" LL64 "u ms",
        (ULONGLONG)(winx_xtime() - time));
}

/**
 * @brief Adds file to the list of fragmented files.
 */
int expand_fragmented_files_list(winx_file_info *f,udefrag_job_parameters *jp)
{
    udefrag_fragmented_file *ff, *ffprev = NULL;

    for(ff = jp->fragmented_files; ff; ff = ff->next){
        if(ff->f->disp.fragments <= f->disp.fragments){
            if(ff != jp->fragmented_files)
                ffprev = ff->prev;
            break;
        }
        if(ff->next == jp->fragmented_files){
            ffprev = ff;
            break;
        }
    }
    
    ff = (udefrag_fragmented_file *)winx_list_insert_item((list_entry **)(void *)&jp->fragmented_files,
            (list_entry *)ffprev,sizeof(udefrag_fragmented_file));
    if(ff == NULL){
        DebugPrint("expand_fragmented_files_list: cannot allocate %u bytes of memory",
            sizeof(udefrag_fragmented_file));
        return (-1);
    } else {
        ff->f = f;
    }
    
    return 0;
}

/**
 * @brief Removes file from the list of fragmented files.
 */
void truncate_fragmented_files_list(winx_file_info *f,udefrag_job_parameters *jp)
{
    udefrag_fragmented_file *ff;
    
    for(ff = jp->fragmented_files; ff; ff = ff->next){
        if(ff->f == f){
            winx_list_remove_item((list_entry **)(void *)&jp->fragmented_files,(list_entry *)ff);
            break;
        }
        if(ff->next == jp->fragmented_files) break;
    }
}

/**
 * @brief Produces list of fragmented files.
 */
static void produce_list_of_fragmented_files(udefrag_job_parameters *jp)
{
    winx_file_info *f;
    
    for(f = jp->filelist; f; f = f->next){
        if(is_fragmented(f) && !is_excluded(f)){
            /* exclude files with empty path */
            if(f->path != NULL){
                if(f->path[0] != 0)
                    expand_fragmented_files_list(f,jp);
            }
        }
        if(f->next == jp->filelist) break;
    }
}

/**
 * @brief Define whether some
 * actions are allowed or not.
 * @return Zero indicates that
 * requested operation is allowed,
 * negative value indicates contrary.
 */
static int define_allowed_actions(udefrag_job_parameters *jp)
{
    int win_version;
    
    win_version = winx_get_os_version();
    
    /*
    * NTFS volumes with cluster size greater than 4 kb
    * cannot be defragmented on Windows 2000.
    * This is a well known limitation of Windows Defrag API.
    */
    if(jp->job_type != ANALYSIS_JOB \
      && jp->fs_type == FS_NTFS \
      && jp->v_info.bytes_per_cluster > 4096 \
      && win_version <= WINDOWS_2K){
        DebugPrint("cannot defragment NTFS volumes with clusters bigger than 4kb on nt4/w2k");
        return UDEFRAG_W2K_4KB_CLUSTERS;
    }
      
    /*
    * UDF volumes can neither be defragmented nor optimized,
    * because the system driver does not support FSCTL_MOVE_FILE.
    *
    * Windows 7 needs more testing, since it seems to allow defrag
    */
    if(jp->job_type != ANALYSIS_JOB \
      && jp->fs_type == FS_UDF \
      /* && win_version <= WINDOWS_VISTA */){
        DebugPrint("cannot defragment/optimize UDF volumes,");
        DebugPrint("because the file system driver does not support FSCTL_MOVE_FILE.");
        return UDEFRAG_UDF_DEFRAG;
    }
    
    /*
    * FAT volumes cannot be optimized.
    */
    if((jp->job_type == FULL_OPTIMIZATION_JOB || jp->job_type == QUICK_OPTIMIZATION_JOB) \
      && (jp->fs_type == FS_FAT12 \
      || jp->fs_type == FS_FAT16 \
      || jp->fs_type == FS_FAT32 \
      || jp->fs_type == FS_FAT32_UNRECOGNIZED)){
        DebugPrint("cannot optimize FAT volumes because of unmovable directories");
        return UDEFRAG_FAT_OPTIMIZATION;
    }
      
    /* MFT optimization is impossible in some cases. */
    if(jp->job_type == MFT_OPTIMIZATION_JOB){
        if(jp->fs_type != FS_NTFS){
            DebugPrint("MFT can be optimized on NTFS volumes only");
            return UDEFRAG_NO_MFT;
        }
        if(winx_get_os_version() <= WINDOWS_2K){
            DebugPrint("MFT is not movable on NT4 and Windows 2000");
            return UDEFRAG_UNMOVABLE_MFT;
        }            
    }

    switch(jp->fs_type){
    case FS_FAT12:
    case FS_FAT16:
    case FS_FAT32:
    case FS_FAT32_UNRECOGNIZED:
        jp->actions.allow_dir_defrag = 0;
        jp->actions.allow_optimize = 0;
        break;
    case FS_NTFS:
        jp->actions.allow_dir_defrag = 1;
        jp->actions.allow_optimize = 1;
        break;
    default:
        jp->actions.allow_dir_defrag = 0;
        jp->actions.allow_optimize = 0;
        /*jp->actions.allow_dir_defrag = 1;
        jp->actions.allow_optimize = 1;*/
        break;
    }
    
    if(jp->actions.allow_dir_defrag)
        DebugPrint("directory defragmentation is allowed");
    else
        DebugPrint("directory defragmentation is denied (because not possible)");
    
    if(jp->actions.allow_optimize)
        DebugPrint("volume optimization is allowed");
    else
        DebugPrint("volume optimization is denied (because not possible)");
    
    if(jp->fs_type == FS_NTFS){
        if(win_version < WINDOWS_XP){
            /*DebugPrint("$mft defragmentation is not possible");*/
            DebugPrint("$mft optimization is not possible");
        } else {
            /*DebugPrint("partial $mft defragmentation is possible");
            DebugPrint("(the first 16 clusters aren\'t movable)");*/
            DebugPrint("$mft optimization is possible");
        }
    }
    
    return 0;
}

/**
 * @brief Displays message like
 * <b>analysis of c: started</b>
 * and returns the current time
 * (needed for stop_timing).
 */
ULONGLONG start_timing(char *operation_name,udefrag_job_parameters *jp)
{
#ifdef LINUX
    winx_dbg_print_header(0,0,"%s of %s started",operation_name,volumes[jp->volume_letter]);
#else
    winx_dbg_print_header(0,0,"%s of %c: started",operation_name,jp->volume_letter);
#endif
    progress_trigger = 0;
    return winx_xtime();
}

/**
 * @brief Displays time needed 
 * for the operation; the second
 * parameter must be obtained from
 * the start_timing procedure.
 */
void stop_timing(char *operation_name,ULONGLONG start_time,udefrag_job_parameters *jp)
{
    ULONGLONG time, seconds;
    char buffer[32];
    
    time = winx_xtime() - start_time;
    seconds = time / 1000;
    winx_time2str(seconds,buffer,sizeof(buffer));
    time -= seconds * 1000;
#ifdef LINUX
    winx_dbg_print_header(0,0,"%s of %s completed in %s %" LL64 "ums",
        operation_name,jp->volume,buffer,(ULONGLONG)time);
#else
    winx_dbg_print_header(0,0,"%s of %c: completed in %s %" LL64 "ums",
        operation_name,jp->volume_letter,buffer,(ULONGLONG)time);
#endif
    progress_trigger = 0;
}

/**
 * @brief Performs a volume analysis.
 * @return Zero for success, negative value otherwise.
 */
int analyze(udefrag_job_parameters *jp)
{
    ULONGLONG time;
    int result;
    
    time = start_timing("analysis",jp);
    jp->pi.current_operation = VOLUME_ANALYSIS;
    
    /* update volume information */
    if(get_volume_information(jp) < 0)
        return (-1);
    
    /* scan volume for free space areas */
    if(get_free_space_layout(jp) < 0)
        return (-1);
    
    /* apply information about mft zones */
    get_mft_zones_layout(jp);
    
    /* search for files */
    if(find_files(jp) < 0)
        return (-1);
    
    /* we'd like to behold well known locked files in green color */
    redraw_well_known_locked_files(jp);

    /* produce list of fragmented files */
    produce_list_of_fragmented_files(jp);

    result = define_allowed_actions(jp);
    if(result < 0)
        return result;
    
    jp->p_counters.analysis_time = winx_xtime() - time;
    stop_timing("analysis",time,jp);
    return 0;
}

/** @} */
