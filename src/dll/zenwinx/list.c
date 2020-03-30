/*
 *  ZenWINX - WIndows Native eXtended library.
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
 * @file list.c
 * @brief Double linked lists.
 * @addtogroup Lists
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/**
 * @brief Inserts an item to a double linked list.
 * @details Allocates memory for the item to be inserted.
 * @param[in,out] phead pointer to a variable pointing to the list head.
 * @param[in] prev pointer to an item preceeding to the new item.
 * If this parameter is NULL, the new head will be inserted.
 * @param[in] size the size of the item to be inserted, in bytes.
 * @return Pointer to the inserted list item. In case of allocation failure
 * this routine calls the killer registered by winx_set_killer and returns NULL then.
 */
list_entry *winx_list_insert(list_entry **phead,list_entry *prev,long size)
{
    list_entry *new_item;
    
    /*
    * Avoid winx_dbg_xxx calls here
    * to avoid recursion.
    */

    if(size < sizeof(list_entry))
        return NULL;

    new_item = (list_entry *)winx_malloc(size);

    /* is list empty? */
    if(*phead == NULL){
        *phead = new_item;
        new_item->prev = new_item->next = new_item;
        return new_item;
    }

    /* insert as the new head? */
    if(prev == NULL){
        prev = (*phead)->prev;
        *phead = new_item;
    }

    /* insert after the item specified by prev argument */
    new_item->prev = prev;
    new_item->next = prev->next;
    new_item->prev->next = new_item;
    new_item->next->prev = new_item;
    return new_item;
}

/**
 * @brief Removes an item from a double linked list.
 * @details Frees memory allocated for the item to be removed.
 * @param[in,out] phead pointer to a variable pointing to the list head.
 * @param[in] item pointer to the item which must be removed.
 */
void winx_list_remove(list_entry **phead,list_entry *item)
{
    /*
    * Avoid winx_dbg_xxx calls here
    * to avoid recursion.
    */

    /* validate the item */
    if(item == NULL) return;
    
    /* is list empty? */
    if(*phead == NULL) return;

    /* remove alone first item? */
    if(item == *phead && item->next == *phead){
        winx_free(item);
        *phead = NULL;
        return;
    }
    
    /* remove first item? */
    if(item == *phead){
        *phead = (*phead)->next;
    }
    item->prev->next = item->next;
    item->next->prev = item->prev;
    winx_free(item);
}

/**
 * @brief Destroys a double linked list.
 * @details Frees memory allocated for all list items.
 * @param[in,out] phead pointer to a variable
 * pointing to the list head.
 */
void winx_list_destroy(list_entry **phead)
{
    list_entry *item, *next, *head;
    
    /* is list empty? */
    if(*phead == NULL) return;

    head = *phead;
    item = head;

    do {
        next = item->next;
        winx_free(item);
        item = next;
    } while (next != head);

    *phead = NULL;
}

/** @} */
