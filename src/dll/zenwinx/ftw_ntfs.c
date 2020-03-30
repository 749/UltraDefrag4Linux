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
 * @file ftw_ntfs.c
 * @brief Fast file tree walk for NTFS.
 * @addtogroup File
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"
#include "ntfs.h"

/*
* Uncomment this definition to test
* NTFS scanner by corrupting MFT records
* in random order. The scanner should
* catch all errors and never hang/crash.
*/
//#define TEST_NTFS_SCANNER

/*
* Uncomment this definition to show
* extra infomation on attribute lists
* analysis.
*/
//#define SHOW_ATTR_LISTS_INFO

/* internal structures */
typedef struct _mft_layout {
    unsigned long file_record_size;         /* size of mft file record, in bytes */
    unsigned long file_record_buffer_size;  /* size of the buffer for file record reading, in bytes */
    ULONGLONG number_of_file_records;       /* total number of mft file records */
    ULONGLONG total_clusters;               /* size of the volume, in clusters */
    ULONGLONG cluster_size;                 /* size of a cluster, in bytes */
    ULONG sectors_per_cluster;              /* number of sectors in a cluster */
    ULONG sector_size;                      /* sector size, in bytes */
} mft_layout;

enum {
    MFT_SCAN_RTL,
    MFT_SCAN_LTR,
};

typedef struct {
    ULONGLONG BaseMftId;             /* base mft index */
    ULONGLONG ParentDirectoryMftId;  /* mft index of parent directory */
    ULONG Flags;                     /* combination of FILE_ATTRIBUTE_xxx flags defined in winnt.h */
    UCHAR NameType;                  /**/
    WCHAR Name[MAX_PATH];            /**/
    ULONGLONG CreationTime;          /* The time when the file was created in the standard time format. */
    ULONGLONG LastWriteTime;         /* The time when the file was last written in the standard time format. */
    ULONGLONG LastAccessTime;        /* The time when the file was last accessed in the standard time format. */
} my_file_information;

typedef struct _mft_scan_parameters {
    int mft_scan_direction;     /* scan direction, right to left in current algorithm */
    mft_layout ml;              /* mft layout structure */
    char volume_letter;         /* volume letter */
    WINX_FILE *f_volume;        /* volume handle */
    unsigned long flags;        /* combination of WINX_FTW_xxx flags */
    ftw_filter_callback fcb;    /**/
    ftw_progress_callback pcb;  /**/
    ftw_terminator t;           /* terminator callback */
    void *user_defined_data;    /* pointer passed to each callback */
    my_file_information mfi;    /* structure receiving file information */
    unsigned long processed_attr_list_entries; /* just for debugging purposes */
    unsigned long errors;       /* number of critical errors preventing gathering complete information */
    winx_file_info **filelist;  /* list of files */
} mft_scan_parameters;

/* structure used in binary search */
typedef struct {
    ULONGLONG mft_id;
    winx_file_info *f;
} file_entry;

typedef struct {
    ATTRIBUTE_TYPE AttributeType; /* The type of the attribute. */
    wchar_t *AttributeName;  /* The default name of the attribute. */
} attribute_name;

typedef void (*attribute_handler)(PATTRIBUTE pattr,mft_scan_parameters *sp);

/* forward declarations */
static void analyze_resident_stream(PRESIDENT_ATTRIBUTE pr_attr,mft_scan_parameters *sp);
static void analyze_non_resident_stream(PNONRESIDENT_ATTRIBUTE pnr_attr,mft_scan_parameters *sp);
static winx_file_info * find_filelist_entry(wchar_t *attr_name,mft_scan_parameters *sp);

void validate_blockmap(winx_file_info *f);

/*
**************************************************
*                Test suite
**************************************************
*/

#ifdef TEST_NTFS_SCANNER

/* a simple pseudo-random number generator */
long r = 1;

static void srnd(long x)
{
    r = x;
}

/* generates numbers in range 0 - 32767 */
static long rnd(void)
{
    return(((r = r * 214013 + 2531011) >> 16) & 0x7fff);
}

/* corrupts file record in random order */
static void randomize_file_record_data(char *record, unsigned long size)
{
    long factors[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };
    long factor, i;
    
    /* randomize a half of records */
    if(rnd() % 2) return;
    
    /* set amount of data to be corrupted */
    i = rnd() % (sizeof(factors) / sizeof(long));
    factor = factors[i];
    
    /* randomize data, skip FileReferenceNumber and FileRecordLength */
    for(i = sizeof(ULONGLONG) + sizeof(ULONG); i < size; i++){
        if(i % factor == 0)
            record[i] = (char)rnd();
    }
}

#endif

/*
**************************************************
*              Common routines
**************************************************
*/

static int ftw_ntfs_check_for_termination(mft_scan_parameters *sp)
{
    if(!(sp->flags & WINX_FTW_ALLOW_PARTIAL_SCAN) && sp->errors)
        return 1;
    
    if(sp->t == NULL)
        return 0;
    
    return sp->t(sp->user_defined_data);
}

/**
 * @note
 * - lsn, buffer, length must be valid before this call
 * - length must be an integral of the sector size
 */
static NTSTATUS read_sectors(ULONGLONG lsn,PVOID buffer,ULONG length,mft_scan_parameters *sp)
{
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER offset;
    NTSTATUS status;

    offset.QuadPart = lsn * sp->ml.sector_size;
    status = NtReadFile(winx_fileno(sp->f_volume),NULL,NULL,NULL,&iosb,buffer,length,&offset,NULL);
    if(NT_SUCCESS(status)){
        status = NtWaitForSingleObject(winx_fileno(sp->f_volume),FALSE,NULL);
        if(NT_SUCCESS(status)) status = iosb.Status;
    }
    if(status == STATUS_SUCCESS && iosb.Information){
        if(iosb.Information > length)
            etrace("more bytes read than needed?");
        else if(iosb.Information < length)
            etrace("less bytes read than needed?");
    }
    return status;
}

/**
 * @brief Retrieves a single file record from MFT.
 * @note sp->f_volume must contain a volume handle.
 * sp->ml.file_record_buffer_size must be properly set.
 */
static NTSTATUS get_file_record(ULONGLONG mft_id,
        NTFS_FILE_RECORD_OUTPUT_BUFFER *nfrob,
        mft_scan_parameters *sp)
{
    NTFS_FILE_RECORD_INPUT_BUFFER nfrib;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    nfrib.FileReferenceNumber = mft_id;

    /* required by x64 system, otherwise it trashes stack */
    RtlZeroMemory(nfrob,sp->ml.file_record_buffer_size);
    
    status = NtFsControlFile(winx_fileno(sp->f_volume),NULL,NULL,NULL,&iosb, \
            FSCTL_GET_NTFS_FILE_RECORD, \
            &nfrib,sizeof(nfrib), \
            nfrob, sp->ml.file_record_buffer_size);
    if(NT_SUCCESS(status)){
        (void)NtWaitForSingleObject(winx_fileno(sp->f_volume),FALSE,NULL);
        status = iosb.Status;
    }
    if(status == STATUS_SUCCESS && iosb.Information){
        if(iosb.Information > sp->ml.file_record_buffer_size)
            etrace("more bytes read than needed?");
        else if(iosb.Information < sp->ml.file_record_buffer_size)
            etrace("less bytes read than needed?");
    }
#ifdef TEST_NTFS_SCANNER
    randomize_file_record_data((char *)(void *)nfrob,sp->ml.file_record_buffer_size);
#endif
    return status;
}

/**
 * @brief Enumerates attributes contained in MFT record.
 * @param[in] frh pointer to the file record header.
 * @param[in] ah pointer to the callback procedure
 * to be called once for each attribute found.
 * @param[in,out] sp pointer to mft scan parameters structure.
 * @note sp->ml.file_record_size must be set before this call.
 */
static void enumerate_attributes(FILE_RECORD_HEADER *frh,attribute_handler ah,mft_scan_parameters *sp)
{
    PATTRIBUTE pattr;
    ULONG attr_length;
    USHORT attr_offset;

    attr_offset = frh->AttributeOffset;
    pattr = (PATTRIBUTE)((char *)frh + attr_offset);

    while(pattr && !ftw_ntfs_check_for_termination(sp)){
        /* is an attribute header inside a record bounds? */
        if(attr_offset + sizeof(ATTRIBUTE) > frh->BytesInUse || \
            attr_offset + sizeof(ATTRIBUTE) > sp->ml.file_record_size) break;
        
        /* is it a valid attribute */
        if(pattr->AttributeType == 0xffffffff) break;
        if(pattr->AttributeType == 0x0) break;
        if(pattr->Length == 0) break;

        /* is an attribute inside a record bounds? */
        if(attr_offset + pattr->Length > frh->BytesInUse || \
            attr_offset + pattr->Length > sp->ml.file_record_size) break;
        
        /* is an attribute length valid? */
        if(pattr->Nonresident){
            if(pattr->Length < (sizeof(NONRESIDENT_ATTRIBUTE) - sizeof(ULONGLONG))){
                etrace("nonresident attribute length is invalid");
                break;
            }
        } else {
            if(pattr->Length < sizeof(RESIDENT_ATTRIBUTE)){
                etrace("resident attribute length is invalid");
                break;
            }
        }

        /* call specified callback procedure */
        ah(pattr,sp);
        
        /* go to the next attribute */
        attr_length = pattr->Length;
        attr_offset += (USHORT)(attr_length);
        pattr = (PATTRIBUTE)((char *)pattr + attr_length);
    }
}

/*
* Attributes listed here represent data streams.
* Streams with default attribute names can be opened
* only with an additional colon sign, like that:
* file::$ATTRIBUTE_LIST
*/
attribute_name default_attribute_names[] = {
    {AttributeAttributeList,       L":$ATTRIBUTE_LIST"       },
    {AttributeEA,                  L":$EA"                   },
    {AttributeEAInformation,       L":$EA_INFORMATION"       },
    {AttributeSecurityDescriptor,  L":$SECURITY_DESCRIPTOR"  },
    {AttributeData,                L":$DATA"                 },
    {AttributeIndexRoot,           L":$INDEX_ROOT"           },
    {AttributeIndexAllocation,     L":$INDEX_ALLOCATION"     },
    {AttributeBitmap,              L":$BITMAP"               },
    {AttributeReparsePoint,        L":$REPARSE_POINT"        },
    {AttributeLoggedUtulityStream, L":$LOGGED_UTILITY_STREAM"}, /* used by EFS */
    {0,                            NULL                      }
};

static wchar_t * get_default_attribute_name(ATTRIBUTE_TYPE attr_type)
{
    int i;
    
    for(i = 0;; i++){
        if(default_attribute_names[i].AttributeName == NULL) break;
        if(default_attribute_names[i].AttributeType == attr_type) break;
    }
    return default_attribute_names[i].AttributeName;
}

static wchar_t * get_attribute_name(ATTRIBUTE *attr,mft_scan_parameters *sp)
{
    ATTRIBUTE_TYPE attr_type;
    wchar_t *default_attr_name = NULL;
    wchar_t *attr_name;

    /* get default name of the attribute */
    attr_type = attr->AttributeType;
    default_attr_name = get_default_attribute_name(attr_type);
    
    /* skip attributes of unknown type */
    if(default_attr_name == NULL){
        if(attr_type != AttributeStandardInformation && \
           attr_type != AttributeFileName && \
           attr_type != AttributeObjectId && \
           attr_type != AttributeVolumeName && \
           attr_type != AttributeVolumeInformation && \
           attr_type != AttributePropertySet){
            etrace("attribute of unknown type 0x%x found",(UINT)attr_type);
        }
        return NULL;
    }
    
    /* allocate memory */
    attr_name = winx_malloc((MAX_PATH + 1) * sizeof(wchar_t));
    
    attr_name[0] = 0;
    if(attr->NameLength){
        /* NameLength is always less than MAX_PATH! */
        (void)wcsncpy(attr_name,(wchar_t *)((char *)attr + attr->NameOffset),
            attr->NameLength);
        attr_name[attr->NameLength] = 0;
    }
    
    if(attr_name[0] == 0){
        (void)wcsncpy(attr_name,default_attr_name,MAX_PATH);
        attr_name[MAX_PATH - 1] = 0;
    }

    /* never append $DATA attribute name */
    if(wcscmp(attr_name,L"$DATA") == 0) attr_name[0] = 0;
    if(wcscmp(attr_name,L":$DATA") == 0) attr_name[0] = 0;
    
    /* do not append index allocation attribute names - required by get_directory_information */
    if(wcscmp(attr_name,L"$I30") == 0) attr_name[0] = 0;
    if(wcscmp(attr_name,L":$I30") == 0) attr_name[0] = 0;
    if(wcscmp(attr_name,L"$INDEX_ALLOCATION") == 0) attr_name[0] = 0;
    if(wcscmp(attr_name,L":$INDEX_ALLOCATION") == 0) attr_name[0] = 0;
    
    return attr_name;
}

/*
**************************************************
*           MFT layout retrieving code
**************************************************
*/

static void get_number_of_file_records_callback(PATTRIBUTE pattr,mft_scan_parameters *sp)
{
    PNONRESIDENT_ATTRIBUTE pnr_attr;

    if(pattr->Nonresident && pattr->AttributeType == AttributeData){
        pnr_attr = (PNONRESIDENT_ATTRIBUTE)pattr;
        if(sp->ml.file_record_size)
            sp->ml.number_of_file_records = pnr_attr->DataSize / sp->ml.file_record_size;
        itrace("mft contains %I64u records",sp->ml.number_of_file_records);
    }
}

/**
 * @brief Retrieves a number of file records containing in MFT.
 * @return Zero for success, negative value otherwise.
 * @note ml->file_record_buffer_size must be set before the call.
 * sp->f_volume must contain volume handle.
 */
static int get_number_of_file_records(mft_scan_parameters *sp)
{
    NTFS_FILE_RECORD_OUTPUT_BUFFER *nfrob;
    FILE_RECORD_HEADER *frh;
    NTSTATUS status;
    
    if(sp == NULL)
        return (-1);
    
    sp->ml.number_of_file_records = 0;
    
    /* allocate memory */
    nfrob = winx_tmalloc(sp->ml.file_record_buffer_size);
    if(nfrob == NULL){
        etrace("cannot allocate %u bytes of memory",
            sp->ml.file_record_buffer_size);
        return (-1);
    }
    
    /* get file record for $Mft */
    status = get_file_record(FILE_MFT,nfrob,sp);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot read $Mft file record");
        winx_free(nfrob);
        return (-1);
    }
    if(GetMftIdFromFRN(nfrob->FileReferenceNumber) != FILE_MFT){
        etrace("cannot get $Mft file record");
        winx_free(nfrob);
        return (-1);
    }
    
    /* validate file record */
    frh = (FILE_RECORD_HEADER *)nfrob->FileRecordBuffer;
    if(!is_file_record(frh)){
        etrace("$Mft file record has invalid type %u",
            frh->Ntfs.Type);
        winx_free(nfrob);
        return (-1);
    }
    if(!(frh->Flags & 0x1)){
        etrace("$Mft file record is marked as free");
        winx_free(nfrob);
        return (-1);
    }
    
    /* get actual number of mft entries */
    enumerate_attributes(frh,get_number_of_file_records_callback,sp);
    
    /* free memory */
    winx_free(nfrob);
    
    /* validate number of mft entries */
    if(sp->ml.number_of_file_records == 0){
        etrace("cannot get number of entries");
        return (-1);
    }
    
    return 0;
}

/**
 * @brief Retrieves MFT layout.
 * @return Zero for success, negative value otherwise.
 * @note sp->f_volume must contain volume handle.
 */
static int get_mft_layout(mft_scan_parameters *sp)
{
    NTFS_DATA *ntfs_data;
    int length;
    
    if(sp == NULL)
        return (-1);
    
    /* reset sp->ml structure */
    memset(&sp->ml,0,sizeof(mft_layout));

    /* allocate memory */
    ntfs_data = winx_malloc(sizeof(NTFS_DATA));

    /* get ntfs data */
    if(winx_ioctl(sp->f_volume,FSCTL_GET_NTFS_VOLUME_DATA,
      "get_mft_layout: ntfs data request",
      NULL,0,ntfs_data,sizeof(NTFS_DATA),&length) < 0){
        winx_free(ntfs_data);
        return (-1);
    }
    if(length){
        if(length > sizeof(NTFS_DATA))
            etrace("less bytes read than needed?");
        else if(length < sizeof(NTFS_DATA))
            etrace("more bytes read than needed?");
    }
    
    sp->ml.file_record_size = ntfs_data->BytesPerFileRecordSegment;
    sp->ml.file_record_buffer_size = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + \
        sp->ml.file_record_size - 1;
    sp->ml.total_clusters = ntfs_data->TotalClusters.QuadPart;
    sp->ml.cluster_size = ntfs_data->BytesPerCluster;
    sp->ml.sector_size = ntfs_data->BytesPerSector;
    if(sp->ml.sector_size){
        sp->ml.sectors_per_cluster = ntfs_data->BytesPerCluster / sp->ml.sector_size;
    } else {
        winx_free(ntfs_data);
        etrace("invalid sector size (zero)");
        return (-1);
    }
    itrace("mft record size = %u",sp->ml.file_record_size);
    itrace("volume has %I64u clusters",sp->ml.total_clusters);
    itrace("cluster size = %I64u",sp->ml.cluster_size);
    itrace("sector size = %u",sp->ml.sector_size);
    itrace("each cluster consists of %u sectors",sp->ml.sectors_per_cluster);
    winx_free(ntfs_data);
    
    if(sp->ml.file_record_size == 0){
        etrace("mft record size equal to zero is invalid");
        return (-1);
    }
    
    if(sp->ml.cluster_size == 0){
        etrace("cluster size equal to zero is invalid");
        return (-1);
    }
    
    if(sp->ml.sectors_per_cluster == 0){
        etrace("sp->ml.sectors_per_cluster equal to zero is invalid");
        return (-1);
    }
    
    /* get number of mft entries */
    if(get_number_of_file_records(sp) < 0)
        return (-1);
      
    return 0;
}

/*
**************************************************
*   Resident lists of attributes analysis code
**************************************************
*/

static void analyze_single_attribute(ULONGLONG mft_id,FILE_RECORD_HEADER *frh,
                ATTRIBUTE_TYPE attr_type,wchar_t *attr_name,USHORT attr_number,mft_scan_parameters *sp)
{
    ATTRIBUTE *attr;
    USHORT attr_offset;

    int name_length;
    ULONG attr_length;
    wchar_t *name = NULL;
    BOOLEAN attribute_found = FALSE;
    //char *resident_status = "";

    attr_offset = frh->AttributeOffset;
    attr = (ATTRIBUTE *)((char *)frh + attr_offset);

    while(attr && !ftw_ntfs_check_for_termination(sp)){
        /* is an attribute header inside a record bounds? */
        if(attr_offset + sizeof(ATTRIBUTE) > frh->BytesInUse || \
            attr_offset + sizeof(ATTRIBUTE) > sp->ml.file_record_size) break;
        
        /* is it a valid attribute */
        if(attr->AttributeType == 0xffffffff) break;
        if(attr->AttributeType == 0x0) break;
        if(attr->Length == 0) break;

        /* is an attribute inside a record bounds? */
        if(attr_offset + attr->Length > frh->BytesInUse || \
            attr_offset + attr->Length > sp->ml.file_record_size) break;
        
        /* is an attribute length valid? */
        if(attr->Nonresident){
            if(attr->Length < (sizeof(NONRESIDENT_ATTRIBUTE) - sizeof(ULONGLONG))){
                etrace("nonresident attribute length is invalid");
                break;
            }
        } else {
            if(attr->Length < sizeof(RESIDENT_ATTRIBUTE)){
                etrace("resident attribute length is invalid");
                break;
            }
        }

        /* do we have found the specified attribute? */
        if(attr->AttributeType == attr_type){
            if(attr->NameOffset && attr->NameLength){
                name = (wchar_t *)((char *)attr + attr->NameOffset);
                if(name[0] == 0) name = NULL;
            }
            if(attr_name == NULL){
                if(name == NULL && attr->AttributeNumber == attr_number)
                    attribute_found = TRUE;
            } else {
                if(name != NULL){
                    name_length = (int)wcslen(attr_name);
                    if(name_length == attr->NameLength){
                        if(memcmp((void *)attr_name,(void *)name,name_length * sizeof(wchar_t)) == 0){
                            if(attr->AttributeNumber == attr_number)
                                attribute_found = TRUE;
                        }
                    }
                }
            }
        }
        
        if(attribute_found){
            /* uncomment next lines if you need debugging information on attribute list entries */
            /*if(attr->Nonresident) resident_status = "Nonresident";
            else resident_status = "Resident";
            dtrace("AttrListEntry: Base MftId = %I64u, MftId = %I64u, Attribute Type = 0x%x, Attribute Number = %u, %s",
                sp->mfi.BaseMftId,mft_id,(UINT)attr_type,(UINT)attr_number,resident_status);
            */
            if(attr->Nonresident) analyze_non_resident_stream((PNONRESIDENT_ATTRIBUTE)attr,sp);
            else analyze_resident_stream((PRESIDENT_ATTRIBUTE)attr,sp);
            sp->processed_attr_list_entries ++;
            return;
        }
        
        /* go to the next attribute */
        attr_length = attr->Length;
        attr_offset += (USHORT)(attr_length);
        attr = (ATTRIBUTE *)((char *)attr + attr_length);
    }
}

static void analyze_attribute_from_mft_record(ULONGLONG mft_id,ATTRIBUTE_TYPE attr_type,
                wchar_t *attr_name,USHORT attr_number,mft_scan_parameters *sp)
{
    NTFS_FILE_RECORD_OUTPUT_BUFFER *nfrob = NULL;
    FILE_RECORD_HEADER *frh;
    NTSTATUS status;
    
    /*
    * Skip attributes stored in the base mft record,
    * because they will be scanned anyway.
    */
    if(mft_id == sp->mfi.BaseMftId){
        //trace(D"Attribute list entry points to 0x%x attribute of the base record",(UINT)attr_type);
        return;
    }
    
    /* allocate memory for a single mft record */
    nfrob = winx_tmalloc(sp->ml.file_record_buffer_size);
    if(nfrob == NULL){
        etrace("cannot allocate %u bytes of memory",
            sp->ml.file_record_buffer_size);
        sp->errors ++;
        return;
    }
    
    /* get specified mft record */
    status = get_file_record(mft_id,nfrob,sp);
    if(!NT_SUCCESS(status)){
        strace(status,"cannot read %I64u file record",mft_id);
        winx_free(nfrob);
        /* file record index seems to be invalid itself */
        /*sp->errors ++;*/
        return;
    }
    if(GetMftIdFromFRN(nfrob->FileReferenceNumber) != mft_id){
        etrace("cannot get %I64u file record",mft_id);
        winx_free(nfrob);
        /*sp->errors ++;*/
        return;
    }
    
    /* validate file record */
    frh = (FILE_RECORD_HEADER *)nfrob->FileRecordBuffer;
    if(!is_file_record(frh)){
        etrace("%I64u file record has invalid type %u",
            mft_id,frh->Ntfs.Type);
        winx_free(nfrob);
        return;
    }
    if(!(frh->Flags & 0x1)){
        etrace("%I64u file record is marked as free",mft_id);
        winx_free(nfrob);
        return;
    }

    if(frh->BaseFileRecord == 0){
        etrace("%I64u is not a child record",mft_id);
        winx_free(nfrob);
        return;
    }

    /* search for a specified attribute */
    analyze_single_attribute(mft_id,frh,attr_type,attr_name,attr_number,sp);

    /* free allocated memory */
    winx_free(nfrob);
}

static void analyze_attribute_from_attribute_list(ATTRIBUTE_LIST *attr_list_entry,mft_scan_parameters *sp)
{
    ULONGLONG child_record_mft_id;
    ATTRIBUTE_TYPE attr_type;
    USHORT attr_number;
    wchar_t *attr_name = NULL;
    wchar_t *name_src;
    int length;
    int empty_name = 0;

    /*
    * 21 Feb 2010
    * The right implementation must analyze a single
    * specific attribute from the MFT record pointed
    * by the attribute list entry.
    */

    /* 1. save the name of the attribute */
    length = attr_list_entry->NameLength;
    name_src = (wchar_t *)((char *)attr_list_entry + attr_list_entry->NameOffset);
    
    if(length == 0)
        empty_name = 1; /* name has zero length */
    else if(attr_list_entry->NameOffset == 0)
        empty_name = 1; /* name location is invalid */
    else if(name_src[0] == 0)
        empty_name = 1; /* name is empty */
    
    if(!empty_name){
        attr_name = winx_malloc((length + 1) * sizeof(wchar_t));
        wcsncpy(attr_name,name_src,length);
        attr_name[length] = 0;
    }

    /* attr_name is NULL here for empty names */
    
    /* 2. save the attribute type */
    attr_type = attr_list_entry->AttributeType;
    
    /* 3. save the identifier of the child record containing the attribute */
    child_record_mft_id = GetMftIdFromFRN(attr_list_entry->FileReferenceNumber);
    
    /* 4. save the AttributeNumber */
    attr_number = attr_list_entry->AttributeNumber;
    
    /* 5. analyze a single attribute */
    analyze_attribute_from_mft_record(child_record_mft_id,attr_type,attr_name,attr_number,sp);
    
    /* 6. free resources */
    winx_free(attr_name);
}

static void analyze_resident_attribute_list(PRESIDENT_ATTRIBUTE pr_attr,mft_scan_parameters *sp)
{
    ATTRIBUTE_LIST *entry;
    USHORT length;
    
    entry = (ATTRIBUTE_LIST *)((char *)pr_attr + pr_attr->ValueOffset);

    while(!ftw_ntfs_check_for_termination(sp)){
        if( ((char *)entry + sizeof(ATTRIBUTE_LIST) - sizeof(entry->AlignmentOrReserved)) > 
            ((char *)pr_attr + pr_attr->ValueOffset + pr_attr->ValueLength) ) break;
        /* is it a valid attribute */
        if(entry->AttributeType == 0xffffffff) break;
        if(entry->AttributeType == 0x0) break;
        if(entry->Length == 0) break;
        //trace(D"@@@@@@@@@ attr_list_entry Length = %u", attr_list_entry->Length);
        analyze_attribute_from_attribute_list(entry,sp);
        /* go to the next attribute list entry */
        length = entry->Length;
        entry = (PATTRIBUTE_LIST)((char *)entry + length);
    }
}

/*
**************************************************
*       Resident file streams analysis code
**************************************************
*/

static void get_file_flags(PRESIDENT_ATTRIBUTE pr_attr,mft_scan_parameters *sp)
{
    STANDARD_INFORMATION *si;
    
    si = (STANDARD_INFORMATION *)((char *)pr_attr + pr_attr->ValueOffset);
    if(pr_attr->ValueLength < 48) /* 48 = size of the shortest STANDARD_INFORMATION structure */
        etrace("STANDARD_INFORMATION attribute is too short");
    else
        sp->mfi.Flags |= si->FileAttributes;
}

static void get_file_access_times(PRESIDENT_ATTRIBUTE pr_attr,mft_scan_parameters *sp)
{
    STANDARD_INFORMATION *si;
    
    si = (STANDARD_INFORMATION *)((char *)pr_attr + pr_attr->ValueOffset);
    if(pr_attr->ValueLength < 48){ /* 48 = size of the shortest STANDARD_INFORMATION structure */
        etrace("STANDARD_INFORMATION attribute is too short");
    } else {
        sp->mfi.CreationTime = si->CreationTime;
        sp->mfi.LastWriteTime = si->LastWriteTime;
        sp->mfi.LastAccessTime = si->LastAccessTime;
    }
}

static void update_file_name(PRESIDENT_ATTRIBUTE pr_attr,mft_scan_parameters *sp)
{
    FILENAME_ATTRIBUTE *fn;
    ULONGLONG parent_mft_id;
    int update_name = 0;
    int length;
    
    fn = (FILENAME_ATTRIBUTE *)((char *)pr_attr + pr_attr->ValueOffset);
    if(pr_attr->ValueLength < sizeof(FILENAME_ATTRIBUTE)){
        etrace("FILENAME_ATTRIBUTE is too short");
        return;
    }
    
    if(fn->NameLength == 0){
        etrace("empty name found (1), mft index = %I64u",
            sp->mfi.BaseMftId);
        return;
    }
    
    if(fn->Name[0] == 0){
        etrace("empty name found (2), mft index = %I64u",
            sp->mfi.BaseMftId);
        return;
    }
    
    parent_mft_id = GetMftIdFromFRN(fn->DirectoryFileReferenceNumber);
    if(parent_mft_id == sp->mfi.BaseMftId && sp->mfi.BaseMftId != FILE_root){
        etrace("recursion found - file identifies themselves "
           "as a parent, mft index = %I64u",sp->mfi.BaseMftId);
        return;
    }
    
    /* update sp->mfi */
    sp->mfi.ParentDirectoryMftId = parent_mft_id;

    /* check whether the name should be updated or not */
    if(sp->mfi.Name[0] == 0)
        update_name = 1; /* because no name has been saved yet */
    else if(sp->mfi.NameType == FILENAME_DOS)
        update_name = 1; /* always update DOS names */
    else if((sp->mfi.NameType == FILENAME_WIN32) && (fn->NameType == FILENAME_POSIX))
        update_name = 1; /* always update Win32 names to POSIX names */
    
    if(update_name){
        sp->mfi.NameType = fn->NameType;
        length = fn->NameLength;
        /* length is always less than MAX_PATH, because fn->NameLength has char type */
        if(length <= MAX_PATH - 1){
            wcsncpy(sp->mfi.Name,fn->Name,length);
            sp->mfi.Name[length] = 0;
        }
    }
}

static void handle_reparse_point(PRESIDENT_ATTRIBUTE pr_attr,mft_scan_parameters *sp)
{
    REPARSE_POINT *rp;

    rp = (REPARSE_POINT *)((char *)pr_attr + pr_attr->ValueOffset);
    if(pr_attr->ValueLength >= sizeof(ULONG))
        itrace("reparse tag = 0x%x",rp->ReparseTag);
    else
        etrace("REPARSE_POINT attribute is too short");
    
    sp->mfi.Flags |= FILE_ATTRIBUTE_REPARSE_POINT;
}

static void get_volume_information(PRESIDENT_ATTRIBUTE pr_attr,mft_scan_parameters *sp)
{
    VOLUME_INFORMATION *vi;
    ULONG mj_ver, mn_ver;
    
    vi = (VOLUME_INFORMATION *)((char *)pr_attr + pr_attr->ValueOffset);
    if(pr_attr->ValueLength < sizeof(VOLUME_INFORMATION)){
        etrace("VOLUME_INFORMATION attribute is too short");
        return;
    }
    
    mj_ver = (ULONG)vi->MajorVersion;
    mn_ver = (ULONG)vi->MinorVersion;
    itrace("NTFS Version %u.%u",mj_ver,mn_ver);
    if(vi->Flags & 0x1)
        etrace("volume is dirty");
}

static void analyze_resident_stream(PRESIDENT_ATTRIBUTE pr_attr,mft_scan_parameters *sp)
{
    wchar_t *attr_name;
    
    /* add resident streams to sp->filelist */
    attr_name = get_attribute_name(&pr_attr->Attribute,sp);
    if(attr_name){
        (void)find_filelist_entry(attr_name,sp);
        winx_free(attr_name);
    }
    
    if(pr_attr->ValueOffset == 0 || pr_attr->ValueLength == 0){
        /*
        * This is an ordinary case when some attribute data was truncated.
        * For example, when some large file becomes empty its $DATA attribute
        * becomes resident and its ValueLength becomes to be equal to zero.
        */
        return;
    }
    
    switch(pr_attr->Attribute.AttributeType){
    case AttributeStandardInformation: /* always resident */
        get_file_flags(pr_attr,sp);
        get_file_access_times(pr_attr,sp);
        break;
    case AttributeFileName: /* always resident */
        update_file_name(pr_attr,sp);
        break;
    case AttributeVolumeInformation: /* always resident */
        get_volume_information(pr_attr,sp);
        break;
    case AttributeAttributeList:
        //trace(D"Resident AttributeList found!");
        analyze_resident_attribute_list(pr_attr,sp);
        break;
    /*case AttributeIndexRoot:  // always resident */
    /*case AttributeIndexAllocation:*/
    case AttributeReparsePoint:
        handle_reparse_point(pr_attr,sp);
        break;
    default:
        break;
    }
}

/*
**************************************************
*  Nonresident lists of attributes analysis code
**************************************************
*/

static void analyze_non_resident_attribute_list(winx_file_info *f,ULONGLONG list_size,mft_scan_parameters *sp)
{
    ULONGLONG cluster_size;
    ULONGLONG clusters_to_read;
    char *cluster;
    char *current_cluster;
    winx_blockmap *block;
    ULONGLONG lsn;
    NTSTATUS status;
    PATTRIBUTE_LIST attr_list_entry;
    int i;
    USHORT length;

#ifdef SHOW_ATTR_LISTS_INFO
    dtrace("allocated size = %I64u bytes",list_size);
#endif
    if(list_size == 0){
        etrace("empty nonresident attribute list found");
        return;
    }
    
    /* allocate memory for an integral number of cluster to hold a whole AttributeList */
    cluster_size = sp->ml.cluster_size;
    clusters_to_read = list_size / cluster_size;
    if(list_size % cluster_size) clusters_to_read ++;
    cluster = (char *)winx_tmalloc((SIZE_T)(cluster_size * clusters_to_read));
    if(!cluster){
        etrace("cannot allocate %I64u bytes of memory",
            cluster_size * clusters_to_read);
        sp->errors ++;
        return;
    }
    
    /* loop through all blocks of file */
    current_cluster = cluster;
    for(block = f->disp.blockmap; block != NULL; block = block->next){
        /* loop through clusters of the current block */
        for(i = 0; i < block->length; i++){
            /* read current cluster */
            lsn = (block->lcn + i) * sp->ml.sectors_per_cluster;
            status = read_sectors(lsn,current_cluster,(ULONG)cluster_size,sp);
            if(!NT_SUCCESS(status)){
                strace(status,"cannot read %I64u sector",lsn);
                /* attribute list seems to be invalid itself, so we'll just skip it */
                /*sp->errors ++;*/
                goto scan_done;
            }
            clusters_to_read --;
            if(clusters_to_read == 0){
                /* is it the last cluster of the file? */
                if(i < (block->length - 1) || block->next != f->disp.blockmap)
                    etrace("attribute list has more clusters than expected");
                goto analyze_list;
            }
            current_cluster += cluster_size;
        }
        if(block->next == f->disp.blockmap) break;
    }

analyze_list:
    if(clusters_to_read){
        etrace("attribute list has less number of clusters than expected");
        etrace("it will be skipped, because anyway we don\'t know its exact size");
        goto scan_done;
    }

#ifdef SHOW_ATTR_LISTS_INFO
    dtrace("attribute list analysis started...");
#endif
    attr_list_entry = (PATTRIBUTE_LIST)cluster;

    while(!ftw_ntfs_check_for_termination(sp)){
        if( ((char *)attr_list_entry + sizeof(ATTRIBUTE_LIST) - sizeof(attr_list_entry->AlignmentOrReserved)) > 
            ((char *)cluster + list_size) ) break;
        /* is it a valid attribute */
        if(attr_list_entry->AttributeType == 0xffffffff) break;
        if(attr_list_entry->AttributeType == 0x0) break;
        if(attr_list_entry->Length == 0) break;
        //trace(D"@@@@@@@@@ attr_list_entry Length = %u", attr_list_entry->Length);
        analyze_attribute_from_attribute_list(attr_list_entry,sp);
        /* go to the next attribute list entry */
        length = attr_list_entry->Length;
        attr_list_entry = (PATTRIBUTE_LIST)((char *)attr_list_entry + length);
    }
#ifdef SHOW_ATTR_LISTS_INFO
    dtrace("attribute list analysis completed");
#endif

scan_done:    
    /* free allocated resources */
    winx_free(cluster);
}

/*
**************************************************
*     Nonresident file streams analysis code
**************************************************
*/

static winx_file_info * find_filelist_entry(wchar_t *attr_name,mft_scan_parameters *sp)
{
    winx_file_info *f;
    
    /* few streams may have the same mft id */
    for(f = *sp->filelist; f != NULL; f = f->next){
        /*
        * we scan mft from the end to the beginning (RTL)
        * and we add new records to the left side of the file list...
        */
        if(sp->mft_scan_direction == MFT_SCAN_RTL){
            if(f->internal.BaseMftId > sp->mfi.BaseMftId) break; /* we have no chance to find record in list */
        } else {
            if(f->internal.BaseMftId < sp->mfi.BaseMftId) break;
        }
        if(!wcscmp(f->name,attr_name) && f->internal.BaseMftId == sp->mfi.BaseMftId)
            return f; /* slow? */
        //if(f->internal.BaseMftId == sp->mfi.BaseMftId) return f; /* safe? */
        if(f->next == *sp->filelist) break;
    }
    
    f = (winx_file_info *)winx_list_insert((list_entry **)(void *)sp->filelist,NULL,sizeof(winx_file_info));

    /* initialize structure */
    f->name = winx_wcsdup(attr_name);
    if(f->name == NULL){
        etrace("cannot allocate %u bytes of memory",
            (wcslen(attr_name) + 1) * sizeof(wchar_t));
        winx_list_remove((list_entry **)(void *)sp->filelist,(list_entry *)f);
        sp->errors ++;
        return NULL;
    }
    
    f->path = NULL;
    f->flags = 0;
    f->user_defined_flags = 0;
    memset(&f->disp,0,sizeof(winx_file_disposition));
    f->internal.BaseMftId = sp->mfi.BaseMftId;
    f->internal.ParentDirectoryMftId = FILE_root;
    f->creation_time = 0;
    f->last_modification_time = 0;
    f->last_access_time = 0;
    return f;
}

static void process_run(winx_file_info *f,ULONGLONG vcn,ULONGLONG lcn,ULONGLONG length,mft_scan_parameters *sp)
{
    winx_blockmap *block, *prev_block = NULL;
    
    /* add information to f->disp */
    if(f->disp.blockmap) prev_block = f->disp.blockmap->prev;
    block = (winx_blockmap *)winx_list_insert((list_entry **)&f->disp.blockmap,
        (list_entry *)prev_block,sizeof(winx_blockmap));
    
    block->vcn = vcn;
    block->lcn = lcn;
    block->length = length;

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

static int check_run(ULONGLONG lcn,ULONGLONG length,mft_scan_parameters *sp)
{
    if((lcn < sp->ml.total_clusters) && (lcn + length <= sp->ml.total_clusters))
        return 1; /* run is inside volume bounds */
    
    return 0;
}

/**
 */
static ULONG RunLength(PUCHAR run)
{
    return (*run & 0xf) + ((*run >> 4) & 0xf) + 1;
}

/**
 */
static LONGLONG RunLCN(PUCHAR run)
{
    LONG i;
    UCHAR n1 = *run & 0xf;
    UCHAR n2 = (*run >> 4) & 0xf;
    LONGLONG lcn = (n2 == 0) ? 0 : (LONGLONG)(((signed char *)run)[n1 + n2]);
    
    for(i = n1 + n2 - 1; i > n1; i--)
        lcn = (lcn << 8) + run[i];
    return lcn;
}

/**
 */
static ULONGLONG RunCount(PUCHAR run)
{
    ULONG i;
    UCHAR n = *run & 0xf;
    ULONGLONG count = 0;
    
    for(i = n; i > 0; i--)
        count = (count << 8) + run[i];
    return count;
}

static void process_run_list(wchar_t *attr_name,PNONRESIDENT_ATTRIBUTE pnr_attr,
                mft_scan_parameters *sp,BOOLEAN is_attr_list)
{
    ULONGLONG lcn, vcn, length;
    PUCHAR run;
    winx_file_info *f;
    
/*  if(pnr_attr->Attribute.Flags & 0x1)
        DbgPrint("[CMP] %ws VCN %I64u - %I64u",attr_name,pnr_attr->LowVcn,pnr_attr->HighVcn);
    else
        DbgPrint("[ORD] %ws VCN %I64u - %I64u",attr_name,pnr_attr->LowVcn,pnr_attr->HighVcn);
*/    

    /* add file stream to the file list */
    f = find_filelist_entry(attr_name,sp);
    if(f == NULL)
        return;
    
    if(pnr_attr->Attribute.Flags & 0x1)
        f->flags |= FILE_ATTRIBUTE_COMPRESSED;
    
    /* don't analyze $BadClus file - it often has wrong number of clusters */
    if(winx_wcsistr(sp->mfi.Name,L"$BadClus")){
        /* this file always exists, regardless of file system state */
        itrace("$BadClus file detected");
        return;
    }
    
    if(is_attr_list || (sp->flags & WINX_FTW_DUMP_FILES)){
        /* loop through runs */
        lcn = 0; vcn = pnr_attr->LowVcn;
        run = (PUCHAR)((char *)pnr_attr + pnr_attr->RunArrayOffset);
        while(*run){
            lcn += RunLCN(run);
            length = RunCount(run);
            
            /* skip virtual runs */
            if(RunLCN(run)){
                /* check for data consistency */
                if(!check_run(lcn,length,sp)){
                    etrace("error in MFT found, run Check Disk program!");
                    break;
                }
                process_run(f,vcn,lcn,length,sp);
            }
            
            /* go to the next run */
            run += RunLength(run);
            vcn += length;
        }
    }
    
    /* analyze nonresident attribute lists */
    if(is_attr_list) analyze_non_resident_attribute_list(f,pnr_attr->InitializedSize,sp);
}

/**
 * @note Name of the file may be not available for this routine.
 */
static void analyze_non_resident_stream(PNONRESIDENT_ATTRIBUTE pnr_attr,mft_scan_parameters *sp)
{
    ATTRIBUTE_TYPE attr_type;
    wchar_t *attr_name;
    BOOLEAN NonResidentAttrListFound = FALSE;
    
    /* handle the type of the attribute */
    attr_type = pnr_attr->Attribute.AttributeType;
    if(attr_type == AttributeAttributeList){
#ifdef SHOW_ATTR_LISTS_INFO
        dtrace("nonresident attribute list found");
#endif
        NonResidentAttrListFound = TRUE;
    }

    if(attr_type == AttributeReparsePoint)
        sp->mfi.Flags |= FILE_ATTRIBUTE_REPARSE_POINT;
    
    attr_name = get_attribute_name(&pnr_attr->Attribute,sp);
    if(attr_name == NULL)
        return;
    
#ifdef SHOW_ATTR_LISTS_INFO
    if(NonResidentAttrListFound)
        dtrace("%ws:%ws",sp->mfi.Name,attr_name);
#endif

    process_run_list(attr_name,pnr_attr,sp,NonResidentAttrListFound);

    /* free allocated memory */
    winx_free(attr_name);
}

/*
**************************************************
*         Single file analysis code
**************************************************
*/

static int update_stream_name(winx_file_info *f,mft_scan_parameters *sp)
{
    wchar_t *new_name;
    int length;
    
    length = (int)wcslen(f->name) + (int)wcslen(sp->mfi.Name) + 1;
    new_name = winx_malloc((length + 1) * sizeof(wchar_t));
    
    if(f->name[0]) /* stream name is not empty */
        _snwprintf(new_name,length + 1,L"%ws:%ws",sp->mfi.Name,f->name);
    else
        wcsncpy(new_name,sp->mfi.Name,length);
    new_name[length] = 0;
    
    winx_free(f->name);
    f->name = new_name;
    return 0;
}

/**
 * @brief Analyzes a single file attribute (stream).
 */
static void analyze_attribute(PATTRIBUTE pattr,mft_scan_parameters *sp)
{
    if(pattr->Nonresident) analyze_non_resident_stream((PNONRESIDENT_ATTRIBUTE)pattr,sp);
    else analyze_resident_stream((PRESIDENT_ATTRIBUTE)pattr,sp);
}

static void analyze_attribute_callback(PATTRIBUTE pattr,mft_scan_parameters *sp)
{
    if(pattr->AttributeType != AttributeAttributeList)
        analyze_attribute(pattr,sp);
}

static void analyze_attribute_list_callback(PATTRIBUTE pattr,mft_scan_parameters *sp)
{
    if(pattr->AttributeType == AttributeAttributeList)
        analyze_attribute(pattr,sp);
}

/**
 * @brief Analyzes base file record.
 * @details Forces all child records to be analyzed.
 */
static void analyze_file_record(NTFS_FILE_RECORD_OUTPUT_BUFFER *nfrob,
                                mft_scan_parameters *sp)
{
    FILE_RECORD_HEADER *frh;
    winx_file_info *f, *next, *head;
    
    /* validate header */
    frh = (FILE_RECORD_HEADER *)nfrob->FileRecordBuffer;
    if(!is_file_record(frh))
        return;
    if(!(frh->Flags & 0x1))
        return; /* skip free records */
    
    /*if(frh->Flags & 0x2)
        dtrace("directory"); // may be wrong?
    */
    
    /* skip child records, we'll scan them later */
    if(frh->BaseFileRecord)
        return;
    
    /*
    * Here we have found a new file, so let's
    * walk throuh all file records belonging to it.
    * First of all, we're setting up sp->mfi
    * structure. Than, we're walking through list
    * of attributes containing in the current file
    * record. During the walk we're updating sp->mfi
    * fields by actual file name and flags.
    * If some related information contains in other
    * (child) file records, we're analyzing them too.
    * All information about file streams found will be
    * added to the list pointed by sp->filelist.
    */
    
    /* initialize mfi structure */
    sp->mfi.BaseMftId = GetMftIdFromFRN(nfrob->FileReferenceNumber);
    sp->mfi.ParentDirectoryMftId = FILE_root;
    sp->mfi.Flags = 0x0;
    if(frh->Flags & 0x2)
        sp->mfi.Flags |= FILE_ATTRIBUTE_DIRECTORY;
    sp->mfi.NameType = 0x0; /* Assume FILENAME_POSIX */
    memset(sp->mfi.Name,0,MAX_PATH);
    sp->mfi.CreationTime = 0;
    sp->mfi.LastWriteTime = 0;
    sp->mfi.LastAccessTime = 0;
    
    /* skip attribute lists */
    enumerate_attributes(frh,analyze_attribute_callback,sp);
    
    /* analyze attribute lists */
    enumerate_attributes(frh,analyze_attribute_list_callback,sp);
    
    //trace(D"%ws",sp->mfi.Name);
    
    /*
    * Here sp->mfi structure contains
    * name of the file and file flags.
    * On the other hand, sp->filelist
    * contains information about all
    * data streams found: the name of
    * the stream, map of its blocks.
    * We are appending here the name
    * of the file to name of streams
    * and updating also flags saved
    * in filelist entries.
    */
    head = *sp->filelist;
    for(f = head; f != NULL;){
        if(sp->mft_scan_direction == MFT_SCAN_RTL){
            if(f->internal.BaseMftId > sp->mfi.BaseMftId) break; /* we have no chance to find record in list */
        } else {
            if(f->internal.BaseMftId < sp->mfi.BaseMftId) break;
        }
        next = f->next;
        if(f->internal.BaseMftId == sp->mfi.BaseMftId){
            /* update flags, because sp->mfi contains more actual data  */
            f->flags = sp->mfi.Flags;
            /* update access times */
            f->creation_time = sp->mfi.CreationTime;
            f->last_modification_time = sp->mfi.LastWriteTime;
            f->last_access_time = sp->mfi.LastAccessTime;
            /* set parent directory id for the stream */
            f->internal.ParentDirectoryMftId = sp->mfi.ParentDirectoryMftId;
            /* add filename to the name of the stream */
            if(update_stream_name(f,sp) < 0){
                winx_list_remove((list_entry **)(void *)sp->filelist,(list_entry *)f);
                if(*sp->filelist == NULL) break;
                if(*sp->filelist != head){
                    head = *sp->filelist;
                    f = next;
                    continue;
                }
            } else {
                /* call progress callback */
                if(sp->pcb)
                    sp->pcb(f,sp->user_defined_data);
            }
        }
        f = next;
        if(f == head) break;
    }
}

/*
**************************************************
*          Full paths building code
**************************************************
*/

static winx_file_info * find_directory_by_mft_id(ULONGLONG mft_id,
    file_entry *f_array,unsigned long n_entries,mft_scan_parameters *sp)
{
    winx_file_info *f;
    ULONG lim, i, k;
    signed long m;
    BOOLEAN ascending_order;

    if(f_array == NULL){ /* use slow linear search */
        for(f = *sp->filelist; f != NULL; f = f->next){
            if(f->internal.BaseMftId == mft_id){
                if(wcsstr(f->name,L":$") == NULL)
                    return f;
            }
            if(f->next == *sp->filelist) break;
        }
        return NULL;
    } else { /* use fast binary search */
        /* Based on bsearch() algorithm copyrighted by DJ Delorie (1994). */
        ascending_order = (sp->mft_scan_direction == MFT_SCAN_RTL) ? TRUE : FALSE;
        i = 0;
        for(lim = n_entries; lim != 0; lim >>= 1){
            k = i + (lim >> 1);
            if(f_array[k].mft_id == mft_id){
                /* search for proper entry in neighbourhood of found entry */
                for(m = k; m >= 0; m --){
                    if(f_array[m].mft_id != mft_id) break;
                }
                for(m = m + 1; (unsigned long)(m) < n_entries; m ++){
                    if(f_array[m].mft_id != mft_id) break;
                    if(wcsstr(f_array[m].f->name,L":$") == NULL)
                        return f_array[m].f;
                }
                etrace("Exit 1");
                return NULL;
            }
            if(ascending_order){
                if(mft_id > f_array[k].mft_id){
                    i = k + 1; lim --; /* move right */
                } /* else move left */
            } else {
                if(mft_id < f_array[k].mft_id){
                    i = k + 1; lim --; /* move right */
                } /* else move left */
            }
        }
        etrace("Exit 2");
        return NULL;
    }
}

/**
 * @param[in] mft_id mft index of directory.
 * @param[out] path pointer to variable receiving directory path.
 * @param[out] parent_mft_id mft index of parent directory.
 * @return Nonzero value indicates that returned path contains full
 * native path, otherwise it contains directory name only.
 */
static int get_directory_information(ULONGLONG mft_id,wchar_t **path,ULONGLONG *parent_mft_id,
    file_entry *f_array,unsigned long n_entries,mft_scan_parameters *sp)
{
    winx_file_info *f;
    
    *path = NULL;
    *parent_mft_id = FILE_root;
    
    f = find_directory_by_mft_id(mft_id,f_array,n_entries,sp);
    if(f == NULL){
        etrace("%I64u directory not found",mft_id);
        sp->errors ++;
        return 0;
    }
    
    *parent_mft_id = f->internal.ParentDirectoryMftId;
    
    if(f->path){
        *path = f->path;
        return 1;
    }
    
    if(f->name){
        *path = f->name;
    }
    return 0;
}

/* ancillary structure used by build_file_path routine */
typedef struct _path_parts {
    wchar_t child[MAX_PATH];   /* already gathered part of the path */
    wchar_t buffer[MAX_PATH];  /* ancillary buffer */
} path_parts;

static void build_file_path(winx_file_info *f,file_entry *f_array,
    unsigned long n_entries,path_parts *p,mft_scan_parameters *sp)
{
    ULONGLONG mft_id,parent_mft_id;
    int full_path_retrieved = 0;
    wchar_t *parent_path,*src;
    
    /* initialize p->child by filename */
    wcsncpy(p->child,f->name,MAX_PATH - 1);
    p->child[MAX_PATH - 1] = 0;
    
    /* loop through parent directories */
    parent_mft_id = f->internal.ParentDirectoryMftId;
    while(parent_mft_id != FILE_root && !full_path_retrieved){
        if(ftw_ntfs_check_for_termination(sp)) return;
        mft_id = parent_mft_id;
        full_path_retrieved = get_directory_information(mft_id,
            &parent_path,&parent_mft_id,f_array,n_entries,sp);
        if(parent_path)
            _snwprintf(p->buffer,MAX_PATH,L"%ws\\%ws",parent_path,p->child);
        else
            _snwprintf(p->buffer,MAX_PATH,L"\\%ws",p->child);
        p->buffer[MAX_PATH - 1] = 0;
        wcscpy(p->child,p->buffer);
    }
    
    /* append root directory path, if not appended yet */
    if(p->child[1] == '?'){
        src = p->child;
    } else {
        _snwprintf(p->buffer,MAX_PATH,L"\\??\\%c:\\%ws",sp->volume_letter,p->child);
        p->buffer[MAX_PATH - 1] = 0;
        src = p->buffer;
    }
    
    /* update f->path */
    f->path = winx_wcsdup(src);
    if(f->path == NULL){
        etrace("cannot allocate %u bytes of memory",
            (wcslen(src) + 1) * sizeof(wchar_t));
        sp->errors ++;
        return;
    }

    //trace(D"%ws",f->path);
}

/**
 * @todo This procedure eats lots of CPU tacts,
 * optimize it for speed or run with thread priority
 * below normal.
 */
static int build_full_paths(mft_scan_parameters *sp)
{
    path_parts *p;
    file_entry *f_array = NULL;
    unsigned long n_entries = 0;
    winx_file_info *f;
    ULONG i;
    ULONGLONG time;
    
    itrace("build_full_paths started...");
    time = winx_xtime();
    
    /* allocate memory */
    p = winx_malloc(sizeof(path_parts));

    /* prepare data for fast binary search */
    for(f = *sp->filelist; f != NULL; f = f->next){
        n_entries++;
        if(f->next == *sp->filelist) break;
    }

    if(n_entries){
        f_array = winx_tmalloc(n_entries * sizeof(file_entry));
        if(f_array == NULL){
            etrace("cannot allocate %u bytes of memory",
                n_entries * sizeof(file_entry));
        }
    }

    if(f_array){
        /* fill array */
        i = 0;
        for(f = *sp->filelist; f != NULL; f = f->next){
            f_array[i].mft_id = f->internal.BaseMftId;
            f_array[i].f = f;
            if(i == (n_entries - 1)){ 
                if(f->next != *sp->filelist)
                    etrace("???");
                break;
            }
            i++;
            if(f->next == *sp->filelist) break;
        }
        itrace("fast binary search will be used");
    } else {
        itrace("slow linear search will be used");
    }
    
    for(f = *sp->filelist; f != NULL; f = f->next){
        if(ftw_ntfs_check_for_termination(sp)) break;
        build_file_path(f,f_array,n_entries,p,sp);
        if(f->next == *sp->filelist) break;
    }
    
    /* free allocated resources */
    winx_free(f_array);
    winx_free(p);
    itrace("build_full_paths completed in %I64u ms",winx_xtime() - time);
    return 0;
}

/*
**************************************************
*       NTFS scan entry point and helpers
**************************************************
*/

/**
 * @brief Scans entire MFT and adds
 * all files found to the file list.
 * @return Zero for success, -1 indicates
 * failure, -2 indicates termination requested by caller.
 * @note sp->f_volume must contain volume handle.
 */
static int scan_mft(mft_scan_parameters *sp)
{
    NTFS_FILE_RECORD_OUTPUT_BUFFER *nfrob;
    ULONGLONG start_time;
    ULONGLONG mft_id, ret_mft_id;
    NTSTATUS status;
    int result;
    
    itrace("mft scan started");
    start_time = winx_xtime();
    
#ifdef TEST_NTFS_SCANNER
    dtrace("NTFS SCANNER TEST STARTED");
    srnd(1);
#endif
    
    /* get mft layout */
    if(get_mft_layout(sp) < 0){
fail:
        etrace("mft scan failed");
        return (-1);
    }

    /* allocate memory */
    nfrob = winx_tmalloc(sp->ml.file_record_buffer_size);
    if(nfrob == NULL){
        etrace("cannot allocate %u bytes of memory",
            sp->ml.file_record_buffer_size);
        return (-1);
    }
    
    /* scan all file records sequentially */
    mft_id = sp->ml.number_of_file_records - 1;
    sp->mft_scan_direction = MFT_SCAN_RTL;
    while(!ftw_ntfs_check_for_termination(sp)){
        status = get_file_record(mft_id,nfrob,sp);
        if(!NT_SUCCESS(status)){
            if(mft_id == 0){
                strace(status,"get_file_record for $Mft failed");
                winx_free(nfrob);
                goto fail;
            }
            /* it returns 0xc000000d (invalid parameter) for non existing records */
            mft_id --; /* try to retrieve a previous record */
            continue;
        }

        /* analyze file record */
        ret_mft_id = GetMftIdFromFRN(nfrob->FileReferenceNumber);
        //trace(D"NTFS record found, id = %I64u",ret_mft_id);
        analyze_file_record(nfrob,sp);

        /* go to the next record */
        if(ret_mft_id == 0 || mft_id == 0)
            break;
        if(ret_mft_id > mft_id){
            /* avoid infinite loops */
            etrace("returned file record index is above expected");
            mft_id --;
        } else {
            mft_id = ret_mft_id - 1;
        }
    }

    itrace("%u attribute list entries have been processed totally",
        sp->processed_attr_list_entries);
    itrace("file records scan completed in %I64u ms",
        winx_xtime() - start_time);
    
    /* build full paths */
    result = build_full_paths(sp);

    winx_free(nfrob);

#ifdef TEST_NTFS_SCANNER
    dtrace("NTFS SCANNER TEST PASSED");
#endif

    itrace("mft scan completed in %I64u ms",
        winx_xtime() - start_time);
    return result;
}

/**
 * @brief Scans entire disk and adds
 * all files found to the file list.
 * @return Zero for success, -1 indicates
 * failure, -2 indicates termination requested
 * by caller.
 */
static int ntfs_scan_disk_helper(char volume_letter,
    int flags, ftw_filter_callback fcb,
    ftw_progress_callback pcb, ftw_terminator t,
    void *user_defined_data, winx_file_info **filelist)
{
    wchar_t path[] = L"\\??\\A:";
    int result;
    mft_scan_parameters sp;
    winx_file_info *f;
    
    sp.filelist = filelist;
    sp.volume_letter = volume_letter;
    sp.processed_attr_list_entries = 0;
    sp.errors = 0;
    sp.flags = flags;
    sp.fcb = fcb;
    sp.pcb = pcb;
    sp.t = t;
    sp.user_defined_data = user_defined_data;
    
    /* open the volume for read access */
    path[4] = winx_toupper(volume_letter);
    sp.f_volume = winx_fopen(path,"r");
    if(sp.f_volume == NULL)
        return (-1);
    
    /* scan mft directly -> add all files to the list */
    result = scan_mft(&sp);
    if(result < 0){
        winx_fclose(sp.f_volume);
        return result;
    }
    
    /* call filter callback for each file found */
    for(f = *filelist; f != NULL; f = f->next){
        if(ftw_ntfs_check_for_termination(&sp)) break;
        validate_blockmap(f);
        if(fcb) (void)fcb(f,sp.user_defined_data);
        if(f->next == *filelist) break;
    }
    
    winx_fclose(sp.f_volume);
    
    if(!(sp.flags & WINX_FTW_ALLOW_PARTIAL_SCAN) && sp.errors)
        return (-1);
    
    return 0;
}

/**
 * @brief winx_scan_disk analog, but
 * optimized for fastest NTFS scan.
 * @note When stop request is sent by the caller,
 * file paths cannot be built, because of missing
 * information about directories not scanned yet.
 */
winx_file_info *ntfs_scan_disk(char volume_letter,
    int flags, ftw_filter_callback fcb, ftw_progress_callback pcb, 
    ftw_terminator t, void *user_defined_data)
{
    winx_file_info *filelist = NULL;
    
    if(ntfs_scan_disk_helper(volume_letter,flags,fcb,pcb,t,user_defined_data,&filelist) == (-1) && \
      !(flags & WINX_FTW_ALLOW_PARTIAL_SCAN)){
        /* destroy list */
        winx_ftw_release(filelist);
        return NULL;
    }
        
    return filelist;
}

/** @} */
