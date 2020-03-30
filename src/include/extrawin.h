/*
 *		Definitions for Windows which should not be needed
 */

#ifndef EXTRAWIN_H
#define EXTRAWIN_H 1

#if WNSC

#define FILE_ATTRIBUTE_SPARSE_FILE      0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT    0x00000400
#define FILE_ATTRIBUTE_COMPRESSED       0x00000800
#define FILE_ATTRIBUTE_ENCRYPTED        0x00004000

#define FILE_SHARE_READ                 0x00000001
#define FILE_SHARE_WRITE                0x00000002
#define FILE_SHARE_DELETE               0x00000004

#define DRIVE_UNKNOWN    0
#define DRIVE_NO_ROOT_DIR 1

/*
 *		Types missing
 */

typedef int SIZE_T;
typedef unsigned long long USN;

typedef struct _OSVERSIONINFOW {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  WCHAR szCSDVersion[128];
} OSVERSIONINFOW, *LPOSVERSIONINFOW;

/*
 *		Prototypes missing
 */

utf16_t ** WINAPI CommandLineToArgvW(const utf16_t*, const int*);
struct _TEB * WINAPI NtCurrentTeb(void);

/*
 *      Types to be correctly defined
 */

typedef int *NT_TIB;
typedef int *POWER_ACTION;
typedef int *SYSTEM_POWER_STATE;

#endif /* WNSC */

BOOLEAN WINAPI IncreaseGoogleAnalyticsCounter(const char*, const char*, const char*);

#endif /* EXTRAWIN_H */
