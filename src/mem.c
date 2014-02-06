/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mem.h"

/* La zone mémoire */
void* mem = 0;

/* Structure de liste */
struct cell {
	unsigned long size;
	struct cell* next;
};
typedef struct cell Cell;

/* Tableau des zones libres */
Cell* tzl[BUDDY_MAX_INDEX + 1];

/*
 * Fonction utilisée pour éviter math.h qui ne marche pas
 * sur l'installation incertaine d'Etienne
 * Ne fonctionne qu'avec les puissances de 2
 */
static int myLog2(unsigned long size) {
	unsigned long mask = 1;
	int i = 0;
	while (!(size & mask)) {
		i++;
		size >>= 1;
	}
	return i;
}

/*
 * Renvoie la plus petite puissance de 2 supérieure à x
 */
static unsigned long getNextPowerOf2(unsigned long x) {
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x;
}

/*
 * Fonction vérifiant que la taille donnée est supportée,
 * renvoie la taille corrigée si besoin, la taille donnée sinon.
 */
static unsigned long fixSize(unsigned long size) {
	// Vérification de taille minimale
	if (size < sizeof(Cell)) {
		size = (unsigned long)sizeof(Cell);
	}

	// Vérification de puissance de 2
	if ((size & (size - 1)) != 0) {
		size = getNextPowerOf2(size);
	}

	return size;
}

/*
 * Fonction qui ajoute une cellule à la fin d'une liste
 */
static void append(Cell** list, Cell* cell) {
	if (*list == NULL) {
		*list = cell;
	} else {
		append(&((*list)->next), cell);
	}
}

/*
 * Fonction qui supprime le première occurence d'un élément
 * dans une liste.
 * Note : Ne libère pas la mémoire.
 */
static void delete(Cell** list, Cell* cell) {
	if (*list != NULL) {
		if (*list == cell) {
			*list = cell->next;
			cell->next = NULL;
		} else {
			delete(&((*list)->next), cell);
		}
	}
}

/*
 * Fonction qui dichotome un block récursivement tant qu'on
 * n'a pas atteint la taille requise.
 * Les nouveaux blocs libres sont ajoutés au tableau des zones libres.
 */
static void* splitBlock(Cell* block, unsigned long requestedSize) {
	if (block->size == 1) {
		return NULL;
	}
	int index = myLog2(block->size);

	Cell** blockList = &tzl[index];
	delete(blockList, block);

	block->size = block->size / 2;
	append(&tzl[index - 1], block);

	// Calcul de l'emplacement du compagnon
	Cell* buddy = (Cell*)(((unsigned long)block) + block->size);
	buddy->size = block->size;
	buddy->next = NULL;

	append(&tzl[index - 1], buddy);

	if (block->size == requestedSize) {
		delete(&tzl[index - 1], block);
		return (void*)block;
	} else {
		return splitBlock(block, requestedSize);
	}
}

/*
 * Fonction qui trouve un bloc libre de la taille requise.
 * Prend le premier élément du tableau des zones libres s'il n'est pas nul,
 * tente de dichotomer un bloc plus grand sinon.
 */
static void* findFreeBlock(int index, unsigned long requestedSize) {
	if (tzl[index] != NULL) {
		Cell* freeBlock = tzl[index];
		if (freeBlock == NULL) {
			return NULL;
		} else if (freeBlock->size == requestedSize) {
			delete(&tzl[index], freeBlock);
			return (void*)freeBlock;
		} else {
			return splitBlock(freeBlock, requestedSize);
		}
	} else if (index < BUDDY_MAX_INDEX) {
		return findFreeBlock(index + 1, requestedSize);
	} else {
		return NULL;
	}
}

/*
 * Initialise la mémoire.
 */
int mem_init() {
	if (!mem) {
		mem = (void*)malloc(ALLOC_MEM_SIZE);
	}

	if (mem == 0) {
		perror("mem_init:");
		return -1;
	}

	for (int i = 0; i <= BUDDY_MAX_INDEX ; i++) {
		tzl[i] = NULL;
	}

	Cell* cell = (Cell*)mem;
	cell->size = ALLOC_MEM_SIZE;
	cell->next = NULL;
	tzl[BUDDY_MAX_INDEX] = mem;

	return 0;
}

/*
 * Essaye d'allouer une zone mémoire de la taille donnée.
 */
void* mem_alloc(unsigned long size) {
	if (mem == 0 || size == 0) {
		return NULL;
	}
	size = fixSize(size);
	if (size > ALLOC_MEM_SIZE) {
		return NULL;
	}

	int index = myLog2(size);

	Cell* cell = findFreeBlock(index, size);
	if (cell == NULL) {
		// On a atteint la fin du tableau des zones libres :
		// la taille demandée est impossible à allouer.
		return NULL;
	} else {
		delete(&tzl[index], cell);
		return (void*)cell;
	}

	return NULL;
}

/*
 * Fonction qui cherche le compagnon d'un bloc et qui le
 * supprime du tableau des zones libres s'il le trouve.
 * Retourne ledit compagnon.
 */
void* findAndRemoveBuddy(void* ptr, unsigned long size) {
	int index = myLog2(size);

	unsigned long buddy = (((unsigned long)ptr - (unsigned long)mem) ^ size)
	                      + (unsigned long)mem;

	if (index < BUDDY_MAX_INDEX) {
		Cell* curr = tzl[index];

		// Vérification de la disponibilité du compagnon
		while (curr != NULL && ((unsigned long)curr != buddy)) {
			curr = curr->next;
		}
		if (curr != NULL) {
			delete(&tzl[index], curr);
			return (void*)curr;
		}
	}
	return NULL;
}

/*
 * Libère le bloc de taille size pointé par ptr.
 */
int mem_free(void* ptr, unsigned long size)
{
	// Vérification des paramètres
	if (ptr == NULL || (unsigned long)ptr == (unsigned long) - 1
	    || size > ALLOC_MEM_SIZE || size == 0
	    || ptr > mem + ALLOC_MEM_SIZE || ptr < mem) {
		return -1;
	}
	size = fixSize(size);

	int index = myLog2(size);
	Cell* buddy = (Cell*)findAndRemoveBuddy(ptr, size);


	if (buddy == NULL) {
		// Le compagnon est alloué, on ne peut donc pas
		// le fusionner avec ce bloc.
		Cell* cell = (Cell*)((unsigned long)ptr);
		cell->size = size;
		cell->next = NULL;
		append(&tzl[index], cell);
		return 0;
	} else {
		// Le compagnon est libre, on le fusionne avec le bloc
		// à l'adresse la plus petite entre celle du bloc et celle
		// du compagnon.
		if ((unsigned long)buddy < (unsigned long)ptr && size < ALLOC_MEM_SIZE) {
			return mem_free((void*)buddy, size * 2);
		} else if (size < ALLOC_MEM_SIZE) {
			return mem_free(ptr, size * 2);
		}
	}
	return -1; // On n'est pas rentré dans un cas correct
}

/*
 * Libère toute la mémoire.
 */
int mem_destroy()
{
	// Déréférence toute la liste
	for (int i = 0 ; i <= BUDDY_MAX_INDEX ; i++) {
		tzl[i] = NULL;
	}
	free(mem);
	mem = 0;
	return 0;
}

