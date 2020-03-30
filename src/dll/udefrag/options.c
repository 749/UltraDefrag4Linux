/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
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
 * @file options.c
 * @brief Job options.
 * @addtogroup Options
 * @{
 */
     
#include "compiler.h"
#include "udefrag-internals.h"

/**
 * @brief Retrieves all ultradefrag
 * related options from the environment.
 * @return Zero for success, negative
 * value otherwise.
 */
int get_options(udefrag_job_parameters *jp)
{
    utf_t *buffer;
    char buf[64];
    int i;

    if(jp == NULL)
        return (-1);
    
    /* reset all options */
    memset(&jp->udo,0,sizeof(udefrag_options));
    jp->udo.refresh_interval = DEFAULT_REFRESH_INTERVAL;
    
    /* allocate memory */
    buffer = winx_heap_alloc(ENV_BUFFER_SIZE * sizeof(short));
    if(buffer == NULL){
        DebugPrint("get_options: cannot allocate %u bytes of memory",
            (unsigned int)(ENV_BUFFER_SIZE * sizeof(short)));
        return (-1);
    }
    
    /* set filters */
    if(winx_query_env_variable(UTF("UD_IN_FILTER"),buffer,ENV_BUFFER_SIZE) >= 0){
        DebugPrint("in_filter = %" WSTR,printableutf(buffer));
        winx_patcomp(&jp->udo.in_filter,buffer,UTF(";\""),WINX_PAT_ICASE);
    }
    if(winx_query_env_variable(UTF("UD_EX_FILTER"),buffer,ENV_BUFFER_SIZE) >= 0){
        DebugPrint("ex_filter = %" WSTR,printableutf(buffer));
        winx_patcomp(&jp->udo.ex_filter,buffer,UTF(";\""),WINX_PAT_ICASE);
    }

    /* set file size threshold */
    if(winx_query_env_variable(UTF("UD_SIZELIMIT"),buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_snprintf(buf,sizeof(buf) - 1,"%" WSTR,buffer);
        buf[sizeof(buf) - 1] = 0;
        jp->udo.size_limit = winx_hr_to_bytes(buf);
    }

    /* set file fragments threshold */
    if(winx_query_env_variable(UTF("UD_FRAGMENTS_THRESHOLD"),buffer,ENV_BUFFER_SIZE) >= 0)
        jp->udo.fragments_limit = (ULONGLONG)utftol(buffer);
    
    /* set time limit */
    if(winx_query_env_variable(UTF("UD_TIME_LIMIT"),buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_snprintf(buf,sizeof(buf) - 1,"%" WSTR,buffer);
        buf[sizeof(buf) - 1] = 0;
        jp->udo.time_limit = winx_str2time(buf);
    }

    /* set progress refresh interval */
    if(winx_query_env_variable(UTF("UD_REFRESH_INTERVAL"),buffer,ENV_BUFFER_SIZE) >= 0)
        jp->udo.refresh_interval = utftoi(buffer);

    /* check for disable_reports option */
    if(winx_query_env_variable(UTF("UD_DISABLE_REPORTS"),buffer,ENV_BUFFER_SIZE) >= 0) {
        if(!utfcmp(buffer,UTF("1")))
            jp->udo.disable_reports = 1;
    }

    /* set debug print level */
    if(winx_query_env_variable(UTF("UD_DBGPRINT_LEVEL"),buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_wcsupr(buffer);
        if(!utfcmp(buffer,UTF("DETAILED")))
            jp->udo.dbgprint_level = DBG_DETAILED;
        else if(!utfcmp(buffer,UTF("PARANOID")))
            jp->udo.dbgprint_level = DBG_PARANOID;
    }
    
    /* set dry_run variable */
    if(winx_query_env_variable(UTF("UD_DRY_RUN"),buffer,ENV_BUFFER_SIZE) >= 0){
        if(!utfcmp(buffer,UTF("1"))){
            DebugPrint("%%UD_DRY_RUN%% environment variable is set to 1,");
            DebugPrint("therefore no actual data moves will be performed on disk");
            jp->udo.dry_run = 1;
        }
    }

    /* print all options */
    winx_dbg_print_header(0,0,"ultradefrag job options");
    if(jp->udo.in_filter.count){
        DebugPrint("in_filter patterns:");
        for(i = 0; i < jp->udo.in_filter.count; i++)
            DebugPrint("  + %" WSTR,printableutf(jp->udo.in_filter.array[i]));
    }
    if(jp->udo.ex_filter.count){
        DebugPrint("ex_filter patterns:");
        for(i = 0; i < jp->udo.ex_filter.count; i++)
            DebugPrint("  - %" WSTR,printableutf(jp->udo.ex_filter.array[i]));
    }
    DebugPrint("file size threshold = %" LL64 "u",(ULONGLONG)jp->udo.size_limit);
    DebugPrint("file fragments threshold = %" LL64 "u",(ULONGLONG)jp->udo.fragments_limit);
    DebugPrint("time limit = %" LL64 "u seconds",(ULONGLONG)jp->udo.time_limit);
    DebugPrint("progress refresh interval = %u msec",
                    (unsigned int)jp->udo.refresh_interval);
    if(jp->udo.disable_reports) DebugPrint("reports disabled");
    else DebugPrint("reports enabled");
    switch(jp->udo.dbgprint_level){
    case DBG_DETAILED:
        DebugPrint("detailed debug level set");
        break;
    case DBG_PARANOID:
        DebugPrint("paranoid debug level set");
        break;
    default:
        DebugPrint("normal debug level set");
    }

    /* cleanup */
    winx_heap_free(buffer);
    return 0;
}

/**
 * @brief Frees all resources
 * allocated by get_options.
 */
void release_options(udefrag_job_parameters *jp)
{
    if(jp){
        winx_patfree(&jp->udo.in_filter);
        winx_patfree(&jp->udo.ex_filter);
    }
}

/** @} */
