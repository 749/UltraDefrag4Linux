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
 * @file file.c
 * @brief File I/O.
 * @addtogroup File
 * @{
 */

#include "zenwinx.h"

/**
 * @brief fopen() native equivalent.
 * @note Only r, w, a, r+, w+, a+ modes are supported.
 */
WINX_FILE *winx_fopen(const char *filename,const char *mode)
{
    ANSI_STRING as;
    UNICODE_STRING us;
    NTSTATUS status;
    HANDLE hFile;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    ACCESS_MASK access_mask = FILE_GENERIC_READ;
    ULONG disposition = FILE_OPEN;
    WINX_FILE *f;

    DbgCheck2(filename,mode,"winx_fopen",NULL);

    RtlInitAnsiString(&as,filename);
    if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
        DebugPrint("winx_fopen: cannot open %s: not enough memory",filename);
        return NULL;
    }
    InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);

    if(!strcmp(mode,"r")){
        access_mask = FILE_GENERIC_READ;
        disposition = FILE_OPEN;
    } else if(!strcmp(mode,"w")){
        access_mask = FILE_GENERIC_WRITE;
        disposition = FILE_OVERWRITE_IF;
    } else if(!strcmp(mode,"r+")){
        access_mask = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
        disposition = FILE_OPEN;
    } else if(!strcmp(mode,"w+")){
        access_mask = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
        disposition = FILE_OVERWRITE_IF;
    } else if(!strcmp(mode,"a")){
        access_mask = FILE_APPEND_DATA;
        disposition = FILE_OPEN_IF;
    } else if(!strcmp(mode,"a+")){
        access_mask = FILE_GENERIC_READ | FILE_APPEND_DATA;
        disposition = FILE_OPEN_IF;
    }
    access_mask |= SYNCHRONIZE;

    status = NtCreateFile(&hFile,
            access_mask,
            &oa,
            &iosb,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            disposition,
            FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0
            );
    RtlFreeUnicodeString(&us);
    if(status != STATUS_SUCCESS){
        DebugPrintEx(status,"winx_fopen: cannot open %s",filename);
        return NULL;
    }
    f = (WINX_FILE *)winx_heap_alloc(sizeof(WINX_FILE));
    if(!f){
        NtClose(hFile);
        DebugPrint("winx_fopen: cannot open %s: not enough memory",filename);
        return NULL;
    }
    f->hFile = hFile;
    f->roffset.QuadPart = 0;
    f->woffset.QuadPart = 0;
    f->io_buffer = NULL;
    f->io_buffer_size = 0;
    f->io_buffer_offset = 0;
    f->wboffset.QuadPart = 0;
    return f;
}

/**
 * @brief winx_fopen analog, but
 * allocates a buffer to speedup
 * sequential write requests.
 * @details The last parameter specifies
 * the buffer size, in bytes. Returns
 * NULL if buffer allocation failed.
 */
WINX_FILE *winx_fbopen(const char *filename,const char *mode,int buffer_size)
{
    WINX_FILE *f;
    
    /* open the file */
    f = winx_fopen(filename,mode);
    if(f == NULL)
        return NULL;
    
    if(buffer_size <= 0)
        return f;
    
    /* allocate memory */
    f->io_buffer = winx_heap_alloc(buffer_size);
    if(f->io_buffer == NULL){
        DebugPrint("winx_fbopen: cannot allocate %u bytes of memory",buffer_size);
        winx_fclose(f);
        return NULL;
    }
    
    f->io_buffer_size = buffer_size;
    return f;
}

/**
 * @brief fread() native equivalent.
 */
size_t winx_fread(void *buffer,size_t size,size_t count,WINX_FILE *f)
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    
    DbgCheck2(buffer,f,"winx_fread",0);

    status = NtReadFile(f->hFile,NULL,NULL,NULL,&iosb,
             buffer,size * count,&f->roffset,NULL);
    if(NT_SUCCESS(status)){
        status = NtWaitForSingleObject(f->hFile,FALSE,NULL);
        if(NT_SUCCESS(status)) status = iosb.Status;
    }
    if(status != STATUS_SUCCESS){
        DebugPrintEx(status,"winx_fread: cannot read from a file");
        return 0;
    }
    if(iosb.Information == 0){ /* encountered on x64 XP */
        f->roffset.QuadPart += size * count;
        return count;
    }
    f->roffset.QuadPart += (size_t)iosb.Information;
    return ((size_t)iosb.Information / size);
}

/**
 * @internal
 * @brief winx_fwrite helper.
 * @details Writes to the file directly
 * regardless of whether it is opened for
 * buffered i/o or not.
 */
static size_t winx_fwrite_helper(const void *buffer,size_t size,size_t count,WINX_FILE *f)
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    
    DbgCheck2(buffer,f,"winx_fwrite_helper",0);

    status = NtWriteFile(f->hFile,NULL,NULL,NULL,&iosb,
             (void *)buffer,size * count,&f->woffset,NULL);
    if(NT_SUCCESS(status)){
        /*DebugPrint("waiting for %p at %I64u started",f,f->woffset.QuadPart);*/
        status = NtWaitForSingleObject(f->hFile,FALSE,NULL);
        /*DebugPrint("waiting for %p at %I64u completed",f,f->woffset.QuadPart);*/
        if(NT_SUCCESS(status)) status = iosb.Status;
    }
    if(status != STATUS_SUCCESS){
        DebugPrintEx(status,"winx_fwrite_helper: cannot write to a file");
        return 0;
    }
    if(iosb.Information == 0){ /* encountered on x64 XP */
        f->woffset.QuadPart += size * count;
        return count;
    }
    f->woffset.QuadPart += (size_t)iosb.Information;
    return ((size_t)iosb.Information / size);
}

/**
 * @brief fwrite() native equivalent.
 */
size_t winx_fwrite(const void *buffer,size_t size,size_t count,WINX_FILE *f)
{
    LARGE_INTEGER nwd_offset; /* offset of data not written yet, in file */
    LARGE_INTEGER new_offset; /* current f->woffset */
    size_t bytes, result;
    
    if(buffer == NULL || f == NULL)
        return 0;
    
    /*
    * Check whether the file was
    * opened for buffered access or not.
    */
    bytes = size * count;
    if(f->io_buffer == NULL || f->io_buffer_size == 0){
        f->io_buffer_offset = 0;
        f->wboffset.QuadPart += bytes;
        return winx_fwrite_helper(buffer,size,count,f);
    }

    /* check whether file pointer has been adjusted or not */
    nwd_offset.QuadPart = f->wboffset.QuadPart - f->io_buffer_offset;
    new_offset.QuadPart = f->woffset.QuadPart;
    if(new_offset.QuadPart != nwd_offset.QuadPart){
        /* flush buffer */
        f->woffset.QuadPart = nwd_offset.QuadPart;
        result = winx_fwrite_helper(f->io_buffer,1,f->io_buffer_offset,f);
        f->io_buffer_offset = 0;
        /* update file pointer */
        f->wboffset.QuadPart = f->woffset.QuadPart = new_offset.QuadPart;
        if(result == 0){
            /* write request failed */
            return 0;
        }
    }

    /* check whether the buffer is full or not */
    if(bytes > f->io_buffer_size - f->io_buffer_offset && f->io_buffer_offset){
        /* flush buffer */
        result = winx_fwrite_helper(f->io_buffer,1,f->io_buffer_offset,f);
        f->io_buffer_offset = 0;
        if(result == 0){
            /* write request failed */
            return 0;
        }
    }
    
    /* check whether the buffer has sufficient size or not */
    if(bytes >= f->io_buffer_size){
        f->wboffset.QuadPart += bytes;
        return winx_fwrite_helper(buffer,size,count,f);
    }
    
    /* append new data to the buffer */
    memcpy((char *)f->io_buffer + f->io_buffer_offset,buffer,bytes);
    f->io_buffer_offset += bytes;
    f->wboffset.QuadPart += bytes;
    return count;
}

/**
 * @brief Sends an I/O control code to the specified device.
 * @param[in] f the file handle.
 * @param[in] code the IOCTL code.
 * @param[in] description the string explaining
 * the meaning of the request, used by error handling code.
 * @param[in] in_buffer the input buffer pointer.
 * @param[in] in_size the input buffer size, in bytes.
 * @param[out] out_buffer the output buffer pointer.
 * @param[in] out_size the output buffer size, in bytes.
 * @param[out] pbytes_returned pointer to the variable receiving
 * the number of bytes written to the output buffer.
 * @return Zero for success, negative value otherwise.
 */
int winx_ioctl(WINX_FILE *f,
    int code,char *description,
    void *in_buffer,int in_size,
    void *out_buffer,int out_size,
    int *pbytes_returned)
{
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;

    DbgCheck1(f,"winx_ioctl",-1);
    
    /* required by x64 system, otherwise it may trash stack */
    if(out_buffer) RtlZeroMemory(out_buffer,out_size);
    
    if(pbytes_returned) *pbytes_returned = 0;
    if((code >> 16) == FILE_DEVICE_FILE_SYSTEM){
        Status = NtFsControlFile(f->hFile,NULL,NULL,NULL,
            &iosb,code,in_buffer,in_size,out_buffer,out_size);
    } else {
        Status = NtDeviceIoControlFile(f->hFile,NULL,NULL,NULL,
            &iosb,code,in_buffer,in_size,out_buffer,out_size);
    }
    if(NT_SUCCESS(Status)){
        Status = NtWaitForSingleObject(f->hFile,FALSE,NULL);
        if(NT_SUCCESS(Status)) Status = iosb.Status;
    }
    if(!NT_SUCCESS(Status)){
        if(description)
            DebugPrintEx(Status,"winx_ioctl: %s failed",description);
        else
            DebugPrintEx(Status,"winx_ioctl: IOCTL %u failed",code);
        return (-1);
    }
    if(pbytes_returned) *pbytes_returned = (int)iosb.Information;
    return 0;
}

/**
 * @brief fflush() native equivalent.
 * @return Zero for success, negative value otherwise.
 */
int winx_fflush(WINX_FILE *f)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    
    DbgCheck1(f,"winx_fflush",-1);

    Status = NtFlushBuffersFile(f->hFile,&iosb);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_fflush: NtFlushBuffersFile failed");
        return (-1);
    }
    return 0;
}

/**
 * @brief Retrieves the size of the file.
 * @param[in] f pointer to structure returned
 * by winx_fopen() call.
 * @return The size of the file, in bytes.
 */
ULONGLONG winx_fsize(WINX_FILE *f)
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    FILE_STANDARD_INFORMATION fsi;

    DbgCheck1(f,"winx_fsize",0);

    memset(&fsi,0,sizeof(FILE_STANDARD_INFORMATION));
    status = NtQueryInformationFile(f->hFile,&iosb,
        &fsi,sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation);
    if(!NT_SUCCESS(status)){
        DebugPrintEx(status,"winx_fsize: NtQueryInformationFile(FileStandardInformation) failed");
        return 0;
    }
    return fsi.EndOfFile.QuadPart;
}

/**
 * @brief fclose() native equivalent.
 */
void winx_fclose(WINX_FILE *f)
{
    if(f == NULL)
        return;
    
    if(f->io_buffer){
        /* write the rest of the data */
        if(f->io_buffer_offset)
            winx_fwrite_helper(f->io_buffer,1,f->io_buffer_offset,f);
        winx_heap_free(f->io_buffer);
    }

    if(f->hFile) NtClose(f->hFile);
    winx_heap_free(f);
}

/**
 * @brief Creates a directory.
 * @param[in] path the native path to the directory.
 * @return Zero for success, negative value otherwise.
 * @note If the requested directory already exists
 * this function completes successfully.
 */
int winx_create_directory(const char *path)
{
    ANSI_STRING as;
    UNICODE_STRING us;
    NTSTATUS status;
    HANDLE hFile;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;

    DbgCheck1(path,"winx_create_directory",-1);

    RtlInitAnsiString(&as,path);
    if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
        DebugPrint("winx_create_directory: cannot create %s: not enough memory",path);
        return (-1);
    }
    InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);

    status = NtCreateFile(&hFile,
            FILE_LIST_DIRECTORY | SYNCHRONIZE | FILE_OPEN_FOR_BACKUP_INTENT,
            &oa,
            &iosb,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_CREATE,
            FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE,
            NULL,
            0
            );
    RtlFreeUnicodeString(&us);
    if(NT_SUCCESS(status)){
        NtClose(hFile);
        return 0;
    }
    /* if it already exists then return success */
    if(status == STATUS_OBJECT_NAME_COLLISION) return 0;
    DebugPrintEx(status,"winx_create_directory: cannot create %s",path);
    return (-1);
}

/**
 * @brief Deletes a file.
 * @param[in] filename the native path to the file.
 * @return Zero for success, negative value otherwise.
 */
int winx_delete_file(const char *filename)
{
    ANSI_STRING as;
    UNICODE_STRING us;
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;

    DbgCheck1(filename,"winx_delete_file",-1);

    RtlInitAnsiString(&as,filename);
    if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
        DebugPrint("winx_delete_file: cannot delete %s: not enough memory",filename);
        return (-1);
    }

    InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
    status = NtDeleteFile(&oa);
    RtlFreeUnicodeString(&us);
    if(!NT_SUCCESS(status)){
        DebugPrintEx(status,"winx_delete_file: cannot delete %s",filename);
        return (-1);
    }
    return 0;
}

/**
 * @brief Reads file entirely and returns
 * pointer to data read.
 * @param[in] filename the native path to the file.
 * @param[out] bytes_read number of bytes read.
 * @return Pointer to data, NULL indicates failure.
 * @note Returned buffer is two bytes larger than
 * the file contents. This allows to add terminal
 * zero easily.
 */
void *winx_get_file_contents(const char *filename,size_t *bytes_read)
{
    WINX_FILE *f;
    ULONGLONG size;
    size_t length, n_read;
    void *contents;
    
    if(bytes_read) *bytes_read = 0;
    
    DbgCheck1(filename,"winx_get_file_contents",NULL);
    
    f = winx_fopen(filename,"r");
    if(f == NULL){
        winx_printf("\nCannot open %s file!\n\n",filename);
        return NULL;
    }
    
    size = winx_fsize(f);
    if(size == 0){
        winx_fclose(f);
        return NULL;
    }
    
#ifndef _WIN64
    if(size > 0xFFFFFFFF){
        winx_printf("\n%s: Files larger than ~4Gb aren\'t supported!\n\n",
            filename);
        winx_fclose(f);
        return NULL;
    }
#endif
    length = (size_t)size;
    
    contents = winx_heap_alloc(length + 2);
    if(contents == NULL){
        winx_printf("\n%s: Cannot allocate %u bytes of memory!\n\n",
            filename,length + 2);
        winx_fclose(f);
        return NULL;
    }
    
    n_read = winx_fread(contents,1,length,f);
    if(n_read == 0 || n_read > length){
        winx_heap_free(contents);
        winx_fclose(f);
        return NULL;
    }
    
    if(bytes_read) *bytes_read = n_read;
    winx_fclose(f);
    return contents;
}

/**
 * @brief Releases memory allocated
 * by winx_get_file_contents().
 */
void winx_release_file_contents(void *contents)
{
    if(contents) winx_heap_free(contents);
}

/**
 * @internal
 */
struct names_pair {
    wchar_t *original_name;
    wchar_t *accepted_name;
};

/**
 * @internal
 * @brief Auxiliary table helping to replace
 * file name by name accepted by Windows
 * in case of special NTFS files.
 */
struct names_pair special_file_names[] = {
    { L"$Secure:$SDH",                  L"$Secure:$SDH:$INDEX_ALLOCATION" },
    { L"$Secure:$SII",                  L"$Secure:$SII:$INDEX_ALLOCATION" },
    { L"$Extend",                       L"$Extend:$I30:$INDEX_ALLOCATION" },
    { L"$Extend\\$Quota:$Q",            L"$Extend\\$Quota:$Q:$INDEX_ALLOCATION" },
    { L"$Extend\\$Quota:$O",            L"$Extend\\$Quota:$O:$INDEX_ALLOCATION" },
    { L"$Extend\\$ObjId:$O",            L"$Extend\\$ObjId:$O:$INDEX_ALLOCATION" },
    { L"$Extend\\$Reparse:$R",          L"$Extend\\$Reparse:$R:$INDEX_ALLOCATION" },
    { L"$Extend\\$RmMetadata",          L"$Extend\\$RmMetadata:$I30:$INDEX_ALLOCATION" },
    { L"$Extend\\$RmMetadata\\$Txf",    L"$Extend\\$RmMetadata\\$Txf:$I30:$INDEX_ALLOCATION" },
    { L"$Extend\\$RmMetadata\\$TxfLog", L"$Extend\\$RmMetadata\\$TxfLog:$I30:$INDEX_ALLOCATION" },
    { NULL, NULL }
};

/**
 * @brief Opens the file for defragmentation related actions.
 * @param[in] f pointer to structure containing the file information.
 * @param[in] action one of the WINX_OPEN_XXX constants
 * indicating an action file needs to be opened for:
 * - WINX_OPEN_FOR_DUMP - open for FSCTL_GET_RETRIEVAL_POINTERS
 * - WINX_OPEN_FOR_BASIC_INFO - open for NtQueryInformationFile(FILE_BASIC_INFORMATION)
 * - WINX_OPEN_FOR_MOVE - open for FSCTL_MOVE_FILE
 * @param[out] phandle pointer to variable receiving file handle.
 * @return NTSTATUS code.
 */
NTSTATUS winx_defrag_fopen(winx_file_info *f,int action,HANDLE *phandle)
{
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    int win_version = winx_get_os_version();
    ACCESS_MASK access_rights = SYNCHRONIZE;
    ULONG flags = FILE_SYNCHRONOUS_IO_NONALERT;
    int length, i;
    char volume_letter;
    wchar_t *path;
    wchar_t buffer[MAX_PATH + 1];

    if(f == NULL || phandle == NULL)
        return STATUS_INVALID_PARAMETER;
    
    if(f->path == NULL)
        return STATUS_INVALID_PARAMETER;
    
    if(f->path[0] == 0)
        return STATUS_INVALID_PARAMETER;
    
    if(is_directory(f)){
        flags |= FILE_OPEN_FOR_BACKUP_INTENT;
    } else {
        flags |= FILE_NO_INTERMEDIATE_BUFFERING;
        
        if(win_version >= WINDOWS_VISTA)
            flags |= FILE_NON_DIRECTORY_FILE;
    }
    
    if(is_reparse_point(f)){
        /* open the point itself, not its target */
        flags |= FILE_OPEN_REPARSE_POINT;
    }

    /*
    * All files except of internal NTFS files
    * can be successfully opened with FILE_GENERIC_READ | SYNCHRONIZE
    * access rights on all of the supported versions of Windows.
    * To open internal NTFS files including $mft, we use more restricted rights.
    */
    if(win_version <= WINDOWS_2K){
        /* on Windows NT and Windows 2000 */
        if(is_encrypted(f)){
            /* encrypted files may require read access */
            access_rights |= FILE_GENERIC_READ;
        } else {
            /*
            * All other files can be successfully opened with a single
            * SYNCHRONIZE access. More advanced FILE_GENERIC_READ
            * rights may prevent opening of internal NTFS files on w2k.
            */
        }
    } else if(win_version == WINDOWS_XP || win_version == WINDOWS_2K3){
        /* On Windows XP and Windows Server 2003 */
        /*
        * All files can be opened with a single SYNCHRONIZE access.
        * More advanced FILE_GENERIC_READ rights prevent opening
        * of $mft file as well as other internal NTFS files.
        * http://forum.sysinternals.com/topic23950.html
        */
    } else if(win_version >= WINDOWS_VISTA){
        /* On Windows Vista and Windows 7 */
        /*
        * $Mft may require more advanced rights, 
        * than a single SYNCHRONIZE.
        */
        access_rights |= FILE_READ_ATTRIBUTES;
    }
    
    /* root folder needs FILE_READ_ATTRIBUTES to successfully retrieve FileBasicInformation,
    see http://msdn.microsoft.com/en-us/library/ff567052(VS.85).aspx */
    if(action == WINX_OPEN_FOR_BASIC_INFO)
        access_rights |= FILE_READ_ATTRIBUTES;
    
    /*
    * FILE_READ_ATTRIBUTES may also be needed 
    * for bitmaps on Windows XP as stated in:
    * http://www.microsoft.com/whdc/archive/2kuptoXP.mspx
    * However, nonresident bitmaps seem to be extraordinary.
    */
    
    /*
    * Handle special cases, according to
    * http://msdn.microsoft.com/en-us/library/windows/desktop/aa363911(v=vs.85).aspx
    */
    path = f->path;
    length = wcslen(f->path);
    if(length >= 9){ /* to ensure that we have at least \??\X:\$x */
        if(f->path[7] == '$'){
            volume_letter = (char)f->path[4];
            for(i = 0; special_file_names[i].original_name; i++){
                if(winx_wcsistr(f->path,special_file_names[i].original_name)){
                    if(wcslen(f->path) == wcslen(special_file_names[i].original_name) + 0x7){
                        _snwprintf(buffer,MAX_PATH,L"\\??\\%c:\\%ws",volume_letter,
                            special_file_names[i].accepted_name);
                        buffer[MAX_PATH] = 0;
                        path = buffer;
                        DebugPrint("winx_defrag_fopen: %ws used instead of %ws",path,f->path);
                        break;
                    }
                }
            }
        }
    }
    
    RtlInitUnicodeString(&us,path);
    InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
    status = NtCreateFile(phandle,access_rights,&oa,&iosb,NULL,0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,flags,NULL,0);
    if(status != STATUS_SUCCESS)
        *phandle = NULL;
    return status;
}

/**
 * @brief Closes a file opened
 * by winx_defrag_fopen.
 */
void winx_defrag_fclose(HANDLE h)
{
    NtCloseSafe(h);
}

/** @} */
