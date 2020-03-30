/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file keytrans.c
 * @brief Keyboard codes translation.
 * @note Only US keyboard layout is supported.
 * @addtogroup Keyboard
 * @{
 */

#include "prec.h"
#include "zenwinx.h"

typedef struct _CONTROL_KEY_STATE {
    USHORT ScanCode;
    USHORT Flags;
    DWORD  State;
} CONTROL_KEY_STATE, *PCONTROL_KEY_STATE;

typedef struct _ASCII_CHAR {
    USHORT ScanCode;
    UCHAR Normal;
    UCHAR Shift;
} ASCII_CHAR, *PASCII_CHAR;

CONTROL_KEY_STATE ControlKeyStates[] = {
    {0x2a, KEY_MAKE,           SHIFT_PRESSED},
    {0x2a, KEY_BREAK,          SHIFT_PRESSED},
    {0x36, KEY_MAKE,           SHIFT_PRESSED},
    {0x36, KEY_BREAK,          SHIFT_PRESSED},
    {0x1d, KEY_MAKE,           LEFT_CTRL_PRESSED},
    {0x1d, KEY_BREAK,          LEFT_CTRL_PRESSED},
    {0x1d, KEY_MAKE | KEY_E0,  RIGHT_CTRL_PRESSED},
    {0x1d, KEY_BREAK | KEY_E0, RIGHT_CTRL_PRESSED},
    {0x38, KEY_MAKE,           LEFT_ALT_PRESSED},
    {0x38, KEY_BREAK,          LEFT_ALT_PRESSED},
    {0x38, KEY_MAKE | KEY_E0,  RIGHT_ALT_PRESSED},
    {0x38, KEY_BREAK | KEY_E0, RIGHT_ALT_PRESSED}
};

ASCII_CHAR AsciiChars[] = {
    {0x1e, 'a', 'A'},
    {0x30, 'b', 'B'},
    {0x2e, 'c', 'C'},
    {0x20, 'd', 'D'},
    {0x12, 'e', 'E'},
    {0x21, 'f', 'F'},
    {0x22, 'g', 'G'},
    {0x23, 'h', 'H'},
    {0x17, 'i', 'I'},
    {0x24, 'j', 'J'},
    {0x25, 'k', 'K'},
    {0x26, 'l', 'L'},
    {0x32, 'm', 'M'},
    {0x31, 'n', 'N'},
    {0x18, 'o', 'O'},
    {0x19, 'p', 'P'},
    {0x10, 'q', 'Q'},
    {0x13, 'r', 'R'},
    {0x1f, 's', 'S'},
    {0x14, 't', 'T'},
    {0x16, 'u', 'U'},
    {0x2f, 'v', 'V'},
    {0x11, 'w', 'W'},
    {0x2d, 'x', 'X'},
    {0x15, 'y', 'Y'},
    {0x2c, 'z', 'Z'},

    {0x02, '1', '!'},
    {0x03, '2', '@'},
    {0x04, '3', '#'},
    {0x05, '4', '$'},
    {0x06, '5', '%'},
    {0x07, '6', '^'},
    {0x08, '7', '&'},
    {0x09, '8', '*'},
    {0x0a, '9', '('},
    {0x0b, '0', ')'},

    {0x29, '\'', '~'},
    {0x0c, '-',  '_'},
    {0x0d, '=',  '+'},
    {0x1a, '[',  '{'},
    {0x1b, ']',  '}'},
    {0x2b, '\\', '|'},
    {0x27, ';',  ':'},
    {0x28, '\'', '"'},
    {0x33, ',',  '<'},
    {0x34, '.',  '>'},
    {0x35, '/',  '?'},

    {0x39, ' ', ' '},

    {0x0e, 0x08, 0x08}, /* backspace */
    {0x1c, '\r', '\r'},

    {0x37, '*', '*'},
    {0x4a, '-', '-'},
    {0x4e, '+', '+'}
};

DWORD dwControlKeyState = 0;

/**
 * @internal
 */
static DWORD IntUpdateControlKeyState(PKEYBOARD_INPUT_DATA InputData)
{
    int i;
    
    for(i = 0; i < sizeof(ControlKeyStates) / sizeof(ControlKeyStates[0]); i++){
        if(ControlKeyStates[i].ScanCode == InputData->MakeCode){
            if(ControlKeyStates[i].Flags == InputData->Flags){
                if(InputData->Flags & KEY_BREAK)
                    dwControlKeyState &= ~ControlKeyStates[i].State;
                else
                    dwControlKeyState |= ControlKeyStates[i].State;
                break;
            }
        }
    }

    return dwControlKeyState;
}

/**
 * @internal
 */
static UCHAR IntGetAsciiChar(PKEYBOARD_INPUT_DATA InputData)
{
    int i;
    
    for(i = 0; i < sizeof(AsciiChars) / sizeof(AsciiChars[0]); i++){
        if(AsciiChars[i].ScanCode == InputData->MakeCode){
            if(dwControlKeyState & SHIFT_PRESSED)
                return AsciiChars[i].Shift;
            return AsciiChars[i].Normal;
        }
    }

    return 0;
}

/**
 * @internal
 */
void IntTranslateKey(PKEYBOARD_INPUT_DATA InputData, KBD_RECORD *kbd_rec)
{
    kbd_rec->wVirtualScanCode = InputData->MakeCode;
    kbd_rec->dwControlKeyState = IntUpdateControlKeyState(InputData);
    if(InputData->Flags & KEY_E0) kbd_rec->dwControlKeyState |= ENHANCED_KEY;
    kbd_rec->AsciiChar = IntGetAsciiChar(InputData);
    kbd_rec->bKeyDown = (InputData->Flags & KEY_BREAK) ? FALSE : TRUE;
}

/** @} */
