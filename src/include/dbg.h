/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Prefixes for debugging messages.
 * @details These prefixes are intended for
 * use with messages passed to winx_dbg_print
 * and winx_dbg_print_header routines. To keep
 * logs clean and suitable for easy analysis
 * always use one of the prefixes listed here.
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
 * @note These flags are intended
 * for the winx_dbg_print routine.
 */
#define NT_STATUS_FLAG  0x1
#define LAST_ERROR_FLAG 0x2

/**
 * @brief Structure for data transfer from
 * UltraDefrag interfaces to UltraDefrag
 * debugger.
 * @note It uses pragma pack directive to force
 * data alignment to be compiler independent.
 */
#pragma pack(push,1)
typedef struct _udefrag_shared_data {
    int version;
    int ready;
    unsigned int exception_code;
    void *exception_address;
    wchar_t tracking_id[32];
} udefrag_shared_data;
#pragma pack(pop)

/*
* Don't forget to increment it after
* any modifications of the structure.
*/
#define SHARED_DATA_VERSION 0x2

/* don't litter reliability reports by test crashes */
#ifndef OFFICIAL_RELEASE
  #undef SEND_CRASH_REPORTS
#endif

/*
* Test runs need to be counted for
* investigation of crashes only.
*/
#ifdef SEND_CRASH_REPORTS
  #define SEND_TEST_REPORTS
#endif

#if defined(__cplusplus)
}
#endif

#endif /* _DBG_H_ */
