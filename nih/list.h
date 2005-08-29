/* libnih
 *
 * Copyright © 2005 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef NIH_LIST_H
#define NIH_LIST_H

#include <nih/macros.h>


/**
 * NihList:
 * @prev: previous entry in the list,
 * @next: next entry in the list,
 * @data: pointer to data attached to the entry.
 *
 * This structure is used both to refer to a linked list and to refer
 * to an individual entry in the list.  The list is circular so the @next
 * pointer of the last entry points to the first, and the @prev pointer
 * of the first entry points to the last.  An empty list simply has the
 * @prev and @next pointers pointing to itself.
 *
 * The list can be iterated with code like:
 * | NihList *iter;
 * |
 * | for (iter = list->next; iter != list; iter = iter->next)
 * |         printf ("%s\n", (char *)iter->data);
 *
 * This visits all of the entries except the one being used to refer to the
 * list.
 */
typedef struct nih_list_entry {
	struct nih_list_entry *prev, *next;

	void *data;
} NihList;


/**
 * nih_list_add_new:
 * @list: entry in the destination list,
 * @data: data to attach to the new entry.
 *
 * Allocates a new list entry, sets the data member to @data and then
 * adds it to a new list immediately before the @list entry.  If @list
 * is the pointer you are using to refer to the list itself, this results
 * in @entry being appended to the list.
 *
 * Returns: the new entry.
 */
#define nih_list_add_new(list, data) nih_list_add ((list), nih_list_new (data))

/**
 * nih_list_add_new_after:
 * @list: entry in the destination list,
 * @data: data to attach to the new entry.
 *
 * Allocates a new list entry, sets the data member to @data and then
 * adds it to a new list immediately after the @list entry.  If @list
 * is the pointer you are using to refer to the list itself and that entry
 * has no data, this results in @entry being pushed onto a stack under it.
 *
 * Returns: the new entry.
 */
#define nih_list_add_new_after(list, data) nih_list_add_after ((list), \
							   nih_list_new (data))


NIH_BEGIN_EXTERN

void     nih_list_init      (NihList *entry);
NihList *nih_list_new       (void *data);
NihList *nih_list_remove    (NihList *entry);
void     nih_list_free      (NihList *entry);
NihList *nih_list_add       (NihList *list, NihList *entry);
NihList *nih_list_add_after (NihList *list, NihList *entry);

NIH_END_EXTERN

#endif /* NIH_LIST_H */