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

/**
 * @file ftw.c
 * @brief File tree walk.
 * @addtogroup File
 * @{
 */

#include "ntndk.h"
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
 * @brief Validates a map of file blocks,
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
    b1 = f->disp.blockmap;
    if(b1) b2 = b1->next;
    if(b1 && b2 && b2 != b1){
        if(b1->vcn == b2->vcn){
            etrace("%ws: wrong map detected:", f->path);
            for(b1 = f->disp.blockmap; b1; b1 = b1->next){
                etrace("VCN = %I64u, LCN = %I64u, LEN = %I64u",
                    b1->vcn, b1->lcn, b1->length);
                if(b1->next == f->disp.blockmap) break;
            }
            winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
        }
    }
#endif
}

/**
 * @brief Retrieves the information
 * on the file disposition.
 * @param[out] f pointer to
 * structure receiving the information.
 * @param[in] t address of procedure to be called
 * each time when winx_ftw_dump_file would like
 * to know whether it must be terminated or not.
 * Nonzero value, returned by terminator,
 * forces the file dump to be terminated.
 * @param[in] user_defined_data pointer to data
 * passed to the registered terminator.
 * @return Zero for success,
 * negative value otherwise.
 * @note 
 * - The callback procedure should complete as quickly
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
    int i;
    winx_blockmap *block = NULL;
    
    DbgCheck1(f,-1);
    
    /* reset disposition related fields */
    f->disp.clusters = 0;
    f->disp.fragments = 0;
    winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
    
    /* open the file */
    status = winx_defrag_fopen(f,WINX_OPEN_FOR_DUMP,&hFile);
    if(status != STATUS_SUCCESS){
        strace(status,"cannot open %ws",f->path);
        return 0; /* file is locked by system */
    }
    
    /* allocate memory */
    filemap = winx_malloc(FILE_MAP_SIZE);
    
    /* dump the file */
    startVcn = 0;
    counter = 0;
    do {
        memset(filemap,0,FILE_MAP_SIZE);
        status = NtFsControlFile(hFile,NULL,NULL,0,
            &iosb,FSCTL_GET_RETRIEVAL_POINTERS,
            &startVcn,sizeof(ULONGLONG),
            filemap,FILE_MAP_SIZE);
        counter ++;
        if(NT_SUCCESS(status)){
            NtWaitForSingleObject(hFile,FALSE,NULL);
            status = iosb.Status;
        }
        if(status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW){
            /* it always returns STATUS_END_OF_FILE for small files placed inside MFT */
            if(status == STATUS_END_OF_FILE) goto empty_map_detected;
            strace(status,"dump failed for %ws",f->path);
            goto dump_failed;
        }

        if(ftw_check_for_termination(t,user_defined_data)){
            if(counter > MAX_COUNT)
                etrace("%ws: infinite main loop?",f->path);
            /* reset incomplete map */
            goto cleanup;
        }
        
        /* check for an empty map */
        if(!filemap->NumberOfPairs && status != STATUS_SUCCESS){
            etrace("%ws: empty map of file detected",f->path);
            goto empty_map_detected;
        }
        
        /* loop through the buffer of number/cluster pairs */
        startVcn = filemap->StartVcn;
        for(i = 0; i < (ULONGLONG)filemap->NumberOfPairs; startVcn = filemap->Pair[i].Vcn, i++){
            /* compressed virtual runs (0-filled) are identified with LCN == -1    */
            if(filemap->Pair[i].Lcn == LLINVALID)
                continue;
            
            /* the following is usual for 3.99 GB files on FAT32 under XP */
            if(filemap->Pair[i].Vcn == 0){
                etrace("%ws: wrong map of file detected",f->path);
                goto dump_failed;
            }
            
            block = (winx_blockmap *)winx_list_insert((list_entry **)&f->disp.blockmap,
                (list_entry *)block,sizeof(winx_blockmap));
            block->lcn = filemap->Pair[i].Lcn;
            block->length = filemap->Pair[i].Vcn - startVcn;
            block->vcn = startVcn;
            
            //trace(D"VCN = %I64u, LCN = %I64u, LENGTH = %I64u",
            //    block->vcn,block->lcn,block->length);
            f->disp.clusters += block->length;
            
            /*
            * Sometimes files have more than one fragment, 
            * but are not fragmented yet. In case of compressed
            * files this happens quite frequently.
            */
            if(block == f->disp.blockmap || \
              block->lcn != (block->prev->lcn + block->prev->length)){
                f->disp.fragments ++;
            }
        }
    } while(status != STATUS_SUCCESS);
    /* small directories placed inside MFT have empty list of fragments... */

    /* the dump is completed */
    validate_blockmap(f);
    winx_free(filemap);
    winx_defrag_fclose(hFile);
    return 0;
    
cleanup:
empty_map_detected:
    f->disp.clusters = 0;
    f->disp.fragments = 0;
    winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
    winx_free(filemap);
    winx_defrag_fclose(hFile);
    return 0;

dump_failed:
    f->disp.clusters = 0;
    f->disp.fragments = 0;
    winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
    winx_free(filemap);
    winx_defrag_fclose(hFile);
    return (-1);
}

/**
 * @internal
 * @brief Adds directory entry to the file list.
 * @return Address of inserted file list entry,
 * NULL indicates failure.
 */
static winx_file_info * ftw_add_entry_to_filelist(wchar_t *path,
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
        etrace("path is empty");
        return NULL;
    }
    
    /* insert new item to the file list */
    f = (winx_file_info *)winx_list_insert((list_entry **)(void *)filelist,
        NULL,sizeof(winx_file_info));
    
    /* extract filename */
    f->name = winx_tmalloc(file_entry->FileNameLength + sizeof(wchar_t));
    if(f->name == NULL){
        etrace("cannot allocate %u bytes of memory",
            file_entry->FileNameLength + sizeof(wchar_t));
        winx_list_remove((list_entry **)(void *)filelist,(list_entry *)f);
        return NULL;
    }
    memset(f->name,0,file_entry->FileNameLength + sizeof(wchar_t));
    memcpy(f->name,file_entry->FileName,file_entry->FileNameLength);
    
    /* detect whether we are in root directory or not */
    length = (int)wcslen(path);
    if(path[length - 1] == '\\')
        is_rootdir = 1; /* only root directory contains trailing backslash */
    
    /* build path */
    length += (int)wcslen(f->name) + 1;
    if(!is_rootdir)
        length ++;
    f->path = winx_tmalloc(length * sizeof(wchar_t));
    if(f->path == NULL){
        etrace("cannot allocate %u bytes of memory",
            length * sizeof(wchar_t));
        winx_free(f->name);
        winx_list_remove((list_entry **)(void *)filelist,(list_entry *)f);
        return NULL;
    }
    if(is_rootdir)
        (void)_snwprintf(f->path,length,L"%ws%ws",path,f->name);
    else
        (void)_snwprintf(f->path,length,L"%ws\\%ws",path,f->name);
    f->path[length - 1] = 0;
    
    /* save file attributes and access times */
    f->flags = file_entry->FileAttributes;
    f->creation_time = file_entry->CreationTime.QuadPart;
    f->last_modification_time = file_entry->LastWriteTime.QuadPart;
    f->last_access_time = file_entry->LastAccessTime.QuadPart;
    
    /* reset user defined flags */
    f->user_defined_flags = 0;
    
    //trace(D"%ws",f->path);
    
    /* reset internal data fields */
    memset(&f->internal,0,sizeof(winx_file_internal_info));
    
    /* reset file disposition */
    memset(&f->disp,0,sizeof(winx_file_disposition));

    /* get file disposition if requested */
    if(flags & WINX_FTW_DUMP_FILES){
        if(winx_ftw_dump_file(f,t,user_defined_data) < 0){
            winx_free(f->name);
            winx_free(f->path);
            winx_list_remove((list_entry **)(void *)filelist,(list_entry *)f);
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
static int ftw_add_root_directory(wchar_t *path, int flags,
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
        etrace("path is empty");
        return (-1);
    }
    
    /* insert new item to the file list */
    f = (winx_file_info *)winx_list_insert((list_entry **)(void *)filelist,
        NULL,sizeof(winx_file_info));
    
    /* build path */
    length = (int)wcslen(path) + 1;
    f->path = winx_malloc(length * sizeof(wchar_t));
    wcscpy(f->path,path);
    
    /* save . filename */
    f->name = winx_malloc(2 * sizeof(wchar_t));
    f->name[0] = '.';
    f->name[1] = 0;
    
    /* get file attributes and access times */
    f->flags = FILE_ATTRIBUTE_DIRECTORY;
    f->creation_time = 0;
    f->last_modification_time = 0;
    f->last_access_time = 0;
    status = winx_defrag_fopen(f,WINX_OPEN_FOR_BASIC_INFO,&hDir);
    if(status == STATUS_SUCCESS){
        memset(&fbi,0,sizeof(FILE_BASIC_INFORMATION));
        status = NtQueryInformationFile(hDir,&iosb,
            &fbi,sizeof(FILE_BASIC_INFORMATION),
            FileBasicInformation);
        if(!NT_SUCCESS(status)){
            strace(status,"cannot get basic file information");
        } else {
            f->flags = fbi.FileAttributes;
            f->creation_time = fbi.CreationTime.QuadPart;
            f->last_modification_time = fbi.LastWriteTime.QuadPart;
            f->last_access_time = fbi.LastAccessTime.QuadPart;
            itrace("root directory flags: %u",f->flags);
        }
        winx_defrag_fclose(hDir);
    } else {
        strace(status,"cannot open %ws",f->path);
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
            winx_free(f->name);
            winx_free(f->path);
            winx_list_remove((list_entry **)(void *)filelist,(list_entry *)f);
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
static HANDLE ftw_open_directory(wchar_t *path)
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
        strace(status,"cannot open %ws",path);
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
static int ftw_helper(wchar_t *path, int flags,
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
    file_listing = winx_malloc(FILE_LISTING_SIZE);
    
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
                    strace(status,"cannot get directory information");
                /* no more entries to read */
                winx_free(file_listing);
                NtClose(hDir);
                return 0;
            }
            file_entry = file_listing;
        }
        
        /* skip . and .. entries */
        if(file_entry->FileNameLength == sizeof(wchar_t)){
            if(file_entry->FileName[0] == '.')
                continue;
        }
        if(file_entry->FileNameLength == 2 * sizeof(wchar_t)){
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
            winx_free(file_listing);
            NtClose(hDir);
            return (-1);
        }
        
        //trace(D"%ws\n%ws",f->name,f->path);
        
        /* check for termination */
        if(ftw_check_for_termination(t,user_defined_data)){
            itrace("terminated by user");
            winx_free(file_listing);
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
                    winx_free(file_listing);
                    NtClose(hDir);
                    return result;
                }
            }
        }
    }
    
    /* terminated */
    winx_free(file_listing);
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
            winx_free(f->name);
            winx_free(f->path);
            winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
            winx_list_remove((list_entry **)(void *)filelist,(list_entry *)f);
        }
        if(*filelist == NULL) break;
        if(next == head) break;
    }
}

/**
 * @internal
 * @brief Removes invalid streams from the file list.
 */
static void ftw_remove_invalid_streams(winx_file_info **filelist)
{
    winx_file_info *f, *head, *next = NULL;
    int invalid_entry;

    for(f = *filelist; f; f = next){
        head = *filelist;
        next = f->next;
        invalid_entry = 0;
        if(f->path == NULL){
            invalid_entry = 1;
        } else if(f->path[0] == 0){
            invalid_entry = 1;
        }
        if(invalid_entry){
            winx_free(f->name);
            winx_free(f->path);
            winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
            winx_list_remove((list_entry **)(void *)filelist,(list_entry *)f);
        }
        if(*filelist == NULL) break;
        if(next == head) break;
    }
}

/**
 * @brief Returns list of files contained
 * in a directory, and all its subdirectories
 * if WINX_FTW_RECURSIVE flag is passed.
 * @param[in] path the native path of the
 * directory to be scanned.
 * @param[in] flags the combination
 * of WINX_FTW_xxx flags, defined in zenwinx.h
 * @param[in] fcb the address of the callback routine
 * to be called for each file; if it returns
 * nonzero value, all the file's children will be
 * skipped. Zero value forces to continue the 
 * subdirectory scan. Note that the filter callback
 * is called when the complete information is gathered
 * for the file.
 * @param[in] pcb the address of the callback routine
 * to be called for each file to update progress
 * information specific for the caller. Note that the
 * progress callback may be called when all the file
 * information is gathered except of the file name and path.
 * If WINX_FTW_SKIP_RESIDENT_STREAMS flag is set, the progress
 * callback will never be called for files of zero length
 * and resident NTFS streams.
 * @param[in] t the address of the procedure to be called
 * each time when winx_ftw would like to know
 * whether it must be terminated or not.
 * Nonzero value, returned by the terminator,
 * forces the file tree walk to be terminated.
 * @param[in] user_defined_data pointer to data
 * passed to all the registered callbacks.
 * @return List of files, NULL indicates failure.
 * @note 
 * - Optimized for little directories scan.
 * - To scan root directory, add trailing backslash
 *   to the path.
 * - fcb parameter may be equal to NULL if no
 *   filtering is needed.
 * - pcb parameter may be equal to NULL.
 * - The callback procedures should complete as quickly
 *   as possible to avoid slowdown of the scan.
 * - Does not recognize additional NTFS data streams.
 * - For resident NTFS streams (small files and
 *   directories located inside MFT) this function resets
 *   all the file disposition structure fields to zero.
 * - WINX_FTW_DUMP_FILES flag must be set to accept
 *   WINX_FTW_SKIP_RESIDENT_STREAMS.
 * - Files with empty paths become excluded from the list,
 *   but may pass through the filter callback.
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
 * filelist = winx_ftw(L"\\??\\c:\\",0,filter,update_progress,
 *     terminator,&user_defined_data);
 * // ...
 * // process list of files
 * // ...
 * winx_ftw_release(filelist);
 * @endcode
 */
winx_file_info *winx_ftw(wchar_t *path, int flags,
        ftw_filter_callback fcb, ftw_progress_callback pcb,
        ftw_terminator t, void *user_defined_data)
{
    winx_file_info *filelist = NULL;
    
    DbgCheck1(path,NULL);
    
    if(flags & WINX_FTW_SKIP_RESIDENT_STREAMS){
        if(!(flags & WINX_FTW_DUMP_FILES)){
            etrace("WINX_FTW_DUMP_FILES flag must be set"
                " to accept WINX_FTW_SKIP_RESIDENT_STREAMS");
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
    
    /* get rid of invalid entries */
    ftw_remove_invalid_streams(&filelist);
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
    wchar_t rootpath[] = L"\\??\\A:\\";
    winx_volume_information v;
    ULONGLONG time;
    
    /* ensure that it will work on w2k */
    volume_letter = winx_toupper(volume_letter);
    
    time = winx_xtime();
    winx_dbg_print_header(0,0,I"winx_scan_disk started");
    
    if(flags & WINX_FTW_SKIP_RESIDENT_STREAMS){
        if(!(flags & WINX_FTW_DUMP_FILES)){
            etrace("WINX_FTW_DUMP_FILES flag must be set"
                " to accept WINX_FTW_SKIP_RESIDENT_STREAMS");
            flags &= ~WINX_FTW_SKIP_RESIDENT_STREAMS;
        }
    }
    
    if(winx_get_volume_information(volume_letter,&v) >= 0){
        itrace("file system is %s",v.fs_name);
        if(!strcmp(v.fs_name,"NTFS")){
            filelist = ntfs_scan_disk(volume_letter,flags,fcb,pcb,t,user_defined_data);
            goto cleanup;
        }
    }
    
    /* collect information about root directory */
    rootpath[4] = (wchar_t)volume_letter;
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
    /* get rid of invalid entries */
    ftw_remove_invalid_streams(&filelist);
        
done:
    winx_dbg_print_header(0,0,I"winx_scan_disk completed in %I64u ms",
        winx_xtime() - time);
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
        winx_free(f->name);
        winx_free(f->path);
        winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
        if(f->next == filelist) break;
    }
    winx_list_destroy((list_entry **)(void *)&filelist);
}

/** @} */
