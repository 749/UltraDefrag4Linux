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
 * @file move.c
 * @brief File moving.
 * @addtogroup Move
 * @{
 */

#include "compiler.h"
#include "udefrag-internals.h"

/************************************************************/
/*                    Internal Routines                     */
/************************************************************/

/*
* The following routines are used to track
* space temporarily allocated by system
* and to release this space before the file
* moving.
*/

/**
 * @brief Adds region to the list of space
 * temporarily allocated by system.
 * @return Zero for success, negative value otherwise.
 * @note Use release_temp_space_regions to free 
 * memory allocated by add_temp_space_region calls.
 */
static int add_temp_space_region(udefrag_job_parameters *jp,ULONGLONG lcn,ULONGLONG length)
{
    winx_volume_region *rgn;
    
    rgn = (winx_volume_region *)winx_list_insert_item(
        (list_entry **)(void *)&jp->temp_space_list,NULL,
        sizeof(winx_volume_region));
    if(rgn == NULL){
        DebugPrint("add_temp_space_region: cannot allocate %u bytes of memory",
            sizeof(winx_volume_region));
        return (-1);
    } else {
        rgn->lcn = lcn;
        rgn->length = length;
    }
    
    return 0;
}

#if 0 /* JPA temporary */

void showregions(const char *text, udefrag_job_parameters *jp)
{
    winx_volume_region *rgn,*next,*first;

    rgn = winx_get_free_volume_regions(jp->volume_letter,
        WINX_GVR_ALLOW_PARTIAL_SCAN,NULL,(void *)jp);
    first = rgn;
    DebugPrint("actual free regions at %s",text);
    if (rgn)
        do {
            DebugPrint(" a 0x%lx lcn 0x%llx-0x%llx lth %ld",(long)rgn,(long long)rgn->lcn,(long long)rgn->lcn+rgn->length,(long)rgn->length);
            next = rgn->next;
            free(rgn);
            rgn = next;
        } while (rgn && (rgn != first));
    DebugPrint("free regions list at %s",text);
    rgn = jp->free_regions;
    if (rgn)
        do {
            DebugPrint(" f 0x%lx lcn 0x%llx-0x%llx lth %ld",(long)rgn,(long long)rgn->lcn,(long long)rgn->lcn+rgn->length,(long)rgn->length);
            rgn = rgn->next;
        } while (rgn && (rgn != jp->free_regions));
    DebugPrint("temp space list at %s",text);
/*
    rgn = jp->temp_space_list;
    if (rgn)
        do {
            DebugPrint(" t 0x%lx lcn 0x%llx-0x%llx lth %ld",(long)rgn,(long long)rgn->lcn,(long long)rgn->lcn+rgn->length,(long)rgn->length);
            rgn = rgn->next;
        } while (rgn && (rgn != jp->temp_space_list));
*/
    fflush(stderr);
}

void checkregions(udefrag_job_parameters *jp, long long lcn, long length)
{
    winx_volume_region *rgn;

    rgn = jp->free_regions;
    if (rgn)
        do {
            if (((unsigned long long)rgn->lcn > 0x100000) || ((unsigned long)rgn->length > 0x10000)) {
                DebugPrint(" bad 0x%lx lcn 0x%llx-0x%llx lth %ld",(long)rgn,(long long)rgn->lcn,(long long)rgn->lcn+rgn->length,(long)rgn->length);
                DebugPrint(" after inserting lcn 0x%llx lth %ld",(long long)lcn,(long)length);
            }
            rgn = rgn->next;
        } while (rgn && (rgn != jp->free_regions));
}

#endif

/**
 * @brief This auxiliary routine intended for use in
 * calculate_starting_point just to speed things up.
 */
void release_temp_space_regions_internal(udefrag_job_parameters *jp)
{
    winx_volume_region *rgn;

    /* update free space pool */
    for(rgn = jp->temp_space_list; rgn; rgn = rgn->next){
        jp->free_regions = winx_add_volume_region(jp->free_regions,rgn->lcn,rgn->length);
        if(rgn->next == jp->temp_space_list) break;
    }
    
    /* redraw map */
    redraw_all_temporary_system_space_as_free(jp);
    
    /* free memory */
    winx_list_destroy((list_entry **)(void *)&jp->temp_space_list);
    jp->temp_space_list = NULL;
}

/**
 * @brief Releases all space regions
 * added by add_temp_space_region calls.
 * @details This routine forces Windows
 * to mark space regions as free. Also
 * it colorizes cluster map to reflect
 * changes and frees memory allocated by
 * add_temp_space_region calls.
 */
void release_temp_space_regions(udefrag_job_parameters *jp)
{
#if 0 /* JPA : use the real free list */
    winx_volume_region *rgn;
    ULONGLONG time = winx_xtime();
    
    /* release space on disk */
    rgn = winx_get_free_volume_regions(jp->volume_letter,
        WINX_GVR_ALLOW_PARTIAL_SCAN,NULL,(void *)jp);
    jp->p_counters.temp_space_releasing_time += winx_xtime() - time;
    
    release_temp_space_regions_internal(jp);
    winx_release_free_volume_regions(jp->free_regions);
    jp->free_regions = rgn;
#else
    release_temp_space_regions_internal(jp);
#endif
}

/**
 * @brief Redraws freed space.
 */
static void redraw_freed_space(udefrag_job_parameters *jp,
        ULONGLONG lcn, ULONGLONG length, int old_color)
{
    /*
    * On FAT partitions after file moving filesystem driver marks
    * previously allocated clusters as free immediately.
    * But on NTFS they are always still allocated by system.
    * Only the next volume analysis frees them.
    */
    if(jp->fs_type == FS_NTFS){
        /* mark clusters as temporarily allocated by system  */
        colorize_map_region(jp,lcn,length,TEMPORARY_SYSTEM_SPACE,old_color);
        add_temp_space_region(jp,lcn,length);
    } else {
        colorize_map_region(jp,lcn,length,FREE_SPACE,old_color);
        jp->free_regions = winx_add_volume_region(jp->free_regions,lcn,length);
    }
}

/**
 * @brief Returns the first file block
 * belonging to the cluster chain.
 */
winx_blockmap *get_first_block_of_cluster_chain(winx_file_info *f,ULONGLONG vcn)
{
    winx_blockmap *block;
    
    for(block = f->disp.blockmap; block; block = block->next){
        if(vcn >= block->vcn && vcn < block->vcn + block->length)
            return block;
        if(block->next == f->disp.blockmap) break;
    }
    return NULL;
}

/**
 * @brief Checks whether the cluster chain 
 * starts on a specified LCN and runs 
 * continuously from there or not.
 */
static int check_cluster_chain_location(winx_file_info *f,ULONGLONG vcn,ULONGLONG length,ULONGLONG startLcn)
{
    winx_blockmap *block, *first_block;
    ULONGLONG clusters_to_check, curr_vcn, curr_target, n;
    
    first_block = get_first_block_of_cluster_chain(f,vcn);
    if(first_block == NULL){
        /* is file empty? */
        if(f->disp.blockmap == NULL)
            return 1; /* let's assume a successful move in case of empty file */
        /* is vcn outside of the current file length? */
        if(vcn >= f->disp.blockmap->prev->vcn + f->disp.blockmap->prev->length)
            return 1; /* let's assume a successful move of non existing part of the file */
        /* sparse file changed its contents, so let's assume a partial move */
        return 0;
    }
    
    if(length == 0){
        /* let's assume a successful move of zero number of clusters */
        return 1;
    }
    
    clusters_to_check = length;
    curr_vcn = vcn;
    curr_target = startLcn;
    for(block = first_block; block; block = block->next){
        if(curr_target != block->lcn + (curr_vcn - block->vcn)){
            /* current block is not found on target location */
            return 0;
        }

        n = min(block->length - (curr_vcn - block->vcn),clusters_to_check);
        curr_target += n;
        clusters_to_check -= n;
        
        if(!clusters_to_check || block->next == f->disp.blockmap) break;
        curr_vcn = block->next->vcn;
    }
    
    /*
    * All clusters passed the check, or blocks
    * exhausted because of a file truncation.
    */
    return 1;
}

/**
 * @brief Moves file clusters.
 * @return Zero for success,
 * negative value otherwise.
 * @note 
 * - On NT4 this function can move
 * no more than 256 kilobytes once.
 * - Volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 */
static int move_file_clusters(HANDLE hFile,ULONGLONG startVcn,
    ULONGLONG targetLcn,ULONGLONG n_clusters,udefrag_job_parameters *jp)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    MOVEFILE_DESCRIPTOR mfd;
ULONGLONG now;

    if(jp->udo.dbgprint_level >= DBG_DETAILED){
        DebugPrint("sVcn: %" LL64 "u,tLcn: %" LL64 "u,n: %" LL64 "u",
             (ULONGLONG)startVcn,(ULONGLONG)targetLcn,(ULONGLONG)n_clusters);
    }
    
    if(jp->termination_router((void *)jp))
        return (-1);
    
    if(jp->udo.dry_run){
        jp->pi.moved_clusters += n_clusters;
        return 0;
    }

        /* JPA update progress every second */
    now = winx_xtime();
    if (now != jp->pi.feedback_time) {
        deliver_progress_info(jp,0); /* 0: running */
        jp->pi.feedback_time = now;
    }

    /* setup movefile descriptor and make the call */
    mfd.FileHandle = hFile;
    mfd.StartVcn.QuadPart = startVcn;
    mfd.TargetLcn.QuadPart = targetLcn;
#ifdef _WIN64
    mfd.NumVcns = n_clusters;
#else
    mfd.NumVcns = (ULONG)n_clusters;
#endif
/*
Dump of NtDeviceIoControlFile input 32 bytes at 0xb3f2d8 (Windows endian)
0000  9c000000 2d000000 5e080000 00000000     ....-...^.......
0010  873e0000 00000000 c2000000 03000000     .>..............
*/
    ExpertDump("NtDeviceIoControlFile input",(char*)&mfd,sizeof(MOVEFILE_DESCRIPTOR));
    Status = NtFsControlFile(winx_fileno(jp->fVolume),NULL,NULL,0,&iosb,
                        FSCTL_MOVE_FILE,&mfd,sizeof(MOVEFILE_DESCRIPTOR),
                        NULL,0);
    if(NT_SUCCESS(Status)){
        NtWaitForSingleObject(winx_fileno(jp->fVolume),FALSE,NULL);
        Status = iosb.Status;
    }
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"cannot move file clusters");
        return (-1);
    }

    /*
    * Actually moving result is unknown here,
    * because API may return success, while
    * actually clusters may be moved partially.
    */
    jp->pi.moved_clusters += n_clusters;
    return 0;
}

/**
 * @brief move_file helper.
 */
static void move_file_helper(HANDLE hFile, winx_file_info *f,
    ULONGLONG vcn, ULONGLONG length, ULONGLONG target,
    udefrag_job_parameters *jp)
{
    winx_blockmap *block, *first_block;
    ULONGLONG curr_vcn, curr_target, j, n, r;
    ULONGLONG clusters_to_process;
    ULONGLONG clusters_to_move;
    ULONGLONG extra_clusters;
    int result;
    
    clusters_to_process = length;
    curr_vcn = vcn;
    curr_target = target;
    
    if(is_compressed(f) || is_sparse(f)){
        /* move blocks of file */
        first_block = get_first_block_of_cluster_chain(f,vcn);
        for(block = first_block; block; block = block->next){
            /* move the current block or its part */
            clusters_to_move = min(block->length - (curr_vcn - block->vcn),clusters_to_process);
            n = clusters_to_move / jp->clusters_per_256k;
            for(j = 0; j < n; j++){
                result = move_file_clusters(hFile,curr_vcn,
                    curr_target,jp->clusters_per_256k,jp);
                if(result < 0)
                    goto done;
                jp->pi.processed_clusters += jp->clusters_per_256k;
                clusters_to_process -= jp->clusters_per_256k;
                curr_vcn += jp->clusters_per_256k;
                curr_target += jp->clusters_per_256k;
            }
            /* try to move rest of the block */
            r = clusters_to_move % jp->clusters_per_256k;
            extra_clusters = r % 16; /* to comply with nt4/w2k rules */
            r -= extra_clusters;
            if(r){
                result = move_file_clusters(hFile,curr_vcn,curr_target,r,jp);
                if(result < 0)
                    goto done;
                jp->pi.processed_clusters += r;
                clusters_to_process -= r;
                curr_vcn += r;
                curr_target += r;
            }
            if(extra_clusters){
                result = move_file_clusters(hFile,curr_vcn,curr_target,extra_clusters,jp);
                if(result < 0)
                    goto done;
                jp->pi.processed_clusters += extra_clusters;
                clusters_to_process -= extra_clusters;
                curr_vcn += extra_clusters;
                curr_target += extra_clusters;
            }
            if(!clusters_to_move || block->next == f->disp.blockmap) break;
            curr_vcn = block->next->vcn;
        }
    } else {
        /* move file clusters regardless of block boundaries */
        /* this works much better on w2k/nt4 NTFS volumes */
        n = clusters_to_process / jp->clusters_per_256k;
        for(j = 0; j < n; j++){
            result = move_file_clusters(hFile,curr_vcn,
                curr_target,jp->clusters_per_256k,jp);
            if(result < 0)
                goto done;
            jp->pi.processed_clusters += jp->clusters_per_256k;
            clusters_to_process -= jp->clusters_per_256k;
            curr_vcn += jp->clusters_per_256k;
            curr_target += jp->clusters_per_256k;
        }
        /* try to move remaining clusters */
        r = clusters_to_process;
        extra_clusters = r % 16; /* to comply with nt4/w2k rules */
        r -= extra_clusters;
        if(r){
            result = move_file_clusters(hFile,curr_vcn,curr_target,r,jp);
            if(result < 0)
                goto done;
            jp->pi.processed_clusters += r;
            clusters_to_process -= r;
            curr_vcn += r;
            curr_target += r;
        }
        if(extra_clusters){
            result = move_file_clusters(hFile,curr_vcn,curr_target,extra_clusters,jp);
            if(result < 0)
                goto done;
            jp->pi.processed_clusters += extra_clusters;
            clusters_to_process -= extra_clusters;
        }
    }

done: /* count all unprocessed clusters here */
    jp->pi.processed_clusters += clusters_to_process;
}

/**
 * @brief Prints list of file blocks.
 */
static void DbgPrintBlocksOfFile(winx_blockmap *blockmap)
{
    winx_blockmap *block;
    
    for(block = blockmap; block; block = block->next){
        DebugPrint("VCN: %" LL64 "u, LCN: %" LL64 "u, LENGTH: %u",
            (ULONGLONG)block->vcn,(ULONGLONG)block->lcn,(unsigned int)block->length);
        if(block->next == blockmap) break;
    }
}

/**
 * @brief Compares two block maps.
 * @return Positive value indicates 
 * difference, zero indicates equality.
 * Negative value indicates that 
 * arguments are invalid.
 */
static int blockmap_compare(winx_file_disposition *disp1, winx_file_disposition *disp2)
{
    winx_blockmap *block1, *block2;

    /* validate arguments */
    if(disp1 == NULL || disp2 == NULL)
        return (-1);
    
    if(disp1->blockmap == disp2->blockmap)
        return 0;
    
    /* empty bitmap is not equal to non-empty one */
    if(disp1->blockmap == NULL || disp2->blockmap == NULL)
        return 1;

    /* ensure that both maps have non-negative number of elements */
    if(disp1->fragments <= 0 || disp2->fragments <= 0)
        return (-1);

    /* if number of file fragments differs, maps are different */
    if(disp1->fragments != disp2->fragments)
        return 1;

    /* comapare maps */    
    for(block1 = disp1->blockmap, block2 = disp2->blockmap;
      block1 && block2; block1 = block1->next, block2 = block2->next){
        if((block1->vcn != block2->vcn) 
          || (block1->lcn != block2->lcn)
          || (block1->length != block2->length))
            return 1;
        if(block1->next == disp1->blockmap || block2->next == disp2->blockmap) break;
    }
      
    if(block1->next != disp1->blockmap || block2->next != disp2->blockmap)
        return 1; /* one map is shorter than another */
    
    return 0;
}

/**
 * @brief Adds a new block to the file map.
 */
static winx_blockmap *add_new_block(winx_blockmap **head,ULONGLONG vcn,ULONGLONG lcn,ULONGLONG length)
{
    winx_blockmap *block, *last_block = NULL;
    
    if(*head != NULL)
        last_block = (*head)->prev;
    
    block = (winx_blockmap *)winx_list_insert_item((list_entry **)head,
                (list_entry *)last_block,sizeof(winx_blockmap));
    if(block != NULL){
        block->vcn = vcn;
        block->lcn = lcn;
        block->length = length;
    }
    return block;
}

/**
 * @brief Reduces number of blocks if possible.
 */
static void optimize_blockmap(winx_file_info *f)
{
    winx_blockmap *block;

    /* remove unnecessary blocks */
repeat_scan:
    for(block = f->disp.blockmap; block; block = block->next){
        if(block->next == f->disp.blockmap)
            break; /* no more pairs to be detected */
        /* does next block follow the current? */
        if(block->next->vcn == block->vcn + block->length \
         && block->next->lcn == block->lcn + block->length){
            /* adjust the current block and remove the next one */
            block->length += block->next->length;
            winx_list_remove_item((list_entry **)(void *)&f->disp.blockmap,(list_entry *)block->next);
            goto repeat_scan;
        }
    }
    
    /* adjust number of fragments and set flags */
    f->disp.fragments = 0;
    f->disp.flags &= ~WINX_FILE_DISP_FRAGMENTED;
    if(f->disp.blockmap != NULL)
        f->disp.fragments ++;
    for(block = f->disp.blockmap; block; block = block->next){
        if(block != f->disp.blockmap && \
          block->lcn != (block->prev->lcn + block->prev->length)){
            f->disp.fragments ++;
            f->disp.flags |= WINX_FILE_DISP_FRAGMENTED;
        }
        if(block->next == f->disp.blockmap) break;
    }
}

/**
 * @brief Calculates and builds a new file map respect 
 * to a successful move of the cluster chain.
 */
static winx_blockmap *calculate_new_blockmap(winx_file_info *f,ULONGLONG vcn,ULONGLONG length,ULONGLONG target)
{
    winx_blockmap *blockmap = NULL;
    winx_blockmap *block, *first_block, *new_block;
    ULONGLONG clusters_to_check, curr_vcn, curr_target, n;
    
    first_block = get_first_block_of_cluster_chain(f,vcn);
    if(first_block == NULL)
        return NULL;
    
    /* copy all blocks prior to the first_block to the new map */
    for(block = f->disp.blockmap; block && block != first_block; block = block->next){
        new_block = add_new_block(&blockmap,block->vcn,block->lcn,block->length);
        if(new_block == NULL) goto fail;
    }
    
    /* copy all remaining blocks */
    clusters_to_check = length;
    curr_vcn = vcn;
    curr_target = target;
    for(block = first_block; block; block = block->next){
        if(!clusters_to_check){
            new_block = add_new_block(&blockmap,block->vcn,block->lcn,block->length);
            if(new_block == NULL) goto fail;
        } else {
            n = min(block->length - (curr_vcn - block->vcn),clusters_to_check);
            
            if(curr_vcn != block->vcn){
                /* we have the second part of block moved */
                new_block = add_new_block(&blockmap,block->vcn,block->lcn,block->length - n);
                if(new_block == NULL) goto fail;
                new_block = add_new_block(&blockmap,curr_vcn,curr_target,n);
                if(new_block == NULL) goto fail;
            } else {
                if(n != block->length){
                    /* we have the first part of block moved */
                    new_block = add_new_block(&blockmap,curr_vcn,curr_target,n);
                    if(new_block == NULL) goto fail;
                    new_block = add_new_block(&blockmap,block->vcn + n,block->lcn + n,block->length - n);
                    if(new_block == NULL) goto fail;
                } else {
                    /* we have entire block moved */
                    new_block = add_new_block(&blockmap,block->vcn,curr_target,block->length);
                    if(new_block == NULL) goto fail;
                }
            }

            curr_target += n;
            clusters_to_check -= n;
        }
        if(block->next == f->disp.blockmap) break;
        curr_vcn = block->next->vcn;
    }
    return blockmap;
    
fail:
    DebugPrint("calculate_new_blockmap: not enough memory");
    winx_list_destroy((list_entry **)(void *)&blockmap);
    return NULL;
}

/**
 * @brief Cuts off range of clusters from the file map.
 */
static void subtract_clusters(winx_file_info *f, ULONGLONG vcn,
    ULONGLONG length, udefrag_job_parameters *jp)
{
    winx_blockmap *block, *first_block, *new_block;
    ULONGLONG clusters_to_cut = length;
    ULONGLONG new_lcn, new_vcn, new_length;
    
    first_block = get_first_block_of_cluster_chain(f,vcn);
    for(block = first_block; block; block = block->next){
        if(vcn > block->vcn){
            /* cut off something inside the first block of sequence */
            if(clusters_to_cut >= (block->length - (vcn - block->vcn))){
                /* cut off right side entirely */
                clusters_to_cut -= block->length - (vcn - block->vcn);
                block->length = vcn - block->vcn;
            } else {
                /* remove a central part of the block */
                new_length = block->length - (vcn - block->vcn) - clusters_to_cut;
                block->length = vcn - block->vcn;
                new_vcn = vcn + clusters_to_cut;
                new_lcn = block->lcn + block->length + clusters_to_cut;
                /* add a new block to the map */
                new_block = (winx_blockmap *)winx_list_insert_item((list_entry **)&f->disp.blockmap,
                    (list_entry *)block,sizeof(winx_blockmap));
                if(new_block == NULL){
                    DebugPrint("subtract_clusters: not enough memory");
                } else {
                    new_block->lcn = new_lcn;
                    new_block->vcn = new_vcn;
                    new_block->length = new_length;
                }
                clusters_to_cut = 0;
            }
        } else if(clusters_to_cut < block->length){
            /* cut off left side of the block */
            block->lcn += clusters_to_cut;
            block->vcn += clusters_to_cut;
            block->length -= clusters_to_cut;
            clusters_to_cut = 0;
        } else {
            /* remove entire block */
            clusters_to_cut -= block->length;
            block->length = 0;
        }
        if(clusters_to_cut == 0 || block->next == f->disp.blockmap) break;
    }

    /*
    * Don't remove blocks of zero length -
    * we'll use them as a markers checked by
    * is_block_excluded macro.
    */
    /* remove blocks of zero length */
/*repeat_scan:
    for(block = f->disp.blockmap; block; block = block->next){
        if(block->length == 0){
            remove_block_from_file_blocks_tree(jp,block);
            winx_list_remove_item((list_entry **)(void *)&f->disp.blockmap,(list_entry *)block);
            goto repeat_scan;
        }
        if(block->next == f->disp.blockmap) break;
    }
*/
}

static int dump_terminator(void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;

    return jp->termination_router((void *)jp);
}

/************************************************************/
/*                    move_file routine                     */
/************************************************************/

/**
 * @internal
 * @brief File moving results
 * intended for use in move_file only.
 */
typedef enum {
    CALCULATED_MOVING_SUCCESS,          /* file has been moved successfully, but its new block map is just calculated */
    DETERMINED_MOVING_FAILURE,          /* nothing has been moved; a real new block map is available */
    DETERMINED_MOVING_PARTIAL_SUCCESS,  /* file has been moved partially; a real new block map is available */
    DETERMINED_MOVING_SUCCESS           /* file has been moved entirely; a real new block map is available */
} ud_file_moving_result;

/**
 * @brief Moves a cluster chain of the file.
 * @details Can move any part of any file regardless of its fragmentation.
 * @param[in] f pointer to structure describing the file to be moved.
 * @param[in] vcn the VCN of the first cluster to be moved.
 * @param[in] length the length of the cluster chain to be moved.
 * @param[in] target the LCN of the target free region.
 * @param[in] flags combination of UD_MOVE_FILE_xxx flags
 * defined in udefrag_internals.h file
 * @param[in] jp job parameters.
 * @return Zero for success, negative value otherwise.
 * @note 
 * - Volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 * - If this function returns negative value indicating failure, 
 * one of the flags listed in udefrag_internals.h under "file status flags"
 * becomes set to display a proper message in fragmentation reports.
 * - On Windows NT 4.0 and Windows 2000 NTFS file system driver
 * has a lot of complex limitations covered in 
 * <a href="http://www.decuslib.com/decus/vmslt99a/nt/defrag.txt">Inside 
 * Windows NT Disk Defragmenting</a> article by Mark Russinovich.
 * Follow rules shown there to successfully move file clusters
 * on nt4/w2k systems. The move_file routine itself guaranties
 * the rules compliance only when either an entire file or 
 * an entire block of a compressed/sparse file is requested
 * to be moved.
 */
int move_file(winx_file_info *f,
              ULONGLONG vcn,
              ULONGLONG length,
              ULONGLONG target,
          int flags,
              udefrag_job_parameters *jp
              )
{
    ULONGLONG time;
    NTSTATUS Status;
    HANDLE hFile;
    int old_color, new_color;
    int was_fragmented, became_fragmented;
    int dump_result;
    winx_blockmap *block, *first_block;
    ULONGLONG clusters_to_redraw;
    ULONGLONG curr_vcn, n;
    winx_file_info new_file_info;
    ud_file_moving_result moving_result;
    char buf[120];
    
    time = winx_xtime();

    /* validate parameters */
    if(f == NULL || jp == NULL){
        DebugPrint("move_file: invalid parameter");
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    if(jp->udo.dbgprint_level >= DBG_DETAILED){
        if(f->path)     DebugPrint("%" WSTR,printableutf(f->path));
        else DebugPrint("empty filename");
        DebugPrint("vcn = %" LL64 "u, length = %" LL64 "u, target = %" LL64 "u",(ULONGLONG)vcn,(ULONGLONG)length,(ULONGLONG)target);
    }
    
    if(length == 0){
        DebugPrint("move_file: move of zero number of clusters requested");
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return 0; /* nothing to move */
    }
    
    if(f->disp.clusters == 0 || f->disp.fragments == 0 || f->disp.blockmap == NULL){
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return 0; /* nothing to move */
    }
    
    if(vcn + length > f->disp.blockmap->prev->vcn + f->disp.blockmap->prev->length){
        DebugPrint("move_file: data move behind the end of the file requested");
        DbgPrintBlocksOfFile(f->disp.blockmap);
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    first_block = get_first_block_of_cluster_chain(f,vcn);
    if(first_block == NULL){
        DebugPrint("move_file: data move out of file bounds requested");
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    if(!check_region(jp,target,length)){
        DebugPrint("move_file: there is no sufficient free space available on target block");
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    /* save file properties */
    old_color = get_file_color(jp,f);
    was_fragmented = is_fragmented(f);

    if (!jp->pi.step)
        jp->pi.current_inode = f->internal.BaseMftId;
    else {
        if (jp->pi.current_inode != f->internal.BaseMftId)
            jp->pi.step = 0;
        jp->pi.current_inode = f->internal.BaseMftId;
    }
    jp->pi.step++;
    snprintf(buf,120,"Moving clusters from file \"%" WSTR "\" inode %" LL64 "d step %ld",
                printableutf(f->name),(long long)f->internal.BaseMftId,jp->pi.step);
    buf[119] = 0; /* not enforced by msvcrt ! */
    if (jp->feedback && !jp->udo.dry_run)
        (*jp->feedback)(&jp->pi,buf);
    /* open the file */
    Status = winx_defrag_fopen(f,WINX_OPEN_FOR_MOVE,&hFile);
    if(Status != STATUS_SUCCESS){
        DebugPrintEx(Status,"move_file: cannot open %" WSTR,printableutf(f->path));
        /* redraw space */
        colorize_file_as_system(jp,f);
        /* however, don't reset file information here */
        f->user_defined_flags |= UD_FILE_LOCKED;
        jp->pi.processed_clusters += length;
        if(jp->progress_router)
            jp->progress_router(jp); /* redraw progress */
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    /* move the file */
    move_file_helper(hFile,f,vcn,length,target,jp);
    winx_defrag_fclose(hFile);

    /* get file moving result */
    memcpy(&new_file_info,f,sizeof(winx_file_info));
    new_file_info.disp.blockmap = NULL;
    if(jp->udo.dry_run){
        dump_result = -1;
    } else {
        dump_result = winx_ftw_dump_file(&new_file_info,dump_terminator,(void *)jp);
        if(dump_result < 0)
            DebugPrint("move_file: cannot redump the file");
    }
    
    if(dump_result < 0){
        /* let's assume a successful move */
        /* we have no new map of file blocks, so let's calculate it */
        memcpy(&new_file_info,f,sizeof(winx_file_info));
        new_file_info.disp.blockmap = calculate_new_blockmap(f,vcn,length,target);
        optimize_blockmap(&new_file_info);
        moving_result = CALCULATED_MOVING_SUCCESS;
    } else {
        /*DebugPrint("OLD MAP:");
        for(block = f->disp.blockmap; block; block = block->next){
            DebugPrint("VCN = %" LL64 "u, LCN = %" LL64 "u, LEN = %" LL64 "u",
                block->vcn, block->lcn, block->length);
            if(block->next == f->disp.blockmap) break;
        }
        DebugPrint("NEW MAP:");
        for(block = new_file_info.disp.blockmap; block; block = block->next){
            DebugPrint("VCN = %" LL64 "u, LCN = %" LL64 "u, LEN = %" LL64 "u",
                block->vcn, block->lcn, block->length);
            if(block->next == new_file_info.disp.blockmap) break;
        }*/
        /* compare old and new block maps */
        if(blockmap_compare(&new_file_info.disp,&f->disp) == 0){
            DebugPrint("move_file: nothing has been moved");
            moving_result = DETERMINED_MOVING_FAILURE;
        } else {
            /* check whether all data has been moved to the target or not */
            if(check_cluster_chain_location(&new_file_info,vcn,length,target)){
                moving_result = DETERMINED_MOVING_SUCCESS;
            } else {
                DebugPrint("move_file: the file has been moved just partially");
                DbgPrintBlocksOfFile(new_file_info.disp.blockmap);
                moving_result = DETERMINED_MOVING_PARTIAL_SUCCESS;
            }
        }
    }
    
    /* handle a case when nothing has been moved */
    if(moving_result == DETERMINED_MOVING_FAILURE){
        winx_list_destroy((list_entry **)(void *)&new_file_info.disp.blockmap);
        f->user_defined_flags |= UD_FILE_MOVING_FAILED;
        if(flags & UD_MOVE_FILE_CUT_OFF_MOVED_CLUSTERS)
            subtract_clusters(f,vcn,length,jp);
#if TRUNCREGION
#else
        /* remove target space from the free space pool */
        jp->free_regions = winx_sub_volume_region(jp->free_regions,target,length);
#endif
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }

    /*
    * Something has been moved, therefore we need to redraw 
    * space, update free space pool and adjust statistics.
    */
    if(moving_result == DETERMINED_MOVING_PARTIAL_SUCCESS)
        f->user_defined_flags |= UD_FILE_MOVING_FAILED;
    
    /* redraw target space */
    new_color = get_file_color(jp,&new_file_info);
    colorize_map_region(jp,target,length,new_color,FREE_SPACE);
    if(jp->progress_router)
        jp->progress_router(jp); /* redraw map */
            
    /* remove target space from the free space pool - before the following map redraw */
    jp->free_regions = winx_sub_volume_region(jp->free_regions,target,length);
    
    /* redraw file clusters in new color */
    if(new_color != old_color){
        for(block = f->disp.blockmap; block; block = block->next){
            colorize_map_region(jp,block->lcn,block->length,new_color,old_color);
            if(block->next == f->disp.blockmap) break;
        }
    }
    
    /* redraw freed range of clusters */
    if(moving_result != DETERMINED_MOVING_PARTIAL_SUCCESS){
        clusters_to_redraw = length;
        curr_vcn = vcn;
        first_block = get_first_block_of_cluster_chain(f,vcn);
        for(block = first_block; block; block = block->next){
            /* redraw the current block or its part */
            n = min(block->length - (curr_vcn - block->vcn),clusters_to_redraw);
            redraw_freed_space(jp,block->lcn + (curr_vcn - block->vcn),n,new_color);
            clusters_to_redraw -= n;
            if(!clusters_to_redraw || block->next == f->disp.blockmap) break;
            curr_vcn = block->next->vcn;
        }
    }

    /* adjust statistics */
    became_fragmented = is_fragmented(&new_file_info);
    if(became_fragmented && !was_fragmented)
        jp->pi.fragmented ++;
    if(!became_fragmented && was_fragmented)
        jp->pi.fragmented --;
    jp->pi.fragments -= (f->disp.fragments - new_file_info.disp.fragments);
    if(jp->progress_router)
        jp->progress_router(jp); /* redraw map and update statistics */

    if(flags & UD_MOVE_FILE_CUT_OFF_MOVED_CLUSTERS){
        /* destroy new blockmap, since we use an old one anyway in this case */
        winx_list_destroy((list_entry **)(void *)&new_file_info.disp.blockmap);
        /* cut off moved range of clusters from the original blockmap */
        subtract_clusters(f,vcn,length,jp);
        /* update statistics though */
        f->disp.fragments = new_file_info.disp.fragments;
        f->disp.clusters = new_file_info.disp.clusters;
        if(became_fragmented)
            f->disp.flags |= WINX_FILE_DISP_FRAGMENTED;
        else
            f->disp.flags &= ~WINX_FILE_DISP_FRAGMENTED;
    } else {
        /* new block map is available - use it */
        for(block = f->disp.blockmap; block; block = block->next){
            if(remove_block_from_file_blocks_tree(jp,block) < 0) break;
            if(block->next == f->disp.blockmap) break;
        }
        winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
        memcpy(&f->disp,&new_file_info.disp,sizeof(winx_file_disposition));
        for(block = f->disp.blockmap; block; block = block->next){
            if(add_block_to_file_blocks_tree(jp,f,block) < 0) break;
            if(block->next == f->disp.blockmap) break;
        }
    }

    /* update list of fragmented files */
    if(!became_fragmented && was_fragmented)
        truncate_fragmented_files_list(f,jp);
    if(became_fragmented && !was_fragmented)
        expand_fragmented_files_list(f,jp);
    
    jp->p_counters.moving_time += winx_xtime() - time;
#ifdef UNTHREADED
    deliver_progress_info(jp,0); /* 0: running */
#endif
    return (moving_result == DETERMINED_MOVING_PARTIAL_SUCCESS) ? (-1) : 0;
}

/** @} */
