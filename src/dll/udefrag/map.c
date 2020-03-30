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
 * @file map.c
 * @brief Cluster map.
 * @addtogroup ClusterMap
 * @{
 */
     
/*
* Revised by Stefan Pendl, 2011
* <stefanpe@users.sourceforge.net>
*/

#include "compiler.h"
#include "udefrag-internals.h"

/*
* Because of huge number of clusters
* on contemporary hard drives, we cannot
* define exacly, which color is on each
* individual cluster.
*
* For example, let's assume that hard disk
* has 1 billion clusters. We need at least
* 4 bits to save color of each cluster.
* Therefore, we need to allocate 500 Mb
* of memory, which isn't acceptable.
*
* Due to this reason, we're splitting
* the disk to cells of fixed size.
* Each cell contains number of clusters
* of each color.
*
* When we're defining cell's color,
* the most occured inside wins.
*/

/**
 * @brief Allocates cluster map.
 * @param[in] map_size the number of cells.
 * @param[in,out] jp pointer to job parameters.
 * @return Zero for success, negative value otherwise.
 */
int allocate_map(unsigned int map_size,udefrag_job_parameters *jp)
{
    ULONG array_size;
    ULONGLONG i;
    
    if(jp == NULL)
        return (-1);
    
    /* reset all internal data */
    jp->pi.cluster_map = NULL;
    jp->pi.cluster_map_size = 0;
    memset(&jp->cluster_map,0,sizeof(cmap));
    
    DebugPrint("map size = %u",map_size);
    if(map_size == 0)
        return 0;
    
    /* allocate memory */
    jp->pi.cluster_map = winx_heap_alloc(map_size);
    if(jp->pi.cluster_map == NULL){
        DebugPrint("allocate_map: cannot allocate %u bytes of memory",
            map_size);
        return (-1);
    }
    array_size = (ULONG)map_size * NUM_OF_SPACE_STATES * sizeof(ULONGLONG);
    jp->cluster_map.array = winx_heap_alloc(array_size);
    if(jp->cluster_map.array == NULL){
        DebugPrint("allocate_map: cannot allocate %lu bytes of memory",
            (unsigned long)array_size);
        winx_heap_free(jp->pi.cluster_map);
        jp->pi.cluster_map = NULL;
        return (-1);
    }
    
    /* get volume information */
    if(winx_get_volume_information(jp->volume_letter,&jp->v_info) < 0){
fail:
        winx_heap_free(jp->pi.cluster_map);
        jp->pi.cluster_map = NULL;
        winx_heap_free(jp->cluster_map.array);
        jp->cluster_map.array = NULL;
        return (-1);
    }

    if(jp->v_info.total_clusters == 0)
        goto fail;

    /* set internal data */
    jp->pi.cluster_map_size = map_size;
    jp->cluster_map.map_size = map_size;
    jp->cluster_map.n_colors = NUM_OF_SPACE_STATES;
    jp->cluster_map.default_color = SYSTEM_SPACE;
    jp->cluster_map.field_size = jp->v_info.total_clusters;
    
    /* reset map */
    memset(jp->cluster_map.array,0,array_size);
    jp->cluster_map.clusters_per_cell = jp->cluster_map.field_size / jp->cluster_map.map_size;
    ExpertPrint("clusters_per_cell %lu",(unsigned long)jp->cluster_map.clusters_per_cell);
    if(jp->cluster_map.clusters_per_cell){
        jp->cluster_map.opposite_order = FALSE;
        jp->cluster_map.clusters_per_last_cell = jp->cluster_map.clusters_per_cell + \
            (jp->cluster_map.field_size - jp->cluster_map.clusters_per_cell * jp->cluster_map.map_size);
        DebugPrint("allocate_map: normal order %" LL64 "u : %" LL64 "u : %" LL64 "u", \
            (ULONGLONG)jp->cluster_map.field_size,(ULONGLONG)jp->cluster_map.clusters_per_cell,(ULONGLONG)jp->cluster_map.clusters_per_last_cell);
        for(i = 0; i < (ULONGLONG)jp->cluster_map.map_size - 1; i++)
            jp->cluster_map.array[i][jp->cluster_map.default_color] = jp->cluster_map.clusters_per_cell;
        jp->cluster_map.array[i][jp->cluster_map.default_color] = jp->cluster_map.clusters_per_last_cell;
    } else {
        jp->cluster_map.opposite_order = TRUE;
        jp->cluster_map.cells_per_cluster = jp->cluster_map.map_size / jp->cluster_map.field_size;
        jp->cluster_map.cells_per_last_cluster = jp->cluster_map.cells_per_cluster + \
            (jp->cluster_map.map_size - jp->cluster_map.cells_per_cluster * jp->cluster_map.field_size);
        DebugPrint("allocate_map: opposite order %" LL64 "u : %" LL64 "u : %" LL64 "u", \
            (ULONGLONG)jp->cluster_map.field_size,(ULONGLONG)jp->cluster_map.cells_per_cluster,(ULONGLONG)jp->cluster_map.cells_per_last_cluster);
        for(i = 0; i < (ULONGLONG)jp->cluster_map.map_size - 1; i++)
            jp->cluster_map.array[i][jp->cluster_map.default_color] = 1;
        jp->cluster_map.array[i][jp->cluster_map.default_color] = 1;
    }

    return 0;
}

/**
 * @brief Fills the map by default color.
 */
void reset_cluster_map(udefrag_job_parameters *jp)
{
    ULONGLONG i;
    
    if(jp == NULL)
        return;
    
    if(jp->cluster_map.array == NULL)
        return;

    memset(jp->cluster_map.array,0,(long)jp->cluster_map.map_size * jp->cluster_map.n_colors * sizeof(ULONGLONG));
    if(jp->cluster_map.opposite_order == FALSE){
        for(i = 0; i < (ULONGLONG)jp->cluster_map.map_size - 1; i++)
            jp->cluster_map.array[i][jp->cluster_map.default_color] = jp->cluster_map.clusters_per_cell;
        jp->cluster_map.array[i][jp->cluster_map.default_color] = jp->cluster_map.clusters_per_last_cell;
    } else {
        for(i = 0; i < (ULONGLONG)jp->cluster_map.map_size - 1; i++)
            jp->cluster_map.array[i][jp->cluster_map.default_color] = 1;
        jp->cluster_map.array[i][jp->cluster_map.default_color] = 1;
    }
}

/**
 * @brief Redraws all temporary system space as free.
 * @note Intended for use from release_temp_space_regions.
 */
void redraw_all_temporary_system_space_as_free(udefrag_job_parameters *jp)
{
    ULONGLONG i;
    ULONGLONG n;
    
    if(jp == NULL)
        return;
    
    if(jp->cluster_map.array == NULL)
        return;

    for(i = 0; i < (ULONGLONG)jp->cluster_map.map_size; i++){
        n = jp->cluster_map.array[i][TEMPORARY_SYSTEM_SPACE];
        if(n){
            jp->cluster_map.array[i][TEMPORARY_SYSTEM_SPACE] = 0;
            jp->cluster_map.array[i][FREE_SPACE] += n;
        }
    }

    if(jp->progress_router)
        jp->progress_router(jp); /* redraw map */
}

/**
 * @brief colorize_map_region helper.
 */
static void colorize_system_or_free_region(udefrag_job_parameters *jp,
                        ULONGLONG lcn, ULONGLONG length, int new_color)
{
    winx_volume_region *r;
    ULONGLONG n, current_cluster, clusters_to_process;
    
    ExpertPrint("colorize_system_or_free_region");
    current_cluster = lcn;
    clusters_to_process = length;
    for(r = jp->free_regions; r; r = r->next){
        /* break if current region follows specified range */
        if(r->lcn >= lcn + length){
            if(clusters_to_process)
                colorize_map_region(jp,current_cluster,clusters_to_process,new_color,SYSTEM_SPACE);
            break;
        }
        /* skip preceding regions */
        if(r->lcn >= current_cluster){
            if(r->lcn > current_cluster){
                n = r->lcn - current_cluster;
                colorize_map_region(jp,current_cluster,n,new_color,SYSTEM_SPACE);
                current_cluster += n;
                clusters_to_process -= n;
            }
            /* now r->lcn is equal to current_cluster always */
            n = min(r->length,clusters_to_process);
            colorize_map_region(jp,current_cluster,n,new_color,FREE_SPACE);
            current_cluster += n;
            clusters_to_process -= n;
        }
        if(r->next == jp->free_regions) break;
    }
}

/**
 * @brief Colorizes specified range of clusters.
 * @note If new color is equal to MFT_ZONE_SPACE,
 * old color is ignored.
 */
void colorize_map_region(udefrag_job_parameters *jp,
        ULONGLONG lcn, ULONGLONG length, int new_color, int old_color)
{
    ULONGLONG i, j, n, cell, offset, ncells;
    ULONGLONG *c;
    
    ExpertPrint("colorize_map_region lcn 0x%llx length %ld old_color %d new_color %d",(long long)lcn,(long)length,(int)old_color,(int)new_color);
    /* validate parameters */
    if(jp == NULL)
        return;
    if(jp->cluster_map.array == NULL)
        return;
    if(!check_region(jp,lcn,length))
        return;
    if(length == 0)
        return;
    
    /* handle special cases */
    if(old_color == SYSTEM_OR_FREE_SPACE){
        colorize_system_or_free_region(jp,lcn,length,new_color);
        return;
    }
    
    ExpertPrint("colorize_map_region1");
    /* validate colors */
    if(new_color < 0 || new_color >= jp->cluster_map.n_colors)
        return;
    if(new_color != MFT_ZONE_SPACE){
        if(old_color < 0 || old_color >= jp->cluster_map.n_colors)
            return;
    }
    
    if(new_color == old_color)
        return;
    
    if(jp->cluster_map.opposite_order == FALSE){
        ExpertPrint("colorize_map_region2 cl p cell %ld",(long)jp->cluster_map.clusters_per_cell);
        /* we're using here less obvious code,
        because _aulldvrm misses on nt4 */
        cell = lcn / jp->cluster_map.clusters_per_cell;
        if(cell >= (ULONGLONG)jp->cluster_map.map_size)
            return;
        offset = lcn - cell * jp->cluster_map.clusters_per_cell;
        while(cell < (ULONGLONG)(jp->cluster_map.map_size - 1) && length){
            n = min(length,jp->cluster_map.clusters_per_cell - offset);
            jp->cluster_map.array[cell][new_color] += n;
            if(new_color != MFT_ZONE_SPACE){
                c = &jp->cluster_map.array[cell][old_color];
                if(*c >= n) *c -= n; else *c = 0;
            }
            length -= n;
            cell ++;
            offset = 0;
        }
        if(length){
            n = min(length,jp->cluster_map.clusters_per_last_cell - offset);
            jp->cluster_map.array[cell][new_color] += n;
            if(new_color != MFT_ZONE_SPACE){
                c = &jp->cluster_map.array[cell][old_color];
                if(*c >= n) *c -= n; else *c = 0;
            }
        }
    } else {
        ExpertPrint("colorize_map_region3");
        /* clusters < cells */
        cell = lcn * jp->cluster_map.cells_per_cluster;
        ncells = length * jp->cluster_map.cells_per_cluster;
        for(i = 0; i < ncells; i++){
            if(new_color != MFT_ZONE_SPACE){
                for(j = 0; j < (ULONGLONG)jp->cluster_map.n_colors; j++)
                    jp->cluster_map.array[cell + i][j] = 0;
            }
            jp->cluster_map.array[cell + i][new_color] = 1;
        }
    /* colorize remaining cells as unused */
        if(lcn + length == jp->v_info.total_clusters){
            cell += ncells;
                ncells = (jp->cluster_map.cells_per_last_cluster - jp->cluster_map.cells_per_cluster);
        
            for(i = 0; i < ncells; i++){
                for(j = 0; j < (ULONGLONG)jp->cluster_map.n_colors; j++)
                    jp->cluster_map.array[cell + i][j] = 0;
                jp->cluster_map.array[cell + i][UNUSED_MAP_SPACE] = 1;
            }
        }
    }
}

/**
 * @brief Defines whether the file is $Mft or not.
 * @return Nonzero value indicates that the file is $Mft.
 */
int is_mft(winx_file_info *f,udefrag_job_parameters *jp)
{
    int length;
    const utf_t mft_name[] = UTF("$Mft");

    if(f == NULL || jp == NULL)
        return 0;
    
    if(jp->fs_type != FS_NTFS)
        return 0;
    
    if(f->path == NULL || f->name == NULL)
        return 0;
    
    length = utflen(f->path);

    if(length == 11){
        if(winx_utfistr(f->name,mft_name))
            return 1;
    }
    
    return 0;
}

/**
 * @brief Defines file's color.
 */
int get_file_color(udefrag_job_parameters *jp, winx_file_info *f)
{
    /* show $MFT file in dark magenta color */
    if(is_mft(f,jp))
        return MFT_SPACE;

    /* show excluded files as not fragmented */
    if(is_fragmented(f) && !is_excluded(f))
        return is_over_limit(f) ? FRAGM_OVER_LIMIT_SPACE : FRAGM_SPACE;

    if(is_directory(f)){
        if(is_over_limit(f))
            return DIR_OVER_LIMIT_SPACE;
        else
            return DIR_SPACE;
    } else if(is_compressed(f)){
        if(is_over_limit(f))
            return COMPRESSED_OVER_LIMIT_SPACE;
        else
            return COMPRESSED_SPACE;
    } else {
        if(is_over_limit(f))
            return UNFRAGM_OVER_LIMIT_SPACE;
        else
            return UNFRAGM_SPACE;
    }
    return UNFRAGM_SPACE; /* this point will never be reached */
}

/**
 * @brief Colorizes space belonging to the file.
 */
void colorize_file(udefrag_job_parameters *jp, winx_file_info *f, int old_color)
{
    winx_blockmap *block;
    int new_color;
    
    ExpertPrint("colorize_file");      
    if(jp == NULL || f == NULL)
        return;
    
    new_color = get_file_color(jp,f);
    for(block = f->disp.blockmap; block; block = block->next){
        colorize_map_region(jp,block->lcn,block->length,new_color,old_color);
        if(block->next == f->disp.blockmap) break;
    }
}

/**
 * @brief Colorizes space belonging to the file as system space.
 */
void colorize_file_as_system(udefrag_job_parameters *jp, winx_file_info *f)
{
    winx_blockmap *block;
    int new_color, old_color;
    
    if(jp == NULL || f == NULL)
        return;
    
    /* never draw MFT in green */
    if(is_mft(f,jp)) return;
    
    new_color = is_over_limit(f) ? SYSTEM_OVER_LIMIT_SPACE : SYSTEM_SPACE;
    old_color = get_file_color(jp,f);
    for(block = f->disp.blockmap; block; block = block->next){
        colorize_map_region(jp,block->lcn,block->length,new_color,old_color);
        if(block->next == f->disp.blockmap) break;
    }
}

/**
 * @brief Frees resources
 * allocated by allocate_map.
 */
void free_map(udefrag_job_parameters *jp)
{
    if(jp != NULL){
        if(jp->pi.cluster_map)
            winx_heap_free(jp->pi.cluster_map);
        if(jp->cluster_map.array)
            winx_heap_free(jp->cluster_map.array);
        jp->pi.cluster_map = NULL;
        jp->pi.cluster_map_size = 0;
        memset(&jp->cluster_map,0,sizeof(cmap));
    }
}

/** @} */
