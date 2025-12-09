#include "lista_archivos.h"
#include <string.h>

void createEmptyList(tList *L){
    *L=LNULL;
}

bool isEmptyList(tList L){
    if(L==LNULL){
        return true;
    }else{
        return false;
    }
}

tPosL first(tList L){
    return L;
}

tPosL last(tList L){
    tPosL i;
    for(i=L; i->nextnodo!=LNULL; i=i->nextnodo);
    return i;
}

tPosL next(tPosL p, tList L){
    return p->nextnodo;
}

tPosL previous(tPosL p, tList L){
    if(p==L){
        return LNULL;
    }else{
        tPosL i;
        for(i=L; i->nextnodo!=p; i=i->nextnodo);
        return i;
    }
}

bool insertItem(tItemL d, tPosL p, tList *L){
    tNodo *nuevo;
    nuevo = malloc(sizeof(struct tNodo));
    if(nuevo==LNULL){
        return false;
    }else{
        nuevo->nextnodo = LNULL;
        nuevo->info = d;

        if(*L == LNULL){
            *L = nuevo;
        }else{
            tPosL actual;
            if(p == LNULL){
                actual = last(*L);
                actual->nextnodo = nuevo;
            }else{
                if(p == *L){
                    nuevo->nextnodo = *L;
                    *L = nuevo;
                }else{
                    actual = previous(p,*L);
                    actual->nextnodo = nuevo;
                    nuevo->nextnodo = p;
                }
            }
        }
        return true;
    }
}

void deleteAtPosition(tPosL p, tList *L){
    if(p==*L){
        *L=p->nextnodo;
    }else{
        tPosL actual;
        actual=*L;
        while(actual!=LNULL && actual->nextnodo!=p){
            actual=actual->nextnodo;
        }
        actual->nextnodo=p->nextnodo;
    }
    free(p);
}
