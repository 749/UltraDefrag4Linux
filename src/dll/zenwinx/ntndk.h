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

/*
* Windows NT Native Development Kit.
* The main header file.
*/

#ifndef _NTNDK_H_
#define _NTNDK_H_

/* Note: this file also replaces standard winioctl.h header. */

/*
* Extremely important notes for the 64-bit compilation.
*
* 1. The following function prototype causes wrong compiled code:
*    NTSTATUS    NTAPI    NtCreateEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *,BOOLEAN,BOOLEAN);
*
*    Right prototype does not contain BOOLEAN keywords:
*    NTSTATUS    NTAPI    NtCreateEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *,SIZE_T,SIZE_T);
*
*    http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=519510
*
*    Always use SIZE_T type for all unknown parameters of the native calls,
*    because actually we don't know, how long they are. SIZE_T type
*    represents a whole register on all platforms, therefore it is safe for
*    system calls prototyping.
*
* 2. Always fill output buffer with zeros before the following system calls:
*    NtDeviceIoControlFile        (?)
*    NtFsControlFile              (!)
*    NtQueryInformationProcess    (?)
*    NtQueryValueKey              (?)
*    NtQueryVolumeInformationFile (!)
*    NtQueryDirectoryFile         (?)
*    NtQuerySystemInformation     (!?)
*
*    Otherwise Windows might trash stack during these calls.
*
* 3. If you are waiting on a file handle for NtWriteFile request completion,
*    don't check for STATUS_PENDING code. Instead of that wait immediately.
*    ReactOS has wrong implementation of WriteFile() function, the following
*    works much better:
*
*    Status = NtWriteFile(...);
*    if(NT_SUCCESS(Status)){
*        Status = NtWaitForSingleObject(hFile,FALSE,NULL);
*        if(NT_SUCCESS(status)) status = iosb.Status;
*    }
*
*    If you wait only in case when STATUS_PENDING is returned, NtWriteFile()
*    returns immediately and than, when memory allocated for IoStatusBlock() is 
*    reallocated for something else, Windows might decide to write there. Therefore 
*    the stack will be corrupted.
*
*    http://blogs.msdn.com/johnsheehan/archive/2007/12/19/when-idle-threads-bugcheck.aspx
*
* 4. When you are using _vsnprintf() don't forget to fill the buffer 
*    passed as the first parameter by zeros before the call. Otherwise
*    it will fail.
*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

/* define base types */
#if defined(_WIN64)
#define ULONG_PTR unsigned __int64
#else
#define ULONG_PTR unsigned long
#endif

typedef int BOOL;
typedef const char *PCSZ;

#ifndef USE_WINDDK
#ifndef USE_WINSDK
#if !defined(__MINGW_EXTENSION)
#define LONG_PTR  signed long*
typedef ULONG_PTR KAFFINITY;
typedef KAFFINITY *PKAFFINITY;
#endif

typedef ULONG (NTAPI *PTHREAD_START_ROUTINE)(PVOID Parameter);
#endif /* USE_WINSDK */
#endif /* USE_WINDDK */

#ifndef NOMINMAX
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#endif

#if defined(__GNUC__)
ULONGLONG __stdcall _aulldiv(ULONGLONG n, ULONGLONG d);
ULONGLONG __stdcall _alldiv(ULONGLONG n, ULONGLONG d);
ULONGLONG __stdcall _aullrem(ULONGLONG u, ULONGLONG v);
#endif

/* define status codes */
/* ifndef directives are used to prevent warnings when gcc on mingw is used */
typedef LONG NTSTATUS;
#define STATUS_SUCCESS                ((NTSTATUS)0x00000000)
#ifndef STATUS_TIMEOUT
#define STATUS_TIMEOUT                ((NTSTATUS)0x00000102)
#endif
#ifndef STATUS_PENDING
#define STATUS_PENDING                ((NTSTATUS)0x00000103)
#endif
#ifndef STATUS_NO_MORE_FILES
#define STATUS_NO_MORE_FILES          ((NTSTATUS)0x80000006)
#endif
#ifndef STATUS_INVALID_HANDLE
#define STATUS_INVALID_HANDLE         ((NTSTATUS)0xC0000008)
#endif
#define STATUS_IMAGE_ALREADY_LOADED   ((NTSTATUS)0xC000010E)
#define STATUS_NOT_ALL_ASSIGNED       ((NTSTATUS)0x00000106)
#define STATUS_UNSUCCESSFUL           ((NTSTATUS)0xC0000001)
#define STATUS_NOT_IMPLEMENTED        ((NTSTATUS)0xC0000002)
#define STATUS_INVALID_INFO_CLASS     ((NTSTATUS)0xC0000003)
#define STATUS_INFO_LENGTH_MISMATCH   ((NTSTATUS)0xC0000004)
#ifndef STATUS_ACCESS_VIOLATION
#define STATUS_ACCESS_VIOLATION       ((NTSTATUS)0xC0000005)
#endif
#ifndef STATUS_INVALID_HANDLE
#define STATUS_INVALID_HANDLE         ((NTSTATUS)0xC0000008)
#endif
#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER      ((NTSTATUS)0xC000000D)
#endif
#define STATUS_NO_SUCH_DEVICE         ((NTSTATUS)0xC000000E)
#define STATUS_NO_SUCH_FILE           ((NTSTATUS)0xC000000F)
#define STATUS_INVALID_DEVICE_REQUEST ((NTSTATUS)0xC0000010)
#define STATUS_END_OF_FILE            ((NTSTATUS)0xC0000011)
#define STATUS_WRONG_VOLUME           ((NTSTATUS)0xC0000012)
#define STATUS_NO_MEDIA_IN_DEVICE     ((NTSTATUS)0xC0000013)
#ifndef STATUS_NO_MEMORY
#define STATUS_NO_MEMORY              ((NTSTATUS)0xC0000017)
#endif
#ifndef STATUS_ALREADY_COMMITTED
#define STATUS_ALREADY_COMMITTED      ((NTSTATUS)0xC0000021)
#endif
#define STATUS_ACCESS_DENIED          ((NTSTATUS)0xC0000022)
#define STATUS_BUFFER_TOO_SMALL       ((NTSTATUS)0xC0000023)
#define STATUS_OBJECT_NAME_INVALID    ((NTSTATUS)0xC0000033)
#define STATUS_OBJECT_NAME_NOT_FOUND  ((NTSTATUS)0xC0000034)
#define STATUS_OBJECT_NAME_COLLISION  ((NTSTATUS)0xC0000035)
#define STATUS_OBJECT_PATH_INVALID    ((NTSTATUS)0xC0000039)
#define STATUS_OBJECT_PATH_NOT_FOUND  ((NTSTATUS)0xC000003A)
#define STATUS_OBJECT_PATH_SYNTAX_BAD ((NTSTATUS)0xC000003B)
#define STATUS_INVALID_INFO_CLASS     ((NTSTATUS)0xC0000003)
#ifndef NT_SUCCESS
#define NT_SUCCESS(x) ((x)>=0)
#endif

#ifndef STATUS_UNRECOGNIZED_VOLUME
#define STATUS_UNRECOGNIZED_VOLUME    ((NTSTATUS)0xC000014F)
#endif
#ifndef STATUS_VARIABLE_NOT_FOUND
#define STATUS_VARIABLE_NOT_FOUND     ((NTSTATUS)0xC0000100)
#endif
#ifndef STATUS_WAIT_0
#define STATUS_WAIT_0                 ((NTSTATUS)0x00000000)
#endif
#ifndef STATUS_SHARING_VIOLATION
#define STATUS_SHARING_VIOLATION      ((NTSTATUS)0xC0000043)
#endif

#define MAX_WAIT_INTERVAL (-0x7FFFFFFFFFFFFFFFLL)

/* define base nt structures */
typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;
  PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

#define OBJ_INHERIT          0x00000002
#define OBJ_PERMANENT        0x00000010
#define OBJ_EXCLUSIVE        0x00000020
#define OBJ_CASE_INSENSITIVE 0x00000040
#define OBJ_OPENIF           0x00000080
#define OBJ_OPENLINK         0x00000100
#define OBJ_KERNEL_HANDLE    0x00000200
#define OBJ_VALID_ATTRIBUTES (OBJ_KERNEL_HANDLE | OBJ_OPENLINK | \
        OBJ_OPENIF | OBJ_CASE_INSENSITIVE | OBJ_EXCLUSIVE | \
        OBJ_PERMANENT | OBJ_INHERIT)
#define InitializeObjectAttributes(p,n,a,r,s) \
    do { \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
        (p)->RootDirectory = r; \
        (p)->Attributes = a; \
        (p)->ObjectName = n; \
        (p)->SecurityDescriptor = s; \
        (p)->SecurityQualityOfService = NULL; \
    } while (0)
    
typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#if defined(_WIN64)
typedef struct _IO_STATUS_BLOCK32 {
    NTSTATUS Status;
    ULONG Information;
} IO_STATUS_BLOCK32, *PIO_STATUS_BLOCK32;
#endif

typedef VOID(*PIO_APC_ROUTINE) (
                PVOID ApcContext,
                PIO_STATUS_BLOCK IoStatusBlock,
                ULONG Reserved
            );

/* additional nt structures */
typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;

#ifndef HEAP_ZERO_MEMORY
#define HEAP_ZERO_MEMORY  0x00000008 // winnt
#endif

typedef struct _RTL_HEAP_DEFINITION {
    ULONG Length; /* = sizeof(RTL_HEAP_DEFINITION) */
    ULONG Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemCpuInformation = 1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3, /* was SystemTimeInformation */
    Unknown4,
    SystemProcessInformation = 5,
    Unknown6,
    Unknown7,
    SystemProcessorPerformanceInformation = 8,
    Unknown9,
    Unknown10,
    SystemModuleInformation = 11,
    Unknown12,
    Unknown13,
    Unknown14,
    Unknown15,
    SystemHandleInformation = 16,
    Unknown17,
    SystemPageFileInformation = 18,
    Unknown19,
    Unknown20,
    SystemCacheInformation = 21,
    Unknown22,
    SystemInterruptInformation = 23,
    SystemDpcBehaviourInformation = 24,
    SystemFullMemoryInformation = 25,
    SystemNotImplemented6 = 25,
    SystemLoadImage = 26,
    SystemUnloadImage = 27,
    SystemTimeAdjustmentInformation = 28,
    SystemTimeAdjustment = 28,
    SystemSummaryMemoryInformation = 29,
    SystemNotImplemented7 = 29,
    SystemNextEventIdInformation = 30,
    SystemNotImplemented8 = 30,
    SystemEventIdsInformation = 31,
    SystemCrashDumpInformation = 32,
    SystemExceptionInformation = 33,
    SystemCrashDumpStateInformation = 34,
    SystemKernelDebuggerInformation = 35,
    SystemContextSwitchInformation = 36,
    SystemRegistryQuotaInformation = 37,
    SystemCurrentTimeZoneInformation = 44,
    SystemTimeZoneInformation = 44,
    SystemLookasideInformation = 45,
    SystemSetTimeSlipEvent = 46,
    SystemCreateSession = 47,
    SystemDeleteSession = 48,
    SystemInvalidInfoClass4 = 49,
    SystemRangeStartInformation = 50,
    SystemVerifierInformation = 51,
    SystemAddVerifier = 52,
    SystemSessionProcessesInformation = 53,
    SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_BASIC_INFORMATION
{
    ULONG Reserved;
    ULONG TimerResolution;
    ULONG PageSize;
    ULONG NumberOfPhysicalPages;
    ULONG LowestPhysicalPageNumber;
    ULONG HighestPhysicalPageNumber;
    ULONG AllocationGranularity;
    ULONG MinimumUserModeAddress;
    ULONG MaximumUserModeAddress;
    KAFFINITY ActiveProcessorsAffinityMask;
    CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleProcessTime;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    ULONG AvailablePages;
    ULONG CommittedPages;
    ULONG CommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
    ULONG PagedPoolPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG FreeSystemPtes;
    ULONG ResidentSystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG NonPagedPoolLookasideHits;
    ULONG PagedPoolLookasideHits;
    ULONG Spare3Count;
    ULONG ResidentSystemCachePage;
    ULONG ResidentPagedPoolPage;
    ULONG ResidentSystemDriverPage;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadResourceMiss;
    ULONG CcFastReadNotPossible;
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

#define DEVICE_TYPE DWORD

/* DEVICE_OBJECT.Characteristics */

#define FILE_REMOVABLE_MEDIA            0x00000001
#define FILE_READ_ONLY_DEVICE           0x00000002
#define FILE_FLOPPY_DISKETTE            0x00000004
#define FILE_WRITE_ONCE_MEDIA           0x00000008
#define FILE_REMOTE_DEVICE              0x00000010
#define FILE_DEVICE_IS_MOUNTED          0x00000020
#define FILE_VIRTUAL_VOLUME             0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME  0x00000080
#define FILE_DEVICE_SECURE_OPEN         0x00000100

#ifndef FILE_DEVICE_FILE_SYSTEM
#define FILE_DEVICE_FILE_SYSTEM           0x00000009
#endif

typedef struct _FILE_FS_VOLUME_INFORMATION {
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    UCHAR Unknown;
    WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_DEVICE_INFORMATION {
  DEVICE_TYPE  DeviceType;
  ULONG  Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessQuotaLimits = 1,
    ProcessIoCounters = 2,
    ProcessVmCounters = 3,
    ProcessTimes = 4,
    ProcessBasePriority = 5,
    ProcessRaisePriority = 6,
    ProcessDebugPort = 7,
    ProcessExceptionPort = 8,
    ProcessAccessToken = 9,
    ProcessLdtInformation = 10,
    ProcessLdtSize = 11,
    ProcessDefaultHardErrorMode = 12,
    ProcessIoPortHandlers = 13,
    ProcessPooledUsageAndLimits = 14,
    ProcessWorkingSetWatch = 15,
    ProcessUserModeIOPL = 16,
    ProcessEnableAlignmentFaultFixup = 17,
    ProcessPriorityClass = 18,
    ProcessWx86Information = 19,
    ProcessHandleCount = 20,
    ProcessAffinityMask = 21,
    ProcessPriorityBoost = 22,
    ProcessDeviceMap = 23,
    ProcessSessionInformation = 24,
    ProcessForegroundInformation = 25,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessLUIDDeviceMapsEnabled = 28,
    ProcessBreakOnTermination = 29,
    ProcessDebugObjectHandle = 30,
    ProcessDebugFlags = 31,
    ProcessHandleTracing = 32,
    MaxProcessInfoClass
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

#define DIRECTORY_QUERY (0x0001)
#define DIRECTORY_TRAVERSE (0x0002)
#define DIRECTORY_CREATE_OBJECT (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY (0x0008)
#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

#define SYMBOLIC_LINK_QUERY       0x0001
#define SYMBOLIC_LINK_ALL_ACCESS  (STANDARD_RIGHTS_REQUIRED | 0x1)

#ifndef FILE_OPEN
#define FILE_OPEN                       1
#endif
#ifndef FILE_CREATE
#define FILE_CREATE                     2
#endif
#ifndef FILE_OPEN_IF
#define FILE_OPEN_IF                    3
#endif
#ifndef FILE_OVERWRITE
#define FILE_OVERWRITE                  4
#endif
#ifndef FILE_OVERWRITE_IF
#define FILE_OVERWRITE_IF               5
#endif

#define FILE_SYNCHRONOUS_IO_NONALERT    0x00000020
#define FILE_OPEN_FOR_BACKUP_INTENT     0x00004000
#define FILE_NON_DIRECTORY_FILE         0x00000040
/* Windows 7 and later */
#define FILE_DISALLOW_EXCLUSIVE         0x00020000

#ifndef FILE_NO_INTERMEDIATE_BUFFERING
#define FILE_NO_INTERMEDIATE_BUFFERING  0x00000008
#endif
#ifndef FILE_OPEN_REPARSE_POINT
#define FILE_OPEN_REPARSE_POINT         0x00200000
#endif

typedef struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING      DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    HANDLE              ConsoleHandle;
    ULONG               ConsoleFlags;
    HANDLE              hStdInput;
    HANDLE              hStdOutput;
    HANDLE              hStdError;
    CURDIR              CurrentDirectory;
    UNICODE_STRING      DllPath;
    UNICODE_STRING      ImagePathName;
    UNICODE_STRING      CommandLine;
    PWSTR               Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING      WindowTitle;
    UNICODE_STRING      Desktop;
    UNICODE_STRING      ShellInfo;
    UNICODE_STRING      RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct tagRTL_BITMAP {
    ULONG  SizeOfBitMap; /* Number of bits in the bitmap */
    PULONG Buffer; /* Bitmap data, assumed sized to a DWORD boundary */
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct _PEB_LDR_DATA
{
    ULONG               Length;
    BOOLEAN             Initialized;
    PVOID               SsHandle;
    LIST_ENTRY          InLoadOrderModuleList;
    LIST_ENTRY          InMemoryOrderModuleList;
    LIST_ENTRY          InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _GDI_TEB_BATCH
{
    ULONG  Offset;
    HANDLE HDC;
    ULONG  Buffer[0x136];
} GDI_TEB_BATCH;

/***********************************************************************
 * PEB data structure
 */
typedef struct _PEB
{
    BOOLEAN                      InheritedAddressSpace;             /*  00 */
    BOOLEAN                      ReadImageFileExecOptions;          /*  01 */
    BOOLEAN                      BeingDebugged;                     /*  02 */
    BOOLEAN                      SpareBool;                         /*  03 */
    HANDLE                       Mutant;                            /*  04 */
    HMODULE                      ImageBaseAddress;                  /*  08 */
    PPEB_LDR_DATA                LdrData;                           /*  0c */
    RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /*  10 */
    PVOID                        SubSystemData;                     /*  14 */
    HANDLE                       ProcessHeap;                       /*  18 */
    PRTL_CRITICAL_SECTION        FastPebLock;                       /*  1c */
    PVOID /*PPEBLOCKROUTINE*/    FastPebLockRoutine;                /*  20 */
    PVOID /*PPEBLOCKROUTINE*/    FastPebUnlockRoutine;              /*  24 */
    ULONG                        EnvironmentUpdateCount;            /*  28 */
    PVOID                        KernelCallbackTable;               /*  2c */
    PVOID                        EventLogSection;                   /*  30 */
    PVOID                        EventLog;                          /*  34 */
    PVOID /*PPEB_FREE_BLOCK*/    FreeList;                          /*  38 */
    ULONG                        TlsExpansionCounter;               /*  3c */
    PRTL_BITMAP                  TlsBitmap;                         /*  40 */
    ULONG                        TlsBitmapBits[2];                  /*  44 */
    PVOID                        ReadOnlySharedMemoryBase;          /*  4c */
    PVOID                        ReadOnlySharedMemoryHeap;          /*  50 */
    PVOID                       *ReadOnlyStaticServerData;          /*  54 */
    PVOID                        AnsiCodePageData;                  /*  58 */
    PVOID                        OemCodePageData;                   /*  5c */
    PVOID                        UnicodeCaseTableData;              /*  60 */
    ULONG                        NumberOfProcessors;                /*  64 */
    ULONG                        NtGlobalFlag;                      /*  68 */
    BYTE                         Spare2[4];                         /*  6c */
    LARGE_INTEGER                CriticalSectionTimeout;            /*  70 */
    ULONG                        HeapSegmentReserve;                /*  78 */
    ULONG                        HeapSegmentCommit;                 /*  7c */
    ULONG                        HeapDeCommitTotalFreeThreshold;    /*  80 */
    ULONG                        HeapDeCommitFreeBlockThreshold;    /*  84 */
    ULONG                        NumberOfHeaps;                     /*  88 */
    ULONG                        MaximumNumberOfHeaps;              /*  8c */
    PVOID                       *ProcessHeaps;                      /*  90 */
    PVOID                        GdiSharedHandleTable;              /*  94 */
    PVOID                        ProcessStarterHelper;              /*  98 */
    PVOID                        GdiDCAttributeList;                /*  9c */
    PVOID                        LoaderLock;                        /*  a0 */
    ULONG                        OSMajorVersion;                    /*  a4 */
    ULONG                        OSMinorVersion;                    /*  a8 */
    ULONG                        OSBuildNumber;                     /*  ac */
    ULONG                        OSPlatformId;                      /*  b0 */
    ULONG                        ImageSubSystem;                    /*  b4 */
    ULONG                        ImageSubSystemMajorVersion;        /*  b8 */
    ULONG                        ImageSubSystemMinorVersion;        /*  bc */
    ULONG                        ImageProcessAffinityMask;          /*  c0 */
    ULONG                        GdiHandleBuffer[34];               /*  c4 */
    ULONG                        PostProcessInitRoutine;            /* 14c */
    PRTL_BITMAP                  TlsExpansionBitmap;                /* 150 */
    ULONG                        TlsExpansionBitmapBits[32];        /* 154 */
    ULONG                        SessionId;                         /* 1d4 */
} PEB, *PPEB;

/***********************************************************************
 * TEB data structure
 */
typedef struct _TEB
{
    NT_TIB          Tib;                        /* 000 */
    PVOID           EnvironmentPointer;         /* 01c */
    CLIENT_ID       ClientId;                   /* 020 */
    PVOID           ActiveRpcHandle;            /* 028 */
    PVOID           ThreadLocalStoragePointer;  /* 02c */
    PPEB            Peb;                        /* 030 */
    ULONG           LastErrorValue;             /* 034 */
    ULONG           CountOfOwnedCriticalSections;/* 038 */
    PVOID           CsrClientThread;            /* 03c */
    PVOID           Win32ThreadInfo;            /* 040 */
    ULONG           Win32ClientInfo[31];        /* 044 used for user32 private data in Wine */
    PVOID           WOW32Reserved;              /* 0c0 */
    ULONG           CurrentLocale;              /* 0c4 */
    ULONG           FpSoftwareStatusRegister;   /* 0c8 */
    PVOID           SystemReserved1[54];        /* 0cc used for kernel32 private data in Wine */
    PVOID           Spare1;                     /* 1a4 */
    LONG            ExceptionCode;              /* 1a8 */
    BYTE            SpareBytes1[40];            /* 1ac */
    PVOID           SystemReserved2[10];        /* 1d4 used for ntdll private data in Wine */
    GDI_TEB_BATCH   GdiTebBatch;                /* 1fc */
    ULONG           gdiRgn;                     /* 6dc */
    ULONG           gdiPen;                     /* 6e0 */
    ULONG           gdiBrush;                   /* 6e4 */
    CLIENT_ID       RealClientId;               /* 6e8 */
    HANDLE          GdiCachedProcessHandle;     /* 6f0 */
    ULONG           GdiClientPID;               /* 6f4 */
    ULONG           GdiClientTID;               /* 6f8 */
    PVOID           GdiThreadLocaleInfo;        /* 6fc */
    PVOID           UserReserved[5];            /* 700 */
    PVOID           glDispachTable[280];        /* 714 */
    ULONG           glReserved1[26];            /* b74 */
    PVOID           glReserved2;                /* bdc */
    PVOID           glSectionInfo;              /* be0 */
    PVOID           glSection;                  /* be4 */
    PVOID           glTable;                    /* be8 */
    PVOID           glCurrentRC;                /* bec */
    PVOID           glContext;                  /* bf0 */
    ULONG           LastStatusValue;            /* bf4 */
    UNICODE_STRING  StaticUnicodeString;        /* bf8 used by advapi32 */
    WCHAR           StaticUnicodeBuffer[261];   /* c00 used by advapi32 */
    PVOID           DeallocationStack;          /* e0c */
    PVOID           TlsSlots[64];               /* e10 */
    LIST_ENTRY      TlsLinks;                   /* f10 */
    PVOID           Vdm;                        /* f18 */
    PVOID           ReservedForNtRpc;           /* f1c */
    PVOID           DbgSsReserved[2];           /* f20 */
    ULONG           HardErrorDisabled;          /* f28 */
    PVOID           Instrumentation[16];        /* f2c */
    PVOID           WinSockData;                /* f6c */
    ULONG           GdiBatchCount;              /* f70 */
    ULONG           Spare2;                     /* f74 */
    ULONG           Spare3;                     /* f78 */
    ULONG           Spare4;                     /* f7c */
    PVOID           ReservedForOle;             /* f80 */
    ULONG           WaitingOnLoaderLock;        /* f84 */
    PVOID           Reserved5[3];               /* f88 */
    PVOID          *TlsExpansionSlots;          /* f94 */
} TEB, *PTEB;

#define NtCurrentProcess() ((HANDLE)-1)
#define NtCurrentThread() ((HANDLE)-2)
/*
* NtCurrentTeb() is imported from ntdll.dll if we use ms c compiler.
* Otherwise (on mingw) this is inline function, defined in one of the
* mingw headers.
*/

#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE  0x3
#define SE_AUDIT_PRIVILEGE               0x15
#define SE_BACKUP_PRIVILEGE              0x11
#define SE_CREATE_PAGEFILE_PRIVILEGE     0x0f
#define SE_CREATE_PERMANENT_PRIVILEGE    0x10
#define SE_CREATE_TOKEN_PRIVILEGE        0x2
#define SE_DEBUG_PRIVILEGE               0x14
#define SE_IMPERSONATE_PRIVILEGE
#define SE_INC_BASE_PRIORITY_PRIVILEGE   0x0e
#define SE_INCREASE_QUOTA_PRIVILEGE      0x5
#define SE_LOAD_DRIVER_PRIVILEGE         0x0a
#define SE_LOCK_MEMORY_PRIVILEGE         0x4
#define SE_MANAGE_VOLUME_PRIVILEGE       0x1c
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE 0x0d
#define SE_RESTORE_PRIVILEGE             0x12
#define SE_SECURITY_PRIVILEGE            0x8
#define SE_SHUTDOWN_PRIVILEGE            0x13
#define SE_SYSTEM_PROFILE_PRIVILEGE      0x0b
#define SE_SYSTEMTIME_PRIVILEGE          0x0c
#define SE_TAKE_OWNERSHIP_PRIVILEGE      0x9
#define SE_TCB_PRIVILEGE                 0x7

typedef enum _SHUTDOWN_ACTION
{
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff
} SHUTDOWN_ACTION;

typedef struct _KEY_FULL_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG         TitleIndex;
    ULONG         ClassOffset;
    ULONG         ClassLength;
    ULONG         SubKeys;
    ULONG         MaxNameLen;
    ULONG         MaxClassLen;
    ULONG         Values;
    ULONG         MaxValueNameLen;
    ULONG         MaxValueDataLen;
    WCHAR         Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef enum _KEY_INFORMATION_CLASS { 
    KeyBasicInformation           = 0,
    KeyNodeInformation            = 1,
    KeyFullInformation            = 2,
    KeyNameInformation            = 3,
    KeyCachedInformation          = 4,
    KeyFlagsInformation           = 5,
    KeyVirtualizationInformation  = 6,
    KeyHandleTagsInformation      = 7,
    MaxKeyInfoClass               = 8
} KEY_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS
{
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef enum _EVENT_TYPE {
  NotificationEvent,
  SynchronizationEvent
} EVENT_TYPE, *PEVENT_TYPE;

#define FILE_DIRECTORY_FILE     0x00000001
#define FILE_RESERVE_OPFILTER   0x00100000

#ifndef FILE_WRITE_THROUGH
#define FILE_WRITE_THROUGH      0x00000002
#endif

/*
* DriveMap member must be declared as unsigned int
* and alignment must be equal to 1, otherwise it fails
* on Windows XP x64.
*/
#pragma pack(push,1)
typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            //ULONG DriveMap;
            UINT DriveMap;
            UCHAR DriveType[32];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PPEB PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;
#pragma pack(pop)

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER   TotalAllocationUnits;
    LARGE_INTEGER   AvailableAllocationUnits;
    ULONG           SectorsPerAllocationUnit;
    ULONG           BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG   FileSystemAttributes;
    ULONG   MaximumComponentNameLength;
    ULONG   FileSystemNameLength;
    WCHAR   FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

typedef struct _KBD_RECORD {
    WORD    wVirtualScanCode;
    DWORD    dwControlKeyState;
    UCHAR    AsciiChar;
    BOOL    bKeyDown;
} KBD_RECORD, *PKBD_RECORD;

typedef LPOSVERSIONINFOW PRTL_OSVERSIONINFOW;

/* keyboard related structures */
/* KEYBOARD_INPUT_DATA.Flags constants */
#define KEY_MAKE     0
#define KEY_BREAK    1
#define KEY_E0       2
#define KEY_E1       4

typedef struct _KEYBOARD_INPUT_DATA {
  USHORT  UnitId;
  USHORT  MakeCode;
  USHORT  Flags;
  USHORT  Reserved;
  ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
  USHORT  UnitId;
  USHORT  LedFlags;
} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;

/* replacement for winioctl.h which has encumbering ntfs related definitions */
#define DEVICE_TYPE DWORD

#define FILE_DEVICE_BEEP                0x00000001
#define FILE_DEVICE_CD_ROM              0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DATALINK            0x00000005
#define FILE_DEVICE_DFS                 0x00000006
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define FILE_DEVICE_INPORT_PORT         0x0000000a
#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MAILSLOT            0x0000000c
#define FILE_DEVICE_MIDI_IN             0x0000000d
#define FILE_DEVICE_MIDI_OUT            0x0000000e
#define FILE_DEVICE_MOUSE               0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
#define FILE_DEVICE_NAMED_PIPE          0x00000011
#define FILE_DEVICE_NETWORK             0x00000012
#define FILE_DEVICE_NETWORK_BROWSER     0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_NULL                0x00000015
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
#define FILE_DEVICE_PRINTER             0x00000018
#define FILE_DEVICE_SCANNER             0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_SCREEN              0x0000001c
#define FILE_DEVICE_SOUND               0x0000001d
#define FILE_DEVICE_STREAMS             0x0000001e
#define FILE_DEVICE_TAPE                0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define FILE_DEVICE_TRANSPORT           0x00000021
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_VIDEO               0x00000023
#define FILE_DEVICE_VIRTUAL_DISK        0x00000024
#define FILE_DEVICE_WAVE_IN             0x00000025
#define FILE_DEVICE_WAVE_OUT            0x00000026
#define FILE_DEVICE_8042_PORT           0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028
#define FILE_DEVICE_BATTERY             0x00000029
#define FILE_DEVICE_BUS_EXTENDER        0x0000002a
#define FILE_DEVICE_MODEM               0x0000002b
#define FILE_DEVICE_VDM                 0x0000002c
#define FILE_DEVICE_MASS_STORAGE        0x0000002d
#define FILE_DEVICE_SMB                 0x0000002e
#define FILE_DEVICE_KS                  0x0000002f
#define FILE_DEVICE_CHANGER             0x00000030
#define FILE_DEVICE_SMARTCARD           0x00000031
#define FILE_DEVICE_ACPI                0x00000032
#define FILE_DEVICE_DVD                 0x00000033
#define FILE_DEVICE_FULLSCREEN_VIDEO    0x00000034
#define FILE_DEVICE_DFS_FILE_SYSTEM     0x00000035
#define FILE_DEVICE_DFS_VOLUME          0x00000036
#define FILE_DEVICE_SERENUM             0x00000037
#define FILE_DEVICE_TERMSRV             0x00000038
#define FILE_DEVICE_KSEC                0x00000039
#define FILE_DEVICE_FIPS                0x0000003A
#define FILE_DEVICE_INFINIBAND          0x0000003B

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

#define FILE_ANY_ACCESS                 0
#define FILE_SPECIAL_ACCESS             (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS                ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS               ( 0x0002 )    // file & pipe


//
// NtDeviceIoControlFile IoControlCode values for the keyboard device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define IOCTL_KEYBOARD_QUERY_ATTRIBUTES      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_SET_TYPEMATIC         CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0001, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_SET_INDICATORS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION   CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0020, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_INSERT_DATA           CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0040, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// These Device IO control query/set IME status to keyboard hardware.
//
#define IOCTL_KEYBOARD_QUERY_IME_STATUS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0400, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_SET_IME_STATUS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0401, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Define the keyboard indicators.
//

#define KEYBOARD_LED_INJECTED     0x8000 //Used by Terminal Server
#define KEYBOARD_SHADOW           0x4000 //Used by Terminal Server
//#if defined(FE_SB) || defined(WINDOWS_FE) || defined(DBCS)
#define KEYBOARD_KANA_LOCK_ON     8 // Japanese keyboard
//#endif // defined(FE_SB) || defined(WINDOWS_FE) || defined(DBCS)
#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1

#define FSCTL_GET_NTFS_VOLUME_DATA      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 25, METHOD_BUFFERED, FILE_ANY_ACCESS) // NTFS_VOLUME_DATA_BUFFER
#define FSCTL_GET_NTFS_FILE_RECORD      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 26, METHOD_BUFFERED, FILE_ANY_ACCESS) // NTFS_FILE_RECORD_INPUT_BUFFER, NTFS_FILE_RECORD_OUTPUT_BUFFER
#define FSCTL_GET_VOLUME_BITMAP         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 27, METHOD_NEITHER,  FILE_ANY_ACCESS) // STARTING_LCN_INPUT_BUFFER, VOLUME_BITMAP_BUFFER
#define FSCTL_GET_RETRIEVAL_POINTERS    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 28, METHOD_NEITHER,  FILE_ANY_ACCESS) // STARTING_VCN_INPUT_BUFFER, RETRIEVAL_POINTERS_BUFFER
#define FSCTL_MOVE_FILE                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 29, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // MOVE_FILE_DATA,
#define FSCTL_IS_VOLUME_DIRTY           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define VOLUME_IS_DIRTY  1

typedef struct _PARTITION_INFORMATION {
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionLength;
    DWORD HiddenSectors;
    DWORD PartitionNumber;
    BYTE  PartitionType;
    BOOLEAN BootIndicator;
    BOOLEAN RecognizedPartition;
    BOOLEAN RewritePartition;
} PARTITION_INFORMATION, *PPARTITION_INFORMATION;

typedef enum _MEDIA_TYPE
{
  Unknown, 
  F5_1Pt2_512, 
  F3_1Pt44_512, 
  F3_2Pt88_512, 
  F3_20Pt8_512, 
  F3_720_512, 
  F5_360_512, 
  F5_320_512, 
  F5_320_1024, 
  F5_180_512, 
  F5_160_512, 
  RemovableMedia, 
  FixedMedia, 
  F3_120M_512, 
  F3_640_512, 
  F5_640_512, 
  F5_720_512, 
  F3_1Pt2_512, 
  F3_1Pt23_1024, 
  F5_1Pt23_1024, 
  F3_128Mb_512, 
  F3_230Mb_512, 
  F8_256_128, 
  F3_200Mb_512, 
  F3_240M_512, 
  F3_32M_512
} MEDIA_TYPE;

typedef struct _DISK_GEOMETRY {
    LARGE_INTEGER Cylinders;
    MEDIA_TYPE MediaType;
    DWORD TracksPerCylinder;
    DWORD SectorsPerTrack;
    DWORD BytesPerSector;
} DISK_GEOMETRY;

#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#define IOCTL_DISK_GET_DRIVE_GEOMETRY   CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO   CTL_CODE(IOCTL_DISK_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation = 1,
    FileFullDirectoryInformation,
    FileBothDirectoryInformation,
    FileBasicInformation,
    FileStandardInformation,
    FileInternalInformation,
    FileEaInformation,
    FileAccessInformation,
    FileNameInformation,
    FileRenameInformation,
    FileLinkInformation,
    FileNamesInformation,
    FileDispositionInformation,
    FilePositionInformation,
    FileFullEaInformation,
    FileModeInformation,
    FileAlignmentInformation,
    FileAllInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileAlternateNameInformation,
    FileStreamInformation,
    FilePipeInformation,
    FilePipeLocalInformation,
    FilePipeRemoteInformation,
    FileMailslotQueryInformation,
    FileMailslotSetInformation,
    FileCompressionInformation,
    FileObjectIdInformation,
    FileCompletionInformation,
    FileMoveClusterInformation,
    FileQuotaInformation,
    FileReparsePointInformation,
    FileNetworkOpenInformation,
    FileAttributeTagInformation,
    FileTrackingInformation,
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_BOTH_DIRECTORY_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    /*
    * The next field may hold zero for 3.99 GB files on FAT32
    * volumes with 32k cluster size (tested on 32-bit XP SP1).
    */
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    ULONG               EaSize;
    CHAR                ShortNameLength;
    WCHAR               ShortName[12];
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION,
  FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

typedef struct _TIME_FIELDS {
    short Year;        // range [1601...]
    short Month;       // range [1..12]
    short Day;         // range [1..31]
    short Hour;        // range [0..23]
    short Minute;      // range [0..59]
    short Second;      // range [0..59]
    short Milliseconds;// range [0..999]
    short Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS *PTIME_FIELDS;

typedef NTSTATUS (NTAPI *PRTL_QUERY_REGISTRY_ROUTINE)(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

#define RTL_REGISTRY_ABSOLUTE     0   // Path is a full path
#define RTL_REGISTRY_SERVICES     1   // \Registry\Machine\System\CurrentControlSet\Services
#define RTL_REGISTRY_CONTROL      2   // \Registry\Machine\System\CurrentControlSet\Control
#define RTL_REGISTRY_WINDOWS_NT   3   // \Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion
#define RTL_REGISTRY_DEVICEMAP    4   // \Registry\Machine\Hardware\DeviceMap
#define RTL_REGISTRY_USER         5   // \Registry\User\CurrentUser
#define RTL_REGISTRY_MAXIMUM      6
#define RTL_REGISTRY_HANDLE       0x40000000    // Low order bits are registry handle
#define RTL_REGISTRY_OPTIONAL     0x80000000    // Indicates the key node is optional

typedef struct _RTL_QUERY_REGISTRY_TABLE {
    PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
    ULONG Flags;
    PWSTR Name;
    PVOID EntryContext;
    ULONG DefaultType;
    PVOID DefaultData;
    ULONG DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

/* native functions prototypes */
NTSTATUS    NTAPI    NtAdjustPrivilegesToken(HANDLE,SIZE_T,PTOKEN_PRIVILEGES,SIZE_T,PTOKEN_PRIVILEGES,PDWORD);
NTSTATUS    NTAPI    NtAllocateVirtualMemory(HANDLE,PVOID*,SIZE_T,SIZE_T *,SIZE_T,SIZE_T);
NTSTATUS    NTAPI    NtCancelIoFile(HANDLE,PIO_STATUS_BLOCK);
NTSTATUS    NTAPI    NtClearEvent(HANDLE);
NTSTATUS    NTAPI    NtClose(HANDLE);
NTSTATUS    NTAPI    NtCreateEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *,SIZE_T,SIZE_T);
NTSTATUS    NTAPI    NtCreateFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,SIZE_T,SIZE_T,SIZE_T,SIZE_T,PVOID,SIZE_T);
NTSTATUS    NTAPI    NtCreateKey(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,SIZE_T,PUNICODE_STRING,SIZE_T,PULONG);
NTSTATUS    NTAPI    NtCreateMutant(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,SIZE_T);
NTSTATUS    NTAPI    NtDeleteFile(POBJECT_ATTRIBUTES);
NTSTATUS    NTAPI    NtDelayExecution(SIZE_T,const LARGE_INTEGER*);
NTSTATUS    NTAPI    NtDeviceIoControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,SIZE_T,PVOID,SIZE_T,PVOID,SIZE_T);
NTSTATUS    NTAPI    NtDisplayString(PUNICODE_STRING); 
NTSTATUS    NTAPI    NtFlushBuffersFile(HANDLE,PIO_STATUS_BLOCK);
NTSTATUS    NTAPI    NtFlushKey(HANDLE KeyHandle);
NTSTATUS    NTAPI    NtFreeVirtualMemory(HANDLE,PVOID*,SIZE_T *,SIZE_T);
NTSTATUS    NTAPI    NtFsControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,SIZE_T,PVOID,SIZE_T,PVOID,SIZE_T);
NTSTATUS    NTAPI    NtInitializeRegistry(SIZE_T);
NTSTATUS    NTAPI    NtLoadDriver(PUNICODE_STRING);
NTSTATUS    NTAPI    NtMapViewOfSection(HANDLE,HANDLE,PVOID*,SIZE_T,SIZE_T,const LARGE_INTEGER*,SIZE_T*,SECTION_INHERIT,SIZE_T,SIZE_T);
NTSTATUS    NTAPI    NtOpenEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *);
NTSTATUS    NTAPI    NtOpenKey(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS    NTAPI    NtOpenMutant(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS    NTAPI    NtOpenProcessToken(HANDLE,ACCESS_MASK,PHANDLE);
NTSTATUS    NTAPI    NtOpenSection(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSTATUS    NTAPI    NtOpenSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS    NTAPI    NtQueryDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,SIZE_T,FILE_INFORMATION_CLASS,SIZE_T,PUNICODE_STRING,SIZE_T);
NTSTATUS    NTAPI    NtQueryInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,SIZE_T,FILE_INFORMATION_CLASS);
NTSTATUS    NTAPI    NtQueryInformationProcess(HANDLE,PROCESSINFOCLASS,PVOID,SIZE_T,PULONG);
NTSTATUS    NTAPI    NtQueryPerformanceCounter(PLARGE_INTEGER,PLARGE_INTEGER);
NTSTATUS    NTAPI    NtQuerySymbolicLinkObject(HANDLE,PUNICODE_STRING,PULONG);
NTSTATUS    NTAPI    NtQuerySystemTime(PLARGE_INTEGER SystemTime);
NTSTATUS    NTAPI    NtQueryKey(HANDLE,KEY_INFORMATION_CLASS,PVOID,SIZE_T,PULONG);
NTSTATUS    NTAPI    NtEnumerateValueKey(HANDLE,ULONG,KEY_VALUE_INFORMATION_CLASS,PVOID,SIZE_T,PULONG);
NTSTATUS    NTAPI    NtQueryValueKey(HANDLE,PUNICODE_STRING,KEY_VALUE_INFORMATION_CLASS,PVOID,SIZE_T,PULONG);
NTSTATUS    NTAPI    NtQueryVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,SIZE_T,FS_INFORMATION_CLASS);
NTSTATUS    NTAPI    NtReadFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,SIZE_T,PLARGE_INTEGER,PULONG);
NTSTATUS    NTAPI    NtReleaseMutant(PHANDLE,SIZE_T *);
NTSTATUS    NTAPI    NtSetEvent(HANDLE,PULONG);
NTSTATUS    NTAPI    NtSetInformationProcess(HANDLE,PROCESS_INFORMATION_CLASS,PVOID,SIZE_T);
NTSTATUS    NTAPI    NtSetSystemPowerState(POWER_ACTION SystemAction,SYSTEM_POWER_STATE MinSystemState,SIZE_T Flags);
NTSTATUS    NTAPI    NtSetValueKey(HANDLE,PUNICODE_STRING,SIZE_T,SIZE_T,PVOID,SIZE_T);
NTSTATUS    NTAPI    NtShutdownSystem(SHUTDOWN_ACTION);
NTSTATUS    NTAPI    NtTerminateProcess(HANDLE,SIZE_T);
NTSTATUS    NTAPI    NtWaitForSingleObject(HANDLE,SIZE_T,const LARGE_INTEGER*);
NTSTATUS    NTAPI    NtWriteFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,SIZE_T,PLARGE_INTEGER,PULONG);
NTSTATUS    NTAPI    NtUnloadDriver(PUNICODE_STRING);
NTSTATUS    NTAPI    NtUnmapViewOfSection(HANDLE,PVOID);

NTSTATUS    NTAPI    RtlAdjustPrivilege(SIZE_T Id,SIZE_T Enable,SIZE_T ForCurrentThread,SIZE_T *WasEnabled);
PVOID       NTAPI    RtlAllocateHeap(HANDLE,SIZE_T,SIZE_T);
NTSTATUS    NTAPI    RtlAnsiStringToUnicodeString(PUNICODE_STRING,PANSI_STRING,SIZE_T);
HANDLE      NTAPI    RtlCreateHeap(SIZE_T,PVOID,SIZE_T,SIZE_T,PVOID,PRTL_HEAP_DEFINITION);
BOOLEAN     NTAPI    RtlCreateUnicodeString(PUNICODE_STRING,LPCWSTR);
NTSTATUS    NTAPI    RtlCreateUserThread(HANDLE,PSECURITY_DESCRIPTOR,SIZE_T,SIZE_T,SIZE_T,SIZE_T,PTHREAD_START_ROUTINE,PVOID,PHANDLE,PCLIENT_ID);
HANDLE      NTAPI    RtlDestroyHeap(HANDLE);
BOOLEAN     NTAPI    RtlDosPathNameToNtPathName_U(PCWSTR,PUNICODE_STRING,PCWSTR*,CURDIR*);
/* VOID     NTAPI    RtlExitUserThread(NTSTATUS); - NEVER use this unreliable call! */
NTSTATUS    NTAPI    RtlExpandEnvironmentStrings_U(PWSTR,const UNICODE_STRING*,UNICODE_STRING*,ULONG*);
NTSTATUS    NTAPI    RtlFindMessage(PVOID BaseAddress,ULONG Type,ULONG Language,ULONG MessageId,MESSAGE_RESOURCE_ENTRY **MessageResourceEntry);
VOID        NTAPI    RtlFreeAnsiString(PANSI_STRING);
BOOLEAN     NTAPI    RtlFreeHeap(HANDLE,SIZE_T,PVOID);
VOID        NTAPI    RtlFreeUnicodeString(PUNICODE_STRING);
VOID        NTAPI    RtlInitAnsiString(PANSI_STRING,PCSZ);
VOID        NTAPI    RtlInitUnicodeString(PUNICODE_STRING,PCWSTR);
PRTL_USER_PROCESS_PARAMETERS NTAPI RtlNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS*);
ULONG       NTAPI    RtlNtStatusToDosError(NTSTATUS);
NTSTATUS    NTAPI    RtlQueryEnvironmentVariable_U(PWSTR,PUNICODE_STRING,PUNICODE_STRING);
NTSTATUS    NTAPI    RtlQueryRegistryValues(ULONG RelativeTo,PCWSTR Path,PRTL_QUERY_REGISTRY_TABLE QueryTable,PVOID Context,PVOID Environment);
NTSTATUS    NTAPI    RtlSetEnvironmentVariable(PWSTR,PUNICODE_STRING,PUNICODE_STRING);
NTSTATUS    NTAPI    RtlSystemTimeToLocalTime(const LARGE_INTEGER* SystemTime,PLARGE_INTEGER LocalTime);
VOID        NTAPI    RtlTimeToTimeFields(PLARGE_INTEGER Time,PTIME_FIELDS TimeFields);
NTSTATUS    NTAPI    RtlUnicodeStringToAnsiString(PANSI_STRING,PUNICODE_STRING,SIZE_T);
NTSTATUS    NTAPI    RtlUnicodeToMultiByteN(PCHAR,ULONG,PULONG,PCWCH,ULONG);
NTSTATUS    NTAPI    RtlWriteRegistryValue(ULONG RelativeTo,PCWSTR Path,PCWSTR ValueName,ULONG ValueType,PVOID ValueData,ULONG ValueLength);

VOID        NTAPI    DbgBreakPoint(VOID);
NTSTATUS    NTAPI    LdrGetDllHandle(SIZE_T,SIZE_T,const UNICODE_STRING*,HMODULE*);
NTSTATUS    NTAPI    LdrGetProcedureAddress(PVOID,PANSI_STRING,SIZE_T,PVOID *);
NTSTATUS    NTAPI    ZwQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS,PVOID,SIZE_T,PULONG);
NTSTATUS    NTAPI    ZwTerminateThread(HANDLE,NTSTATUS);

/*
* This is the definition for the data structure that is passed in to
* FSCTL_MOVE_FILE
*/
#ifndef _WIN64
typedef struct {
     HANDLE            FileHandle; 
     ULONG             Reserved;   
     LARGE_INTEGER     StartVcn; 
     LARGE_INTEGER     TargetLcn;
     ULONG             NumVcns; 
     ULONG             Reserved1;    
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;
#else
typedef struct {
     HANDLE            FileHandle; 
     LARGE_INTEGER     StartVcn; 
     LARGE_INTEGER     TargetLcn;
     ULONGLONG         NumVcns; 
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;
#endif

/* This is the definition for a VCN/LCN (virtual cluster/logical cluster)
 * mapping pair that is returned in the buffer passed to 
 * FSCTL_GET_RETRIEVAL_POINTERS
 */
typedef struct {
    ULONGLONG            Vcn;
    ULONGLONG            Lcn;
} MAPPING_PAIR, *PMAPPING_PAIR;

/* This is the definition for the buffer that FSCTL_GET_RETRIEVAL_POINTERS
 * returns. It consists of a header followed by mapping pairs
 */
typedef struct {
    ULONG                NumberOfPairs;
    ULONGLONG            StartVcn;
    MAPPING_PAIR         Pair[1];
} GET_RETRIEVAL_DESCRIPTOR, *PGET_RETRIEVAL_DESCRIPTOR;

/* This is the definition of the buffer that FSCTL_GET_VOLUME_BITMAP
 * returns. It consists of a header followed by the actual bitmap data
 */
typedef struct {
    ULONGLONG            StartLcn;
    ULONGLONG            ClustersToEndOfVol;
    UCHAR                Map[1];
} BITMAP_DESCRIPTOR, *PBITMAP_DESCRIPTOR; 

#pragma pack(push, 1)
/*
* This is the definition for the data structure
* that is passed in to FSCTL_GET_NTFS_VOLUME_DATA.
*/
typedef struct _NTFS_DATA {
    LARGE_INTEGER VolumeSerialNumber;
    LARGE_INTEGER NumberSectors;
    LARGE_INTEGER TotalClusters;
    LARGE_INTEGER FreeClusters;
    LARGE_INTEGER TotalReserved;
    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    ULONG BytesPerFileRecordSegment;
    ULONG ClustersPerFileRecordSegment;
    LARGE_INTEGER MftValidDataLength;
    LARGE_INTEGER MftStartLcn;
    LARGE_INTEGER Mft2StartLcn;
    LARGE_INTEGER MftZoneStart;
    LARGE_INTEGER MftZoneEnd;
} NTFS_DATA, *PNTFS_DATA;
#pragma pack(pop)

#ifndef TAG
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#endif

#ifndef STATUS_BUFFER_OVERFLOW
#define STATUS_BUFFER_OVERFLOW    ((NTSTATUS)0x80000005)
#endif

/* Based on http://www.osronline.com/showthread.cfm?link=185567 */
typedef struct {
    DWORD dwSize;              /* the size of the structure, in bytes; 12 on NT 5.1, 32 on NT 6.1 */
    DWORD NtProductType;       /* NtProductWinNt, NtProductLanManNt, NtProductServer */
    UCHAR RecoveryFlag;        /* Defines whether "Time to display recovery options when needed" is checked or not. */
    UCHAR RecoveryMenuTimeout; /* Timeout, in seconds, of the recovery menu. */
    UCHAR BootSuccessFlag;     /* Set to 1 on successful boot. */
    UCHAR OrderlyShutdownFlag; /* Set to 1 on orderly shutdown. */
} BOOT_STATUS_DATA, *PBOOT_STATUS_DATA;

#endif /* _NTNDK_H_ */
