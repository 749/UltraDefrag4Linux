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
 * @file config.c
 * @brief Configuration.
 * @addtogroup Config
 * @{
 */

#include "wgx-internals.h"

typedef struct _escape_sequence {
    char c;
    char *sq;
} escape_sequence;

escape_sequence esq[] = {
    {'\\', "\\\\"},
    {'\a', "\\a" },
    {'\b', "\\b" },
    {'\f', "\\f" },
    {'\n', "\\n" },
    {'\r', "\\r" },
    {'\t', "\\t" },
    {'\v', "\\v" },
    {'\'', "\\\'" },
    {'\"', "\\\"" },
    {0,    NULL  }
};

/**
 * @brief Reads options from a configuration file.
 * @details The configuration files must be written
 * in Lua language to be accepted.
 * @param[in] path the path to the configuration file.
 * @param[in,out] table the options table.
 * @return TRUE on success, FALSE otherwise.
 * @note 
 * - NULL can be passed as first parameter to
 *   initialize options by their default values.
 * - The option name equal to NULL indicates the end of the table.
 * - Both WGX_CFG_EMPTY and WGX_CFG_COMMENT options are ignored.
 * - If option type is WGX_CFG_INT both number and default_number
 *   members of the structure must be set.
 * - If option type is WGX_CFG_STING, both string and
 *   default_string members of the structure must be
 *   set, as well as string_length.
 */
BOOL WgxGetOptions(char *path,WGX_OPTION *table)
{
    lua_State *L;
    int status;
    const char *msg;
    int i;
    char *s;
    
    if(table == NULL)
        return FALSE;
    
    /* set defaults */
    for(i = 0; table[i].name; i++){
        if(table[i].type == WGX_CFG_INT){
            *(table[i].number) = table[i].default_number;
        } else if(table[i].type == WGX_CFG_STRING){
            strncpy(table[i].string,
                table[i].default_string,
                table[i].string_length);
            table[i].string[table[i].string_length - 1] = 0;
        }
    }
    
    if(path == NULL)
        return TRUE; /* nothing to update */
    
    L = lua_open();
    if(L == NULL){
        etrace("cannot initialize Lua library");
        return FALSE;
    }
    
    /* stop collector during initialization */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, 0);

    status = luaL_dofile(L,path);
    if(status != 0){
        etrace("cannot interprete %s",path);
        if(!lua_isnil(L, -1)){
            msg = lua_tostring(L, -1);
            if(msg == NULL) msg = "(error object is not a string)";
            etrace("%s",msg);
            lua_pop(L, 1);
        }
        lua_close(L);
        return FALSE;
    }
    
    /* search for variables */
    for(i = 0; table[i].name; i++){
        lua_getglobal(L, table[i].name);
        if(!lua_isnil(L, lua_gettop(L))){
            if(table[i].type == WGX_CFG_INT){
                *(table[i].number) = (int)lua_tointeger(L, lua_gettop(L));
            } else if(table[i].type == WGX_CFG_STRING){
                s = (char *)lua_tostring(L, lua_gettop(L));
                if(s != NULL){
                    strncpy(table[i].string,s,table[i].string_length);
                    table[i].string[table[i].string_length - 1] = 0;
                } else {
                    strcpy(table[i].string,"");
                }
            }
        }
        /* important: remove received variable from stack */
        lua_pop(L, 1);
    }
    
    /*for(i = 0; table[i].name; i++){
        if(table[i].type == WGX_CFG_EMPTY){
            trace(D"\n");
        } else if(table[i].type == WGX_CFG_COMMENT){
            trace(D"-- %s\n",table[i].name);
        } else if(table[i].type == WGX_CFG_INT){
            trace(D"%s = %i\n",table[i].name,*(table[i].number));
        } else if(table[i].type == WGX_CFG_STRING){
            trace(D"%s = \"%s\"\n",table[i].name,table[i].string);
        }
    }*/
    
    /* cleanup */
    lua_close(L);
    return TRUE;
}

/**
 * @brief Saves options to a configuration file.
 * The configuration files produced are actually 
 * Lua programs.
 * @param[in] path the path to the configuration file.
 * @param[in,out] table the options table.
 * @param[in] cb the address of the callback procedure
 * intended for being called in case of errors.
 * @return TRUE on success, FALSE otherwise.
 * @note
 * - Option name equal to NULL inicates the end of the table.
 * - WGX_CFG_EMPTY option forces to insert an empty line.
 * - WGX_CFG_COMMENT inserts a comment with text passed in option's name.
 * - WGX_CFG_INT saves an integer number.
 * - WGX_CFG_STRING saves a string.
 */
BOOL WgxSaveOptions(char *path,WGX_OPTION *table,WGX_SAVE_OPTIONS_CALLBACK cb)
{
    FILE *f;
    char *msg;
    int i, result = 0;
    unsigned int j, k, n;
    char c;
    char *sq;
    
    if(path == NULL || table == NULL){
        if(cb != NULL)
            cb("WgxSaveOptions: invalid parameter");
        return FALSE;
    }
    
    f = fopen(path,"wt");
    if(f == NULL){
        msg = wgx_sprintf("Cannot open %s "
            "file: %s",path,_strerror(NULL));
        if(msg){
            etrace("%s",msg);
            if(cb) cb(msg);
        } else {
            etrace("Cannot open file: not enough memory!");
            if(cb) cb("Cannot open file: not enough memory!");
        }
        free(msg);
        return FALSE;
    }

    for(i = 0; table[i].name; i++){
        if(table[i].type == WGX_CFG_EMPTY){
            //trace(D"\n");
            result = fprintf(f,"\n");
        } else if(table[i].type == WGX_CFG_COMMENT){
            //trace(D"-- %s\n",table[i].name);
            result = fprintf(f,"-- %s\n",table[i].name);
        } else if(table[i].type == WGX_CFG_INT){
            //trace(D"%s = %i\n",table[i].name,*(table[i].number));
            result = fprintf(f,"%s = %i\n",table[i].name,*(table[i].number));
        } else if(table[i].type == WGX_CFG_STRING){
            //trace(D"%s = \"%s\"\n",table[i].name,table[i].string);
            /*result = fprintf(f,"%s = \"%s\"\n",table[i].name,table[i].string);*/
            result = fprintf(f,"%s = \"",table[i].name);
            if(result < 0)
                goto fail;
        
            n = strlen(table[i].string);
            for(j = 0; j < n; j++){
                c = table[i].string[j];
                /* replace character by escape sequence when needed */
                sq = NULL;
                for(k = 0; esq[k].c; k++){
                    if(esq[k].c == c)
                        sq = esq[k].sq;
                }
                if(sq)
                    result = fprintf(f,"%s",sq);
                else
                    result = fprintf(f,"%c",c);
                if(result < 0)
                    goto fail;
            }
            
            result = fprintf(f,"\"\n");
        }
        if(result < 0){
fail:
            fclose(f);
            msg = wgx_sprintf("Cannot write to %s "
                "file: %s",path,_strerror(NULL));
            if(msg){
                etrace("%s",msg);
                if(cb) cb(msg);
            } else {
                etrace("Cannot write to file: not enough memory!");
                if(cb) cb("Cannot write to file: not enough memory!");
            }
            free(msg);
            return FALSE;
        }
    }

    /* cleanup */
    fclose(f);
    return TRUE;
}

/** @} */
