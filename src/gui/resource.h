/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  Resource header for graphical interface.
 */

#define IDD_MAIN                        101
#define IDD_ABOUT                       102
#define IDD_ABOUT_8_BIT                 103
#define IDD_SHUTDOWN                    104
#define IDD_CHECK_CONFIRM               105
#define IDI_APP                         106
#define IDI_DIR                         107
#define IDI_UNFRAGM                     110
#define IDI_FRAGM                       111
#define IDI_CMP                         112
#define IDI_MFT                         113
#define IDB_ABOUTBOX_PICTURE            127
#define IDB_ABOUTBOX_PICTURE_8_BIT      128

#define IDI_FIXED                       129
#define IDI_REMOVABLE                   130

#define IDR_ACCELERATOR2                131

#define IDI_SHUTDOWN                    140
#define IDR_MAIN_ACCELERATOR            150

/* for 24/32 bits per pixel (xp etc) */
#define IDB_TOOLBAR                     160
#define IDB_TOOLBAR_DISABLED            161
#define IDB_TOOLBAR_HIGHLIGHTED         162

/* for 8 bits per pixel displays (nt4 etc) */
#define IDB_TOOLBAR_8_BIT               163
#define IDB_TOOLBAR_DISABLED_8_BIT      164
#define IDB_TOOLBAR_HIGHLIGHTED_8_BIT   165

/* for 16 bits per pixel displays (w2k etc) */
#define IDB_TOOLBAR_16_BIT              166
#define IDB_TOOLBAR_DISABLED_16_BIT     167
#define IDB_TOOLBAR_HIGHLIGHTED_16_BIT  168

#define IDB_MENU_ICONS_15               170
#define IDB_MENU_ICONS_19               171
#define IDB_MENU_ICONS_25               172
#define IDB_MENU_ICONS_31               173

#define IDM_STATUSBAR                   500

#define IDC_CREDITS                     1020
#define IDC_LICENSE                     1021
#define IDC_HOMEPAGE                    1035

/* Confirm Dialog Constants */
#define IDC_MESSAGE                     1040
#define IDC_YES_BUTTON                  1041
#define IDC_NO_BUTTON                   1042
#define IDC_DELAY_MSG                   1043
#define IDC_SHUTDOWN_ICON               1044
#define IDC_PIC1                        1059
#define IDC_SHUTDOWN                    1060

/* menu item constants */
#define IDM_ACTION                      1100
#define IDM_ANALYZE                     1110
#define IDM_DEFRAG                      1120
#define IDM_QUICK_OPTIMIZE              1130
#define IDM_FULL_OPTIMIZE               1131
#define IDM_OPTIMIZE_MFT                1132
#define IDM_STOP                        1135
#define IDM_REPEAT_ACTION               1137
#define IDM_IGNORE_REMOVABLE_MEDIA      1140
#define IDM_RESCAN                      1150

#define IDM_WHEN_DONE                   1160
#define IDM_WHEN_DONE_NONE              1161
#define IDM_WHEN_DONE_EXIT              1162
#define IDM_WHEN_DONE_STANDBY           1163
#define IDM_WHEN_DONE_HIBERNATE         1164
#define IDM_WHEN_DONE_LOGOFF            1165
#define IDM_WHEN_DONE_REBOOT            1166
#define IDM_WHEN_DONE_SHUTDOWN          1167
/* IDM_EXIT must follow the last IDM_WHEN_DONE_xxx */
#define IDM_EXIT                        1170

#define IDM_REPORT                      1300
#define IDM_SHOW_REPORT                 1310

#define IDM_SETTINGS                    1400

#define IDM_TRANSLATIONS_FOLDER         1405
#define IDM_LANGUAGE                    1410
/* IDM_CFG_GUI must follow IDM_LANGUAGE */
#define IDM_CFG_GUI                     2680
#define IDM_CFG_GUI_FONT                2681
#define IDM_CFG_GUI_SETTINGS            2682
#define IDM_CFG_BOOT                    2685
#define IDM_CFG_BOOT_ENABLE             2686
#define IDM_CFG_BOOT_SCRIPT             2687
#define IDM_CFG_REPORTS                 2690

#define IDM_HELP                        2700
#define IDM_CONTENTS                    2710
#define IDM_BEST_PRACTICE               2720
#define IDM_FAQ                         2730
#define IDM_ABOUT                       2740

#define IDM_SELECT_ALL                  2750

/* preview menu items */
enum {
    IDM_PREVIEW = 5000,
//    IDM_PREVIEW_MOVE_FRONT,
    IDM_PREVIEW_LARGEST,
    IDM_PREVIEW_MATCHING,
//    IDM_PREVIEW_SKIP_PARTIAL,
    IDM_PREVIEW_LAST_ITEM    /* must always be the last entry */
};
