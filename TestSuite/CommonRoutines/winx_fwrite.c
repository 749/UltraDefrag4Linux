/*
 *  Test suite for winx_fwrite().
 *  Copyright (c) 2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

static int winx_fwrite_test_buffered_io(char *filename,char **strings,int test_number,int buffer_size);
static int winx_fwrite_test_compare(char *filename,char **strings,int total_length,int test_number);

/*
* accepts native filenames like that:
* \??\C:\test.txt
* returns zero on success, negative value otherwise
*/
int winx_fwrite_test(char *filename)
{
    WINX_FILE *f;
    char *strings[] = {
        "Sherlock Holmes took his bottle from the corner of the mantelpiece, \r\n",
        "and his hypodermic syringe from its neat morocco case. With his long, \r\n",
        "white, nervous fingers he adjusted the delicate needle and rolled back \r\n",
        "his left shirtcuff. For some little time his eyes rested thoughtfully \r\n",
        "upon the sinewy forearm and wrist, all dotted and scarred with innumerable \r\n",
        NULL
    };
    int total_length;
    int i;
    
    /* 1. test not buffered i/o */
    f = winx_fopen(filename,"w");
    if(f == NULL){
        DebugPrint("@winx_fwrite_test: winx_fopen failed in test 1\n");
        return (-1);
    }
    total_length = 0;
    for(i = 0; strings[i]; i++){
        total_length += strlen(strings[i]);
        if(!winx_fwrite(strings[i],sizeof(char),strlen(strings[i]),f)){
            DebugPrint("@winx_fwrite_test: winx_fwrite failed in test 1 for %s\n",strings[i]);
            winx_fclose(f);
            return (-1);
        }
    }
    winx_fclose(f);
    if(winx_fwrite_test_compare(filename,strings,total_length,1) < 0){
        return (-1);
    }
    
    /* 2. test buffered i/o with negative buffer size */
    if(winx_fwrite_test_buffered_io(filename,strings,2,-100) < 0)
        return (-1);
    
    /* 3. test buffered i/o with zero buffer size */
    if(winx_fwrite_test_buffered_io(filename,strings,3,0) < 0)
        return (-1);
    
    /* 4. test buffered i/o with small buffer size */
    if(winx_fwrite_test_buffered_io(filename,strings,4,10) < 0)
        return (-1);
    
    /* 5. test buffered i/o with medium buffer size */
    if(winx_fwrite_test_buffered_io(filename,strings,5,170) < 0)
        return (-1);
    
    /* 6. test buffered i/o with large buffer size */
    if(winx_fwrite_test_buffered_io(filename,strings,6,10000) < 0)
        return (-1);
    
    /* 7. test buffered i/o with buffer size equal to the length of the first string */
    if(winx_fwrite_test_buffered_io(filename,strings,7,strlen(strings[0])) < 0)
        return (-1);
    
    /* 8. test buffered i/o with buffer size equal to the length of the first two strings */
    if(winx_fwrite_test_buffered_io(filename,strings,8,strlen(strings[0]) + strlen(strings[1])) < 0)
        return (-1);
    
    /* 9. test buffered i/o with buffer size equal to the length of the first three strings */
    if(winx_fwrite_test_buffered_io(filename,strings,9,strlen(strings[0]) + strlen(strings[1]) + strlen(strings[2])) < 0)
        return (-1);
    
    /* 10. test buffered i/o with buffer size equal to the length of all strings */
    if(winx_fwrite_test_buffered_io(filename,strings,10,total_length) < 0)
        return (-1);
    
    DebugPrint("@winx_fwrite_test: all tests passed!\n");
    return 0;
}

static int winx_fwrite_test_buffered_io(char *filename,char **strings,int test_number,int buffer_size)
{
    WINX_FILE *f;
    int total_length;
    int i;

    f = winx_fbopen(filename,"w",buffer_size);
    if(f == NULL){
        DebugPrint("@winx_fwrite_test: winx_fbopen failed in test %i\n",test_number);
        return (-1);
    }
    total_length = 0;
    for(i = 0; strings[i]; i++){
        total_length += strlen(strings[i]);
        if(!winx_fwrite(strings[i],sizeof(char),strlen(strings[i]),f)){
            DebugPrint("@winx_fwrite_test: winx_fwrite failed in test %i for %s\n",test_number,strings[i]);
            winx_fclose(f);
            return (-1);
        }
    }
    winx_fclose(f);
    if(winx_fwrite_test_compare(filename,strings,total_length,test_number) < 0){
        return (-1);
    }
    return 0;
}

static int winx_fwrite_test_compare(char *filename,char **strings,int total_length,int test_number)
{
    int i;
    char *written_strings;
    size_t written_data_length;
    int j, n;
    
    /* read the entire file contents */
    written_strings = (char *)winx_get_file_contents(filename,&written_data_length);
    if(written_strings == NULL){
        DebugPrint("@winx_fwrite_test: winx_get_file_contents failed in test %i\n",test_number);
        return (-1);
    }
    if(written_data_length != total_length){
        DebugPrint("@winx_fwrite_test: size mismatch in test %i\n",test_number);
        winx_release_file_contents((void *)written_strings);
        return (-1);
    }
    /* compare the file contents with original data */
    j = 0;
    for(i = 0; strings[i]; i++){
        n = strlen(strings[i]);
        if(memcmp(written_strings + j,strings[i],n) != 0){
            DebugPrint("@winx_fwrite_test: contents mismatch in test %i\n",test_number);
            winx_release_file_contents((void *)written_strings);
            return (-1);
        }
        j += n;
    }
    winx_release_file_contents((void *)written_strings);
    DebugPrint("@winx_fwrite_test: test %i passed\n",test_number);
    return 0;
}
