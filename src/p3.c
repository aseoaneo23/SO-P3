/*Authors:
Sofía Oubiña Falcón - sofía.oubiña.falcon@udc.es
Antonio Seoane de Ois - antonio.seoane.deois@udc.gal*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "./utils.h"

#define MAX_LENGTH 100

int main(int argc, char *argv[], char *envp[])
{
    bool terminado = false;
    tList historical;
    tList memoryBlockList;
    tList proccessList;
    struct dirOps ops = {0, 0, 0, 0};

    //Historical list intialization
    createEmptyList(&historical);
    inicializarFicherosEstandar();

    //Memory list inicialization
    createEmptyList(&memoryBlockList);
    createEmptyList(&proccessList);

    while (!terminado) {
        char *comando = (char*)malloc(MAX_LENGTH * sizeof(char));
        imprimirPrompt();
        leerEntrada(comando);
        terminado = procesarEntrada(comando, &historical, &memoryBlockList, &ops, envp, &proccessList);
        free(comando);
    }

    cleanListFromMemory(&historical);
    cleanMemListFromMemory(&memoryBlockList);
    free(proccessList);
    return 0;
}