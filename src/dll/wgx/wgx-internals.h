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

#ifndef _WGX_INTERNALS_H_
#define _WGX_INTERNALS_H_

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <objbase.h>
#include <shellapi.h>

#define lua_c
#include "../../lua5.1/lua.h"
#include "../../lua5.1/lauxlib.h"
#include "../../lua5.1/lualib.h"

#define WgxTraceHandler InternalTraceHandler
#include "wgx.h"

extern WGX_TRACE_HANDLER InternalTraceHandler;

#endif /* _WGX_INTERNALS_H_ */
