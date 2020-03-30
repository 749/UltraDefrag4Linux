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
 * @file volume.c
 * @brief Disk validation.
 * @addtogroup Disks
 * @{
 */

#include "compiler.h"
#include "udefrag-internals.h"

static int internal_validate_volume(char letter,int skip_removable,volume_info *v);

/**
 * @brief Retrieves a list of volumes
 * available for defragmentation.
 * @param[in] skip_removable boolean value
 * indicating whether removable drives
 * should be skipped or not.
 * @return Pointer to the list of volumes.
 * NULL indicates failure.
 * @note if skip_removable is equal to FALSE
 * and you have a floppy drive without a disk
 * inside, then you will hear noise :-)
 * @par Example:
 * @code
 * volume_info *v;
 * int i;
 *
 * v = udefrag_get_vollist(TRUE);
 * if(v != NULL){
 *     for(i = 0; v[i].letter != 0; i++){
 *         // process volume list entry
 *     }
 *     udefrag_release_vollist(v);
 * }
 * @endcode
 */
volume_info *udefrag_get_vollist(int skip_removable)
{
    volume_info *v;
    ULONG i, index;
    char letter;
    
    /* allocate memory */
    v = winx_heap_alloc((MAX_DOS_DRIVES + 1) * sizeof(volume_info));
    if(v == NULL){
        DebugPrint("udefrag_get_vollist: cannot allocate %u bytes of memory",
            (MAX_DOS_DRIVES + 1) * sizeof(volume_info));
        return NULL;
    }

    /* set error mode to ignore missing removable drives */
    if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0){
        winx_heap_free(v);
        return NULL;
    }

    /* cycle through drive letters */
    for(i = 0, index = 0; i < MAX_DOS_DRIVES; i++){
        letter = 'A' + (char)i; /* uppercase required by w2k! */
        if(internal_validate_volume(letter, skip_removable,v + index) >= 0)
            index ++;
    }
    v[index].letter = 0;

    /* try to restore error mode to default state */
    winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
    return v;
}

/**
 * @brief Releases list of volumes
 * returned by udefrag_get_vollist.
 */
void udefrag_release_vollist(volume_info *v)
{
    if(v) winx_heap_free(v);
}

/**
 * @brief Retrieves volume information.
 * @param[in] volume_letter the volume letter.
 * @param[in] v pointer to structure receiving the information.
 * @return Zero for success, negative value otherwise.
 */
int udefrag_get_volume_information(char volume_letter,volume_info *v)
{
    int result;
    
    /* set error mode to ignore missing removable drives */
    if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0)
        return (-1);

    result = internal_validate_volume(volume_letter,0,v);

    /* try to restore error mode to default state */
    winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
    return result;
}

/**
 * @brief Checks a volume for the defragmentation possibility.
 * @param[in] volume_letter the volume letter.
 * @param[in] skip_removable the boolean value 
 * defining, must removable drives be skipped or not.
 * @return Zero for success, negative value otherwise.
 * @note if skip_removable is equal to FALSE and you want 
 * to validate a floppy drive without a floppy disk
 * then you will hear noise :))
 */
int udefrag_validate_volume(char volume_letter,int skip_removable)
{
    volume_info v;
    int error_code;

    /* set error mode to ignore missing removable drives */
    if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0)
        return (-1);
    error_code = internal_validate_volume(volume_letter,skip_removable,&v);
    if(error_code < 0) return error_code;
    /* try to restore error mode to default state */
    winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
    return 0;
}

/**
 * @brief Retrieves a volume parameters.
 * @param[in] volume_letter the volume letter.
 * @param[in] skip_removable the boolean value defining,
 * must removable drives be treated as invalid or not.
 * @param[out] v pointer to structure receiving volume
 * parameters.
 * @return Zero for success, negative value otherwise.
 * @note
 * - Internal use only.
 * - if skip_removable is equal to FALSE and you want 
 *   to validate a floppy drive without a floppy disk
 *   then you will hear noise :))
 */
static int internal_validate_volume(char volume_letter,int skip_removable,volume_info *v)
{
    winx_volume_information volume_info;
    int type;
    
    if(v == NULL)
        return (-1);

    /* convert volume letter to uppercase - needed for w2k */
    volume_letter = winx_toupper(volume_letter);
    
    v->letter = volume_letter;
    v->is_removable = FALSE;
    v->is_dirty = FALSE;
    type = winx_get_drive_type(volume_letter);
    if(type < 0) return (-1);
    if(type == DRIVE_CDROM){
        DebugPrint("Disk %c: is cdrom drive.",volume_letter);
        return UDEFRAG_CDROM;
    }
    if(type == DRIVE_REMOTE){
        DebugPrint("Disk %c: is remote drive.",volume_letter);
        return UDEFRAG_REMOTE;
    }
    if(type == DRIVE_ASSIGNED_BY_SUBST_COMMAND){
        DebugPrint("It seems that %c: drive letter is assigned by \'subst\' command.",volume_letter);
        return UDEFRAG_ASSIGNED_BY_SUBST;
    }
    if(type == DRIVE_REMOVABLE){
        v->is_removable = TRUE;
        if(skip_removable){
            DebugPrint("Disk %c: is removable media.",volume_letter);
            return UDEFRAG_REMOVABLE;
        }
    }

    /*
    * Get volume information; it is strongly 
    * required to exclude missing floppies.
    */
    if(winx_get_volume_information(volume_letter,&volume_info) < 0) {
        DebugPrint("Failed to get volume info");
        return (-1);
    }
    ExpertDump("volume info",(void*)&volume_info,32);
/*
Dump of volume info at 0x12f994
0000  464e5446 53000000 00000000 00000000     FNTFS...........
0010  00000000 00000000 00000000 00000000     ................
*/
    
    v->total_space.QuadPart = volume_info.total_bytes;
    v->free_space.QuadPart = volume_info.free_bytes;
    strncpy(v->fsname,volume_info.fs_name,MAXFSNAME - 1);
    v->fsname[MAXFSNAME - 1] = 0;
    utf16ncpy(v->label,volume_info.label,MAX_PATH);
    v->label[MAX_PATH] = 0;
    v->is_dirty = volume_info.is_dirty;
    return v->is_dirty ? UDEFRAG_DIRTY_VOLUME : 0;
}

/** @} */
