/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
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

#ifndef _DBG_H_
#define _DBG_H_

/**
 * @brief Prefixes for debugging messages.
 * @details These prefixes are intended for
 * use with messages passed to winx_dbg_print,
 * winx_dbg_print_header, udefrag_dbg_print
 * routines. To keep logs clean and suitable
 * for easy analysis always use one of the
 * prefixes listed here.
 */
#define I "INFO:  "  /* for general purpose progress information */
#define E "ERROR: "  /* for errors */
#define D "DEBUG: "  /* for debugging purposes */
/* after addition of new prefixes don't forget to adjust winx_dbg_print_header code */

/**
 * @brief Flags for debugging routines.
 * @details If NT_STATUS_FLAG is set, the
 * last nt status value will be appended
 * to the debugging message as well as its
 * description. If LAST_ERROR_FLAG is set,
 * the same stuff will be done for the last
 * error value.
 * @note These flags are intended for use with
 * winx_dbg_print and udefrag_dbg_print routines.
 */
#define NT_STATUS_FLAG  0x1
#define LAST_ERROR_FLAG 0x2

#endif /* _DBG_H_ */
