/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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

/*
* Internal NTFS structures.
*/

#ifndef _NTFS_H_
#define _NTFS_H_

/*
* Sources: 
* 1. http://www.apnilife.com/E-Books_apnilife/Windows%20Programming_apnilife/
*    Windows%20NT%20Undocumented%20APIs/1996%20AppE_apnilife.pdf
* 2. http://www.opensource.apple.com/source/ntfs/ntfs-52/kext/ntfs_layout.h
*
* Both files can also be found in /doc subdirectory of source tree in svn repository.
*/

/*
* NOTE: All these structures and function prototypes
* are internal - for ftw_ntfs.c file only.
*/

/* extracts low 48 bits of File Reference Number */
/* LL suffix is not supported by MSVC 6.0 ! */
#ifndef USE_MSVC
#define GetMftIdFromFRN(n) ((n) & 0xffffffffffffLL)
#else
#define GetMftIdFromFRN(n) ((n) & 0xffffffffffff)
#endif

#undef TAG

#ifdef BIGENDIAN
#define TAG(A, B, C, D) ((((((((ULONG)(A)) << 8) + (B)) << 8) + (C)) << 8) + (D))
#else
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#endif

#pragma pack (1)
//#if defined(__GNUC__)
typedef struct {
    ULONGLONG FileReferenceNumber; /* JPA not LE */
} NTFS_FILE_RECORD_INPUT_BUFFER, *PNTFS_FILE_RECORD_BUFFER;

typedef struct {
    LONGLONG FileReferenceNumber; /* JPA not LE */
    LONG FileRecordLength; /* JPA not LE */
    UCHAR FileRecordBuffer[1];
} NTFS_FILE_RECORD_OUTPUT_BUFFER, *PNTFS_FILE_RECORD_OUTPUT_BUFFER;
//#endif

/*
 * System files mft record numbers. All these files are always marked as used
 * in the bitmap attribute of the mft; presumably in order to avoid accidental
 * allocation for random other mft records. Also, the sequence number for each
 * of the system files is always equal to their mft record number and it is
 * never modified.
 */
typedef enum {
    FILE_MFT       = 0,    /* Master file table (mft). Data attribute
                              contains the entries and bitmap attribute
                              records which ones are in use (bit==1). */
    FILE_MFTMirr   = 1,    /* Mft mirror: copy of first four mft records
                              in data attribute. If cluster size > 4kiB,
                              copy of first N mft records, with
                              N = cluster_size / mft_record_size. */
    FILE_LogFile   = 2,    /* Journalling log in data attribute. */
    FILE_Volume    = 3,    /* Volume name attribute and volume information
                              attribute (flags and ntfs version). Windows
                              refers to this file as volume DASD (Direct
                              Access Storage Device). */
    FILE_AttrDef   = 4,    /* Array of attribute definitions in data
                              attribute. */
    FILE_root      = 5,    /* Root directory. */
    FILE_Bitmap    = 6,    /* Allocation bitmap of all clusters (lcns) in
                              data attribute. */
    FILE_Boot      = 7,    /* Boot sector (always at cluster 0) in data
                              attribute. */
    FILE_BadClus   = 8,    /* Contains all bad clusters in the non-resident
                              data attribute. */
    FILE_Secure    = 9,    /* Shared security descriptors in data attribute
                              and two indexes into the descriptors.
                              Appeared in Windows 2000. Before that, this
                              file was named $Quota but was unused. */
    FILE_UpCase    = 10,   /* Uppercase equivalents of all 65536 Unicode
                              characters in data attribute. */
    FILE_Extend    = 11,   /* Directory containing other system files (eg.
                              $ObjId, $Quota, $Reparse and $UsnJrnl). This
                              is new to NTFS3.0. */
    FILE_reserved12 = 12,  /* Reserved for future use (records 12-15). */
    FILE_reserved13 = 13,
    FILE_reserved14 = 14,
    FILE_reserved15 = 15,
    FILE_first_user = 16,  /* First user file, used as test limit for
                              whether to allow opening a file or not. */
} NTFS_SYSTEM_FILES;

typedef struct {
    LELONG Type;       /* The type of NTFS record, possible values are 'FILE', 'INDX', 'BAAD', 'HOLE', CHKD'. */
    LESHORT UsaOffset; /* The offset, in bytes, from the start of the structure to the Update Sequence Array. */
    LESHORT UsaCount;  /* The number of values in the Update Sequence Array. */
    USN Usn;          /* The Update Sequence Number of the NTFS record. */
} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

typedef struct {
    NTFS_RECORD_HEADER Ntfs;    /* It has 'FILE' type. */
    LESHORT SequenceNumber;      /* The number of times that the MFT entry has been reused. */
    LESHORT LinkCount;           /* The number of directory links to the MFT entry. */
    LESHORT AttributeOffset;     /* The offset, in bytes, from the start of the structure to the first attribute of the MFT entry. */
    LESHORT Flags;               /* A bit array of flags specifying properties of the MFT entry: 0x1 - InUse, 0x2 - Directory, ... */
    LELONG BytesInUse;           /* The number of bytes used by the MFT entry. */
    LELONG BytesAllocated;       /* The number of bytes allocated for the MFT entry. */
    LELONGLONG BaseFileRecord;   /* If the MFT entry contains attributes that overflowed a base MFT entry,
                                 * this member contains the FRN of the base entry; otherwise it contains zero. */
    LESHORT NextAttributeNumber; /* The number that will be assigned to the next attribute added to the MFT entry. */
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;

#ifdef _MYENDIANS_H /* temporary */
#define is_file_record(pFileRecordHeader) ((pFileRecordHeader)->Ntfs.Type == SET_LELONG(0x454c4946))
#else
#define is_file_record(pFileRecordHeader) ((pFileRecordHeader)->Ntfs.Type == TAG('F','I','L','E'))
#endif

/* MFT entry consists of FILE_RECORD_HEADER followed by a sequence of attributes. */

typedef enum {
    AttributeStandardInformation = 0x10,
    AttributeAttributeList = 0x20,
    AttributeFileName = 0x30,
    AttributeObjectId = 0x40,
    AttributeSecurityDescriptor = 0x50,
    AttributeVolumeName = 0x60,
    AttributeVolumeInformation = 0x70,
    AttributeData = 0x80,
    AttributeIndexRoot = 0x90,
    AttributeIndexAllocation = 0xA0,
    AttributeBitmap = 0xB0,
    AttributeReparsePoint = 0xC0,
    AttributeEAInformation = 0xD0,
    AttributeEA = 0xE0,
    AttributePropertySet = 0xF0,
    AttributeLoggedUtulityStream = 0x100
} ATTRIBUTE_TYPE;

typedef LELONG LEATTRIBUTE_TYPE, *PLEATTRIBUTE_TYPE;

typedef struct {
    LEATTRIBUTE_TYPE AttributeType; /* The type of the attribute. */
    LELONG Length;                 /* The size, in bytes, of the resident part of the attribute. */
    BOOLEAN Nonresident;          /* Specifies, when true, that the attribute value is nonresident. */
    UCHAR NameLength;             /* The size, in characters, of the name (if any) of the attribute. */
    LESHORT NameOffset;            /* The offset, in bytes, from the start of the structure to the attribute name 
                                   * (stored as a Unicode string). */
    LESHORT Flags;                 /* A bit array of flags specifying properties of the attribute. 0x1 - Compressed, ... */
    LESHORT AttributeNumber;       /* A numeric identifier for the instance of the attribute. */
} ATTRIBUTE, *PATTRIBUTE;

typedef struct {
    ATTRIBUTE Attribute;
    LELONG ValueLength;   /* The size, in bytes, of the attribute value. */
    LESHORT ValueOffset;  /* The offset, in bytes, from the start of the structure to the attribute value. */
    LESHORT Flags;        /* A bit array of flags specifying properties of the attribute. 0x1 - Indexed ... */
} RESIDENT_ATTRIBUTE, *PRESIDENT_ATTRIBUTE;

typedef struct {
    ATTRIBUTE Attribute;
    LELONGLONG LowVcn;             /* The lowest valid VCN of this portion of the attribute value. */
    LELONGLONG HighVcn;            /* The highest valid VCN of this portion of the attribute value. */
    LESHORT RunArrayOffset;        /* The offset, in bytes, from the start of the structure to the run array. */
    UCHAR CompressionUnit;        /* Logarithm to the base two of the number of clusters in a compression unit. */
    UCHAR AlignmentOrReserved[5];
    LELONGLONG AllocatedSize;      /* The size, in bytes, of disk space allocated to hold the attribute value. */
    LELONGLONG DataSize;           /* The size, in bytes, of the attribute value. Maybe larger than AllocatedSize 
                                   * for compressed and sparse files. */
    LELONGLONG InitializedSize;    /* The size, in bytes, of the initialized portion of the attribute value. */
    LELONGLONG CompressedSize;     /* The size, in bytes, of the attribute value after compression. */
                                  /* is presented only when the attribute is really compressed */
} NONRESIDENT_ATTRIBUTE, *PNONRESIDENT_ATTRIBUTE;

typedef struct {
    LELONGLONG CreationTime;
    LELONGLONG ChangeTime;
    LELONGLONG LastWriteTime;
    LELONGLONG LastAccessTime;
    LELONG FileAttributes;                  /* The attributes of the file: FILE_ATTRIBUTE_READONLY, FILE_ATTRIBUTE_HIDDEN etc. */
    LELONG AlignmentOrReservedOrUnknown[3];
    LELONG QuotaId;                         /* Optional member. */
    LELONG SecurityId;                      /* Optional member. */
    LELONGLONG QuotaCharge;                 /* Optional member. */
    USN Usn;                               /* Optional member. */
} STANDARD_INFORMATION, *PSTANDARD_INFORMATION;

/*
 * Possible namespaces for filenames in ntfs (8-bit).
 */
typedef enum {
    FILENAME_POSIX         = 0x00,
    /* This is the largest namespace. It is case sensitive and allows all
       Unicode characters except for: '\0' and '/'.  Beware that in
       WinNT/2k/2003 by default files which eg have the same name except
       for their case will not be distinguished by the standard utilities
       and thus a "del filename" will delete both "filename" and "fileName"
       without warning.  However if for example Services For Unix (SFU) are
       installed and the case sensitive option was enabled at installation
       time, then you can create/access/delete such files.
       Note that even SFU places restrictions on the filenames beyond the
       '\0' and '/' and in particular the following set of characters is
       not allowed: '"', '/', '<', '>', '\'.  All other characters,
       including the ones no allowed in WIN32 namespace are allowed.
       Tested with SFU 3.5 (this is now free) running on Windows XP. */
    FILENAME_WIN32         = 0x01,
    /* The standard WinNT/2k NTFS long filenames. Case insensitive.  All
       Unicode chars except: '\0', '"', '*', '/', ':', '<', '>', '?', '\',
       and '|'.  Further, names cannot end with a '.' or a space. */
    FILENAME_DOS           = 0x02,
    /* The standard DOS filenames (8.3 format). Uppercase only.  All 8-bit
       characters greater space, except: '"', '*', '+', ',', '/', ':', ';',
       '<', '=', '>', '?', and '\'. */
    FILENAME_WIN32_AND_DOS = 0x03,
    /* 3 means that both the Win32 and the DOS filenames are identical and
       hence have been saved in this single filename record. */
} FILENAME_TYPE;

typedef struct {
    LELONGLONG DirectoryFileReferenceNumber; /* The FRN of the directory in which the filename is entered. */
    LELONGLONG CreationTime;                 /* Updated only when the filename changes. May have invalid data. */
    LELONGLONG ChangeTime;                   /* Updated only when the filename changes. May have invalid data. */
    LELONGLONG LastWriteTime;                /* Updated only when the filename changes. May have invalid data. */
    LELONGLONG LastAccessTime;               /* Updated only when the filename changes. May have invalid data. */
    LELONGLONG AllocatedSize;                /* Updated only when the filename changes. May have invalid data. */
    LELONGLONG DataSize;                     /* Updated only when the filename changes. May have invalid data. */
    LELONG FileAttributes;                   /* Updated only when the filename changes. May have invalid data. */
    LELONG AlignmentOrReserved;
    UCHAR NameLength;                       /* The size, in characters, of the filename. */
    UCHAR NameType;                         /* The type of the name. 0x0 - POSIX, 0x1 - Long, 0x2 - Short */
    WCHAR Name[1];                          /* The name of file, in Unicode. */
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;

typedef struct {
    LELONG Unknown[2];
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    LESHORT Flags;       /* 0x1 - VolumeIsDirty */
} VOLUME_INFORMATION, *PVOLUME_INFORMATION;

typedef struct {
    LELONG ReparseTag;
    LESHORT ReparseDataLength;
    LESHORT Reserved;
    UCHAR ReparseData[1];
} REPARSE_POINT, *PREPARSE_POINT;

/* the following structure may have variable length! */
typedef struct {
    LEATTRIBUTE_TYPE AttributeType;  /* The type of the attribute. */
    LESHORT Length;                 /* The size, in bytes, of the attribute list entry. */
    UCHAR NameLength;              /* The size, in characters, of the name (if any) of the attribute. */
    UCHAR NameOffset;              /* The offset, in bytes, from the start of the structure to the attribute name (Unicode). */
    LELONGLONG LowVcn;              /* The lowest VCN of this portion of the attribute value. */
    LELONGLONG FileReferenceNumber; /* The FRN of the MFT entry containing the NONRESIDENT_ATTRIBUTE structure for this portion 
                                    * of the attribute value. */
    LESHORT AttributeNumber;        /* A numeric identifier for the instance of the attribute. */
    LESHORT AlignmentOrReserved[3]; /* optional? */
} ATTRIBUTE_LIST, *PATTRIBUTE_LIST;
#pragma pack (16)

#ifndef FSCTL_GET_NTFS_VOLUME_DATA
#define FSCTL_GET_NTFS_VOLUME_DATA      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#ifndef FSCTL_GET_NTFS_FILE_RECORD
#define FSCTL_GET_NTFS_FILE_RECORD      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 26, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#endif /* _NTFS_H_ */
