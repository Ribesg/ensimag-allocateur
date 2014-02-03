/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "mem.h"

/** squelette du TP allocateur memoire */

void *mem = 0;

struct cell {
	unsigned long size;
	struct cell* next;
};
typedef struct cell Cell;

Cell* tzl[BUDDY_MAX_INDEX + 1];

/**Function used only to avoid math.h log2**/
int getIndexOf(unsigned long size) {
	unsigned long mask = 1;
	int i = 0;
	while (!(size & mask)) {
		i++;
		size >>= 1;
	}
	return i;
}

/**Function that adds a cell to a cell's linked list's end**/
void add_last(Cell ** list, Cell * cell) {
	if (*list == NULL) {
		*list = cell;
	} else {
		add_last(&((*list)->next), cell);
	}
}

/**Function that remove a element from a linked list**/
/**Note : the element is not freed, just removed **/
void delete(Cell ** list, Cell* cell) {
	if (*list != NULL) {
		if (*list == cell) {
			*list = cell->next;
			cell->next = NULL;
		} else {
			delete(&((*list)->next), cell);
		}
	}
}

/**Function that splits a block in 2 recursively,**/
/** as long as we didn't reach the requestedSize **/
/**Note : splitted blocks are added to the TZL linked lists **/
void* split_block(Cell * block, unsigned long requestedSize) {
	if (block->size == 1) return (void *)NULL;
	int index = getIndexOf(block->size);

	Cell ** list_block = &tzl[index];
	delete(list_block, block);

	block->size = block->size / 2;
	add_last(&tzl[index - 1], block);

	//we calculate buddy's location
	unsigned long buddy = ((unsigned long)block) + block->size;
	*(Cell*)buddy = (Cell) { block->size, NULL };

	add_last(&tzl[index - 1], (Cell*)buddy);

	if (block->size == requestedSize) {
		delete(&tzl[index - 1], block);
		return (void *)block;
	} else {
		return split_block(block, requestedSize);
	}
}

/**Function that finds a free_block of the requestedSize**/
/**Basically, it takes the first element of the TZL linked list if not null**/
/**else, we split a higher free block if we can**/
static void * find_free_block(int index, unsigned long requestedSize) {
	if (tzl[index] != NULL) {
		Cell * freeBlock = tzl[index];
		if (freeBlock == NULL) {return NULL;}
		else {
			if (freeBlock->size == requestedSize) {
				delete(&tzl[index], freeBlock);
				return (void *) freeBlock;
			} else {
				return split_block(freeBlock, requestedSize); 
			}
		}

	} else if (index < BUDDY_MAX_INDEX) { 
		return find_free_block(index + 1, requestedSize);
		//find out what to do when it's not the right size;
	} else {
		return NULL;
	}
}

int mem_init()
{
	if (!mem) {
		mem = (void *) malloc(ALLOC_MEM_SIZE);
	}

	if (mem == 0) {
		perror("mem_init:");
		return -1;
	}

	for (int i = 0; i <= BUDDY_MAX_INDEX ; i++) {
		tzl[i] = NULL;
	}

	Cell * cell = (Cell *) mem;
	cell->size = ALLOC_MEM_SIZE;
	cell->next = NULL;
	tzl[BUDDY_MAX_INDEX] = mem;

	return 0;
}

void * mem_alloc(unsigned long size)
{
	if (mem == 0) return NULL;
	if (size == 0) return NULL;
	if (size > ALLOC_MEM_SIZE) return 0; //size is too high
	
	int index = getIndexOf(size);

	Cell* cell = find_free_block(index, size); //step 1 : find a free block
	if (cell == NULL) {
		return (void *)0; //we reached the end of TZL         
	} else {
		delete(&tzl[index], cell); //step 2 : block is allocated  
		return (void *)cell;
	}

	printf("%d", index);
	return 0;  
}

/**function that looks for a block's buddy **/
/**note : if we find it, we remobe it from the TZL's linked list**/
void * find_buddy(void * ptr, unsigned long size) {
	int index = getIndexOf(size);

	unsigned long buddy = (unsigned long)ptr ^ size; //buddy's location

	if (index < BUDDY_MAX_INDEX) {
		Cell * curr = tzl[index];
		
		//checking if buddy's free 
		while (curr != NULL && 
				((unsigned long)curr != buddy))
			curr = curr->next;
		if (curr != NULL) {
			delete(&tzl[index], curr);
			return (void *) curr;
		} else {
			//problem with xor operation may make program unable 
			//to find the right buddy address (for some odd reason)
			buddy = (unsigned long) ptr + size;		
			curr = tzl[index];
			while (curr != NULL && 
					((unsigned long)curr != buddy))
				curr = curr->next;
			if (curr != NULL) {
				delete(&tzl[index], curr);
				return (void *) curr;
			} 
		} 
	}
	return NULL;
}

int mem_free(void *ptr, unsigned long size)
{
	/**guard check **/
	if (ptr == NULL || (unsigned long)ptr == (unsigned long)-1) return -1;
	if (size > ALLOC_MEM_SIZE || size == 0) return -1;
	if (ptr > mem + ALLOC_MEM_SIZE || ptr < mem) return -1; 
	
	int index = getIndexOf(size);
	Cell * buddy = (Cell *)find_buddy(ptr, size);
	
	//if buddy is null, then we write the block's cell
	if (buddy == NULL) {
		Cell* cell = (Cell*)((unsigned long)ptr);
		cell->size = size;
		cell->next = NULL;	
		add_last(&tzl[index], cell);
		return 0;
	} else {
		//else we merge them, and the new block is written
		//at the smallest location between buddy and ptr's 
		if ((unsigned long)buddy < (unsigned long)ptr) {
			if (size < ALLOC_MEM_SIZE) {
				return mem_free((void*)buddy, size * 2);
			} else return -1;
		} else if (size < ALLOC_MEM_SIZE) {
			return mem_free(ptr, size * 2);
		} else {
			return -1;
		}
	}
	return -1;
}


int mem_destroy()
{
	/**we free all the linked lists**/
	for (int i = 0 ; i < BUDDY_MAX_INDEX ; i++) {
		if (tzl[i]!= NULL) {
			free(tzl[i]);
			tzl[i] = NULL;
		}
	}
	free(mem);
	mem = 0;
	return 0;
}

