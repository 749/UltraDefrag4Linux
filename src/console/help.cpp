//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2015 Dmitri Arkhangelski (dmitriar@gmail.com).
//  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//////////////////////////////////////////////////////////////////////////

/**
 * @file help.cpp
 * @brief Help screen.
 * @addtogroup Help
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                            Help screen
// =======================================================================

void show_help(void)
{
    printf(
        "===============================================================================\n"
        VERSIONINTITLE " - a powerful disk defragmentation tool for Windows NT\n"
        "Copyright (c) UltraDefrag Development Team, 2007-2015.\n"
        "\n"
        "===============================================================================\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n"
        "===============================================================================\n"
        "\n"
        "Usage: udefrag [command] [options] [driveletter:] [path(s)]\n"
        "\n"
        "  The default action is to display this help screen.\n"
        "\n"
        "Commands:\n"
        "  -a,  --analyze                      analyze specified objects\n"
        "  -o,  --optimize                     perform full optimization\n"
        "  -q,  --quick-optimize               perform quick optimization\n"
        "       --optimize-mft                 optimize master file tables only\n"
        "  -l,  --list-available-volumes       list all fixed disks available\n"
        "                                      for defragmentation\n"
        "  -la, --list-available-volumes=all   list all available disks,\n"
        "                                      including removable\n"
        "  -h,  --help                         show this help screen\n"
        "  -?                                  show this help screen\n"
        "\n"
        "  The commands are exclusive and can't be combined with each other.\n"
        "  If none is specified the program will defragment selected objects.\n"
        "\n"
        "Options:\n"
        "  -r,  --repeat                       repeat the disk processing multiple\n"
        "                                      times whenever it makes sense\n"
        "  -b,  --use-system-color-scheme      disable colorization of output\n"
        "  -p,  --suppress-progress-indicator  hide progress indicator and cluster map\n"
        "  -v,  --show-volume-information      show disk information after the job\n"
        "  -m,  --show-cluster-map             show cluster map\n"
        "       --map-border-color=color       set cluster map border color;\n"
        "                                      available colors: black, white, red,\n"
        "                                      green, blue, yellow, magenta, cyan,\n"
        "                                      darkred, darkgreen, darkblue, darkyellow,\n"
        "                                      darkmagenta, darkcyan, gray;\n"
        "                                      yellow is used by default\n"
        "       --map-symbol=x                 set character do draw cluster map with;\n"
        "                                      type it directly or use its hexadecimal\n"
        "                                      number (in range 0x1 ... 0xFF);\n"
        "                                      \'%%\' symbol is used by default\n"
        "       --map-rows=n                   cluster map height (10 by default)\n"
        "       --map-symbols-per-line=n       cluster map width (68 by default)\n"
        "       --use-entire-window            expand map to use entire window\n"
        "       --wait                         wait for completion of other\n"
        "                                      instances before the job startup\n"
        "                                      (useful for scheduled tasks)\n"
        "       --shellex                      list selected objects and display\n"
        "                                      a prompt to hit any key after the job\n"
        "                                      completion (intended to handle context\n"
        "                                      menu entries in Windows Explorer)\n"
        "\n"
        "Drive letters:\n"
        "  Space separated drive letters or one of the following switches:\n"
        "\n"
        "  --all                               process all available drives\n"
        "  --all-fixed                         process all non-removable drives\n"
        "\n"
        "Paths:\n"
        "  Space separated paths which need to be defragmented.\n"
        "  Both absolute and relative paths are supported, as well as wildcards.\n"
        "  Paths including spaces must be enclosed by double quotes (\").\n"
        "\n"
        "Accepted environment variables:\n"
        "\n"
        "  UD_IN_FILTER                        semicolon separated paths which need\n"
        "                                      to be defragmented; the empty list means\n"
        "                                      that everything needs to be defragmented\n"
        "\n"
        "  UD_EX_FILTER                        semicolon separated paths which need\n"
        "                                      to be skipped, i.e. left untouched\n"
        "\n"
        "  UD_FRAGMENT_SIZE_THRESHOLD          eliminate only fragments smaller than\n"
        "                                      specified; accepted size suffixes:\n"
        "                                      KB, MB, GB, TB, PB, EB\n"
        "\n"
        "  UD_FILE_SIZE_THRESHOLD              exclude all files larger than specified;\n"
        "                                      accepted size suffixes:\n"
        "                                      KB, MB, GB, TB, PB, EB\n"
        "\n"
        "  UD_OPTIMIZER_FILE_SIZE_THRESHOLD    for optimization only, exclude all files\n"
        "                                      larger than specified; accepted size\n"
        "                                      suffixes: KB, MB, GB, TB, PB, EB;\n"
        "                                      the default value is 20MB\n"
        "\n"
        "  UD_FRAGMENTS_THRESHOLD              exclude files having less fragments\n"
        "                                      than specified\n"
        "\n"
        "  UD_SORTING                          set sorting criteria for the disk\n"
        "                                      optimization; PATH is used by default,\n"
        "                                      it forces to sort files by their paths;\n"
        "                                      four more options are available:\n"
        "                                      SIZE (sort by size), C_TIME (sort by\n"
        "                                      creation time), M_TIME (sort by last\n"
        "                                      modification time) and A_TIME (sort by\n"
        "                                      last access time)\n"
        "\n"
        "  UD_SORTING_ORDER                    set sorting order for the disk\n"
        "                                      optimization; ASC (ascending) is used\n"
        "                                      by default, DESC (descending) forces\n"
        "                                      to sort files in reverse order\n"
        "\n"
        "  UD_FRAGMENTATION_THRESHOLD          cancel all tasks except of the MFT\n"
        "                                      optimization when fragmentation level\n"
        "                                      is below than specified\n"
        "\n"
        "  UD_TIME_LIMIT                       terminate the job automatically when\n"
        "                                      the specified time interval elapses;\n"
        "                                      the following time format is accepted:\n"
        "                                      Ay Bd Ch Dm Es; here A,B,C,D,E represent\n"
        "                                      integer numbers while y,d,h,m,s represent\n"
        "                                      years, days, hours, minutes and seconds\n"
        "\n"
        "  UD_REFRESH_INTERVAL                 set the progress refresh interval,\n"
        "                                      in milliseconds; the default value is 100\n"
        "\n"
        "  UD_DISABLE_REPORTS                  set it to 1 (one) to disable generation\n"
        "                                      of the file fragmentation reports\n"
        "\n"
        "  UD_DBGPRINT_LEVEL                   set amount of debugging output;\n"
        "                                      NORMAL is used by default, DETAILED\n"
        "                                      can be used to collect information for\n"
        "                                      a bug report, PARANOID turns on really\n"
        "                                      huge amount of debugging information\n"
        "\n"
        "  UD_LOG_FILE_PATH                    set log file path (including file name)\n"
        "                                      to redirect debugging output to a file\n"
        "\n"
        "  UD_DRY_RUN                          set it to 1 (one) to avoid physical\n"
        "                                      movements of files, i.e. to simulate\n"
        "                                      the disk processing; this allows to\n"
        "                                      check out algorithms quickly\n"
        "\n"
        "Note:\n"
        "  All the environment variables are ignored when the --shellex switch is\n"
        "  on the command line. Instead of taking environment variables into account\n"
        "  the program interpretes the %%UD_INSTALL_DIR%%\\options.lua file.\n"
        "\n"
        "Samples:\n"
        "\n"
        "  set UD_LOG_FILE_PATH=C:\\Windows\\Temp\\udefrag.log\n"
        "  set UD_TIME_LIMIT=6h 30m\n"
        "  set UD_EX_FILTER=*temp*;*tmp*\n"
        "  set UD_FRAGMENT_SIZE_THRESHOLD=20MB\n"
        "  udefrag c: d: e: \"h:\\my documents\\movies\\*\"\n"
        "\n"
        "More information and samples can be found in UltraDefrag Handbook.\n"
        "If you have not received it along with this program go to:\n"
        "\n"
        "http://ultradefrag.sourceforge.net/handbook/\n"
        "\n"
        );
}

/** @} */
