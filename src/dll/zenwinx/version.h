/*
 *  ZenWINX - WIndows Native eXtended library.
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
 *  Common version header.
 */

#include "zenwinxver.h"

#define VS_VERSION_INFO     1
#ifndef WINVER
#define WINVER 0x0400
#endif
#include <winresrc.h>
#ifdef IDC_STATIC
#undef IDC_STATIC
#endif
#define IDC_STATIC      (-1)
