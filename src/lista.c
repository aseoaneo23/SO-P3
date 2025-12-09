#include "lista.h"
#include "utils.h"

#include <stdio.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <unistd.h>

bool createNode(tPosL *p) {
    *p = (tPosL)malloc(sizeof(struct tNode));
    return *p != LNULL;
}

void createEmptyList(tList *L) { *L = LNULL; }

bool isEmptyList(tList L) { return L == LNULL; }

tPosL first(tList L) { return L; }

tPosL last(tList L) {
    tPosL p = L;
    while (p->next != NULL) {
        p = p->next;
    }
    return p;
}

tPosL next(tPosL p) { return p->next; }

tPosL previous(tPosL p, tList L) {
    tPosL pAux = L;
    if (p == L) {
        pAux = LNULL;
    } else {
        while (pAux->next != p) {
            pAux = pAux->next;
        }
    }
    return pAux;
}

void updateItem(tItemL d, tPosL p) { p->data = d; }

bool insertItem(tItemL d, tPosL p, tList *L) {
    bool aux = false;
    tPosL q;
    if (!createNode(&q)) {
        // Si el nodo no se pudo crear devuelve false
        aux = false;
    } else {
        q->data = d;
        q->next = LNULL;
        if (isEmptyList(*L)) {
            // Si la lista esta vacia crea el nodo y lo añade manualmente
            *L = q;
            aux = true;
        } else {
            if (p == LNULL) {
                // Lo añade al final
                tPosL pAux = last(*L);
                pAux->next = q;
            } else {
                // En lugar de hallar el anterior de p, lo añadimos despues e
                // intercambiamos informacion entre ellos
                tItemL auxItem;
                q->next = p->next;
                auxItem = p->data;
                p->data = q->data;
                q->data = auxItem;
                p->next = q;
            }
            aux = true;
        }
    }
    return aux;
}

void deleteAtPosition(tPosL p, tList *L) {
    tPosL q = p;
    tItemL d;
    if (p == *L) {
        *L = p->next;
    } else {
        if (p->next == LNULL) {
            // Va al ultimo elemento y lo borras
            previous(p, *L)->next = LNULL;
        } else {
            // En lugar de hallar el anterior de p, lo añadimos despues e
            // intercambiamos informacion entre ellos para poder borrarlos
            p = p->next;
            d = p->data;
            p->data = q->data;
            q->data = d;
            q->next = p->next;
        }
    }
    free(p);
}

void *getItem(tPosL p) { return p->data; }

tPosL findItem(tItemL c, tList L) {
    tPosL p;
    if (isEmptyList(L)) {
        p = LNULL;
    } else {
        for (p = L; (p != LNULL) && strcmp(p->data, c) != 0;
             p = p->next) { // compara si es el ultimo o lo encontro
        }
    }
    return p;
}

// Devuelve la posición de un elemento de la lista partiendo de initPos y
// desplazándose offset-posiciones.
tPosL findItemByOffset(tPosL initPos, int offset, tList list) {
    int i = 0;
    tPosL desiredP = initPos;

    if (offset < 0) {
        while (i > offset && desiredP != first(list)) {
            desiredP = previous(desiredP, list);
            i--;
        }
    } else if (offset > 0) {
        while (i < offset && desiredP != last(list)) {
            desiredP = next(desiredP);
            i++;
        }
    }

    return desiredP;
}

void cleanListFromMemory(tList *L) {
    tPosL p;
    while (!isEmptyList(*L)) {
        p = *L;
        if (p->data != NULL)
            free((char *)p->data);

        *L = next(p);
        free(p);
    }
}

int countItems(tList *L) {
    tPosL pi = first(*L);
    tPosL lastItem = last(*L);
    int count = 0;

    while (pi != lastItem) {
        pi = pi->next;
        count++;
    }

    return count + 1;
}

tPosL getMemByAddress(void *address, tList *memList) {
    tPosL pos = first(*memList);
    tMemItemL *it;

    while (pos != LNULL) {
        it = getItem(pos);
        if (it->direccion == address)
            return pos;
        pos = next(pos);
    }
    return LNULL;
}

void liberarBloque(tItemL item) {
    tMemItemL *i = (tMemItemL *)item;

    if (i->tipo == MALLOC) {
        free(i->direccion);
    } else if (i->tipo == MAPPED) {
        munmap(i->direccion, i->tamano);
        if (i->otra_info->descriptor >= 0)
            close(i->otra_info->descriptor);
    } else if (i->tipo == SHARED) {
        shmdt(i->direccion);
    }

    free(i->fecha);
    free(i->otra_info->nomFichero);
    free(i->otra_info);
    free(i);
}

void cleanMemListFromMemory(tList *memList) {
    tItemL *i;
    tPosL pos = first(*memList);
    tPosL nextPos;
    while (pos != LNULL) {
        nextPos = pos->next;
        i = getItem(pos);
        liberarBloque(i);
        free(pos);
        pos = nextPos;
    }
    *memList = LNULL;
}
