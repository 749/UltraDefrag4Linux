/*
 *  WGX - Windows GUI Extended Library.
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
 * @file console.c
 * @brief Console.
 * @addtogroup Console
 * @{
 */

#include "wgx-internals.h"

/* definitions of routines missing on some Windows editions */
#define WIN_TMPF_TRUETYPE 0x04
typedef struct WIN_CONSOLE_FONT_INFO_EX {
  ULONG cbSize;
  DWORD nFont;
  COORD dwFontSize;
  UINT  FontFamily;
  UINT  FontWeight;
  WCHAR FaceName[LF_FACESIZE];
} WIN_CONSOLE_FONT_INFO_EX, *PWIN_CONSOLE_FONT_INFO_EX;
typedef BOOL (WINAPI *GET_CURRENT_CONSOLE_FONT_EX_PROC)(HANDLE hConsoleOutput,BOOL bMaximumWindow,PWIN_CONSOLE_FONT_INFO_EX lpConsoleCurrentFontEx);

/**
 * @brief Displays Unicode string
 * on the screen; intended to be used
 * in Windows console applications.
 * @details Based on two articles:
 * http://blogs.msdn.com/b/michkap/archive/2010/05/07/10008232.aspx
 * http://www.codeproject.com/Articles/34068/Unicode-Output-to-the-Windows-Console
 * @param[in] string the string to be displayed.
 * @param[in] f pointer to the output stream - 
 * can be either stdout or stderr.
 * @note
 * - On Windows NT 4.0 this routine always
 * converts the string to the OEM encoding
 * because NT 4.0 doesn't support the UTF-8
 * encoding.
 * - Since Windows 2000 the UTF-8 encoding is always
 * used when the output is redirected to a file.
 * - Since Windows Vista the UTF-8 encoding is used
 * when a True Type font is selected for the console
 * window.
 * - In all other cases the string becomes converted
 * to the OEM encoding; thus all the characters which
 * cannot be converted become replaced by the question
 * marks.
 */
void WgxPrintUnicodeString(wchar_t *string,FILE *f)
{
    OSVERSIONINFO osvi;
    HANDLE hOut;
    DWORD file_type;
    DWORD mode;
    HMODULE hKernel32Dll = NULL;
    GET_CURRENT_CONSOLE_FONT_EX_PROC pGetCurrentConsoleFontEx = NULL;
    WIN_CONSOLE_FONT_INFO_EX cfie;
    UINT old_code_page, new_code_page = CP_OEMCP;
    char *cnv_string = NULL;
    int length;

    if(string == NULL) return;
    
    old_code_page = GetConsoleOutputCP();

    memset(&osvi,0,sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);
    if(osvi.dwMajorVersion <= 4){
        /* Windows NT 4.0 supports OEM encoding only */
        goto convert;
    }
    
    /* output redirected to a file can be converted to UTF-8 */
    hOut = GetStdHandle(f == stdout ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    if(hOut == INVALID_HANDLE_VALUE){
        letrace("GetStdHandle failed");
    } else {
        file_type = GetFileType(hOut);
        if(file_type == FILE_TYPE_UNKNOWN && GetLastError() != NO_ERROR){
            letrace("GetFileType failed");
        } else {
            file_type &= ~(FILE_TYPE_REMOTE);
            if(file_type == FILE_TYPE_CHAR){
                if(!GetConsoleMode(hOut,&mode)){
                    if(GetLastError() == ERROR_INVALID_HANDLE){
                        new_code_page = CP_UTF8;
                        goto convert;
                    } else {
                        letrace("GetConsoleMode failed");
                    }
                }
            } else {
                new_code_page = CP_UTF8;
                goto convert;
            }
        }
    }
    
    /* if the console uses a TrueType font, UTF-8 can be used too */
    hKernel32Dll = LoadLibrary("kernel32.dll");
    if(hKernel32Dll == NULL){
        letrace("cannot load kernel32.dll library");
    } else {
        pGetCurrentConsoleFontEx = (GET_CURRENT_CONSOLE_FONT_EX_PROC)GetProcAddress(hKernel32Dll,"GetCurrentConsoleFontEx");
        if(pGetCurrentConsoleFontEx){
            memset(&cfie,0,sizeof(cfie));
            cfie.cbSize = sizeof(cfie);
            if(!pGetCurrentConsoleFontEx(hOut,FALSE,&cfie)){
                letrace("GetCurrentConsoleFontEx failed");
            } else {
                if((cfie.FontFamily & WIN_TMPF_TRUETYPE) == WIN_TMPF_TRUETYPE){
                    if(!SetConsoleOutputCP(CP_UTF8)){
                        letrace("SetConsoleOutputCP failed");
                    } else {
                        new_code_page = CP_UTF8;
                    }
                }
            }
        }
    }

    /* if the console runs inside of the PowerShell ISE, UTF-8 can be used too */
    /* however, this approach failed being tested on a Win7 driven machine */

convert:
    /* convert string to the acceptable encoding */
    length = (wcslen(string) + 1) * 4; /* should be enough */
    cnv_string = malloc(length);
    if(cnv_string == NULL){
        mtrace();
        goto done;
    }
    if(!WideCharToMultiByte(new_code_page,0,string,-1,cnv_string,length,NULL,NULL)){
        letrace("WideCharToMultiByte failed");
        goto done;
    }
    
    /* print the converted string */
    fprintf(f,"%s",cnv_string);

done:    
    /* cleanup */
    SetConsoleOutputCP(old_code_page);
    free(cnv_string);
}

/** @} */
