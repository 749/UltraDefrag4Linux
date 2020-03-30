/*
 *  ZenWINX - WIndows Native eXtended library.
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

/**
 * @file mem.c
 * @brief Memory.
 * @addtogroup Memory
 * @{
 */

#include "prec.h"
#include "zenwinx.h"

int default_killer(size_t n);

HANDLE hGlobalHeap = NULL;
char *reserved_memory = NULL;
winx_killer killer = default_killer;

/**
 * @internal
 * @brief Aborts the application in the out of memory condition
 * when no custom killer is set by the winx_set_killer routine.
 */
int default_killer(size_t n)
{
    /* terminate process with exit code 3 */
    NtTerminateProcess(NtCurrentProcess(),3);
    return 0;
}

/**
 * @brief Sets custom routine to be called
 * in the out of memory condition.
 * @details The killer should either abort
 * the application and return zero or return
 * a nonzero value. In the latter case zenwinx
 * library will try to allocate memory again.
 */
void winx_set_killer(winx_killer k)
{
    killer = k;
}

/**
 * @brief Allocates a block of memory.
 * @param size bytes to allocate.
 * @param flags a combination of MALLOC_XXX
 * flags defined in zenwinx.h file.
 * @return A pointer to the allocated block or
 * NULL if there's no sufficient memory available.
 * @note The allocated block may be larger
 * than requested.
 */
void *winx_heap_alloc(size_t size,int flags)
{
    void *p = NULL;

    /*
    * Avoid winx_dbg_xxx calls here
    * to avoid recursion.
    */
    
    if(!hGlobalHeap) return NULL;

    if(!(flags & MALLOC_ABORT_ON_FAILURE))
        return RtlAllocateHeap(hGlobalHeap,0,size);
    
    do {
        p = RtlAllocateHeap(hGlobalHeap,0,size);
        if(!p) if(!killer(size)) break;
    } while(!p);
    
    return p;
}

/**
 * @brief Releases a memory block allocated by winx_heap_alloc().
 * @param[in] addr the memory block to be released.
 */
void winx_heap_free(void *addr)
{
    /*
    * Avoid winx_dbg_xxx calls here
    * to avoid recursion.
    */
    if(hGlobalHeap && addr)
        (void)RtlFreeHeap(hGlobalHeap,0,addr);
}

/**
 * @internal
 * @brief Creates global memory heap.
 * @return Zero for success, a negative value otherwise.
 */
int winx_create_global_heap(void)
{
    /* create growable heap with initial size of 100 KB */
    if(hGlobalHeap == NULL){
        hGlobalHeap = RtlCreateHeap(HEAP_GROWABLE,NULL,0,100 * 1024,NULL,NULL);
    }
    if(hGlobalHeap == NULL){
        /* trace macro cannot be used here */
        /* winx_printf cannot be used here */
        winx_print("\nCannot create global memory heap!\n");
        return (-1);
    }
    
    /* reserve 1 MB of memory for the out of memory condition handling */
    reserved_memory = (char *)winx_tmalloc(1024 * 1024);
    return 0;
}

/**
 * @internal
 * @brief Destroys global memory heap.
 */
void winx_destroy_global_heap(void)
{
    if(hGlobalHeap){
        (void)RtlDestroyHeap(hGlobalHeap);
        hGlobalHeap = NULL;
    }
}

/** @} */
