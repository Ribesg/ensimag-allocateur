/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mem.h"

/** squelette du TP allocateur memoire */

void *mem = 0;

struct cell {
	unsigned long size;
	struct cell* next;
};
typedef struct cell Cell;

Cell* tzl[BUDDY_MAX_INDEX];

int mem_init()
{
	if (!mem) {
		mem = (void *) malloc(ALLOC_MEM_SIZE);
	}

	if (mem == 0) {
		perror("mem_init:");
		return -1;
	}

	for (int i = 0; i < BUDDY_MAX_INDEX - 1; i++) {
		tzl[i] = NULL;
	}
	*(Cell*)mem = (Cell){ ALLOC_MEM_SIZE - sizeof(Cell), NULL };
	tzl[BUDDY_MAX_INDEX - 1] = mem;

	return 0;
}

void * mem_alloc(unsigned long size)
{
	/*  ecrire votre code ici */
	return 0;  
}

int mem_free(void *ptr, unsigned long size)
{
	/* ecrire votre code ici */
	return 0;
}


int mem_destroy()
{
	/* ecrire votre code ici */

	free(mem);
	mem = 0;
	return 0;
}

