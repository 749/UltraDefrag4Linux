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
 * @file i18n.c
 * @brief Internationalization.
 * @addtogroup Internationalization
 * @{
 */

#include "wgx-internals.h"

/* synchronization objects */
HANDLE hSynchEvent = NULL;

void WgxInitSynchObjects(void)
{
    hSynchEvent = CreateEvent(NULL,FALSE,TRUE,NULL);
    if(hSynchEvent == NULL){
        letrace("event creation failed");
        etrace("internationalization routines will not work therefore");
    }
}

void WgxDestroySynchObjects(void)
{
    if(hSynchEvent)
        CloseHandle(hSynchEvent);
}

/**
 * @brief Extracts localization strings from a file to a table.
 * @param[in,out] table pointer to the table.
 * @param[in] path path of the file.
 * @return Boolean value. TRUE indicates success.
 * @note Path must use double back slashes
 * instead of single ones.
 */
BOOL WgxBuildResourceTable(PWGX_I18N_RESOURCE_ENTRY table,char *path)
{
    lua_State *L;
    int status;
    char *script;
    const char *sctemplate = ""
        "BOM = string.char(0xEF,0xBB,0xBF); "
        "f = assert(io.open(\"%s\",\"r\")); "
        "for line in f:lines() do "
        "   i, j, pair = string.find(line,string.format(\"^%%s(.-)$\",BOM)); "
        "   if not pair then pair = line end; "
        "   i, j, key, value = string.find(pair,\"^%%s*(.-)%%s*=%%s*(.-)%%s*$\"); "
        "   if key and value then "
        "       _G[key] = string.gsub(value,\"\\\\n\",\"\\n\") "
        "   end "
        "end; "
        "f:close()";
    int length;
    const char *msg;
    int i;
    const char *value;

    /* parameters validation */
    if(!table || !path)
        return FALSE;

    L = lua_open();
    if(L == NULL){
        etrace("cannot initialize Lua library");
        return FALSE;
    }
    
    /* stop collector during initialization */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, 0);
    
    /* prepare Lua script */
    length = strlen(sctemplate) + strlen(path) + 1;
    script = malloc(length);
    if(script == NULL){
        mtrace();
        lua_close(L);
        return FALSE;
    }
    _snprintf(script,length,sctemplate,path);
    script[length-1] = 0;
    
    /* extract strings from the file */
    status = luaL_dostring(L, script);
    if(status){
        etrace("script execution failed");
        if(!lua_isnil(L, -1)){
            msg = lua_tostring(L, -1);
            if(msg == NULL) msg = "(error object is not a string)";
            etrace("%s",msg);
            lua_pop(L, 1);
        }
        lua_close(L);
        free(script);
        return FALSE;
    }

    /* fill the table */
    if(hSynchEvent){
        if(WaitForSingleObject(hSynchEvent,INFINITE) != WAIT_OBJECT_0){
            letrace("synchronization failed");
        } else {
            for(i = 0; table[i].Key; i++){
                lua_getglobal(L, table[i].Key);
                if(!lua_isnil(L, lua_gettop(L))){
                    value = lua_tostring(L, lua_gettop(L));
                    if(value){
                        length = strlen(value);
                        table[i].LoadedString = malloc((length + 1) * 2);
                        if(table[i].LoadedString == NULL){
                            mtrace();
                        } else {
                            if(!MultiByteToWideChar(CP_UTF8,0,value,-1,table[i].LoadedString,length + 1)){
                                letrace("MultiByteToWideChar failed");
                                free(table[i].LoadedString);
                                table[i].LoadedString = NULL;
                            }
                        }
                    }
                }
                lua_pop(L, 1);
            }
            /* end of synchronization */
            SetEvent(hSynchEvent);
        }
    }
    
    lua_close(L);
    free(script);
    return TRUE;
}

/**
 * @brief Applies an i18n table to the dialog window.
 * @param[in] table pointer to the i18n table.
 * @param[in] hWindow handle to the window.
 */
void WgxApplyResourceTable(PWGX_I18N_RESOURCE_ENTRY table,HWND hWindow)
{
    int i;
    HWND hChild;
    wchar_t *text = NULL;

    if(!table || !hWindow) return;
    
    for(i = 0; table[i].Key; i++){
        hChild = GetDlgItem(hWindow,table[i].ControlID);
        /* synchronize access to the LoadedString */
        if(hSynchEvent){
            if(WaitForSingleObject(hSynchEvent,INFINITE) != WAIT_OBJECT_0){
                letrace("synchronization failed");
            } else {
                if(table[i].LoadedString){
                    text = _wcsdup(table[i].LoadedString);
                    if(text == NULL)
                        mtrace();
                }
                /* end of synchronization */
                SetEvent(hSynchEvent);
            }
        }
        if(text == NULL){
            text = _wcsdup(table[i].DefaultString);
            if(text == NULL)
                mtrace();
        }
        if(text){
            (void)SetWindowTextW(hChild,text);
            free(text);
        }
    }
}

/**
 * @brief Applies an i18n table to individual GUI control.
 */
void WgxSetText(HWND hWnd, PWGX_I18N_RESOURCE_ENTRY table, char *key)
{
    wchar_t *text;
    
    text = WgxGetResourceString(table,key);
    if(text){
        (void)SetWindowTextW(hWnd,text);
        free(text);
    }
}

/**
 * @brief Retrieves a localization string.
 * @param[in] table pointer to the i18n table.
 * @param[in] key pointer to the key string.
 * @return A pointer to the localized string if
 * available or to the default string otherwise.
 * @note The returned string should be freed by
 * a call to the free() routine after a use.
 */
wchar_t *WgxGetResourceString(PWGX_I18N_RESOURCE_ENTRY table,char *key)
{
    int i;
    wchar_t *text = NULL;
    
    if(!table || !key)
        return NULL;
    
    /* synchronize access to the table */
    if(hSynchEvent){
        if(WaitForSingleObject(hSynchEvent,INFINITE) != WAIT_OBJECT_0){
            letrace("synchronization failed");
            goto synch_failed;
        } else {
            for(i = 0; table[i].Key; i++){
                if(!strcmp(table[i].Key,key)){
                    if(table[i].LoadedString)
                        text = _wcsdup(table[i].LoadedString);
                    else
                        text = _wcsdup(table[i].DefaultString);
                    if(text == NULL)
                        mtrace();
                    break;
                }
            }
            /* end of synchronization */
            SetEvent(hSynchEvent);
        }
    } else {
synch_failed:
        for(i = 0; table[i].Key; i++){
            if(!strcmp(table[i].Key,key)){
                text = _wcsdup(table[i].DefaultString);
                if(text == NULL)
                    mtrace();
                break;
            }
        }
    }
    return text;
}

/**
 * @brief Destroys an i18n table.
 * @param[in] table pointer to the i18n table.
 */
void WgxDestroyResourceTable(PWGX_I18N_RESOURCE_ENTRY table)
{
    int i;
    
    if(!table) return;
    
    /* synchronize access to the table */
    if(hSynchEvent){
        if(WaitForSingleObject(hSynchEvent,INFINITE) != WAIT_OBJECT_0){
            letrace("synchronization failed");
        } else {
            for(i = 0; table[i].Key; i++){
                if(table[i].LoadedString){
                    free(table[i].LoadedString);
                    table[i].LoadedString = NULL;
                }
            }
            /* end of synchronization */
            SetEvent(hSynchEvent);
        }
    }
}

/** @} */
