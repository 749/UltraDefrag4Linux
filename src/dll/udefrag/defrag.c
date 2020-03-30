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
 * @file defrag.c
 * @brief Volume defragmentation.
 * @addtogroup Defrag
 * @{
 */

/*
* Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
* and Dmitri Arkhangelski <dmitriar@gmail.com>.
*/

#include "compiler.h"
#include "udefrag-internals.h"

/*
* Uncomment it to test defragmentation
* of various special files like reparse
* points, attribute lists and others.
*/
//#define TEST_SPECIAL_FILES_DEFRAG

/* Test suite for special files. */
#ifdef TEST_SPECIAL_FILES_DEFRAG
void test_move(winx_file_info *f,udefrag_job_parameters *jp)
{
    winx_volume_region *target_rgn;
    ULONGLONG source_lcn = f->disp.blockmap->lcn;
    
    /* try to move the first cluster to the last free region */
    target_rgn = find_last_free_region(jp,1);
    if(target_rgn == NULL){
        DebugPrint("test_move: no free region found on disk");
        return;
    }
    if(move_file(f,f->disp.blockmap->vcn,1,target_rgn->lcn,0,jp) < 0){
        DebugPrint("test_move: move failed for %" WSTR,printableutf(f->path));
        return;
    } else {
        DebugPrint("test_move: move succeeded for %" WSTR,printableutf(f->path));
    }
    /* release temporary allocated clusters */
    release_temp_space_regions(jp);
    /* try to move the first cluster back */
    if(can_move(f,jp)){
        if(move_file(f,f->disp.blockmap->vcn,1,source_lcn,0,jp) < 0){
            DebugPrint("test_move: move failed for %" WSTR,printableutf(f->path));
            return;
        } else {
            DebugPrint("test_move: move succeeded for %" WSTR,printableutf(f->path));
        }
    } else {
        DebugPrint("test_move: file became unmovable %" WSTR,printableutf(f->path));
    }
}

/*
* Tests defragmentation of reparse points,
* encrypted files, bitmaps and attribute lists.
*/
void test_special_files_defrag(udefrag_job_parameters *jp)
{
    winx_file_info *f;
    int special_file = 0;
    
    DebugPrint("test of special files defragmentation started");

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return;

    for(f = jp->filelist; f; f = f->next){
        if(can_move(f,jp)){
            special_file = 0;
            if(is_reparse_point(f)){
                DebugPrint("reparse point detected: %" WSTR,printableutf(f->path));
                special_file = 1;
            } else if(is_encrypted(f)){
                DebugPrint("encrypted file detected: %" WSTR,printableutf(f->path));
                special_file = 1;
            } else if(winx_utfistr(f->path,UTF("$BITMAP"))){
                DebugPrint("bitmap detected: %" WSTR,printableutf(f->path));
                special_file = 1;
            } else if(winx_utfistr(f->path,UTF("$ATTRIBUTE_LIST"))){
                DebugPrint("attribute list detected: %" WSTR,printableutf(f->path));
                special_file = 1;
            }
            if(special_file)
                test_move(f,jp);
        }
        if(f->next == jp->filelist) break;
    }
    
    winx_fclose(jp->fVolume);
    DebugPrint("test of special files defragmentation completed");
}
#endif /* TEST_SPECIAL_FILES_DEFRAG */

/**
 * @brief Calculates total number of fragmented clusters.
 */
static ULONGLONG fragmented_clusters(udefrag_job_parameters *jp)
{
    udefrag_fragmented_file *f;
    ULONGLONG n = 0;
    
    for(f = jp->fragmented_files; f; f = f->next){
        if(jp->termination_router((void *)jp)) break;
        /*
        * Count all fragmented files which can be processed.
        */
        if(can_defragment(f->f,jp))
            n += f->f->disp.clusters;
        if(f->next == jp->fragmented_files) break;
    }
    return n;
}

/**
 * @brief Performs a volume defragmentation.
 * @details To avoid infinite data moves in multipass
 * processing, we exclude files for which moving failed.
 * On the other hand, number of fragmented files instantly
 * decreases, so we'll never have infinite loops here.
 * @return Zero for success, negative value otherwise.
 */
int defragment(udefrag_job_parameters *jp)
{
    disk_processing_routine defrag_routine = NULL;
    int result, overall_result = -1;
    
    /* perform volume analysis */
    if(jp->job_type != MFT_OPTIMIZATION_JOB){
        result = analyze(jp); /* we need to call it once, here */
        if(result < 0) return result;
    }
    
#ifdef TEST_SPECIAL_FILES_DEFRAG
    if(jp->job_type == DEFRAGMENTATION_JOB){
        test_special_files_defrag(jp);
        return 0;
    }
#endif
    
    /* choose defragmentation strategy */
    if(jp->pi.fragmented >= jp->free_regions_count || \
       jp->udo.job_flags & UD_PREVIEW_MATCHING){
        DebugPrint("defragment: walking fragmented files list");
        defrag_routine = defragment_small_files_walk_fragmented_files;
    } else {
        DebugPrint("defragment: walking free regions list");
        defrag_routine = defragment_small_files_walk_free_regions;
    }
    
    /* do the job */
    jp->pi.pass_number = 0;
    while(!jp->termination_router((void *)jp)){
        /* reset counters */
        jp->pi.processed_clusters = 0;
        jp->pi.clusters_to_process = fragmented_clusters(jp);
        
        result = defrag_routine(jp);
        if(result == 0){
            /* defragmentation succeeded at least once */
            overall_result = 0;
        }
        
        /* break if nothing moved */
        if(result < 0 || jp->pi.moved_clusters == 0) break;
        
        /* break if no repeat */
        if(!(jp->udo.job_flags & UD_JOB_REPEAT)) break;
        
        /* go to the next pass */
        jp->pi.pass_number ++;
    }
    
    if(jp->termination_router((void *)jp)) return 0;
    
    if(jp->job_type == MFT_OPTIMIZATION_JOB) return overall_result;
    
    /* perform partial defragmentation */
    jp->pi.processed_clusters = 0;
    jp->pi.clusters_to_process = fragmented_clusters(jp);
    result = defragment_big_files(jp);
    if(result == 0){
        /* at least partial defragmentation succeeded */
        overall_result = 0;
    }

    if(jp->termination_router((void *)jp)) return 0;
    
    return overall_result;
}

/** @} */
