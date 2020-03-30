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
 * @file stdio.c
 * @brief Console I/O.
 * @addtogroup Console
 * @{
 */

#include "zenwinx.h"

#define INTERNAL_BUFFER_SIZE 2048

int kb_read(PKEYBOARD_INPUT_DATA pKID,int msec_timeout);
void IntTranslateKey(PKEYBOARD_INPUT_DATA InputData, KBD_RECORD *kbd_rec);

/**
 * @brief Displays ANSI string on the screen.
 * @details This routine requires no library
 * initialization, therefore may be used when
 * it fails.
 * @param[in] string the string to be displayed.
 * @note Does not recognize \\b character.
 */
void winx_print(char *string)
{
    ANSI_STRING as;
    UNICODE_STRING us;
    int i;
    
    /*
    * Use neither memory allocation nor debugging
    * routines: they may be not available here.
    */

    if(!string)
        return;
    
    /* use slower winx_putch based code if \t detected */
    if(strchr(string,'\t'))
        goto second_algorithm;

    RtlInitAnsiString(&as,string);
    if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) == STATUS_SUCCESS){
        NtDisplayString(&us);
        RtlFreeUnicodeString(&us);
    } else {
second_algorithm:
        for(i = 0; string[i]; i ++)
            winx_putch(string[i]);
    }
}

/**
 * @brief putch() native equivalent.
 * @note Does not recognize \\b character.
 */
int winx_putch(int ch)
{
    UNICODE_STRING us;
    int i;
    short t[DEFAULT_TAB_WIDTH + 1];
    short s[2];

    /*
    * Use neither memory allocation nor debugging
    * routines: they may be not available here.
    */
    
    if(ch == '\t'){
        for(i = 0; i < DEFAULT_TAB_WIDTH; i++) t[i] = 0x20;
        t[DEFAULT_TAB_WIDTH] = 0;
        RtlInitUnicodeString(&us,t);
    } else {
        s[0] = (short)ch; s[1] = 0;
        RtlInitUnicodeString(&us,s);
    }
    NtDisplayString(&us);
    return ch;
}

/**
 * @brief puts() native equivalent.
 * @note Does not recognize \\b character.
 */
int winx_puts(const char *string)
{
    if(!string) return (-1);
    return winx_printf("%s\n",string) ? 0 : (-1);
}

/**
 * @brief printf() native equivalent.
 * @note Does not recognize \\b character.
 */
int winx_printf(const char *format, ...)
{
    va_list arg;
    char *string;
    int result = 0;
    
    if(format){
        va_start(arg,format);
        string = winx_vsprintf(format,arg);
        if(string){
            winx_print(string);
            result = strlen(string);
            winx_heap_free(string);
        }
        va_end(arg);
    }
    
    return result;
}

/**
 * @brief Checks the console for keyboard input.
 * @details Waits for an input during the specified time interval.
 * @param[in] msec the time interval, in milliseconds.
 * @return If any key was pressed, the return value is
 * the ascii character or zero for the control keys.
 * Negative value indicates failure.
 * @note If an INFINITE time constant is passed, 
 * the time-out interval never elapses.
 */
int winx_kbhit(int msec)
{
    KBD_RECORD kbd_rec;

    return winx_kb_read(&kbd_rec,msec);
}

/**
 * @brief Low level winx_kbhit() equivalent.
 * @param[in] kbd_rec pointer to structure receiving key information.
 * @param[in] msec the timeout interval, in milliseconds.
 * @return If any key was pressed, the return value is
 * the ascii character or zero for the control keys.
 * Negative value indicates failure.
 * @note If an INFINITE time constant is passed, 
 * the time-out interval never elapses.
 */
int winx_kb_read(KBD_RECORD *kbd_rec,int msec)
{
    KEYBOARD_INPUT_DATA kbd;
    
    DbgCheck1(kbd_rec,"winx_kb_read",-1);

    do{
        if(kb_read(&kbd,msec) < 0) return (-1);
        IntTranslateKey(&kbd,kbd_rec);
    } while(!kbd_rec->bKeyDown); /* skip key up events */
    return (int)kbd_rec->AsciiChar;
}

/**
 * @brief Checks the console for the 'Break'
 * character on the keyboard input.
 * @details Waits for an input during the specified time interval.
 * @param[in] msec the time interval, in milliseconds.
 * @return If the 'Break' key was pressed, the return value is zero.
 * Otherwise it returns negative value.
 * @note If an INFINITE time constant is passed, 
 * the time-out interval never elapses.
 */
int winx_breakhit(int msec)
{
    KEYBOARD_INPUT_DATA kbd;
    KBD_RECORD kbd_rec;

    do{
        if(kb_read(&kbd,msec) < 0) return (-1);
        IntTranslateKey(&kbd,&kbd_rec);
    } while(!kbd_rec.bKeyDown); /* skip key up events */
    if((kbd.Flags & KEY_E1) && (kbd.MakeCode == 0x1d)) return 0;
    /*winx_printf("\nwinx_breakhit(): Other key was pressed.\n");*/
    return (-1);
}

/**
 * @brief getch() native equivalent.
 */
int winx_getch(void)
{
    KBD_RECORD kbd_rec;
    
    return winx_kb_read(&kbd_rec,INFINITE);
}

/**
 * @brief getche() native equivalent.
 * @note 
 * - Does not recognize \\b character.
 * - Does not recognize tabulation.
 */
int winx_getche(void)
{
    int ch;

    ch = winx_getch();
    if(ch != -1 && ch != 0 && ch != 0x08) /* skip backspace */
        winx_putch(ch);
    return ch;
}

/**
 * @brief gets() native equivalent
 * with limited number of characters to read.
 * @param[out] string the storage for the input string.
 * @param[in] n the maximum number of characters to read.
 * @return Number of characters read including terminal zero.
 * Negative value indicates failure.
 * @note Does not recognize tabulation.
 */
int winx_gets(char *string,int n)
{
    return winx_prompt_ex(NULL,string,n,NULL);
}

/**
 * @brief Initializes commands history.
 * @param[in] h pointer to structure holding
 * the commands history.
 */
void winx_init_history(winx_history *h)
{
    if(h == NULL){
        DebugPrint("winx_init_history: h = NULL!");
        return;
    }
    h->head = h->current = NULL;
    h->n_entries = 0;
}

/**
 * @brief Destroys commands history.
 * @param[in] h pointer to structure holding
 * the commands history.
 * @note There is no need to call winx_init_history()
 * after this call to reinitialize the structure.
 */
void winx_destroy_history(winx_history *h)
{
    winx_history_entry *entry;
    
    if(h == NULL){
        DebugPrint("winx_destroy_history: h = NULL!");
        return;
    }

    for(entry = h->head; entry != NULL; entry = entry->next){
        if(entry->string) winx_heap_free(entry->string);
        if(entry->next == h->head) break;
    }
    winx_list_destroy((list_entry **)(void *)&h->head);
    h->current = NULL;
    h->n_entries = 0;
}

/**
 * @internal
 * @brief Adds an entry to commands history list.
 */
static void winx_add_history_entry(winx_history *h,char *string)
{
    winx_history_entry *entry, *last_entry = NULL;
    int length;
    
    if(h == NULL || string == NULL) return;
    
    if(h->head) last_entry = h->head->prev;
    entry = (winx_history_entry *)winx_list_insert_item((list_entry **)(void *)&h->head,
        (list_entry *)last_entry,sizeof(winx_history_entry));
    if(entry == NULL){
        DebugPrint("winx_add_winx_history_entry: cannot allocate %u bytes of memory",sizeof(winx_history_entry));
        winx_printf("\nNot enough memory for winx_add_winx_history_entry()!\n");
        return;
    }
    
    entry->string = winx_strdup(string);
    if(entry->string == NULL){
        length = strlen(string) + 1;
        DebugPrint("winx_add_winx_history_entry: cannot allocate %u bytes of memory",length);
        winx_printf("\nCannot allocate %u bytes of memory for winx_add_winx_history_entry()!\n",length);
        winx_list_remove_item((list_entry **)(void *)&h->head,(list_entry *)entry);
    } else {
        h->n_entries ++;
        h->current = entry;
    }
}

/**
 * @brief Displays prompt on the screen and waits for
 * the user input. When user hits the return key it
 * fills the string pointed by the second parameter 
 * by characters read.
 * @param[in] prompt the string to be printed as prompt.
 * @param[out] string the storage for the input string.
 * @param[in] n the size of the string, in characters.
 * @param[in,out] h pointer to structure holding
 * the commands history. May be NULL.
 * @return Number of characters read including terminal zero.
 * Negative value indicates failure.
 * @note
 * - Recognizes properly both backslash and escape keys.
 * - The sentence above works fine only when user input
 * stands in a single line of the screen.
 * - Recognizes arrow keys to walk through commands history.
 * @note Does not recognize tabulation.
 */
int winx_prompt_ex(char *prompt,char *string,int n,winx_history *h)
{
    KEYBOARD_INPUT_DATA kbd;
    KBD_RECORD kbd_rec;
    char *buffer;
    int buffer_length;
    char format[16];
    int i, ch, line_length;
    int history_listed_to_the_last_entry = 0;

    if(!string){
        winx_printf("\nwinx_prompt_ex: invalid string!\n");
        return (-1);
    }
    if(n <= 0){
        winx_printf("\nwinx_prompt_ex: invalid string length %d!\n",n);
        return (-1);
    }
    
    if(!prompt) prompt = "";
    buffer_length = strlen(prompt) + n;
    buffer = winx_heap_alloc(buffer_length);
    if(buffer == NULL){
        winx_printf("\nNot enough memory for winx_prompt_ex()!\n");
        return (-1);
    }
    
    winx_printf("%s",prompt);
    
    /* keep string always null terminated */
    RtlZeroMemory(string,n);
    for(i = 0; i < (n - 1); i ++){
        /* read keyboard until an ordinary character appearance */
        do {
            do{
                if(kb_read(&kbd,INFINITE) < 0) goto fail;
                IntTranslateKey(&kbd,&kbd_rec);
            } while(!kbd_rec.bKeyDown);
            ch = (int)kbd_rec.AsciiChar;
            /*
            * Truncate the string if either backspace or escape pressed.
            * Walk through history if one of arrow keys pressed.
            */
            if(ch == 0x08 || kbd_rec.wVirtualScanCode == 0x1 || \
              kbd_rec.wVirtualScanCode == 0x48 || kbd_rec.wVirtualScanCode == 0x50){
                line_length = strlen(prompt) + strlen(string);
                /* handle escape key */
                if(kbd_rec.wVirtualScanCode == 0x1){
                    /* truncate the string if escape pressed */
                    RtlZeroMemory(string,n);
                    i = 0;
                }
                /* handle backspace key */
                if(ch == 0x08){
                    /*
                    * make the string one character shorter
                    * if backspace pressed
                    */
                    if(i > 0){
                        i--;
                        string[i] = 0;
                    }
                }
                /* handle arrow keys */
                if(h){
                    if(h->head && h->current){
                        if(kbd_rec.wVirtualScanCode == 0x48){
                            /* list history back */
                            if(h->current == h->head->prev && \
                              !history_listed_to_the_last_entry){
                                /* set the flag and don't list back */
                                history_listed_to_the_last_entry = 1;
                            } else {
                                if(h->current != h->head)
                                    h->current = h->current->prev;
                                history_listed_to_the_last_entry = 0;
                            }
                            if(h->current->string){
                                RtlZeroMemory(string,n);
                                strcpy(string,h->current->string);
                                i = strlen(string);
                            }
                        } else if(kbd_rec.wVirtualScanCode == 0x50){
                            /* list history forward */
                            if(h->current->next != h->head){
                                h->current = h->current->next;
                                if(h->current->string){
                                    RtlZeroMemory(string,n);
                                    strcpy(string,h->current->string);
                                    i = strlen(string);
                                }
                                if(h->current == h->head->prev)
                                    history_listed_to_the_last_entry = 1;
                                else
                                    history_listed_to_the_last_entry = 0;
                            }
                        }
                    }
                }
                
                /* clear history_listed_to_the_last_entry flag */
                if(ch == 0x08 || kbd_rec.wVirtualScanCode == 0x1)
                    history_listed_to_the_last_entry = 0;
                
                /* redraw the prompt */
                _snprintf(buffer,buffer_length,"%s%s",prompt,string);
                buffer[buffer_length - 1] = 0;
                _snprintf(format,sizeof(format),"\r%%-%us",line_length);
                format[sizeof(format) - 1] = 0;
                winx_printf(format,buffer);
                /*
                * redraw the prompt again to set carriage position
                * exactly behind the string printed
                */
                line_length = strlen(prompt) + strlen(string);
                _snprintf(format,sizeof(format),"\r%%-%us",line_length);
                format[sizeof(format) - 1] = 0;
                winx_printf(format,buffer);
                continue;
            }
        } while(ch == 0 || ch == 0x08 || kbd_rec.wVirtualScanCode == 0x1 || \
              kbd_rec.wVirtualScanCode == 0x48 || kbd_rec.wVirtualScanCode == 0x50);
        
        /* print a character read */
        winx_putch(ch);

        /* return when \r character appears */
        if(ch == '\r'){
            winx_putch('\n');
            goto done;
        }
        
        /* we have an ordinary character, append it to the string */
        string[i] = (char)ch;
        
        /* clear the flag in case of ordinary characters typed */
        history_listed_to_the_last_entry = 0;
    }
    winx_printf("\nwinx_prompt_ex: buffer overflow!\n");

done:
    winx_heap_free(buffer);
    /* add nonempty strings to the history */
    if(string[0]){
        winx_add_history_entry(h,string);
    }
    return (i+1);
    
fail:
    winx_heap_free(buffer);
    return (-1);
}

/**
 * @brief Simplified analog of winx_prompt_ex() call.
 * Has no support of commands history.
 * @note Does not recognize tabulation.
 */
int winx_prompt(char *prompt,char *string,int n)
{
    return winx_prompt_ex(prompt,string,n,NULL);
}

/* returns 1 if break or escape was pressed, zero otherwise */
static int print_line(char *line_buffer,char *prompt,int max_rows,int *rows_printed,int last_line)
{
    KBD_RECORD kbd_rec;
    int escape_detected = 0;
    int break_detected = 0;

    winx_printf("%s\n",line_buffer);
    (*rows_printed) ++;
    
    if(*rows_printed == max_rows && !last_line){
        *rows_printed = 0;
        winx_printf("\n%s\n",prompt);
        /* wait for any key */
        if(winx_kb_read(&kbd_rec,INFINITE) < 0){
            return 1; /* break in case of errors */
        }
        /* check for escape */
        if(kbd_rec.wVirtualScanCode == 0x1){
            escape_detected = 1;
        } else if(kbd_rec.wVirtualScanCode == 0x1d){
            /* distinguish between control keys and break key */
            if(!(kbd_rec.dwControlKeyState & LEFT_CTRL_PRESSED) && \
              !(kbd_rec.dwControlKeyState & RIGHT_CTRL_PRESSED)){
                break_detected = 1;
            }
        }
        winx_printf("\n");
        if(escape_detected || break_detected) return 1;
    }
    return 0;
}

/**
 * @brief Displays text on the screen,
 * divided to pages if requested so.
 * Accepts array of strings as an input.
 * @param[in] strings - array of strings
 * to be displayed. Last entry of it
 * must be NULL to indicate the end of
 * array.
 * @param[in] line_width - maximum line
 * width, in characters.
 * @param[in] max_rows - maximum number
 * of lines on the screen.
 * @param[in] prompt - the string to be
 * displayed as a prompt to hit any key
 * to list forward.
 * @param[in] divide_to_pages - boolean
 * value indicating whether the text
 * must be divided to pages or not.
 * If this parameter is zero, all others,
 * except of the first one, will be
 * ignored.
 * @note If user hits Escape or Pause
 * at the prompt, text listing breaks
 * immediately.
 * @return Zero for success, negative
 * value otherwise.
 * @note Does not recognize \\b character.
 */
int winx_print_array_of_strings(char **strings,int line_width,int max_rows,char *prompt,int divide_to_pages)
{
    int i, j, k, index, length;
    char *line_buffer, *second_buffer;
    int n, r;
    int rows_printed;
    
    /* check the main parameter for correctness */
    if(!strings){
        DebugPrint("winx_print_array_of_strings: strings = NULL!");
        return (-1);
    }
    
    /* handle situation when text must be displayed entirely */
    if(!divide_to_pages){
        for(i = 0; strings[i] != NULL; i++)
            winx_printf("%s\n",strings[i]);
        return 0;
    }

    /* check other parameters */
    if(!line_width){
        DebugPrint("winx_print_array_of_strings: line_width = 0!");
        return (-1);
    }
    if(!max_rows){
        DebugPrint("winx_print_array_of_strings: max_rows = 0!");
        return (-1);
    }
    if(prompt == NULL) prompt = DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY;
    
    /* allocate space for prompt on the screen */
    max_rows -= 4;
    
    /* allocate memory for line buffer */
    line_buffer = winx_heap_alloc(line_width + 1);
    if(!line_buffer){
        DebugPrint("winx_print_array_of_strings: cannot allocate %u bytes of memory",
            line_width + 1);
        return (-1);
    }
    /* allocate memory for second ancillary buffer */
    second_buffer = winx_heap_alloc(line_width + 1);
    if(!second_buffer){
        DebugPrint("winx_print_array_of_strings: cannot allocate %u bytes of memory",
            line_width + 1);
        winx_heap_free(line_buffer);
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
    winx_heap_free(line_buffer);
    winx_heap_free(second_buffer);
    return 0;
}

/** @} */
