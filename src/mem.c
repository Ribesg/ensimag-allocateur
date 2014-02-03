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

int getIndexOf(unsigned long size) {
	unsigned long mask = 1;
	int i = 0;
	while (!(size & mask)) {
		i++;
		size >>= 1;
	}
	return i;
}

void add_last(Cell ** list, Cell * cell) {
	if (*list == NULL) {
		*list = cell;
	} else {
		add_last(&((*list)->next), cell);
	}
}

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

void append(Cell ** list, Cell * newCell) {
	if (list == NULL ) list = &newCell;
	else { 
		if (*list  == NULL) {
			*list = newCell;
		} else {
			Cell* current = *list;
			while (current->next != NULL) {
				current = current->next;
			} 
			current->next = newCell;
		}  
	}
} 

void delete_cell(Cell ** list, Cell * cellToDelete) {
	if (list != NULL && *list != NULL) {
		Cell * currentCell = *list;
		if (currentCell == cellToDelete) {
			if (currentCell->next == NULL) 
				*list = NULL;
			else {
				list = &(currentCell->next);
			}
		} else {
			while (currentCell->next != NULL && currentCell->next != cellToDelete) {
				currentCell = currentCell->next;
			}
			if (currentCell->next == cellToDelete) {
				currentCell->next = currentCell->next->next;
			}
		}            
	} 
}

void* split_block(Cell * block, unsigned long requestedSize) {
	if (block->size == 1) return (void *)NULL;
	int index = getIndexOf(block->size);

	Cell ** list_block = &tzl[index];
	delete(list_block, block);

	block->size = block->size / 2;
//	append(&tzl[index - 1], block);
	add_last(&tzl[index - 1], block);

	size_t buddy =((size_t)block) + (size_t)(block->size);
	*(Cell*)buddy = (Cell) { block->size, NULL };
	// (Cell *) block + block->size + sizeof(Cell);
	//	*buddy = 
//	append(&tzl[index - 1], (Cell*)buddy);
	add_last(&tzl[index - 1], (Cell*)buddy);

	if (block->size == requestedSize) {
		delete(&tzl[index - 1], block);
		return (void *)block;
	} else {
		return split_block(block, requestedSize);
	}
}

//function that appends a new cell to a given list
//used for TZL

static void * find_free_block(int index, unsigned long requested_size) {
	if (tzl[index] != NULL) {
		Cell * free_block = tzl[index];
		if (free_block == NULL) {return NULL;}
		else {
			if (free_block->size == requested_size) {
				delete(&tzl[index], free_block);
				return (void *) free_block;
			} else {
				return split_block(free_block, requested_size); 
			}
		}

	} else if (index < BUDDY_MAX_INDEX) { 
		return find_free_block(index + 1, requested_size);
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
	
	*(Cell*)mem = (Cell){ ALLOC_MEM_SIZE, NULL };
	tzl[BUDDY_MAX_INDEX] = mem;

	return 0;
}

void * mem_alloc(unsigned long size)
{
	if (mem == 0) return NULL;
	//step 1 : find a free bloc
	//step 1.1 : find index in tzl
	if (size > ALLOC_MEM_SIZE) return 0; //size is too high
	int index = getIndexOf(size);
	Cell* cell = find_free_block(index, size);
	if (cell == NULL) {
		return (void *)0; //we reached the end of TZL         
	} else {
		delete(&tzl[index], cell);
		return (void *)cell;
	}

	printf("%d", index);    
	/*  ecrire votre code ici */
	return 0;  
}

void * find_buddy(void * ptr, unsigned long size) {
	int index = getIndexOf(size);
	size_t buddy = (size_t)ptr ^ (size_t)size;
	if (index <= BUDDY_MAX_INDEX) {
		Cell * curr = tzl[index];
		while (curr != NULL && ((size_t)curr != buddy))
			curr = curr->next;
		if (curr != NULL) {
			delete(&tzl[index], curr);
			return (void *) curr;
		} else {
			return NULL;
		} 
		return NULL;
	} else {
		return NULL;
	}
}


int mem_free(void *ptr, unsigned long size)
{
	if (ptr == NULL) return 0;
	int index = getIndexOf(size);
	Cell * buddy = (Cell *)find_buddy(ptr, size);
	if (buddy == NULL) {
		*((Cell*)ptr) = (Cell){size, NULL};
		add_last(&tzl[index], ptr);
	} else {
		if (size < ALLOC_MEM_SIZE) {
			mem_free(ptr, size * 2);
		} else {
			return 0;
		}
	}
	return 0;
}


int mem_destroy()
{
	/* ecrire votre code ici */
	for (int i = 0 ; i <= BUDDY_MAX_INDEX ; i++) {
		free(tzl[i]);
	}
	free(mem);
	mem = 0;
	return 0;
}

/*
   void is_free(void * zone) {
   return *(zone + sizeof(Cell)) == NULL;
   }

   void is_buddy_free(void * zone) {
   return *(zone * 2 + sizeof(Cell)) == NULL; 
   }*/
