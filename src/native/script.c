/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
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
* UltraDefrag boot time (native) interface - boot time script parsing code.
*/

#include "defrag_native.h"

/**
 * @brief Indicates whether the script
 * execution must be aborted or not.
 */
int escape_flag = 0;

/**
 * @brief Processes the boot time script.
 * @param[in] filename the name of file,
 * with full path. If NULL is passed,
 * default boot time script will be used.
 * @return Zero for success, negative
 * value otherwise.
 */
int ProcessScript(wchar_t *filename)
{
    wchar_t *windir;
    wchar_t path[MAX_PATH + 1];
    wchar_t *buffer;
    size_t filesize, i, n;
    int line_detected;
    KBD_RECORD kbd_rec;
    
    scripting_mode = 1;
    escape_flag = 0;

    /* read script file entirely */
    if(filename == NULL){
        windir = winx_get_windows_directory();
        if(windir == NULL){
            winx_printf("\nProcessScript: cannot get %%windir%% path\n\n");
            return (-1);
        }
        (void)_snwprintf(path,MAX_PATH,L"%ws\\system32\\ud-boot-time.cmd",windir);
        path[MAX_PATH] = 0;
        winx_free(windir);
    } else {
        (void)_snwprintf(path,MAX_PATH,L"\\??\\%ws",filename);
        path[MAX_PATH] = 0;
    }

    buffer = winx_get_file_contents(path,&filesize);
    if(buffer == NULL)
        return 0; /* file is empty or some error */

    /* get file size, in characters */
    n = filesize / sizeof(wchar_t);
    if(n == 0)
        goto cleanup; /* file has no valuable contents */

    /* terminate buffer */
    buffer[n] = 0;
    
    /* replace all newline characters by zeros */
    for(i = 0; i < n; i++){
        if(buffer[i] == '\n' || buffer[i] == '\r')
            buffer[i] = 0;
    }
    /* skip first 0xFEFF character added by Notepad */
    if(buffer[0] == 0xFEFF)
        buffer[0] = 0;

    /* parse lines */
    line_detected = 0;
    for(i = 0; i < n; i++){
        if(buffer[i] != 0){
            if(!line_detected){
                (void)parse_command(buffer + i);
                line_detected = 1;
                /* check for escape key hits */
                if(escape_flag)
                    goto cleanup;
                if(winx_kb_read(&kbd_rec,0) == 0){
                    if(kbd_rec.wVirtualScanCode == 0x1)
                        goto cleanup;
                }
            }
        } else {
            line_detected = 0;
        }
    }

cleanup:
    winx_release_file_contents(buffer);
    return 0;
}
