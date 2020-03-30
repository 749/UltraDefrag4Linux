//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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

#ifndef _UDEFRAG_CONSOLE_PREC_H_
#define _UDEFRAG_CONSOLE_PREC_H_

#define wxUSE_GUI 0
#include <wx/wx.h>
#include <wx/cmdline.h>
#include <wx/dynlib.h>
#include <wx/filename.h>
#include <wx/thread.h>

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <conio.h>

#if defined(__GNUC__)
extern "C" {
HRESULT WINAPI URLDownloadToCacheFileW(
    /* LPUNKNOWN */ void *lpUnkcaller,
    LPCWSTR szURL,
    LPWSTR szFileName,
    DWORD cchFileName,
    DWORD dwReserved,
    /*IBindStatusCallback*/ void *pBSC
);
}
#endif

#endif /* _UDEFRAG_CONSOLE_PREC_H_ */
