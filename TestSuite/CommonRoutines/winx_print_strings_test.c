/*
 *  Test suite for winx_print_array_of_strings().
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#define DEFAULT_PROMPT_TO_HIT_ANY_KEY "      Hit any key to display next page..."
#define DEFAULT_TAB_WIDTH 2
int __cdecl winx_print_array_of_strings(char **strings,int line_width,int max_rows,char *prompt,int divide_to_pages);

char *strings[] = {
    "Sherlock Holmes took his bottle from the corner of the mantelpiece, "
    "and his hypodermic syringe from its neat morocco case. With his long, "
    "white, nervous fingers he adjusted the delicate needle and rolled back "
    "his left shirtcuff. For some little time his eyes rested thoughtfully "
    "upon the sinewy forearm and wrist, all dotted and scarred with innumerable "
    "puncture-marks. Finally, he thrust the sharp point home, pressed down the tiny "
    "piston, and sank back into the velvet-lined armchair with a long sigh of satisfaction.",
    "",
    "Three times a day for many months I had witnessed this performance, "
    "but custom had not reconciled my mind to it. On the contrary, from "
    "day to day I had become more irritable at the sight, and my conscience "
    "swelled nightly within me at the thought that I had lacked the courage "
    "to protest. Again and again I had registered a vow that I should deliver "
    "my soul upon the subject; but there was that in the cool, nonchalant air "
    "of my companion which made him the last man with whom one would care to "
    "take anything approaching to a liberty. His great powers, his masterly manner, "
    "and the experience which I had had of his many extraordinary qualities, all "
    "made me diffident and backward in crossing him. ",
    "",
    "Yet upon that afternoon, whether it was the Beaune which I had taken with "
    "my lunch or the additional exasperation produced by the extreme deliberation "
    "of his manner, I suddenly felt that I could hold out no longer. ",
    "",
    "this_is_a_very_very_long_word_qwertyuiop[]asdfghjkl;'zxcvbnm,./1234567890~!@#$#$#%$^%$^%&^&(*&(**)(*",
    "",
    "first line \rsecond\nthird\r\n4th\n\r5th\n\n\n8th",
    "",
    "before_tab\tafter_tab\t\tafter_two_tabs",
    "",
    NULL
};

int main(void)
{
    winx_print_array_of_strings(strings,60,24,"    Hit any key to continue...",1);
    printf("Test completed, hit any key to exit...\n");
    getch();
    return 0;
}

/* returns 1 if break or escape was pressed, zero otherwise */
static int print_line(char *line_buffer,char *prompt,int max_rows,int *rows_printed,int last_line)
{
    printf("%s\n",line_buffer);
    (*rows_printed) ++;
    
    if(*rows_printed == max_rows && !last_line){
        *rows_printed = 0;
        printf("\n%s\n",prompt);
        if(getch() == 0x1b){ /* esc */
            printf("\n");
            return 1;
        }
        printf("\n");
    }
    return 0;
}

int __cdecl winx_print_array_of_strings(char **strings,int line_width,int max_rows,char *prompt,int divide_to_pages)
{
    int i, j, k, index, length;
    char *line_buffer, *second_buffer;
    int n, r;
    int rows_printed;
    
    if(!strings) return (-1);
    
    /* handle situation when text must be displayed entirely */
    if(!divide_to_pages){
        for(i = 0; strings[i] != NULL; i++)
            printf("%s\n",strings[i]);
        return 0;
    }

    if(!line_width || !max_rows) return (-1);
    if(prompt == NULL) prompt = DEFAULT_PROMPT_TO_HIT_ANY_KEY;
    
    /* allocate space for prompt on the screen */
    max_rows -= 3;
    
    /* allocate memory for line buffer */
    line_buffer = malloc(line_width + 1);
    if(!line_buffer){
        printf("Cannot allocate %u bytes of memory for winx_print_array_of_strings()!",
            line_width + 1);
        return (-1);
    }
    /* allocate memory for second ancillary buffer */
    second_buffer = malloc(line_width + 1);
    if(!second_buffer){
        printf("Cannot allocate %u bytes of memory for winx_print_array_of_strings()!",
            line_width + 1);
        free(line_buffer);
        return (-1);
    }
    
    /* start to display strings */
    rows_printed = 0;
    for(i = 0; strings[i] != NULL; i++){
        line_buffer[0] = 0;
        index = 0;
        length = strlen(strings[i]);
        for(j = 0; j < length; j++){
            /* handle \n, \r, \r\n, \n\r sequencies */
            n = r = 0;
            if(strings[i][j] == '\n') n = 1;
            else if(strings[i][j] == '\r') r = 1;
            if(n || r){
                /* print buffer */
                line_buffer[index] = 0;
                if(print_line(line_buffer,prompt,max_rows,&rows_printed,0))
                    goto cleanup;
                /* reset buffer */
                line_buffer[0] = 0;
                index = 0;
                /* skip sequence */
                j++;
                if(j == length) goto print_rest_of_string;
                if((strings[i][j] == '\n' && r) || (strings[i][j] == '\r' && n)){
                    continue;
                } else {
                    if(strings[i][j] == '\n' || strings[i][j] == '\r'){
                        /* process repeating new lines */
                        j--;
                        continue;
                    }
                    /* we have an ordinary character or tabulation -> process them */
                }
            }
            /* handle horizontal tabulation by replacing it by DEFAULT_TAB_WIDTH spaces */
            if(strings[i][j] == '\t'){
                for(k = 0; k < DEFAULT_TAB_WIDTH; k++){
                    line_buffer[index] = 0x20;
                    index ++;
                    if(index == line_width){
                        if(j == length - 1) goto print_rest_of_string;
                        line_buffer[index] = 0;
                        if(print_line(line_buffer,prompt,max_rows,&rows_printed,0))
                            goto cleanup;
                        line_buffer[0] = 0;
                        index = 0;
                        break;
                    }
                }
                continue;
            }
            /* handle ordinary characters */
            line_buffer[index] = strings[i][j];
            index ++;
            if(index == line_width){
                if(j == length - 1) goto print_rest_of_string;
                line_buffer[index] = 0;
                /* break line between words, if possible */
                for(k = index - 1; k > 0; k--){
                    if(line_buffer[k] == 0x20) break;
                }
                if(line_buffer[k] == 0x20){ /* space character found */
                    strcpy(second_buffer,line_buffer + k + 1);
                    line_buffer[k] = 0;
                    if(print_line(line_buffer,prompt,max_rows,&rows_printed,0))
                        goto cleanup;
                    strcpy(line_buffer,second_buffer);
                    index = strlen(line_buffer);
                } else {
                    if(print_line(line_buffer,prompt,max_rows,&rows_printed,0))
                        goto cleanup;
                    line_buffer[0] = 0;
                    index = 0;
                }
            }
        }
print_rest_of_string:
        line_buffer[index] = 0;
        if(print_line(line_buffer,prompt,max_rows,&rows_printed,
            (strings[i+1] == NULL) ? 1 : 0)) goto cleanup;
    }

cleanup:
    free(line_buffer);
    free(second_buffer);
    return 0;
}
