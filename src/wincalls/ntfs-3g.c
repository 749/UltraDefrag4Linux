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
 *        To avoid symbol conflicts, every interface must go through
 *        this module, and must only rely on types defined in compiler.h
 *        (universal C types tolerated)
 *
 *	Only trace level 3 should be used
 */

#include "compiler.h"
#if LXSC | L8SC | STSC | SPGC | PPGC | ARGC
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>

#include "layout.h"
#include "attrib.h"
#include "volume.h"
#include "mft.h"
#include "dir.h"
#include "inode.h"
#include "bitmap.h"
#include "lcnalloc.h"
#include "ntfstime.h"
#include "ntfs-3g.h"

#if (LXSC | L8SC | STSC | SPGC | ARGC) & !NOMEMCHK

void *ntfs_mymalloc(size_t, const char*, int); /* extra relay desirable */
void *ntfs_mycalloc(size_t,size_t, const char*, int); /* extra relay desirable */
void *ntfs_myrealloc(void*,size_t, const char*, int); /* extra relay desirable */
void ntfs_myfree(void*, const char*, int); /* extra relay desirable */
char *ntfs_mystrdup(const char*, const char*, int);
#define malloc(sz) ntfs_mymalloc(sz,__FILE__,__LINE__)
#define calloc(sz,cnt) ntfs_mycalloc(sz,cnt,__FILE__,__LINE__)
#define realloc(old,sz) ntfs_myrealloc(old,sz,__FILE__,__LINE__)
#define free(old) ntfs_myfree(old,__FILE__,__LINE__)
#define ntfs_malloc(sz) ntfs_mymalloc(sz,__FILE__,__LINE__)   
#define ntfs_calloc(sz,cnt) ntfs_mymalloc(sz,cnt,__FILE__,__LINE__)
#define ntfs_realloc(old,sz) ntfs_mymalloc(old,sz,__FILE__,__LINE__)
#define ntfs_free(old) ntfs_myfree(old,__FILE__,__LINE__)
#undef strdup
#define strdup(p) ntfs_mystrdup(p,__FILE__,__LINE__)

#else /*  LXSC | L8SC | STSC | SPGC | ARGC */

void *ntfs_malloc(size_t);
void *ntfs_calloc(size_t); 
void *ntfs_realloc(size_t,void*);
#define malloc(sz) ntfs_malloc(sz)
#define calloc(sz,cnt) ntfs_calloc(sz*cnt)
#define realloc(sz,old) ntfs_realloc(sz,old)

#endif /*  LXSC | L8SC | STSC | SPGC | ARGC */

/* temporary : this module should never get out, just return conditions */
void stop_there(int, const char*, int);
#define get_out(n) stop_there(n,__FILE__,__LINE__)


struct TYPES_TABLE {
	le32 type;
	char *name;
} ;

/*
 *		What type of attribute should be searched for a given stream
 *	Only attribute which may be non-resident should be present here
 */
struct TYPES_TABLE types_table[] = {
	{ AT_BITMAP,		    "$BITMAP" },
	{ AT_ATTRIBUTE_LIST,	    "$ATTRIBUTE_LIST" },
	{ AT_SECURITY_DESCRIPTOR,   "$SECURITY_DESCRIPTOR" },
	{ AT_REPARSE_POINT,	    "$REPARSE_POINT" },
	{ AT_EA,           	    "$EA" },
	{ AT_EA_INFORMATION,	    "$EA_INFORMATION" },
	{ AT_LOGGED_UTILITY_STREAM, "$LOGGED_UTILITY_STREAM" },
	{ AT_INDEX_ALLOCATION,	    "$INDEX_ALLOCATION" },
	{ AT_BITMAP,                "$BITMAP" },
	{ const_cpu_to_le32(0), (char*)NULL } /* end marker */
} ;

static ntfschar I30[] = { const_cpu_to_le16('$'), const_cpu_to_le16('I'),
                          const_cpu_to_le16('3'), const_cpu_to_le16('0') } ;

extern int trace;
extern int do_nothing;

extern int ntfs_toutf16(utf16_t*, const char*, int);
extern int ntfs_toutf8(char*, const utf_t*, int);

#if PPGC

#undef bswap_64

unsigned long long bswap_64(unsigned long long x)
{
	unsigned long long y;
	int i;

	y = 0;
	for (i=0; i<=56; i+=8)
		y = (y << 8) + ((x >> i) & 255);
	return (y);
}

#endif /* PPGC */

int ntfs_mounted_device(const char *path)
{
	unsigned long existing_mount;

	return (!ntfs_check_if_mounted(path, &existing_mount)
		&& (existing_mount & NTFS_MF_MOUNTED));
}

HANDLE ntfs_open(const utf_t *device)
{
	ntfs_volume *vol;

	if (do_nothing)
		vol = ntfs_mount(device,NTFS_MNT_RDONLY);
	else
		vol = ntfs_mount(device,0);
#ifdef NVolSetNoFixupWarn
	if (vol)
		NVolSetNoFixupWarn(vol);
#endif
	if (trace > 2)
		safe_fprintf(stderr,"device %s opened handle 0x%lx\n",device,(long)vol);
	return ((HANDLE)vol);
}

int ntfs_close(HANDLE h)
{
	ntfs_volume *vol;

		/*
		 * Having relocated parts of the MFT, we have updated its
		 * mapping pairs, so the MFT is marked dirty and we have
		 * to sync it, which implies modifications to the parent
		 * directory, which in turn implies a modification to
		 * the MFT, and so on.
		 * So we first try a sync, then clear the dirty flags
		 * caused by the updating of parent directory, so that
		 * the volume can be closed with no error.
		 */
	vol = (ntfs_volume*)h;
	ntfs_inode_sync(vol->mft_ni);
	NInoFileNameClearDirty(vol->mft_ni);
	NInoClearDirty(vol->mft_ni);
	return (ntfs_umount(vol,FALSE));
}

int ntfs_sync(HANDLE h)
{
	ntfs_volume *vol;

	vol = (ntfs_volume*)h;
	return (ntfs_device_sync(vol->dev));
}

/*
 *		Read from the device
 */

int ntfs_read(HANDLE h, char *buf, ULONG size, ULONGLONG pos)
{
	ntfs_volume *vol;
	int r;

	r = -1;
	if (h) {
		vol = (ntfs_volume*)h;
		if (ntfs_pread(vol->dev, pos, size, buf)
				== (SLONGLONG)size)
			r = 0;
	}
	if (r && (trace > 2))
		safe_fprintf(stderr,"** Failed to read %lu bytes from device at 0x%llx\n",
			(unsigned long)size,(long long)pos);
	return (r);
}

/*
 *		Write to the device
 */

int ntfs_write(HANDLE h, const char *buf, ULONG size, ULONGLONG pos)
{
	ntfs_volume *vol;
	int r;

	r = -1;
	if (h) {
		vol = (ntfs_volume*)h;
		if (ntfs_pwrite(vol->dev, pos, size, buf)
				== (SLONGLONG)size)
			r = 0;
	}
	if (r && (trace > 2))
		safe_fprintf(stderr,"** Failed to write %lu bytes to device at 0x%llx\n",
			(unsigned long)size,(long long)pos);
	return (r);
}

/*
 *		Create an inner file or attribute
 */

HANDLE ntfs_create_file(HANDLE h, const char *path)
{
	char *name; 
	ntfschar *uname = NULL;
	ntfs_volume *vol;
	ntfs_inode *dir_ni = NULL, *ni;
	ntfs_attr *na;
	char *dir_path;
	le32 securid;
	int res = 0, uname_len;

	if (trace > 2)
		safe_fprintf(stderr,"ntfs_create_file %s\n",path);
	na = (ntfs_attr*)NULL;
	ni = (ntfs_inode*)NULL;
	if (!h) {
		res = -EINVAL;
		goto reject;
	}
	vol = (HANDLE)h;
	dir_path = strdup(path);
	if (!dir_path) {
		res = -errno;
		goto reject;
	}
	/* Generate unicode filename. */
	name = strrchr(dir_path, '/');
	name++;
	uname_len = ntfs_mbstoucs(name, &uname);
	if (uname_len < 0) {
		res = -errno; 
		goto exit;
	}
	/* Open parent directory. */
	*--name = 0;
	dir_ni = ntfs_pathname_to_inode(vol, NULL, dir_path);
	if (!dir_ni) {
		res = -errno;
		goto exit;
	}
	securid = 0;
	if (trace > 2)
		safe_fprintf(stderr,"parent directory %s %ld\n",dir_path,(long)dir_ni->mft_no);
	ni = ntfs_create(dir_ni, securid, uname, uname_len, S_IFREG);
	if (ni) {
		na = ntfs_attr_open(ni, AT_DATA, NULL, 0);
		if (!na && !res)
			res = errno;
		NInoSetDirty(ni);
		ntfs_inode_update_times(dir_ni, NTFS_UPDATE_MCTIME);
		if (trace > 2)
			safe_fprintf(stderr,"file %s created ni 0x%lx %ld na 0x%lx\n",path,
						(long)ni,(long)ni->mft_no,(long)na);
	} else
		res = -errno;
exit:
	free(uname);
	if (ntfs_inode_close(dir_ni) && !res)
		res = -errno;
	if (!na && ntfs_inode_close(ni) && !res)
		res = -errno;
	free(dir_path);
reject :
	return (res ? (HANDLE)NULL : (HANDLE)na);
}

static ntfs_attr *open_stream(ntfs_volume *vol, const char *name)
{
	const char *suff;
	ntfs_inode *ni;
	ntfs_attr *na;
	le32 type;
	char *base_name;
	char *post_name;
	char *type_name;
	BOOL dyn_name;
	ntfschar *stream_name;
	struct TYPES_TABLE *t;
	int name_lth;

	suff = strstr(name,"//");
	type = const_cpu_to_le32(0);
	ni = (ntfs_inode*)NULL;
	dyn_name = FALSE;
	if (suff && suff[2]) {
		base_name = strdup(name);
		stream_name = (ntfschar*)NULL;
		type_name = (char*)NULL;
		name_lth = 0;
		if (base_name) {
			post_name = strstr(base_name,"//");
			if (post_name) {
				*post_name++ = 0;
				post_name++;
				ni = ntfs_pathname_to_inode(vol,NULL,base_name);
				if (trace > 2)
					safe_fprintf(stderr,"base_name %s ni 0x%lx %lld stream %s\n",
						base_name,(long)ni,
						(long long)ni->mft_no,post_name);
				type_name = strchr(post_name,'/');
				if (type_name)
					*type_name++ = 0;
			}
		}
		if (ni) {
			t = types_table;
			if (type_name)
				while (t->type && strcmp(t->name,type_name))
					t++;
			if (type_name
			    && t->type
			    && (t->type != AT_INDEX_ALLOCATION)) {
				type = t->type;
			} else {
					/* named data or directory stream */
				stream_name = (ntfschar*)ntfs_malloc(2*strlen(post_name));
				if (stream_name) {
					name_lth = ntfs_toutf16(stream_name,post_name,strlen(post_name));
					dyn_name = TRUE;
				}
				if (t->type == AT_INDEX_ALLOCATION)
					type = AT_INDEX_ALLOCATION;
				else
					type = AT_DATA;
			}
			if (!type) {
				ntfs_inode_close(ni);
				ni = (ntfs_inode*)NULL;
			}
		}
		if (base_name)
			free(base_name);
	} else {
		ni = ntfs_pathname_to_inode(vol,NULL,name);
		if (ni && (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY)) {
			type = AT_INDEX_ALLOCATION;
			stream_name = I30;
			name_lth = 4;
		} else {
			type = AT_DATA;
			stream_name = (ntfschar*)NULL;
			name_lth = 0;
		}
	}
	if (type && ni) {
		na = ntfs_attr_open(ni, type, stream_name, name_lth);
		if (!na) {
			ntfs_inode_close(ni);
			if (trace > 2)
				safe_fprintf(stderr,"** Could not open %s\n",name);
		}
		if (dyn_name)
			free(stream_name);
	} else {
		if (trace > 2)
			safe_fprintf(stderr,"** Unsupported stream opening %s\n",name);
		na = (ntfs_attr*)NULL;
	}
	return (na);
}

/*
 *		Open an inner file (its data or an attribute)
 */

HANDLE ntfs_open_file(HANDLE h, const char *name)
{
	ntfs_volume *vol;
	ntfs_attr *na;

	na = (ntfs_attr*)NULL;
	if (h) {
		if (trace > 2)
			safe_fprintf(stderr,"ntfs_open_file %s\n",name);
		vol = (ntfs_volume*)h;
			/*
			 * A few system files are kept open, do not
			 * reopen them.
			 * Moreover $DATA from $MFTMirr and $LogFile have
			 * runlists to be replicated into $MFTMirr, do not
			 * process them until replication is implemented
			 */
		if (!strcmp(name,"/$MFT")
		    || !strncmp(name,"/$MFT//",7)
		    || !strcmp(name,"/$MFTMirr")
		    || !strcmp(name,"/$LogFile")
		    || !strcmp(name,"/$Bitmap")) {
			if (!strcmp(name,"/$MFT"))
				na = vol->mft_na;
			else
				if (!strcmp(name,"/$MFT///$BITMAP"))
					na = vol->mftbmp_na;
				else
					if (!strcmp(name,"/$Bitmap"))
						na = vol->lcnbmp_na;
			/* else return NULL to mean locked */
			if (na && (trace > 2))
				safe_fprintf(stderr,"Special file %s, na 0x%lx ni 0x%lx inode %lld\n",
					name,(long)na,(long)na->ni,
					(long long)na->ni->mft_no);
		} else
			na = open_stream(vol,name);
	}
	return ((HANDLE)na);
}

int ntfs_close_file(HANDLE h)
{
	ntfs_volume *vol;
	ntfs_inode *ni;
	ntfs_attr *na;
	void *buf;
	int r;

	r = -EINVAL;
	na = (ntfs_attr*)h;
	if (na && na->ni && na->ni->vol) {
		ni = na->ni;
			/*
			 * Do not close system streams which must be kept open
			 */
		switch (na->ni->mft_no) {
		case FILE_MFT :
			vol = ni->vol;
			ntfs_inode_sync(ni);
			buf = (void*)ntfs_malloc(vol->mft_record_size);
			if (trace > 2)
				safe_fprintf(stderr,"Copying MFT record to MFTMirr\n");
			if (buf
			    && (ntfs_attr_pread(vol->mft_na, 0,
				vol->mft_record_size, buf)
					== vol->mft_record_size)
			    && (ntfs_attr_pwrite(vol->mftmirr_na, 0,
				vol->mft_record_size, buf)
					== vol->mft_record_size))
				r = 0;
			free(buf);
			if (trace > 2)
				safe_fprintf(stderr,"Avoiding closing ni 0x%lx inode %lld na 0x%lx\n",(long)ni,(long long)ni->mft_no,(long)na);
			break;
		case FILE_Bitmap :
			if (trace > 2)
				safe_fprintf(stderr,"Avoiding closing ni 0x%lx inode %lld na 0x%lx\n",(long)ni,(long long)ni->mft_no,(long)na);
			break;
		default :
			ntfs_attr_close(na);
			if (trace > 2)
				safe_fprintf(stderr,"Closing ni 0x%lx inode %lld na 0x%lx\n",(long)ni,(long long)ni->mft_no,(long)na);
			r = ntfs_inode_close(ni);
			break;
		}
	}
	if (trace > 2)
		safe_fprintf(stderr,"inode closed, r %d\n",r);
	return (r);
}

/*
 *		Write to an inner file
 */

int ntfs_write_file(HANDLE h, const char *buf, ULONG size, ULONGLONG offset)
{
	ntfs_inode *ni;
	ntfs_attr *na;
	int res;
	ULONGLONG total;

	res = 0;
	total = 0;
	if (h) {
		na = (ntfs_attr*)h;
		ni = na->ni;
		if (trace > 2)
			safe_fprintf(stderr,"writing %ld bytes into ni 0x%lx %ld\n",(long)size,(long)ni,(long)ni->mft_no);
		while (size) {
			s64 ret = ntfs_attr_pwrite(na, offset, size, buf + total);
			if (ret <= 0) {
				res = -errno;
				goto exit;
			}
			size   -= ret;
			offset += ret;
			total  += ret;
		}
		ntfs_inode_update_times(ni, NTFS_UPDATE_MCTIME);
exit:
	if (trace > 2)
		safe_fprintf(stderr,"written %ld bytes res %d\n",(long)total,(int)res);
	}
	return (res);
}

/*
 *		Unlink an inner file
 */

int ntfs_unlink(HANDLE h, const char *org_path)
{
	char *name;
	ntfs_volume *vol;
	ntfschar *uname = NULL;
	ntfs_inode *dir_ni = NULL, *ni;
	char *path;
	int res = 0, uname_len;

	if (trace > 2)
		safe_fprintf(stderr,"unlinking %s\n",org_path);
	vol = (ntfs_volume*)h;
	path = strdup(org_path);
	if (!path)
		return -errno;
	/* Open object for delete. */
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni) { 
		res = -errno;  
		goto exit;
	}
	if (trace > 2)
		safe_fprintf(stderr,"ni 0x%lx inode %ld\n",(long)ni,(long)ni->mft_no);
	/* deny unlinking metadata files */
	if (ni->mft_no < FILE_first_user) { 
		errno = EPERM;
		res = -errno;
		goto exit;
	}
        
	/* Generate unicode filename. */
	name = strrchr(path, '/');   
	name++;
	uname_len = ntfs_mbstoucs(name, &uname);
	if (uname_len < 0) {   
		res = -errno;
		goto exit;
	}
	/* Open parent directory. */
	*--name = 0;
	dir_ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!dir_ni) {
		res = -errno;
		goto exit;
	}
	if (trace > 2)
		safe_fprintf(stderr,"dir_ni 0x%lx inode %ld\n",(long)dir_ni,(long)dir_ni->mft_no);
	if (ntfs_delete(vol, org_path, ni, dir_ni,
			 uname, uname_len))
		res = -errno;
	/* ntfs_delete() always closes ni and dir_ni */
	ni = dir_ni = NULL;
exit:
	if (ntfs_inode_close(dir_ni) && !res)
		res = -errno;
	if (ntfs_inode_close(ni) && !res)
		res = -errno;
	free(uname);
	free(path);
	if (trace > 2)
		safe_fprintf(stderr,"delete res %d\n",res);
	return res;
}

/*
 *		       get the file system sizes.
 *	Returns zero if successful
 */

int ntfs_fill_fs_sizes(HANDLE h, LARGE_INTEGER *ptotal, LARGE_INTEGER *pavail,
			       ULONG *psectperclust, ULONG *pbytespersect)
{
	ntfs_volume *vol;
	int r;

	if (h) {
		vol = (ntfs_volume*)h;
		if (trace > 2)
			safe_fprintf(stderr,"vol 0x%lx na 0x%lx clusters %lld\n",(long)vol,(long)vol->lcnbmp_na,vol->nr_clusters);
		vol->free_clusters = ntfs_attr_get_free_bits(vol->lcnbmp_na);
		if (trace > 2)
			safe_fprintf(stderr,"free_clusters %lld\n",vol->free_clusters);
		ptotal->QuadPart = vol->nr_clusters;
		pavail->QuadPart = vol->free_clusters;
		*psectperclust = vol->cluster_size >> vol->sector_size_bits;
		*pbytespersect = vol->sector_size;
                if (trace > 2)
			safe_fprintf(stderr,"total %lld avail %lld spc %ld bps %ld\n",
                            (long long)ptotal->QuadPart,(long long)pavail->QuadPart,
                            (long)*psectperclust,(long)*pbytespersect);
		r = 0;
	} else {
		r = -1;
		errno = EINVAL;
	}
	return (r);
}

int ntfs_fill_vol_info(HANDLE h, LARGE_INTEGER *pcrtime, ULONG *pserial,
			ULONG *plablth, UCHAR *psupobj, WCHAR *plabel)
{
	NTFS_BOOT_SECTOR *bs;
	ULONGLONG serial;
	ntfs_volume *vol;
	ntfs_inode *ni;
	int r;

	if (h) {
		vol = (ntfs_volume*)h;
		ni = vol->vol_ni;
		serial = 0;
		bs = (NTFS_BOOT_SECTOR*)ntfs_malloc(sizeof(NTFS_BOOT_SECTOR));
		if (bs) {
			if (ntfs_pread(vol->dev, 0, sizeof(NTFS_BOOT_SECTOR), bs)
				== sizeof(NTFS_BOOT_SECTOR)) {
				serial = le64_to_cpu(bs->volume_serial_number);
			if (trace > 2)
				safe_fprintf(stderr,"serial 0x%llx\n",serial);
			}
			free(bs);
		}

		pcrtime->QuadPart = le64_to_cpu(ni->creation_time);
		*plablth = strlen(vol->vol_name)*2;
		ntfs_toutf16(plabel,vol->vol_name,strlen(vol->vol_name));
		/* low-order part of serial number in cpu endianness */
		*pserial = (ULONG)serial;
		*psupobj = 1;
		if (trace > 2)
			safe_fprintf(stderr,"vol name %s\n",vol->vol_name);
		r = 0;
	} else {
		r = -1;
		errno = EINVAL;
	}
	return (r);
}

int ntfs_fill_vol_flags(HANDLE h, ULONG *flags)
{
	ntfs_volume *vol;
	int r;

	if (h) {
		vol = (ntfs_volume*)h;
		*flags = le16_to_cpu(vol->flags);
		r = 0;
	} else {
		r = -1;
		errno = EINVAL;
	}
	return (r);
}

int ntfs_fill_ntfs_data(HANDLE h, LARGE_INTEGER *data1,
			ULONG *data2, LARGE_INTEGER *data3)
{
	int r;
/*
struct _NTFS_DATA {
    LARGE_INTEGER VolumeSerialNumber;
    LARGE_INTEGER NumberSectors;
    LARGE_INTEGER TotalClusters;
    LARGE_INTEGER FreeClusters;
    LARGE_INTEGER TotalReserved;
    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    ULONG BytesPerFileRecordSegment;
    ULONG ClustersPerFileRecordSegment;
    LARGE_INTEGER MftValidDataLength;
    LARGE_INTEGER MftStartLcn;
    LARGE_INTEGER Mft2StartLcn;
    LARGE_INTEGER MftZoneStart;
    LARGE_INTEGER MftZoneEnd;
} ;
*/
	NTFS_BOOT_SECTOR *bs;
	ntfs_volume *vol;
	runlist_element *rl;
	LONGLONG mft_size;
	char *p;

	if (h && data1 && data2 && data3) {
		vol = (ntfs_volume*)h;
		bs = (NTFS_BOOT_SECTOR*)ntfs_malloc(sizeof(NTFS_BOOT_SECTOR));
		if (bs) {
			if (ntfs_pread(vol->dev, 0, sizeof(NTFS_BOOT_SECTOR), bs)
					== sizeof(NTFS_BOOT_SECTOR)) {
				data1[0].QuadPart
					= le64_to_cpu(bs->volume_serial_number);
				data1[1].QuadPart
					= le64_to_cpu(bs->number_of_sectors);
				data1[2].QuadPart
					= data1[1].QuadPart
						/ bs->bpb.sectors_per_cluster;
// FreeClusters not known yet
				data1[3].QuadPart = 0;
					/* this is badly aligned */
				p = (char*)&bs->bpb.reserved_sectors;
				data1[4].QuadPart
					= (p[0] & 255) + ((p[1] & 255) << 8);
				data2[0] = vol->sector_size;
				data2[1] = vol->sector_size
						* bs->bpb.sectors_per_cluster;
				data2[2] = vol->mft_record_size;
				/* beware : the following may be negative */
				data2[3] = bs->clusters_per_mft_record;
				data3[0].QuadPart
					= vol->mft_na->allocated_size;
				data3[1].QuadPart
					= le64_to_cpu(bs->mft_lcn);
				data3[2].QuadPart
					= le64_to_cpu(bs->mftmirr_lcn);
/*
 *               in boot sector                     returned by Windows
 *   zonestart   zoneend   mftstart mftend         zonestart    zoneend
 *     0          0x1eb3     0x20   0x1a2d           0x1a20      0x1ec0
 *     0         0x1de20      0x4    0x6bf            0x6a0      0xc820
 *
 *   Windows probably replies :
 *   MftZoneStart : last MFT cluster used rounded down mod 32
 *   MftZoneEnd : min (200MiB beyond start, zoneend) rounded up mod 32
 */
				rl = vol->mft_na->rl;
				while (rl && rl[0].length && rl[1].length)
					rl++;
				if (rl && rl[0].length) {
					data3[3].QuadPart
						= (rl->lcn + rl->length) & -32;
					mft_size = 209715200 >> vol->cluster_size_bits;
					if (mft_size > (vol->nr_clusters >> 3))
						mft_size = vol->nr_clusters >> 3;
					data3[4].QuadPart
						= ((rl->lcn + rl->length
						    + mft_size - 1) | 31) + 1;
					if (data3[4].QuadPart > vol->nr_clusters)
						data3[4].QuadPart
							= vol->nr_clusters;
				if (trace > 2)
					safe_fprintf(stderr,"mft zone start 0x%llx end 0x%llx\n",
						(long long)data3[3].QuadPart,
						(long long)data3[4].QuadPart);
				}
			}
			free(bs);
		}
		if (trace > 2)
			safe_fprintf(stderr,"vol name %s\n",vol->vol_name);
		r = 0;
	} else {
		r = -1;
		errno = EINVAL;
	}
	return (r);
}

int ntfs_fill_bitmap(HANDLE h, PVOID buf, ULONG buflen, ULONGLONG pos,
				ULONGLONG *pfullsize, ULONG *pretlen)
{
	int r;
	ntfs_volume *vol;
	ntfs_attr *bmp_na;
	SLONGLONG got;
	ULONGLONG size;

	*pretlen = 0;
	if (h && buf && buflen && pretlen) {
		vol = (ntfs_volume*)h;
		bmp_na = vol->lcnbmp_na;
		*pfullsize = vol->nr_clusters;
		if (pos < (ULONGLONG)bmp_na->data_size) {
			size = bmp_na->data_size - pos;
			if (size > buflen)
				size = buflen;
			got = ntfs_attr_pread(bmp_na, pos, size, buf);
			if (trace > 2)
				safe_fprintf(stderr,"Read bitmap pos %lld of %lld size %ld got %ld\n",(long long)pos,(long long)bmp_na->data_size,(long)size,(long)got);
			if (got <= 0)
				r = -1;
			else {
				*pretlen = got;
				r = 0;
			}
		} else
			r = -1; /* beyond the end */
	} else {
		r = -1;
		errno = EINVAL;
	}
	return (r);
}

int ntfs_fill_inode_data(HANDLE h, PVOID buf, ULONG buflen, ULONGLONG mref,
						ULONG *pretlen)
{
	int r;
	int count;
	ntfs_volume *vol;

	r = -1;
	if (h && buf && buflen) {
		vol = (ntfs_volume*)h;
			/* read full MFT records */
		count = buflen/vol->mft_record_size;
		if (count) {
			r = ntfs_mft_records_read(vol, mref, count, buf);
			if (!r)
				*pretlen = count*vol->mft_record_size;
		if (trace > 2)
			safe_fprintf(stderr,"getting %d MFT records, r %d retlen %ld\n",count,r,(long)*pretlen);
		}
	} else {
		errno = EINVAL;
	}
	return (r);
}

/*
 *			Get a (partial) runlist
 */

void showrl(FILE *f, const char *text, const runlist_element *rl, const runlist_element *cur)
   {
   int k;
       
   if (cur && rl)
      safe_fprintf(f,"  runlist at \"%s\" 0x%lx cur at index %d\n",text,(long)rl,(int)(cur-rl));
   else
      safe_fprintf(f,"  runlist at \"%s\" 0x%lx\n",text,(long)rl);
   if (rl)
      {
      k = 0;
      do {
         safe_fprintf(f,"  %d vcn 0x%04llx lcn 0x%016llx length 0x%lx\n",k,(long long)rl[k].vcn,(long long)rl[k].lcn,(long)rl[k].length);
         } while ((k < 600) && rl[k++].length);
      }
   }

/*
 *	Returns 0 if successful and the end is reached
 *		1 if successful and the end is not reached
 *		-1 if there was an error
 */

int ntfs_fill_runlist(HANDLE h, ULONGLONG vcn, ULONG *pcnt,
				PVOID buf, ULONG size)
{
	ntfs_attr *na;
	ntfs_inode *ni;
	runlist_element *rl;
	ULONGLONG *data;
	unsigned long i;
	unsigned long rlcnt;
	unsigned long first;
	unsigned long outcnt;
	int r;

	r = -1;
	na = (ntfs_attr*)h;
	if (na && na->ni && na->ni->vol && buf) {
		ni = na->ni;
		if (trace > 2)
			safe_fprintf(stderr,"Getting runlist for inode %lld\n",(long long)ni->mft_no);
		if (!ntfs_attr_map_whole_runlist(na)) {
			rlcnt = 0;
			rl = na->rl;
			while (rl[rlcnt].length)
				rlcnt++;
			first = 0;
			while (rl[first].length && ((ULONGLONG)rl[first].vcn < vcn))
				first++;
			if ((ULONGLONG)rl[first].vcn == vcn) {
				outcnt = rlcnt - first;
				r = 0;
				if (outcnt*16 > size) {
					outcnt = size >> 4;
					r = 1;
				}
				*pcnt = outcnt;
				data = (ULONGLONG*)buf;
				data[1] = rl[first].lcn;
				for (i=1; i<outcnt; i++) {
					data[2*i - 2] = rl[i + first].vcn;
					data[2*i + 1] = rl[i + first].lcn;
				}
				data[2*outcnt - 2] = rl[outcnt + first].vcn;
			} else {
				if (trace > 2)
					safe_fprintf(stderr,"** Runlist request not at beginning of run\n");
			}
		}
	}
	return (r);
}

/*
 *		Basic allocation of consecutive clusters
 *	The designated clusters are supposed to be unallocated
 *
 *	Returns zero if successful
 */

int ntfs_cluster_alloc_basic(ntfs_volume *vol, s64 lcn, s64 count)
{
	int ret = -1;
	runlist_element *rl;

		/* this makes sure the requested zone is free */
	rl = ntfs_cluster_alloc(vol, 0, count, lcn, DATA_ZONE);
	if (rl && !rl->vcn && (rl->lcn == lcn)
	    && (rl->length == count) && !rl[1].length) {
		free(rl);
		ret = 0;
	} else {
		if (trace > 2)
			safe_fprintf(stderr,"** Failed to allocate %lld clusters from 0x%llx\n",
					(long long)count,(long long)lcn);
		if (rl)
			ntfs_cluster_free_from_rl(vol, rl);
	}
	return ret;
}

/*
 *		Copy data from consecutive clusters
 *
 *	Returns zero if successful
 */

static int copy_clusters(ntfs_volume *vol, LONGLONG target,
					LONGLONG source, ULONG cnt)
{
	char * buf;
	int r;
	ULONGLONG bytesrc;
	ULONGLONG bytetgt;
	ULONGLONG size;
	unsigned long bufsz;
	unsigned long chunk;

	r = 0;
	bytesrc = source << vol->cluster_size_bits;
	bytetgt = target << vol->cluster_size_bits;
	size = ((ULONGLONG)cnt) << vol->cluster_size_bits;
	bufsz = (size < 32768 ? size : 32768);
	buf = (char*)ntfs_malloc(bufsz);
	if (buf) {
		do {
			chunk = (size < bufsz ? size : bufsz);
			if (trace > 2)
				safe_fprintf(stderr,"copying 0x%llx to 0x%llx chunk %ld\n",(long long)bytesrc,(long long)bytetgt,(long)chunk);
			if ((ntfs_pread(vol->dev, bytesrc, chunk, buf) == (long)chunk)
			    &&  (ntfs_pwrite(vol->dev, bytetgt, chunk, buf) == (long)chunk)) {
				bytesrc += chunk;
				bytetgt += chunk;
				size -= chunk;
			} else
				r = -1;
		} while (size && !r);
		free(buf);
	}
	return (r);
}

/*
 *		Merge a run with adjacent runs, if possible
 *
 *	Returns zero if successful or unneeded
 *
 *	Note : this is supposed to restore original runlist if merging fails
 */

static int merge_runs(LONGLONG svcn, LONGLONG tlcn, ULONG cnt,
			runlist_element *rl, unsigned long moved)
{
	int r;

	r = 0;
	if (svcn && ((rl[moved-1].lcn + rl[moved-1].length) == tlcn)) {
				/* merge with previous */
		if (trace > 2)
			safe_fprintf(stderr,"merging with previous\n");
		rl[moved-1].length += cnt;
		do {
			rl[moved] = rl[moved+1];
			moved++;
		} while (rl[moved].length);
	} else {
		if (rl[moved+1].length
		    && ((tlcn + cnt) == rl[moved+1].lcn)) {
			if (trace > 2)
				safe_fprintf(stderr,"merging with next\n");
			/* merge with next */
			rl[moved].lcn = tlcn;
			rl[moved].length = rl[moved+1].length + cnt;
			moved++;
			do {
				rl[moved] = rl[moved+1];
				moved++;
			} while (rl[moved].length);
		} else {
			/* cannot merge */
			rl[moved].lcn = tlcn;
		}
	}
	return (r);
}

int ntfs_actual_move_clusters(ntfs_attr *na, runlist_element *rl,
			unsigned long moving, LONGLONG tlcn, ULONG cnt)
{
	ntfs_volume *vol;
	LONGLONG slcn;
	LONGLONG svcn;
	int r;

	if (trace > 2)
		safe_fprintf(stderr,"inode %lld move vcn 0x%llx lth %ld slcn 0x%llx tlcn 0x%llx\n",
				(long long)na->ni->mft_no,
				(long long)rl[moving].vcn,(long)cnt,
				(long long)rl[moving].lcn,(long long)tlcn);
	r = -1;
	vol = na->ni->vol;
	slcn = rl[moving].lcn;
	svcn = rl[moving].vcn;
						/* copy clusters */
	if (!copy_clusters(vol,tlcn,slcn,cnt)
						/* build the new runlist */
	    && !merge_runs(svcn, tlcn, cnt, rl, moving)) {
			/*
			 * This is where we enter the dangerous zone
			 * currently we cannot roll back if something fails
			 */
						/* update the runlist */
		if (!ntfs_attr_update_mapping_pairs(na,svcn)
						/* and free unused clusters */
		    && !ntfs_cluster_free_basic(vol,slcn,cnt)) {
			r = 0;
		} else {
			if (trace > 2)
				safe_fprintf(stderr,"** Severe failure while updating the runlist\n");
		}
	} else {
			/* copying or merging failed, just free target */
		if (ntfs_cluster_free_basic(vol,tlcn,cnt)) {
			if (trace > 2)
				safe_fprintf(stderr,"** Could not roll back after failed cluster copy\n");
		} else {
			if (trace > 2)
				safe_fprintf(stderr,"** Rolled back after failed cluster copy\n");
		}
	}
	if (r && (trace > 2)) {
		safe_fprintf(stderr,"** Updated inode %lld slcn 0x%llx tlcn 0x%llx cnt %ld\n",
			(long long)na->ni->mft_no,(long long)slcn,
			(long long)tlcn, (long)cnt);
		safe_fprintf(stderr,"** The above data may be useful for repairing !\n");
	}
	return (r);
}

static ULONG ntfs_partial_move_clusters(runlist_element *rl,
			LONGLONG tlcn, ULONG cnt,
			ntfs_attr *na, unsigned long moving)
{
	unsigned long last;
	ULONG cando;
	ULONG r;

	r = 0;
		/* have to split the run */
	if (rl[moving].length > cnt) {
		if (trace > 2)
			safe_fprintf(stderr,"must split, length %ld cnt %ld\n",
					(long)rl[moving].length,(long)cnt);
			/* split the run if not fully moved */
		rl = ntfs_rl_extend(na, rl, 1);
		if (rl) {
			last = moving;
			while (rl[last].length)
				last++;
			while (last > moving) {
				rl[last + 1] = rl[last];
				last--;
			}
			rl[moving + 1].lcn = rl[moving].lcn + cnt;
			rl[moving + 1].vcn = rl[moving].vcn + cnt;
			rl[moving + 1].length = rl[moving].length - cnt;
			rl[moving].length = cnt;
		} else {
			if (trace > 2)
				safe_fprintf(stderr,"** Failed to split a run\n");
		}
	}
	if (rl && (rl[moving].length < cnt))
		cando = rl[moving].length;
	else
		cando = cnt;

	if (rl
	    && cando
	    && !ntfs_actual_move_clusters(na, rl, moving, tlcn, cando))
		r = cando;
	return (r);
}


/*
 *			Move clusters
 *
 *	Returns zero if successful or unneeded
 */

int ntfs_move_clusters(HANDLE h, LONGLONG svcn, LONGLONG tlcn, ULONG cnt)
{
	ntfs_attr *na;
	ntfs_inode *ni;
	runlist_element *rl;
	unsigned long moving;
	ULONG moved;
	ULONG done;
	int r;

	r = -1;
	na = (ntfs_attr*)h;
	if ((trace > 2) && na && na->ni)
		safe_fprintf(stderr,"moving clusters inode %lld vcn 0x%llx cnt %ld to lcn 0x%llx\n",(long long)na->ni->mft_no,(long long)svcn,(long)cnt,(long long)tlcn);
			/* first check we can allocate target */
	if (na
	    && na->ni
	    && na->ni->vol
	    && !ntfs_cluster_alloc_basic(na->ni->vol,tlcn,cnt)) {
		ni = na->ni;
			/* the runlist up to the end is needed for updating */
		if (!ntfs_attr_map_whole_runlist(na)) {
			if (trace > 2)
				showrl(stderr,"original",na->rl,NULL);
			done = 0;
			do {
				rl = na->rl;
				moving = 0;
				while (rl[moving].length
				    && (rl[moving].vcn < (svcn + done)))
					moving++;
				if (trace > 2)
					safe_fprintf(stderr,"moving %ld lth %ld vcn %lld svcn %lld\n",(long)moving,(long)rl[moving].length,rl[moving].vcn,svcn+done);
				moved = 0;
				if (rl[moving].length
				    && (rl[moving].vcn == (svcn + done))) {
					moved = ntfs_partial_move_clusters(rl,
							tlcn + done, cnt - done,
							na, moving);
					if (moved)
						done += moved;
				}
			} while (moved && (done < cnt));
			if (done < cnt) {
				/* failed : free the unused clusters */
				ntfs_bitmap_clear_run(ni->vol->lcnbmp_na,
						tlcn + done, cnt - done);
				if (trace > 2) {
					safe_fprintf(stderr,"** Could not find the run to move\n");
					safe_fprintf(stderr,"svcn 0x%llx tlcn 0x%llx count 0x%lx moving %ld\n",(long long)svcn,(long long)tlcn,(long)cnt,(long)moving);
				}
			}
			if (trace > 2)
				showrl(stderr,"final",na->rl,NULL);
			if (done == cnt)
				r = 0;
		} else {
			ntfs_bitmap_clear_run(ni->vol->lcnbmp_na, tlcn, cnt);
			if (trace > 2)
				safe_fprintf(stderr,"** Could not map the runlist\n");
		}
	}
	return (r);
}
