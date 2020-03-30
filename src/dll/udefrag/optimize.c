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
 * @file optimize.c
 * @brief Volume optimization.
 * @addtogroup Optimizer
 * @{
 */

#include "compiler.h"
#include "udefrag-internals.h"

static void calculate_free_rgn_size_threshold(udefrag_job_parameters *jp);
static int increase_starting_point(udefrag_job_parameters *jp, ULONGLONG *sp);

/**
 * @brief Performs a volume optimization.
 * @details The volume optimization consists of two
 * phases, repeated in case of multipass processing.
 *
 * First of all, we split disk into two parts - the
 * first one is already optimized while the second one
 * needs to be optimized. So called starting point
 * represents the bound between parts.
 *
 * Then, we move all fragmented files from the beginning
 * of the volume to its terminal part. After that we
 * move everything locating after the starting point in 
 * the same direction (in full optimization only).
 * When all the operations complete, we have two arrays
 * of data (already optimized and moved to the end)
 * separated by a free space region.
 *
 * Then, we begin to move files from the end to the beginning.
 * Before each move we ensure that the file will be not moved back
 * on the next pass.
 *
 * If nothing has been moved, we advance starting point to the next
 * suitable free region and continue the volume processing.
 *
 * This algorithm guarantees that no repeated moves are possible.
 * @return Zero for success, negative value otherwise.
 */
int optimize(udefrag_job_parameters *jp)
{
    int result, overall_result = -1;
    ULONGLONG start_lcn = 0, new_start_lcn;
    ULONGLONG remaining_clusters;

    /* perform volume analysis */
    result = analyze(jp); /* we need to call it once, here */
    if(result < 0) return result;
    
    /* set free region size threshold, reset counters */
    calculate_free_rgn_size_threshold(jp);
    jp->pi.processed_clusters = 0;
    /* we have a chance to move everything to the end and then in contrary direction */
    jp->pi.clusters_to_process = (jp->v_info.total_clusters - \
        jp->v_info.free_bytes / jp->v_info.bytes_per_cluster) * 2;
    
    /* optimize MFT separately to keep its optimal location */
    result = optimize_mft_helper(jp);
    if(result == 0){
        /* at least mft optimization succeeded */
        overall_result = 0;
    }

    /* do the job */
    jp->pi.pass_number = 0;
    while(!jp->termination_router((void *)jp)){
        /* exclude already optimized part of the volume */
        new_start_lcn = calculate_starting_point(jp,start_lcn);
        if(0){//if(new_start_lcn <= start_lcn && jp->pi.pass_number){
            DebugPrint("volume optimization completed: old_sp = %" LL64 "u, new_sp = %" LL64 "u",
                (ULONGLONG)start_lcn, (ULONGLONG)new_start_lcn);
            break;
        }
        if(new_start_lcn > start_lcn)
            start_lcn = new_start_lcn;
        
        /* reset counters */
        remaining_clusters = get_number_of_movable_clusters(jp,start_lcn,jp->v_info.total_clusters,MOVE_ALL);
        jp->pi.processed_clusters = 0; /* reset counter */
        jp->pi.clusters_to_process = (jp->v_info.total_clusters - \
            jp->v_info.free_bytes / jp->v_info.bytes_per_cluster) * 2;
        jp->pi.processed_clusters = jp->pi.clusters_to_process - \
            remaining_clusters * 2; /* set counter */
                
        DebugPrint("volume optimization pass #%u, starting point = %" LL64 "u, remaining clusters = %" LL64 "u",
            jp->pi.pass_number, (ULONGLONG)start_lcn, (ULONGLONG)remaining_clusters);
        
        /* move fragmented files to the terminal part of the volume */
        result = move_files_to_back(jp, 0, MOVE_FRAGMENTED);
        if(result == 0){
            /* at least something succeeded */
            overall_result = 0;
        }
        
        /* break if nothing moveable exist after starting point */
        if(jp->pi.moved_clusters == 0 && remaining_clusters == 0) break;
        
        /* full opt: cleanup space after start_lcn */
        if(jp->job_type == FULL_OPTIMIZATION_JOB){
            result = move_files_to_back(jp, start_lcn, MOVE_ALL);
            if(result == 0){
                /* at least something succeeded */
                overall_result = 0;
            }
        }
        
        /* move files back to the beginning */
        result = move_files_to_front(jp, start_lcn, MOVE_ALL);
        if(result == 0){
            /* at least something succeeded */
            overall_result = 0;
        }
        
        /* move starting point to the next free region if nothing moved */
        if(result < 0 || jp->pi.moved_clusters == 0){
            if(increase_starting_point(jp, &start_lcn) < 0)
                break; /* end of disk reached */
        }
        
        /* break if no repeat */
        if(!(jp->udo.job_flags & UD_JOB_REPEAT)) break;
        
        /* go to the next pass */
        jp->pi.pass_number ++;
    }
    
    if(jp->termination_router((void *)jp)) return 0;
    
    return overall_result;
}

/**
 * @brief Optimizes MFT.
 */
int optimize_mft(udefrag_job_parameters *jp)
{
    int result;
    winx_file_info *file;
    
    /* perform volume analysis */
    result = analyze(jp); /* we need to call it once, here */
    if(result < 0) return result;
    
    /* reset counters */
    jp->pi.processed_clusters = 0;
    jp->pi.clusters_to_process = 1;
    for(file = jp->filelist; file; file = file->next){
        if(is_mft(file,jp)){
            /* we'll move no more than double size of MFT */
            jp->pi.clusters_to_process = file->disp.clusters * 2;
            break;
        }
        if(file->next == jp->filelist) break;
    }
    
    /* optimize MFT */
    result = optimize_mft_helper(jp);
    if(result < 0) return result;
    
    /* defragment files fragmented by MFT optimizer */
    result = defragment(jp);
    return result;
}

/************************* Internal routines ****************************/

/**
 * @brief Calculates free region size
 * threshold used in volume optimization.
 */
static void calculate_free_rgn_size_threshold(udefrag_job_parameters *jp)
{
    winx_volume_region *rgn;
    ULONGLONG length = 0;

    if(jp->v_info.free_bytes >= jp->v_info.total_bytes / 10){
        /*
        * We have at least 10% of free space on the volume, so
        * it seems to be reasonable to put all data together
        * even if the free space is split to many little pieces.
        */
        DebugPrint("calculate_free_rgn_size_threshold: strategy #1 because of at least 10%% of free space on the volume");
        for(rgn = jp->free_regions; rgn; rgn = rgn->next){
            if(rgn->length > length)
                length = rgn->length;
            if(rgn->next == jp->free_regions) break;
        }
        /* Threshold = 0.5% of the volume or a half of the largest free space region. */
        jp->free_rgn_size_threshold = min(jp->v_info.total_clusters / 200, length / 2);
    } else {
        /*
        * On volumes with less than 10% of free space
        * we're searching for the free space region
        * at least 0.5% long.
        */
        DebugPrint("calculate_free_rgn_size_threshold: strategy #2 because of less than 10%% of free space on the volume");
        jp->free_rgn_size_threshold = jp->v_info.total_clusters / 200;
    }
    //jp->free_rgn_size_threshold >>= 1;
    if(jp->free_rgn_size_threshold < 2) jp->free_rgn_size_threshold = 2;
    DebugPrint("free region size threshold = %" LL64 "u clusters",(ULONGLONG)jp->free_rgn_size_threshold);
}

/**
 * @brief Calculates starting point
 * for the volume optimization process
 * to skip already optimized data.
 * All the clusters before it will
 * be skipped in the move_files_to_back
 * routine.
 */
ULONGLONG calculate_starting_point(udefrag_job_parameters *jp, ULONGLONG old_sp)
{
    ULONGLONG new_sp;
    ULONGLONG fragmented, free, lim, i, max_new_sp;
    winx_volume_region *rgn;
    udefrag_fragmented_file *f;    
    winx_blockmap *block;
    ULONGLONG time;

    /* free temporarily allocated space */
    release_temp_space_regions_internal(jp);

    /* search for the first large free space gap after an old starting point */
    new_sp = old_sp; time = winx_xtime();
    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(jp->udo.dbgprint_level >= DBG_PARANOID)
            DebugPrint("Free block start: %" LL64 "u len: %" LL64 "u",(ULONGLONG)rgn->lcn,(ULONGLONG)rgn->length);
        if(rgn->lcn >= old_sp && rgn->length >= jp->free_rgn_size_threshold){
            new_sp = rgn->lcn;
            break;
        }
        if(rgn->next == jp->free_regions) break;
    }
    jp->p_counters.searching_time += winx_xtime() - time;
    
    /* move starting point back to release heavily fragmented data */
    /* allow no more than 5% of fragmented data inside of a skipped part of the disk */
    fragmented = get_number_of_fragmented_clusters(jp,old_sp,new_sp);
    if(fragmented > (new_sp - old_sp) / 20){
        /* based on bsearch() algorithm from ReactOS */
        i = old_sp;
        for(lim = new_sp - old_sp + 1; lim; lim >>= 1){
            new_sp = i + (lim >> 1);
            fragmented = get_number_of_fragmented_clusters(jp,old_sp,new_sp);
            if(fragmented > (new_sp - old_sp) / 20){
                /* move left */
            } else {
                /* move right */
                i = new_sp + 1; lim --;
            }
        }
    }
    if(new_sp == old_sp)
        return old_sp;
    
    //DebugPrint("*** 1 old = %" LL64 "u, new = %" LL64 "u ***",(ULONGLONG)old_sp,(ULONGLONG)new_sp);
    /* cut off heavily fragmented free space */
    i = old_sp; max_new_sp = new_sp;
    for(lim = new_sp - old_sp + 1; lim; lim >>= 1){
        new_sp = i + (lim >> 1);
        free = get_number_of_free_clusters(jp,new_sp,max_new_sp);
        if(free > (max_new_sp - new_sp) / 3){
            /* move left */
            //max_new_sp = new_sp;
        } else {
            /* move right */
            i = new_sp + 1; lim --;
        }
    }
    if(new_sp == old_sp)
        return old_sp;
    //DebugPrint("*** 2 old = %" LL64 "u, new = %" LL64 "u ***",(ULONGLONG)old_sp,(ULONGLONG)new_sp);
    
    /* is starting point inside a fragmented file block? */
    time = winx_xtime();
    for(f = jp->fragmented_files; f; f = f->next){
        for(block = f->f->disp.blockmap; block; block = block->next){
            if(new_sp >= block->lcn && new_sp <= block->lcn + block->length - 1){
                if(can_move(f->f,jp) && !is_mft(f->f,jp) && !is_file_locked(f->f,jp)){
                    /* include block */
                    jp->p_counters.searching_time += winx_xtime() - time;
                    return block->lcn;
                } else {
                    goto skip_unmovable_files;
                }
            }
            if(block->next == f->f->disp.blockmap) break;
        }
        if(f->next == jp->fragmented_files) break;
    }
    jp->p_counters.searching_time += winx_xtime() - time;

skip_unmovable_files:    
    /* skip not movable contents */
    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(rgn->lcn > new_sp){
            if(get_number_of_movable_clusters(jp,new_sp,rgn->lcn,MOVE_ALL) != 0){
                break;
            } else {
                new_sp = rgn->lcn;
            }
        }
        if(rgn->next == jp->free_regions) break;
    }
    return new_sp;
}

/**
 * @brief Moves starting point to the next free region.
 * @return Zero for success, negative value otherwise.
 */
static int increase_starting_point(udefrag_job_parameters *jp, ULONGLONG *sp)
{
    ULONGLONG new_sp;
    winx_volume_region *rgn;
    ULONGLONG time;
    
    if(sp == NULL)
        return (-1);
    
    new_sp = calculate_starting_point(jp,*sp);

    /* go to the first large free space gap after a new starting point */
    time = winx_xtime();
    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(rgn->lcn > new_sp && rgn->length >= jp->free_rgn_size_threshold){
            *sp = rgn->lcn;
            jp->p_counters.searching_time += winx_xtime() - time;
            return 0;
        }
        if(rgn->next == jp->free_regions) break;
    }
    /* end of disk reached */
    jp->p_counters.searching_time += winx_xtime() - time;
    return (-1);
}

/** @} */
