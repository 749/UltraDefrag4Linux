/*
 *  ZenWINX - WIndows Native eXtended library.
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
 * @file ftw.c
 * @brief File tree walk.
 * @addtogroup File
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Size of the buffer dedicated
 * to list directory entries into.
 * @note Must be a multiple of sizeof(void *).
 */
#define FILE_LISTING_SIZE (16*1024)

/**
 * @brief Size of the buffer dedicated
 * to list file fragments into.
 * @details It is large enough to hold
 * GET_RETRIEVAL_DESCRIPTOR structure 
 * as well as 512 MAPPING_PAIR structures.
 */
#define FILE_MAP_SIZE (sizeof(GET_RETRIEVAL_DESCRIPTOR) - sizeof(MAPPING_PAIR) + 512 * sizeof(MAPPING_PAIR))

/**
 * @internal
 * @brief LCN of virtual clusters.
 */
#define LLINVALID ((ULONGLONG) -1)

/* external functions prototypes */
winx_file_info *ntfs_scan_disk(char volume_letter,
    int flags, ftw_filter_callback fcb, ftw_progress_callback pcb, 
    ftw_terminator t, void *user_defined_data);

/**
 * @internal
 * @brief Checks whether the file
 * tree walk must be terminated or not.
 * @return Nonzero value indicates that
 * termination is requested.
 */
static int ftw_check_for_termination(ftw_terminator t,void *user_defined_data)
{
    if(t == NULL)
        return 0;
    
    return t(user_defined_data);
}

/**
 * @internal
 * @brief Validates map of file blocks,
 * destroys it in case of errors found.
 */
void validate_blockmap(winx_file_info *f)
{
    winx_blockmap *b1 = NULL, *b2 = NULL;
    
#if 0
    /* seems to be too slow */
    for(b1 = f->disp.blockmap; b1; b1 = b1->next){
        if(b1->next == f->disp.blockmap) break;
        for(b2 = b1->next; b2; b2 = b2->next){
            /* does two blocks overlap? */
            //if(yes){
            //    destroy_blockmap();
            //    return;
            //}
            if(b2->next == f->disp.blockmap) break;
        }
    }
#else
    /* a simplified version */
    /* multiple blocks of zero VCN were detected on reparse points on 32-bit XP SP2 */
    b1 = f->disp.blockmap;
    if(b1) b2 = b1->next;
    if(b1 && b2 && b2 != b1){
        if(b1->vcn == b2->vcn){
            DebugPrint("validate_blockmap: %" WSTR ": wrong map detected:", printableutf(f->path));
            for(b1 = f->disp.blockmap; b1; b1 = b1->next){
                DebugPrint("VCN = %" LL64 "u, LCN = %" LL64 "u, LEN = %" LL64 "u",
                    b1->vcn, b1->lcn, b1->length);
                if(b1->next == f->disp.blockmap) break;
            }
            winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
        }
    }
#endif
}

/**
 * @brief Retrieves information
 * about the file disposition.
 * @param[out] f pointer to
 * structure receiving the information.
 * @param[in] t address of procedure to be called
 * each time when winx_ftw_dump_file would like
 * to know whether it must be terminated or not.
 * Nonzero value, returned by terminator,
 * forces file dump to be terminated.
 * @param[in] user_defined_data pointer to data
 * passed to the registered terminator.
 * @return Zero for success,
 * negative value otherwise.
 * @note 
 * - Callback procedure should complete as quickly
 * as possible to avoid slowdown of the scan.
 * - For resident NTFS streams (small files and
 * directories located inside MFT) this function resets
 * all the file disposition structure fields to zero.
 */
int winx_ftw_dump_file(winx_file_info *f,
        ftw_terminator t, void *user_defined_data)
{
    GET_RETRIEVAL_DESCRIPTOR *filemap;
    HANDLE hFile;
    ULONGLONG startVcn;
    long counter; /* counts number of attempts to get information */
    #define MAX_COUNT 1000
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    unsigned int i;
    winx_blockmap *block = NULL;
    
    DbgCheck1(f,"winx_ftw_dump_file",-1);
    
    /* reset disposition related fields */
    f->disp.clusters = 0;
    f->disp.fragments = 0;
    f->disp.flags = 0;
    winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
    
    DebugPrint("* Getting the runlist for file %" WSTR " %" LL64 "d descriptor at 0x%lx",printablepath(f->path,0),(long long)f->internal.BaseMftId,(long)f);
    /* open the file */
    status = winx_defrag_fopen(f,WINX_OPEN_FOR_DUMP,&hFile);
    if(status != STATUS_SUCCESS){
        DebugPrintEx(status,"winx_ftw_dump_file: cannot open %" WSTR,printablepath(f->path,0));
        return 0; /* file is locked by system */
    }
    
    /* allocate memory */
    filemap = winx_heap_alloc(FILE_MAP_SIZE);
    if(filemap == NULL){
        DebugPrint("winx_ftw_dump_file: cannot allocate %u bytes of memory",
            FILE_MAP_SIZE);
        winx_defrag_fclose(hFile);
        return (-1);
    }
    
    /* dump the file */
    startVcn = 0;
    counter = 0;
    do {
        memset(filemap,0,FILE_MAP_SIZE);

        DebugPrint("NtDeviceIoControlFile ftw.c code 0x%lx position 0x%" LL64 "x",(long)FSCTL_GET_RETRIEVAL_POINTERS,startVcn);
        ExpertDump("NtDeviceIoControlFile input",(char*)&startVcn,sizeof(ULONGLONG));
        iosb.Information = 0; /* JPA not returned in error cases */
        status = NtFsControlFile(hFile,NULL,NULL,0,
            &iosb,FSCTL_GET_RETRIEVAL_POINTERS,
            &startVcn,sizeof(ULONGLONG),
            filemap,FILE_MAP_SIZE);
        ExpertDump("NtDeviceIoControlFile output",(char*)filemap,iosb.Information);
/*
Dump of NtDeviceIoControlFile output 8204 bytes at 0x11b4250 (Windows endian)
0000  20000000 00000000 00000000 00000000      ...............
0010  7a020000 00000000 84660000 00000000     z........f......
0020  07060000 00000000 7db80000 00000000     ........}.......
0030  5e080000 00000000 6cce0000 00000000     ^.......l.......
0040  3f090000 00000000 873e0000 00000000     ?........>......
0050  b60b0000 00000000 51560000 00000000     ........QV......
0060  300e0000 00000000 c8a40000 00000000     0...............
0070  6f100000 00000000 b3d20000 00000000     o...............
0080  0b110000 00000000 cae20000 00000000     ................
*/
        counter ++;
        if(NT_SUCCESS(status)){
            NtWaitForSingleObject(hFile,FALSE,NULL);
            status = iosb.Status;
        }
        if(status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW){
            /* it always returns STATUS_END_OF_FILE for small files placed inside MFT */
            if(status == STATUS_END_OF_FILE) goto empty_map_detected;
            DebugPrintEx(status,"winx_ftw_dump_file: dump failed for %" WSTR,printablepath(f->path,0));
            goto dump_failed;
        }

        if(ftw_check_for_termination(t,user_defined_data)){
            if(counter > MAX_COUNT)
                DebugPrint("winx_ftw_dump_file: %" WSTR ": infinite main loop?",printablepath(f->path,0));
            /* reset incomplete map */
            goto cleanup;
        }
        
        /* check for an empty map */
        if(!filemap->NumberOfPairs && status != STATUS_SUCCESS){
            DebugPrint("winx_ftw_dump_file: %" WSTR ": empty map of file detected",printablepath(f->path,0));
            goto empty_map_detected;
        }
        
        /* loop through the buffer of number/cluster pairs */
        startVcn = filemap->StartVcn;
        for(i = 0; i < (ULONGLONG)filemap->NumberOfPairs; startVcn = filemap->Pair[i].Vcn, i++){
            /* compressed virtual runs (0-filled) are identified with LCN == -1    */
            if(filemap->Pair[i].Lcn == LLINVALID)
                continue;
            
            /* the following is usual for 3.99 Gb files on FAT32 under XP */
            if(filemap->Pair[i].Vcn == 0){
                DebugPrint("winx_ftw_dump_file: %" WSTR ": wrong map of file detected",printablepath(f->path,0));
                goto dump_failed;
            }
            
            block = (winx_blockmap *)winx_list_insert_item((list_entry **)&f->disp.blockmap,
                (list_entry *)block,sizeof(winx_blockmap));
            if(block == NULL){
                DebugPrint("winx_ftw_dump_file: cannot allocate %u bytes of memory",
                    sizeof(winx_blockmap));
                goto dump_failed;
            }
            block->lcn = filemap->Pair[i].Lcn;
            block->length = filemap->Pair[i].Vcn - startVcn;
            block->vcn = startVcn;
            
            //DebugPrint("VCN = %" LL64 "u, LCN = %" LL64 "u, LENGTH = %" LL64 "u",
            //    (ULONGLONG)block->vcn,(ULONGLONG)block->lcn,(ULONGLONG)block->length);

            f->disp.clusters += block->length;
            if(block == f->disp.blockmap)
                f->disp.fragments ++;
            
            /*
            * Sometimes files have more than one fragment, 
            * but are not fragmented yet. In case of compressed
            * files this happens quite frequently.
            */
            if(block != f->disp.blockmap && \
              block->lcn != (block->prev->lcn + block->prev->length)){
                f->disp.fragments ++;
                f->disp.flags |= WINX_FILE_DISP_FRAGMENTED;
            }
        }
    } while(status != STATUS_SUCCESS);
    /* small directories placed inside MFT have empty list of fragments... */

    /* the dump is completed */
    validate_blockmap(f);
    winx_heap_free(filemap);
    winx_defrag_fclose(hFile);
    return 0;
    
cleanup:
empty_map_detected:
    f->disp.clusters = 0;
    f->disp.fragments = 0;
    f->disp.flags = 0;
    winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
    winx_heap_free(filemap);
    winx_defrag_fclose(hFile);
    return 0;

dump_failed:
    f->disp.clusters = 0;
    f->disp.fragments = 0;
    f->disp.flags = 0;
    winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
    winx_heap_free(filemap);
    winx_defrag_fclose(hFile);
    return (-1);
}

/**
 * @internal
 * @brief Adds directory entry to the file list.
 * @return Address of inserted file list entry,
 * NULL indicates failure.
 */
static winx_file_info * ftw_add_entry_to_filelist(utf_t *path,
    int flags, ftw_filter_callback fcb, ftw_progress_callback pcb,
    ftw_terminator t, void *user_defined_data,
    winx_file_info **filelist,
    FILE_BOTH_DIR_INFORMATION *file_entry)
{
    winx_file_info *f;
    int length;
    int is_rootdir = 0;
    
    if(path == NULL || file_entry == NULL)
        return NULL;
    
    if(path[0] == 0){
        DebugPrint("ftw_add_entry_to_filelist: path is empty");
        return NULL;
    }
    
    /* insert new item to the file list */
    f = (winx_file_info *)winx_list_insert_item((list_entry **)(void *)filelist,
        NULL,sizeof(winx_file_info));
    if(f == NULL){
        DebugPrint("ftw_add_entry_to_filelist: cannot allocate %u bytes of memory",
            sizeof(winx_file_info));
        return NULL;
    }
    
    /* extract filename */
    f->name = winx_heap_alloc(file_entry->FileNameLength + sizeof(short));
    if(f->name == NULL){
        DebugPrint("ftw_add_entry_to_filelist: cannot allocate %u bytes of memory",
            file_entry->FileNameLength + sizeof(short));
        winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
        return NULL;
    }
    memset(f->name,0,file_entry->FileNameLength + sizeof(short));
    memcpy(f->name,file_entry->FileName,file_entry->FileNameLength);
    
    /* detect whether we are in root directory or not */
    length = utflen(path);
    if(path[length - 1] == '\\')
        is_rootdir = 1; /* only root directory contains trailing backslash */
    
    /* build path */
    length += utflen(f->name) + 1;
    if(!is_rootdir)
        length ++;
    f->path = winx_heap_alloc(length * sizeof(utf_t));
    if(f->path == NULL){
        DebugPrint("ftw_add_entry_to_filelist: cannot allocate %u bytes of memory",
            length * sizeof(short));
        winx_heap_free(f->name);
        winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
        return NULL;
    }
    if(is_rootdir)
        (void)_snwprintf(f->path,length,UTF("%" WSTR "%" WSTR),path,f->name);
    else
        (void)_snwprintf(f->path,length,UTF("%" WSTR "\\%" WSTR),path,f->name);
    f->path[length - 1] = 0;
    
    /* save file attributes */
    f->flags = file_entry->FileAttributes;
    
    /* reset user defined flags */
    f->user_defined_flags = 0;
    
    //DebugPrint("%" WSTR,printablepath(f->path,0));
    
    /* reset internal data fields */
    memset(&f->internal,0,sizeof(winx_file_internal_info));
    
    /* reset file disposition */
    memset(&f->disp,0,sizeof(winx_file_disposition));

    /* get file disposition if requested */
    if(flags & WINX_FTW_DUMP_FILES){
        if(winx_ftw_dump_file(f,t,user_defined_data) < 0){
            winx_heap_free(f->name);
            winx_heap_free(f->path);
            winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
            return NULL;
        }
    }    
    
    return f;
}

/**
 * @internal
 * @brief Adds information about
 * root directory to file list.
 */
static int ftw_add_root_directory(utf_t *path, int flags,
    ftw_filter_callback fcb, ftw_progress_callback pcb, 
    ftw_terminator t, void *user_defined_data,
    winx_file_info **filelist)
{
    winx_file_info *f;
    int length;
    FILE_BASIC_INFORMATION fbi;
    HANDLE hDir;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    
    if(path == NULL)
        return (-1);
    
    if(path[0] == 0){
        DebugPrint("ftw_add_root_directory: path is empty");
        return (-1);
    }
    
    /* insert new item to the file list */
    f = (winx_file_info *)winx_list_insert_item((list_entry **)(void *)filelist,
        NULL,sizeof(winx_file_info));
    if(f == NULL){
        DebugPrint("ftw_add_root_directory: cannot allocate %u bytes of memory",
            sizeof(winx_file_info));
        return (-1);
    }
    
    /* build path */
    length = utflen(path) + 1;
    f->path = winx_heap_alloc(length * sizeof(short));
    if(f->path == NULL){
        DebugPrint("ftw_add_root_directory: cannot allocate %u bytes of memory",
            length * sizeof(short));
        winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
        return (-1);
    }
    utfcpy(f->path,path);
    
    /* save . filename */
    f->name = winx_heap_alloc(2 * sizeof(short));
    if(f->name == NULL){
        DebugPrint("ftw_add_root_directory: cannot allocate %u bytes of memory",
            2 * sizeof(short));
        winx_heap_free(f->path);
        winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
        return (-1);
    }
    f->name[0] = '.';
    f->name[1] = 0;
    
    /* get file attributes */
    f->flags |= FILE_ATTRIBUTE_DIRECTORY;
    status = winx_defrag_fopen(f,WINX_OPEN_FOR_BASIC_INFO,&hDir);
    if(status == STATUS_SUCCESS){
        memset(&fbi,0,sizeof(FILE_BASIC_INFORMATION));
        status = NtQueryInformationFile(hDir,&iosb,
            &fbi,sizeof(FILE_BASIC_INFORMATION),
            FileBasicInformation);
        if(!NT_SUCCESS(status)){
            DebugPrintEx(status,"ftw_add_root_directory: NtQueryInformationFile(FileBasicInformation) failed");
        } else {
            f->flags = fbi.FileAttributes;
            DebugPrint("ftw_add_root_directory: root directory flags: %u",f->flags);
        }
        winx_defrag_fclose(hDir);
    } else {
        DebugPrintEx(status,"ftw_add_root_directory: cannot open %" WSTR,printablepath(f->path,0));
    }
    
    /* reset user defined flags */
    f->user_defined_flags = 0;
    
    /* reset internal data fields */
    memset(&f->internal,0,sizeof(winx_file_internal_info));
    
    /* reset file disposition */
    memset(&f->disp,0,sizeof(winx_file_disposition));

    /* get file disposition if requested */
    if(flags & WINX_FTW_DUMP_FILES){
        if(winx_ftw_dump_file(f,t,user_defined_data) < 0){
            winx_heap_free(f->name);
            winx_heap_free(f->path);
            winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
            return (-1);
        }
    }
    
    /* call callbacks */
    if(pcb != NULL)
        pcb(f,user_defined_data);
    if(fcb != NULL)
        fcb(f,user_defined_data);
    
    return 0;
}

/**
 * @internal
 * @brief Opens directory for file listing.
 * @return Handle to the directory, NULL
 * indicates failure.
 */
static HANDLE ftw_open_directory(utf_t *path)
{
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    HANDLE hDir;
    
    if(path == NULL)
        return NULL;
    
    RtlInitUnicodeString(&us,path);
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    status = NtCreateFile(&hDir, FILE_LIST_DIRECTORY | FILE_RESERVE_OPFILTER,
        &oa, &iosb, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
        NULL, 0);
    if(status != STATUS_SUCCESS){
        DebugPrintEx(status,"ftw_open_directory: cannot open %" WSTR,printableutf(path));
        return NULL;
    }
    
    return hDir;
}

/**
 * @internal
 * @brief Scans directory and adde information
 * about files found to the passed file list.
 * @return Zero for success, -1 indicates
 * failure, -2 indicates termination requested
 * by caller.
 */
static int ftw_helper(utf_t *path, int flags,
        ftw_filter_callback fcb, ftw_progress_callback pcb,
        ftw_terminator t, void *user_defined_data,
        winx_file_info **filelist)
{
    FILE_BOTH_DIR_INFORMATION *file_listing, *file_entry;
    HANDLE hDir;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    winx_file_info *f;
    int skip_children, result;
    
    /* open directory */
    hDir = ftw_open_directory(path);
    if(hDir == NULL)
        return 0; /* directory is locked by system, skip it */
    
    /* allocate memory */
    file_listing = winx_heap_alloc(FILE_LISTING_SIZE);
    if(file_listing == NULL){
        DebugPrint("ftw_helper: cannot allocate %u bytes of memory",
            FILE_LISTING_SIZE);
        NtClose(hDir);
        return (-1);
    }
    
    /* reset buffer */
    memset((void *)file_listing,0,FILE_LISTING_SIZE);
    file_entry = file_listing;

    /* list directory entries */
    while(!ftw_check_for_termination(t,user_defined_data)){
        /* get directory entry */
        if(file_entry->NextEntryOffset){
            /* go to the next directory entry */
            file_entry = (FILE_BOTH_DIR_INFORMATION *)((char *)file_entry + \
                file_entry->NextEntryOffset);
        } else {
            /* read next portion of directory entries */
            memset((void *)file_listing,0,FILE_LISTING_SIZE);
            status = NtQueryDirectoryFile(hDir,NULL,NULL,NULL,
                &iosb,(void *)file_listing,FILE_LISTING_SIZE,
                FileBothDirectoryInformation,
                FALSE /* return multiple entries */,
                NULL,
                FALSE /* do not restart scan */
                );
            if(status != STATUS_SUCCESS){
                if(status != STATUS_NO_MORE_FILES)
                    DebugPrintEx(status,"ftw_helper failed");
                /* no more entries to read */
                winx_heap_free(file_listing);
                NtClose(hDir);
                return 0;
            }
            file_entry = file_listing;
        }
        
        /* skip . and .. entries */
        if(file_entry->FileNameLength == sizeof(short)){
            if(file_entry->FileName[0] == '.')
                continue;
        }
        if(file_entry->FileNameLength == 2 * sizeof(short)){
            if(file_entry->FileName[0] == '.' && file_entry->FileName[1] == '.')
                continue;
        }

        /* validate entry */
        if(file_entry->FileNameLength == 0)
            continue;
        
        /* add entry to the file list */
        f = ftw_add_entry_to_filelist(path,flags,fcb,pcb,t,
                user_defined_data,filelist,file_entry);
        if(f == NULL){
            winx_heap_free(file_listing);
            NtClose(hDir);
            return (-1);
        }
        
        //DebugPrint("%" WSTR "\n%" WSTR,printableutf(f->name),printablepath(f->path,0));
        
        /* check for termination */
        if(ftw_check_for_termination(t,user_defined_data)){
            DebugPrint("ftw_helper: terminated by user");
            winx_heap_free(file_listing);
            NtClose(hDir);
            return (-2);
        }
        
        /* call the callback routines */
        if(pcb != NULL)
            pcb(f,user_defined_data);
        
        skip_children = 0;
        if(fcb != NULL)
            skip_children = fcb(f,user_defined_data);

        /* scan subdirectories if requested */
        if(is_directory(f) && (flags & WINX_FTW_RECURSIVE) && !skip_children){
            /* don't follow reparse points! */
            if(!is_reparse_point(f)){
                result = ftw_helper(f->path,flags,fcb,pcb,t,user_defined_data,filelist);
                if(result < 0){
                    winx_heap_free(file_listing);
                    NtClose(hDir);
                    return result;
                }
            }
        }
    }
    
    /* terminated */
    winx_heap_free(file_listing);
    NtClose(hDir);
    return (-2);
}

/**
 * @internal
 * @brief Removes resident streams from the file list.
 */
static void ftw_remove_resident_streams(winx_file_info **filelist)
{
    winx_file_info *f, *head, *next = NULL;

    for(f = *filelist; f; f = next){
        head = *filelist;
        next = f->next;
        if(f->disp.fragments == 0){
            winx_heap_free(f->name);
            winx_heap_free(f->path);
            winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
            winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
        }
        if(*filelist == NULL) break;
        if(next == head) break;
    }
}

/**
 * @brief Returns list of files contained
 * in directory, and all its subdirectories
 * if WINX_FTW_RECURSIVE flag is passed.
 * @param[in] path the native path of the
 * directory to be scanned.
 * @param[in] flags combination
 * of WINX_FTW_xxx flags, defined in zenwinx.h
 * @param[in] fcb address of callback routine
 * to be called for each file; if it returns
 * nonzero value, all file's children will be
 * skipped. Zero value forces to continue
 * subdirectory scan. Note that the filter callback
 * is called when the complete information is gathered
 * for the file.
 * @param[in] pcb address of callback routine
 * to be called for each file to update progress
 * information specific for the caller. Note that the
 * progress callback may be called when all the file
 * information is gathered except of the file name and path.
 * If WINX_FTW_SKIP_RESIDENT_STREAMS flag is set, the progress
 * callback will never be called for files of zero length
 * and resident NTFS streams.
 * @param[in] t address of procedure to be called
 * each time when winx_ftw would like to know
 * whether it must be terminated or not.
 * Nonzero value, returned by terminator,
 * forces file tree walk to be terminated.
 * @param[in] user_defined_data pointer to data
 * passed to all registered callbacks.
 * @return List of files, NULL indicates failure.
 * @note 
 * - Optimized for little directories scan.
 * - To scan root directory, add trailing backslash
 *   to the path.
 * - fcb parameter may be equal to NULL if no
 *   filtering is needed.
 * - pcb parameter may be equal to NULL.
 * - Callback procedures should complete as quickly
 *   as possible to avoid slowdown of the scan.
 * - Does not recognize additional NTFS data streams.
 * - For resident NTFS streams (small files and
 *   directories located inside MFT) this function resets
 *   all the file disposition structure fields to zero.
 * - WINX_FTW_DUMP_FILES flag must be set to accept
 *   WINX_FTW_SKIP_RESIDENT_STREAMS.
 * @par Example:
 * @code
 * int filter(winx_file_info *f, void *user_defined_data)
 * {
 *     if(skip_directory(f))
 *         return 1;    // skip contents of the current directory
 *
 *     return 0; // continue walk
 * }
 *
 * void update_progress(winx_file_info *f, void *user_defined_data)
 * {
 *     if(is_directory(f))
 *         dir_count ++;
 *     // etc.
 * }
 *
 * int terminator(void *user_defined_data)
 * {
 *     if(stop_event)
 *         return 1; // terminate walk
 *
 *     return 0;     // continue walk
 * }
 *
 * // list all files on disk c:
 * filelist = winx_ftw(UTF("\\??\\c:\\"),0,filter,update_progress,
 *     terminator,&user_defined_data);
 * // ...
 * // process list of files
 * // ...
 * winx_ftw_release(filelist);
 * @endcode
 */
winx_file_info *winx_ftw(utf_t *path, int flags,
        ftw_filter_callback fcb, ftw_progress_callback pcb,
        ftw_terminator t, void *user_defined_data)
{
    winx_file_info *filelist = NULL;
    
    DbgCheck1(path,"winx_ftw",NULL);
    
    if(flags & WINX_FTW_SKIP_RESIDENT_STREAMS){
        if(!(flags & WINX_FTW_DUMP_FILES)){
            DebugPrint("winx_ftw: WINX_FTW_DUMP_FILES flag"
                " must be set to accept WINX_FTW_SKIP_RESIDENT_STREAMS");
            flags &= ~WINX_FTW_SKIP_RESIDENT_STREAMS;
        }
    }
    
    if(ftw_helper(path,flags,fcb,pcb,t,user_defined_data,&filelist) == (-1) && \
      !(flags & WINX_FTW_ALLOW_PARTIAL_SCAN)){
        /* destroy list */
        winx_ftw_release(filelist);
        return NULL;
    }
      
    if(flags & WINX_FTW_SKIP_RESIDENT_STREAMS)
        ftw_remove_resident_streams(&filelist);
        
    return filelist;
}

/**
 * @brief winx_ftw analog, but optimized
 * for the entire disk scan.
 * @note NTFS is scanned directly through reading
 * MFT records, because this highly (25 times)
 * speeds up the scan. For FAT we have noticed
 * no speedup (even slowdown) while trying
 * to walk trough FAT entries. This is because
 * Windows file cache makes access even faster.
 * UDF has been never tested in direct mode
 * because of its highly complicated standard.
 */
winx_file_info *winx_scan_disk(char volume_letter, int flags,
        ftw_filter_callback fcb, ftw_progress_callback pcb, ftw_terminator t,
        void *user_defined_data)
{
    winx_file_info *filelist = NULL;
    utf_t rootpath[] = UTF("\\??\\A:\\");
    winx_volume_information v;
    ULONGLONG time;
    
    /* ensure that it will work on w2k */
    volume_letter = winx_toupper(volume_letter);
    
    time = winx_xtime();
    winx_dbg_print_header(0,0,"winx_scan_disk started");
    
    if(flags & WINX_FTW_SKIP_RESIDENT_STREAMS){
        if(!(flags & WINX_FTW_DUMP_FILES)){
            DebugPrint("winx_ftw: WINX_FTW_DUMP_FILES flag"
                " must be set to accept WINX_FTW_SKIP_RESIDENT_STREAMS");
            flags &= ~WINX_FTW_SKIP_RESIDENT_STREAMS;
        }
    }
    
    if(winx_get_volume_information(volume_letter,&v) >= 0){
        DebugPrint("winx_scan_disk: file system is %s",v.fs_name);
        if(!strcmp(v.fs_name,"NTFS")){
            filelist = ntfs_scan_disk(volume_letter,flags,fcb,pcb,t,user_defined_data);
            goto cleanup;
        }
    }
    
    /* collect information about root directory */
    rootpath[4] = (short)volume_letter;
    if(ftw_add_root_directory(rootpath,flags,fcb,pcb,t,user_defined_data,&filelist) == (-1) && \
      !(flags & WINX_FTW_ALLOW_PARTIAL_SCAN)){
        /* destroy list */
        winx_ftw_release(filelist);
        filelist = NULL;
        goto done;
    }

    /* collect information about entire directory tree */
    flags |= WINX_FTW_RECURSIVE;
    if(ftw_helper(rootpath,flags,fcb,pcb,t,user_defined_data,&filelist) == (-1) && \
      !(flags & WINX_FTW_ALLOW_PARTIAL_SCAN)){
        /* destroy list */
        winx_ftw_release(filelist);
        filelist = NULL;
        goto done;
    }

cleanup:
    if(flags & WINX_FTW_SKIP_RESIDENT_STREAMS)
        ftw_remove_resident_streams(&filelist);
        
done:
    winx_dbg_print_header(0,0,"winx_scan_disk completed in %" LL64 "u ms",
        (ULONGLONG)(winx_xtime() - time));
    return filelist;
}

/**
 * @brief Releases resources
 * allocated by winx_ftw
 * or winx_scan_disk.
 * @param[in] filelist pointer
 * to list of files.
 */
void winx_ftw_release(winx_file_info *filelist)
{
    winx_file_info *f;

    /* walk through list of files and free allocated memory */
    for(f = filelist; f != NULL; f = f->next){
        if(f->name)
            winx_heap_free(f->name);
        if(f->path)
            winx_heap_free(f->path);
        winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
        if(f->next == filelist) break;
    }
    winx_list_destroy((list_entry **)(void *)&filelist);
}

/** @} */
