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

#ifndef NIH_ALLOC_H
#define NIH_ALLOC_H

#include <nih/macros.h>


/**
 * NihAllocDestructor:
 *
 * A destructor is a function that can be associated with a NihAllocCtx
 * and is called when the block is freed; the pointer given is that of
 * the block being freed.
 *
 * This can be used, for example, to close a file descriptor when the
 * structure for it is being closed.
 */
typedef int (*NihAllocDestructor) (void *);


/* Hack to let us stringyfy __LINE__ */
#define _STRINGYFY(_s)       #_s
#define _STRINGYFY_AGAIN(_s) _STRINGYFY(_s)
#define LINE_STRING          _STRINGYFY_AGAIN(__LINE__)


/**
 * nih_alloc:
 * @parent: parent block for new allocation,
 * @type: type of data to store.
 *
 * Allocates a block of memory large enough to store a @type object and
 * returns a pointer to it.
 *
 * If @parent is not %NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the d_alloc_set_destructor function.
 *
 * Returns: requested memory block.
 */
#define nih_alloc(parent, type) nih_alloc_named(parent, sizeof (type), \
					    __FILE__ ":" LINE_STRING " " #type)

/**
 * nih_alloc_size:
 * @parent: parent block for new allocation,
 * @size: size of block to allocate.
 *
 * Allocates a block of memory of at least @size bytes and returns a
 * pointer to it.
 *
 * If @parent is not %NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the d_alloc_set_destructor function.
 *
 * Returns: requested memory block.
 */
#define nih_alloc_size(parent, size) nih_alloc_named(parent, size, \
						 __FILE__ ":" LINE_STRING)


NIH_BEGIN_EXTERN

void *      nih_alloc_new (void *parent, size_t size, const char *name);
void *      nih_alloc_named (void *parent, size_t size, const char *name);
int         nih_free (void *ptr);

void        nih_alloc_set_name (void *ptr, const char *name);
void        nih_alloc_set_destructor (void *ptr, NihAllocDestructor destructor);
const char *nih_alloc_name (void *ptr);

void        nih_alloc_return_unused (int large);


NIH_END_EXTERN

#endif /* NIH_ALLOC_H */
