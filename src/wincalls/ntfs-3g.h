/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2011      by Jean-Pierre Andre for the Linux version
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
 *                Interface with libntfs-3g
 * 
 *        To avoid symbol conflicts, every interface must only rely on
 *        types defined in compiler.h (universal C types tolerated)
 */

HANDLE ntfs_open(const utf_t*);
int ntfs_close(HANDLE);
int ntfs_unlink(HANDLE, const char*);
int ntfs_sync(HANDLE);
int ntfs_read(HANDLE, char*, ULONG, ULONGLONG);
int ntfs_write(HANDLE, const char*, ULONG, ULONGLONG);
HANDLE ntfs_open_file(HANDLE, const char*);
HANDLE ntfs_create_file(HANDLE, const char*);
int ntfs_close_file(HANDLE);
int ntfs_write_file(HANDLE, const char*, ULONG, ULONGLONG);

int ntfs_fill_fs_sizes(HANDLE, LARGE_INTEGER*, LARGE_INTEGER*,
				ULONG*, ULONG*);
int ntfs_fill_vol_info(HANDLE, LARGE_INTEGER*, ULONG*, ULONG*,
				UCHAR*, WCHAR*);
int ntfs_fill_vol_flags(HANDLE, ULONG*);
int ntfs_fill_ntfs_data(HANDLE, LARGE_INTEGER*, ULONG*, LARGE_INTEGER*);
int ntfs_fill_bitmap(HANDLE, PVOID, ULONG, ULONGLONG, ULONGLONG*, ULONG*);
int ntfs_fill_inode_data(HANDLE, PVOID, ULONG, ULONGLONG, ULONG*);
int ntfs_move_clusters(HANDLE, LONGLONG, LONGLONG, ULONG);
int ntfs_fill_runlist(HANDLE, ULONGLONG, ULONG*, PVOID, ULONG);
