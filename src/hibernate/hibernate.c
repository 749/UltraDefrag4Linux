/*
 *  Hibernate for Windows - a command line tool for Windows hibernation.
 *  Copyright (c) 2009-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
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
* Some people believe SetSuspendState is more reliable than SetSystemPowerState:
* http://msdn.microsoft.com/en-us/library/aa373206%28VS.85%29.aspx
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <powrprof.h>

#include "../dll/zenwinx/zenwinx.h"

static void show_help(void)
{
    printf(
        "===============================================================================\n"
        "Hibernate for Windows - a command line tool for Windows hibernation.\n"
        "Copyright (c) UltraDefrag Development Team, 2009-2013.\n"
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
        "Usage: \n"
        "  hibernate now - hibernate the computer\n"
        "  hibernate /?  - display this help\n"
        );
}

int __cdecl main(int argc,char **argv)
{
    int i, now = 0;

    for(i = 1; i < argc; i++){
        if(!_stricmp(argv[i],"now"))
            now = 1;
    }

    if(now == 0){
        show_help();
        return EXIT_SUCCESS;
    }

    printf("Hibernate for Windows - a command line tool for Windows hibernation.\n");
    printf("Copyright (c) UltraDefrag Development Team, 2009-2013.\n\n");
    
    if(winx_init_library() < 0){
        fprintf(stderr,"Initialization failed!\n");
        return EXIT_FAILURE;
    }

    if(winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE) < 0){
        fprintf(stderr,"Cannot enable shutdown privilege!\n"
            "Use DbgView program to get more information.\n");
        return EXIT_FAILURE;
    }

    /* hibernate, request permission from apps and drivers */
    if(!SetSuspendState(TRUE,FALSE,FALSE)){
        letrace("cannot hibernate the computer");
        fprintf(stderr,"Cannot hibernate the computer!\n"
            "Use DbgView program to get more information.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
