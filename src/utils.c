#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define _GNU_SOURCE

#include "lista.h"
#include "utils.h"

#define MAX_HOST_LENGTH 256

#define MAX_FICHEROS 100
#define MAX_NAME 256

#define TAM_FECHA 20
#define TAM_HORA 20

#define MAX_PATH 4096
#define TAMANO 4096

#define MAXVAR 2097152

#define AZUL_BLD "\e[1;34m"
#define RESET_COL "\e[0m"
#define VERDE_SCS "\e[0;32m"
#define ROJO_ERR "\e[0;31m"
#define AMARILLO_WARN "\e[0;33m"
#define ORANGE "\x1b[38;2;255;165;0m"
#define RECA 1
#define RECB 2
#define NOREC 0

Fichero listaFicheros[MAX_FICHEROS];
extern char **environ;

int var2; // Variables para el mem -vars
int var3; // Variables para el mem -vars

// Variables para el mem -vars
static char estatico_global[TAMANO];
static char permisos_global[12];
static int contador_estatico;

// 3 estáticas INICIALIZADAS
static int contador_llamadas = 0;
static double version = 1.0;
static const char mensaje[] = "Shell SO 2025";

void imprimirPrompt() {

    char *userName = getlogin();
    char host[MAX_HOST_LENGTH];

    if (gethostname(host, sizeof(host)) != 0) {
        strcpy(host, "maquina_desconocida");
    }
    if (userName == NULL) {
        userName = "desconocido";
    }
    printf(AZUL_BLD "%s@%s~:" RESET_COL, userName, host);
}

void leerEntrada(char *comando) {
    if (fgets(comando, 100, stdin) == NULL) {
        printf("\n");
        exit(EXIT_SUCCESS); // Controlar EOF (CTRL + D)
    }
}

void inicializarFicherosEstandar() {
    AnadirFicherosAbiertos(STDIN_FILENO, "entrada estándar",
                           fcntl(STDIN_FILENO, F_GETFL));
    AnadirFicherosAbiertos(STDOUT_FILENO, "salida estándar",
                           fcntl(STDOUT_FILENO, F_GETFL));
    AnadirFicherosAbiertos(STDERR_FILENO, "error estándar",
                           fcntl(STDERR_FILENO, F_GETFL));
}

bool procesarEntrada(char *comando, tList *historical, tList *memoryBlocksList,
                     struct dirOps *ops, char *envp[]) {

    bool terminado = false;

    updateHistorical(historical, comando);

    char **trozos = malloc(10 * sizeof(char *));
    int num_trozos = TrocearCadena(comando, trozos);

    if (num_trozos == 0) {
        free(trozos);
        return false;
    }

    if (strcmp(trozos[0], "authors") == 0)
        authors(trozos[1]);
    else if (strcmp(trozos[0], "getpid") == 0)
        getShellPid(trozos[1]);
    else if (strcmp(trozos[0], "chdir") == 0 || strcmp(trozos[0], "cd") == 0)
        changeDir(trozos[1]);
    else if (strcmp(trozos[0], "getcwd") == 0)
        printCurrentDir();
    else if (strcmp(trozos[0], "historic") == 0)
        if (trozos[1] == NULL)
            printHistorical(*historical);
        else
            manageHistoricalWMods(trozos[1], historical, memoryBlocksList, ops,
                                  envp);
    else if (strcmp(trozos[0], "exit") == 0 || strcmp(trozos[0], "quit") == 0 ||
             strcmp(trozos[0], "bye") == 0)
        terminado = true;
    else if (strcmp(trozos[0], "help") == 0)
        helpCmd(trozos[1]);
    else if (strcmp(trozos[0], "infosys") == 0)
        infosys(trozos[1]);
    else if (strcmp(trozos[0], "date") == 0)
        dateCmd(trozos[1]);
    else if (strcmp(trozos[0], "close") == 0)
        closeCmd(trozos[1]);
    else if (strcmp(trozos[0], "dup") == 0)
        dupCmd(trozos[1]);
    else if (strcmp(trozos[0], "open") == 0)
        Cmd_open(trozos, num_trozos);
    else if (strcmp(trozos[0], "listopen") == 0)
        Cmd_open(trozos, 0);
    else if (strcmp(trozos[0], "create") == 0)
        if (strcmp(trozos[1], "-f") == 0)
            Cmd_open(trozos, num_trozos);
        else
            create(trozos);
    else if (strcmp(trozos[0], "dir") == 0)
        dirCmd(trozos, num_trozos, ops);
    else if (strcmp(trozos[0], "setdirparams") == 0) {
        *ops = setDirParams(trozos, num_trozos, ops);
    } else if (strcmp(trozos[0], "getdirparams") == 0) {
        getDirParams(ops);
    } else if (strcmp(trozos[0], "erase") == 0)
        erase(trozos, num_trozos);
    else if (strcmp(trozos[0], "lseek") == 0)
        lseekCmd(trozos, num_trozos);
    else if (strcmp(trozos[0], "writestr") == 0)
        writestrCmd(trozos, num_trozos);
    else if (strcmp(trozos[0], "mmap") == 0) {
        do_Mmap(trozos, memoryBlocksList);
    } else if (strcmp(trozos[0], "shared") == 0) {
        if (trozos[1] != NULL && strcmp(trozos[1], "-create") == 0) {
            do_SharedCreate(trozos, memoryBlocksList);
        } else if (trozos[1] != NULL && strcmp(trozos[1], "-free") == 0) {
            if (trozos[2] != NULL)
                doSharedFree(trozos[2], memoryBlocksList);
        } else if (trozos[1] != NULL && strcmp(trozos[1], "-delkey") == 0) {
            if (trozos[2] != NULL)
                do_SharedDelkey(trozos[2]);
        } else {
            do_Shared(trozos, memoryBlocksList);
        }
    } else if (strcmp(trozos[0], "memfill") == 0) {
        if (trozos[1] != NULL && trozos[2] != NULL && trozos[3] != NULL) {
            void *addr = CadenatoPointer(trozos[1]);
            memfill(addr, trozos[2], *trozos[3], memoryBlocksList);
        }
    } else if (strcmp(trozos[0], "memdump") == 0) {
        if (trozos[1] != NULL && trozos[2] != NULL) {
            void *addr2 = CadenatoPointer(trozos[1]);
            memdump(addr2, trozos[2]);
        }
    } else if (strcmp(trozos[0], "recurse") == 0) {
        if (num_trozos > 1) {
            Recursiva(atoi(trozos[1]));
        }
    } else if (strcmp(trozos[0], "mem") == 0) {
        if (num_trozos > 1) {
            mem(trozos[1], memoryBlocksList);
        }
    } else if (strcmp(trozos[0], "malloc") == 0) {
        mallocCmd(trozos, memoryBlocksList);
    } else if (strcmp(trozos[0], "free") == 0) {
        freeCmd(trozos, num_trozos, memoryBlocksList);
    } else if (strcmp(trozos[0], "readfile") == 0) {
        readfileCmd(trozos);
    } else if (strcmp(trozos[0], "writefile") == 0) {
        writefileCmd(trozos);
    } else if (strcmp(trozos[0], "read") == 0)
        readCmd(&trozos[1]);
    else if (strcmp(trozos[0], "write") == 0)
        writeCmd(&trozos[1]);
    else if (strcmp(trozos[0], "envvar") == 0) {
        envvar(trozos, envp);
    } else
        printf(ROJO_ERR "Comando '%s' desconocido\n" RESET_COL, trozos[0]);

    free(trozos);
    return terminado;
}

int TrocearCadena(char *cadena, char *trozos[]) {
    int i = 1;
    if ((trozos[0] = strtok(cadena, " \n\t")) == NULL)
        return 0;
    while ((trozos[i] = strtok(NULL, " \n\t")) != NULL)
        i++;
    return i;
}

// Comandos p0

void authors(char *mod) {
    if (mod == NULL) {
        printf("Antonio Seoane: antonio.seoane.deois@udc.es\n");
        printf("Sofía Oubiña: sofía.oubiña.falcon@udc.es\n");
    } else if (strcmp(mod, "-l") == 0) {
        printf("antonio.seoane.deois@udc.es\n");
        printf("sofía.oubiña.falcon@udc.es\n");
    } else if (strcmp(mod, "-n") == 0) {
        printf("Antonio Seoane\n");
        printf("Sofía Oubiña\n");
    }
}

void getShellPid(char *mod) {
    if (mod != NULL && strcmp(mod, "-p") == 0) {
        printf("Pid del padre del shell: %d\n", getppid());
        return;
    }
    printf("Pid de shell: %d\n", getpid());
}

void printCurrentDir() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror(ROJO_ERR "\ngetcwd() error" RESET_COL);
    }
}

void changeDir(char *path) {
    if (path == NULL) {
        printCurrentDir();
        return;
    }
    int result = chdir(path);

    if (result != 0) {
        perror(ROJO_ERR "Imposible cambiar de directorio" RESET_COL);
    }
}

bool updateHistorical(tList *historical, char *command) {
    if (strcmp(command, "\n") == 0)
        return false;

    char *cmdCopy = strdup(command);
    if (cmdCopy == NULL)
        return false;
    bool insertado = insertItem((tItemL)cmdCopy, LNULL, historical);
    if (!insertado)
        free(cmdCopy);
    return insertado;
}

void printHistorical(tList historical) {
    tPosL pos = first(historical);
    int i = 1;
    while (pos != LNULL) {
        char *cmd = (char *)getItem(pos);
        printf("%d->%s\n", i, cmd);
        pos = next(pos);
        i++;
    }
}

void customHistoricalPrint(MOD type, char *mod, tList historical, tList memList,
                           struct dirOps *ops, char *envp[]) {
    tPosL p = historical;
    int extractedDigit = extractDigit(mod, type), i = 1, totalItems, offset;
    tPosL lastElement = last(historical);

    if (type == N) {
        while (i < extractedDigit && p != LNULL && p != lastElement) {
            p = next(p);
            i++;
        }

        tItemL command = getItem(p);

        if (command != NULL) {
            procesarEntrada(command, &historical, &memList, ops, envp);
        } else {
            printf(
                AMARILLO_WARN
                "No hay comando en la posición %d del historial.\n" RESET_COL,
                extractedDigit);
        }
    } else {
        totalItems = countItems(&historical);
        offset = totalItems - extractedDigit;

        p = findItemByOffset(first(historical), offset, historical);

        while (p != LNULL && extractedDigit > 0) {
            printf("%d: %s\n", offset + 1, (char *)getItem(p));
            p = next(p);
            offset++;
            extractedDigit--;
        };
    }
}

void infosys(char *mod) {
    (void)mod;
    struct utsname info;

    if (uname(&info) == -1) {
        printf(
            ROJO_ERR
            "Error: no se pudo obtener la información del sistema\n" RESET_COL);
        return;
    }
    printf("Información del sistema:\n");
    printf("Sistema Operativo: %s\n", info.sysname);
    printf("Nombre del Nodo: %s\n", info.nodename);
    printf("Versión del Sistema: %s\n", info.version);
    printf("Release del Sistema: %s\n", info.release);
    printf("Arquitectura de la Máquina: %s\n", info.machine);
}

void helpCmd(char *mod) {
    if (mod == NULL) {
        printf("Lista de comandos disponibles:\n");
        printf("authors [-l|-n]\n");
        printf("getpid [-p]\n");
        printf("chdir [dir]\n");
        printf("getcwd\n");
        printf("date [-d|-t]\n");
        printf("hour\n");
        printf("historic [N|-N|-clear|-count]\n");
        printf("open [file] [mode]\n");
        printf("close [df]\n");
        printf("dup [df]\n");
        printf("listopen\n");
        printf("infosys\n");
        printf("help [cmd]\n");
        printf("exit | quit | bye\n");
    } else if (strcmp(mod, "authors") == 0) {
        printf(
            "authors [-l|-n]: Muestra los autores. -l logins, -n nombres.\n");
    } else if (strcmp(mod, "getpid") == 0) {
        printf("getpid [-p]: Muestra el pid del shell o el de su padre.\n");
    } else if (strcmp(mod, "chdir") == 0) {
        printf("chdir [dir]: Cambia el directorio actual. Sin argumento lo "
               "muestra.\n");
    } else if (strcmp(mod, "getcwd") == 0) {
        printf("getcwd: Imprime el directorio actual.\n");
    } else if (strcmp(mod, "date") == 0) {
        printf("date [-d|-t]: Muestra la fecha y la hora actuales. -d solo "
               "fecha, -t solo hora.\n");
    } else if (strcmp(mod, "hour") == 0) {
        printf("hour: Muestra solo la hora actual (igual que date -t).\n");
    } else if (strcmp(mod, "historic") == 0) {
        printf("historic [N|-N|-clear|-count]: Gestiona o muestra el histórico "
               "de comandos.\n");
    } else if (strcmp(mod, "open") == 0) {
        printf("open [file] [modo]: Abre un fichero (cr, ap, ex, ro, rw, wo, "
               "tr). Sin argumentos lista abiertos.\n");
    } else if (strcmp(mod, "close") == 0) {
        printf("close [df]: Cierra el descriptor y lo quita de la lista de "
               "ficheros abiertos.\n");
    } else if (strcmp(mod, "dup") == 0) {
        printf("dup [df]: Duplica el descriptor de fichero y lo añade a la "
               "lista.\n");
    } else if (strcmp(mod, "listopen") == 0) {
        printf("listopen: Lista los ficheros abiertos (igual que open sin "
               "argumentos).\n");
    } else if (strcmp(mod, "infosys") == 0) {
        printf("infosys: Muestra información básica del sistema.\n");
    } else if (strcmp(mod, "help") == 0) {
        printf("help [cmd]: Muestra todos los comandos o la ayuda de un "
               "comando concreto.\n");
    } else if (strcmp(mod, "exit") == 0 || strcmp(mod, "quit") == 0 ||
               strcmp(mod, "bye") == 0) {
        printf("exit | quit | bye: Termina la ejecución del shell.\n");
    } else if (strcmp(mod, "envvar") == 0) {
        printf(AMARILLO_WARN
               "Uso de envvar:\n"
               "  envvar [-show v1 : ver la variable de entorno v1 |\n"
               "          -change [-a : acceso mediante el tercer argumento de "
               "main |\n"
               "                   -e : acceso mediante environ |\n"
               "                   -p : acceso/creación si no existe mediante "
               "putenv]\n"
               "          v val : cambia el valor de la variable v a "
               "val]\n" RESET_COL);
    } else {
        printf(ROJO_ERR "Comando '%s' desconocido en help\n" RESET_COL, mod);
    }
}

void dateCmd(char *mod) {
    time_t ahora = time(NULL);
    struct tm *lt = localtime(&ahora);
    char fecha[TAM_FECHA];
    char hora[TAM_HORA];

    if (ahora == (time_t)-1) {
        perror("time");
        return;
    }

    if (lt == NULL) {
        perror("localtime");
        return;
    }

    if (strftime(fecha, sizeof(fecha), "%d/%m/%Y", lt) == 0)
        return;
    if (strftime(hora, sizeof(hora), "%H:%M:%S", lt) == 0)
        return;

    if (mod == NULL) {
        printf("%s %s\n", fecha, hora);
    } else if (strcmp(mod, "-d") == 0) {
        printf("%s\n", fecha);
    } else if (strcmp(mod, "-t") == 0) {
        printf("%s\n", hora);
    } else {
        printf(AMARILLO_WARN "Uso: date [-d|-t]\n" RESET_COL);
    }
}

void ListaFichAbiertos(void) {
    int aux = 0, modo; // 0 = ninguno encontrado todavía
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (listaFicheros[i].ocupado) {

            printf("fd=%d  nombre=%s flags=", listaFicheros[i].df,
                   listaFicheros[i].name);

            modo = listaFicheros[i].modo;

            // Usamos O_ACCMODE para aislar los bits de modo de acceso
            if ((modo & O_ACCMODE) == O_RDONLY)
                printf("O_RDONLY");
            else if ((modo & O_ACCMODE) == O_WRONLY)
                printf("O_WRONLY");
            else if ((modo & O_ACCMODE) == O_RDWR)
                printf("O_RDWR");

            if (modo & O_CREAT)
                printf("|O_CREAT");
            if (modo & O_EXCL)
                printf("|O_EXCL");
            if (modo & O_APPEND)
                printf("|O_APPEND");
            if (modo & O_TRUNC)
                printf("|O_TRUNC");

            printf("\n");
            aux = 1; // encontramos al menos uno
        }
    }
    if (aux == 0) { // si no se encontró ninguno
        printf("Tabla de ficheros vacía\n");
    }
}

void EliminarFichAbiertos(int df) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (listaFicheros[i].ocupado && listaFicheros[i].df == df) {
            listaFicheros[i].ocupado = 0;
            return;
        }
    }
}

char *NameFicheroDescriptor(int df) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (listaFicheros[i].ocupado && listaFicheros[i].df == df) {
            return listaFicheros[i].name;
        }
    }
    return NULL;
}

void AnadirFicherosAbiertos(int df, const char *name, int modo) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (!listaFicheros[i].ocupado) {
            listaFicheros[i].ocupado = 1;
            listaFicheros[i].df = df;
            listaFicheros[i].modo = modo;
            strncpy(listaFicheros[i].name, name,
                    sizeof(listaFicheros[i].name) - 1);
            listaFicheros[i].name[sizeof(listaFicheros[i].name) - 1] = '\0';
            return;
        }
    }
    printf(AMARILLO_WARN
           "La lista está llena, no se pudo registrar el fichero\n" RESET_COL);
}

void closeCmd(char *mod) {
    int df;

    if (mod == NULL || (df = atoi(mod)) < 0) {
        ListaFichAbiertos();
        return;
    }

    if (close(df) == -1) {
        perror(ROJO_ERR "Imposible cerrar descriptor" RESET_COL);
    } else {
        EliminarFichAbiertos(df);
    }
}

void dupCmd(char *mod) {
    int df, duplicado;
    char aux[MAX_NAME];
    char *p;

    if (mod == NULL || (df = atoi(mod)) < 0) {
        ListaFichAbiertos();
        return;
    }
    p = NameFicheroDescriptor(df);

    if (p == NULL)
        p = "sin nombre registrado";
    duplicado = dup(df);

    if (duplicado == -1) {
        perror("dup");
        return;
    }

    sprintf(aux, "dup %d (%s)", df, p);
    AnadirFicherosAbiertos(duplicado, aux, fcntl(duplicado, F_GETFL));
    printf("%d -> %d\n", df, duplicado);
}

MOD identifyModifier(char *mod) {
    regex_t regex;
    char *permittedMod = "-[0-9]{1,2}";
    int reti;
    MOD result = N;

    regcomp(&regex, permittedMod, REG_EXTENDED);
    reti = regexec(&regex, mod, 0, NULL, 0);

    if (*mod == '-') {
        if (strcmp(mod, "-clear") == 0) {
            result = CLEAR;
        } else if (strcmp(mod, "-count") == 0) {
            result = COUNT;
        } else if (!reti) {
            result = LAST_N;
        } else if (!isdigit(mod[1])) {
            printf("Modificador no reconocido!: %c", mod[1]);
            result = NO_MOD;
        }
    }
    regfree(&regex);
    return result;
}

int extractDigit(char *mod, MOD modifierType) {
    int i = (modifierType == N) ? 0 : 1, retDigit = 0, j = 0;
    size_t modLength = strlen(mod);
    char *digit = malloc((modLength + 1) * sizeof(char));

    while (mod[i] != '\0' && isdigit(mod[i])) {
        digit[j++] = mod[i];
        i++;
    }

    digit[j] = '\n';

    retDigit = atoi(digit);
    free(digit);

    return retDigit;
}

void manageHistoricalWMods(char *mod, tList *historical, tList *memList,
                           struct dirOps *ops, char *envp[]) {
    MOD selectedMod = identifyModifier(mod);

    switch (selectedMod) {
    case N:
    case LAST_N:
        customHistoricalPrint(selectedMod, mod, *historical, *memList, ops,
                              envp);
        break;
    case COUNT:
        printf("Total de elementos en el histórico: %d\n",
               countItems(historical));
        break;
    case CLEAR:
        cleanListFromMemory(historical);
        createEmptyList(historical);
        printf("Historial limpiado\n");
        break;
    default:
        printf(ROJO_ERR "Modificador no reconocido!: %s\n" RESET_COL, mod);
        break;
    }
}

void Cmd_open(char *tr[], int num_trozos) {
    int i, df, mode = 0;
    char *file = tr[1];
    bool isCreate = strcmp(tr[0], "create") == 0;

    if (tr[1] == NULL && !isCreate) { /*no hay parametro*/
        ListaFichAbiertos();
        return;
    }

    if (isCreate) {
        file = tr[2];
        mode |= O_CREAT;
    } else {
        for (i = 2; i < num_trozos && tr[i] != NULL; i++) {
            if (!strcmp(tr[i], "cr"))
                mode |= O_CREAT;
            else if (!strcmp(tr[i], "ex"))
                mode |= O_EXCL;
            else if (!strcmp(tr[i], "ro"))
                mode |= O_RDONLY;
            else if (!strcmp(tr[i], "wo"))
                mode |= O_WRONLY;
            else if (!strcmp(tr[i], "rw"))
                mode |= O_RDWR;
            else if (!strcmp(tr[i], "ap"))
                mode |= O_APPEND;
            else if (!strcmp(tr[i], "tr"))
                mode |= O_TRUNC;
            else
                break;
        }
    }

    if ((df = open(file, mode, 0644)) == -1)
        perror("Imposible abrir fichero");
    else if (!isCreate) {
        AnadirFicherosAbiertos(df, tr[1], mode);
        printf(VERDE_SCS "Anadida entrada %d a la tabla ficheros "
                         "abiertos..................\n" RESET_COL,
               df);
    }

    // Si es una creación, libera el descriptor, para que no ocupe uno en la
    // tabla de ficheros.
    if (isCreate)
        close(df);
}

void create(char *trozos[]) {
    char *pathName;
    pathName = trozos[1];
    if (mkdir(pathName, 0755) == -1) {
        perror(ROJO_ERR "Imposible crear el directorio" RESET_COL);
    }
}

struct dirOps setDirParams(char *trozos[], int numTrozos, struct dirOps *ops) {
    int i = 1;
    if (numTrozos > 1) {
        while (i < numTrozos) {
            if (strcmp(trozos[i], "long") == 0) {
                ops->dLong = 1;
            } else if (strcmp(trozos[i], "short") == 0) {
                ops->dLong = 0;
            } else if (strcmp(trozos[i], "link") == 0) {
                ops->dLink = 1;
            } else if (strcmp(trozos[i], "nolink") == 0) {
                ops->dLink = 0;
            } else if (strcmp(trozos[i], "hid") == 0) {
                ops->dHide = 1;
            } else if (strcmp(trozos[i], "nohid") == 0) {
                ops->dHide = 0;
            } else if (strcmp(trozos[i], "reca") == 0) {
                ops->dRec = 1;
            } else if (strcmp(trozos[i], "recb") == 0) {
                ops->dRec = 2;
            } else if (strcmp(trozos[i], "norec") == 0) {
                ops->dRec = 0;
            } else {
                printf(ROJO_ERR "Se ha indicado un modificador incorrecto para "
                                "el comando 'dir'\n" RESET_COL);
            }

            i++;
        }
    }
    return *ops;
}

char LetraTF(mode_t m) {
    switch (m & S_IFMT) { /*and bit a bit con los bits de formato,0170000 */
    case S_IFSOCK:
        return 's'; /*socket */
    case S_IFLNK:
        return 'l'; /*symbolic link*/
    case S_IFREG:
        return '-'; /* fichero normal*/
    case S_IFBLK:
        return 'b'; /*block device*/
    case S_IFDIR:
        return 'd'; /*directorio */
    case S_IFCHR:
        return 'c'; /*char device*/
    case S_IFIFO:
        return 'p'; /*pipe*/
    default:
        return '?'; /*desconocido, no deberia aparecer*/
    }
}

int EsDirectorio(char *dir) /*para saber si algo es directorio o no*/
{
    struct stat s;
    if (lstat(dir, &s) == -1) /*si no puedo acceder: para mi no es directorio*/
        return 0;
    return (S_ISDIR(s.st_mode));
}

void erase(char *trozos[], int numTrozos) {
    int i = 1;
    struct stat info;

    while (i < numTrozos) {
        if (lstat(trozos[i], &info) == -1) {
            perror(ROJO_ERR "lstat error" RESET_COL);
            i++;
            continue;
        }
        char tipo = LetraTF(info.st_mode);

        if (EsDirectorio(trozos[i])) {
            if (rmdir(trozos[i]) == -1) {
                perror(AMARILLO_WARN "Imposible borrar .." RESET_COL);
            };
        } else if (tipo == '-' || tipo == 'l') {
            if (unlink(trozos[i]) == -1) {
                perror(AMARILLO_WARN "Imposible borrar .." RESET_COL);
            };
        } else {
            printf(ROJO_ERR "No se puede borrar este tipo ..\n" RESET_COL);
        }
        i++;
    }
}

void getDirParams(struct dirOps *ops) {
    printf("Listado: ");
    printf(ops->dLong == 0 ? "corto " : "largo ");
    printf(ops->dLink == 0 ? "sin links " : "con links ");
    if (ops->dRec == 0) {
        printf("no recursivo ");
    } else if (ops->dRec == 1) {
        printf("recursivo (antes) ");
    } else {
        printf("recursivo (después) ");
    }
    printf(ops->dHide == 0 ? "\n" : "con ficheros ocultos\n");
}

// void delRec(char *trozos, int numTrozos) {
// }

void dirCmd(char *trozos[], int numTrozos, struct dirOps *ops) {
    int i = 1;
    struct stat info;
    DIR *dir;
    struct dirent *entrada;
    char ruta[1024];
    bool modoDir = false;
    bool modoHide = ops->dHide == 1;

    // Si no hay argumentos, mostramos el directorio actual
    if (numTrozos == 1) {
        dir = opendir(".");
        if (dir == NULL) {
            perror("Imposible abrir directorio actual");
            return;
        }

        printf("Contenido del directorio actual:\n");

        while ((entrada = readdir(dir)) != NULL) {
            snprintf(ruta, sizeof(ruta), "%c/%s", '.', entrada->d_name);
            if (lstat(ruta, &info) == -1) {
                printf(ROJO_ERR "Fallo lstat directorio %s\n" RESET_COL,
                       entrada->d_name);
                perror("Imposible acceder al directorio");
            };
            // Evita los archivos ocultos si así está indicado en los
            // parámetros.
            if (entrada->d_name[0] == '.' && !modoHide) {
                continue;
            }
            dirPrintCustom(ops, &info, entrada, ruta);
        }

        if (closedir(dir) == -1) {
            perror("Imposible cerrar el directorio");
        };
        return;
    }

    // Si el primer argumento es "-d"
    if (strcmp(trozos[1], "-d") == 0) {
        modoDir = true;
        i = 2;
    }

    while (i < numTrozos) {
        if (lstat(trozos[i], &info) == -1) {
            perror("lstat error");
            i++;
            continue;
        }

        if (EsDirectorio(trozos[i])) {
            if (modoDir) {
                dir = opendir(trozos[i]);
                if (dir == NULL) {
                    perror("Imposible abrir directorio");
                    i++;
                    continue;
                }

                printf("*****Contenido de %s:\n", trozos[i]);
                while ((entrada = readdir(dir)) != NULL) {
                    snprintf(ruta, sizeof(ruta), "%s/%s", trozos[i],
                             entrada->d_name);
                    if (lstat(ruta, &info) == -1) {
                        printf(ROJO_ERR "Fallo lstat directorio %s\n" RESET_COL,
                               entrada->d_name);
                        perror("Imposible acceder al directorio");
                    };
                    // Evita los archivos ocultos si así está indicado en los
                    // parámetros.
                    if (entrada->d_name[0] == '.' && !modoHide) {
                        continue;
                    }
                    dirPrintCustom(ops, &info, entrada, ruta);
                }

                printf("****\n");
                closedir(dir);
            } else {
                printf("%s/\n", trozos[i]);
            }
        } else {
            char *nombreArchivo = trozos[i];
            if (modoHide || nombreArchivo[0] != '.') {
                printf("%ld\t%s\n", (long)info.st_size, trozos[i]);
            }
        }

        i++;
    }
}

char *extractRealPath(char *path) {
    char *pathResult;
    if (path == NULL) {
        printf("El path no se puede extraer si está vacío.");
        return NULL;
    }
    strcat(path, "\0");
    pathResult = realpath(path, NULL);
    return pathResult;
}

// void dirRecAfter(char *dirPath, struct dirOps *ops, char *modoDir) {
//     struct dirent *entrada;
//     DIR *dir;
//     struct stat info;
//     char *realpath;

//     while ((entrada = readdir(dir) != NULL)){

//     }
// }

void formatLastEditDate(struct stat *dirInfo, char *fecha, char *hora) {
    struct tm *lt = localtime(&dirInfo->st_mtime);

    // Fecha en formato: YYYY/MM/DD
    strftime(fecha, TAM_FECHA, "%Y/%m/%d ", lt);

    // Hora en formato: HH:MM
    strftime(hora, TAM_HORA, " %H:%M", lt);
}

void formatDate(time_t time, char *formattedDate) {
    char formattedDateTime[40];
    if (time == 0)
        strcpy(formattedDate, "ERROR_ON_FORMAT_DATE");

    struct tm *lt = localtime(&time);
    size_t bytesFormatted = strftime(formattedDateTime, TAM_FECHA + TAM_HORA,
                                     "%Y/%m/%d - %H:%M", lt);

    if (bytesFormatted < 1)
        perror("ERROR AL FORMATEAR LA FECHA.");
    else
        strcpy(formattedDate, formattedDateTime);
}

char *ConvierteModo2(mode_t m) {
    static char permisos[12];
    strcpy(permisos, "---------- ");

    permisos[0] = LetraTF(m);
    if (m & S_IRUSR)
        permisos[1] = 'r'; /*propietario*/
    if (m & S_IWUSR)
        permisos[2] = 'w';
    if (m & S_IXUSR)
        permisos[3] = 'x';
    if (m & S_IRGRP)
        permisos[4] = 'r'; /*grupo*/
    if (m & S_IWGRP)
        permisos[5] = 'w';
    if (m & S_IXGRP)
        permisos[6] = 'x';
    if (m & S_IROTH)
        permisos[7] = 'r'; /*resto*/
    if (m & S_IWOTH)
        permisos[8] = 'w';
    if (m & S_IXOTH)
        permisos[9] = 'x';
    if (m & S_ISUID)
        permisos[3] = 's'; /*setuid, setgid y stickybit*/
    if (m & S_ISGID)
        permisos[6] = 's';
    if (m & S_ISVTX)
        permisos[9] = 't';

    return permisos;
}

void dirPrintCustom(struct dirOps *ops, struct stat *dirInfo,
                    struct dirent *entrada, char *ruta) {
    char fecha[20], hora[10];
    formatLastEditDate(dirInfo, fecha, hora);
    struct passwd *userData = getpwuid(dirInfo->st_uid);
    struct group *group = getgrgid(dirInfo->st_gid);
    char fileType = LetraTF(dirInfo->st_mode);

    char *prop = userData != NULL ? userData->pw_name : "desconocido";
    char *grp = group != NULL ? group->gr_name : "desconocido";

    char *permisos = ConvierteModo2(dirInfo->st_mode);

    if (fileType == 'l' && ops->dLink == 1) {
        char destino[1024];
        ssize_t len = readlink(ruta, destino, sizeof(destino) - 1);
        if (len != -1) {
            destino[len] = '\0'; // añadir terminador
        } else {
            snprintf(destino, sizeof(destino), "??");
        }

        if (ops->dLong == 0) {
            printf("%zd\t%s -> %s\n", dirInfo->st_size, entrada->d_name,
                   destino);
        } else if (ops->dLong == 1) {
            printf("%s-%s %2ld (%8ld)  %-8s %-8s %-10s %8ld %s -> %s\n", fecha,
                   hora, dirInfo->st_nlink, dirInfo->st_ino, prop, grp,
                   permisos, dirInfo->st_size, entrada->d_name, destino);
        }
    } else {
        if (ops->dLong == 0) {
            printf("%zd\t%s\n", dirInfo->st_size, entrada->d_name);
        } else if (ops->dLong == 1) {
            printf("%s-%s %2ld (%8ld)  %-8s %-8s %-10s %8ld %s\n", fecha, hora,
                   dirInfo->st_nlink, dirInfo->st_ino, prop, grp, permisos,
                   dirInfo->st_size, entrada->d_name);
        }
    }
}

void lseekCmd(char *trozos[], int numTrozos) {
    int df, desplazamiento, referencia;
    off_t nuevaPos;

    if (numTrozos != 4) {
        printf("Uso: lseek df off ref\n");
        return;
    }

    df = atoi(trozos[1]);
    desplazamiento = atoi(trozos[2]);

    if (strcmp(trozos[3], "SEEK_SET") == 0)
        referencia = SEEK_SET;
    else if (strcmp(trozos[3], "SEEK_CUR") == 0)
        referencia = SEEK_CUR;
    else if (strcmp(trozos[3], "SEEK_END") == 0)
        referencia = SEEK_END;
    else {
        printf("Referencia incorrecta. Use SEEK_SET, SEEK_CUR o SEEK_END\n");
        return;
    }

    nuevaPos = lseek(df, desplazamiento, referencia);

    if (nuevaPos == (off_t)-1) {
        perror("Error en lseek");
    } else {
        printf("Nueva posición del puntero: %ld\n", (long)nuevaPos);
    }
}

void writestrCmd(char *trozos[], int numTrozos) {
    int i = 2;
    char texto[4096];
    if (numTrozos < 3) {
        printf("Uso: writestr df texto\n");
        return;
    }

    int df = atoi(trozos[1]);
    if (df < 0) {
        printf("Descriptor de archivo no válido.\n");
        return;
    }
    texto[0] = '\0';
    while (i < numTrozos) {
        strcat(texto, trozos[i]);
        if (i < numTrozos - 1) {
            strcat(texto, " ");
        }
        i++;
    }

    ssize_t bytesEscritos = write(df, texto, strlen(texto));

    if (bytesEscritos == -1) {
        perror("Error al escribir en el fichero");
    } else {
        printf("Se han escrito %zd bytes en el descriptor %d\n", bytesEscritos,
               df);
    }
}

void *MapearFichero(char *fichero, int protection, tList *memBlocksList) {
    int df, map = MAP_PRIVATE, modo = O_RDONLY;
    struct stat s;
    void *p;
    char date[TAM_FECHA + TAM_HORA];
    char descriptorListName[1024];

    if (protection & PROT_WRITE)
        modo = O_RDWR;
    if (stat(fichero, &s) == -1 || (df = open(fichero, modo)) == -1)
        return NULL;
    if ((p = mmap(NULL, s.st_size, protection, map, df, 0)) == MAP_FAILED)
        return NULL;

    tMemItemL *bloque = malloc(sizeof(tMemItemL));
    otraInfo *oi = malloc(sizeof(otraInfo));
    bloque->direccion = p;
    bloque->tamano = s.st_size;
    formatDate(time(NULL), date);
    bloque->fecha = strdup(date);
    bloque->tipo = MAPPED;
    oi->nomFichero = strdup(fichero);
    oi->descriptor = df;
    oi->shm_key =
        -1; // Valor centinela para cuando no es necesario guardar clave
    bloque->otra_info = oi;

    sprintf(descriptorListName, "Mapeado de %s", fichero);
    insertItem(bloque, LNULL, memBlocksList);
    AnadirFicherosAbiertos(df, descriptorListName, modo);
    return p;
}

bool unmapFichero(char *fichero, tList *memList) {
    struct stat s;
    bool result = true;

    if (stat(fichero, &s) == -1)
        return false;

    tPosL pos = first(*memList);

    while (pos != LNULL) {
        tMemItemL *bloque = getItem(pos);
        if (strcmp(bloque->otra_info->nomFichero, fichero) == 0) {
            if (munmap(bloque->direccion, s.st_size) == -1) {
                perror("No se pudo hacer (munmap) munmap");
                return false;
            }
            deleteAtPosition(pos, memList);
            printf("Se ha desmapeado %s de memoria\n", fichero);
            return result;
        }
        pos = next(pos);
    }
    result = false;
    return result;
}

void do_Mmap(char *arg[], tList *memBlocksList) {
    char *perm;
    void *p;
    int protection = 0;

    if (arg[1] == NULL) {
        imprimirListaMem(memBlocksList, MAPPED);
        return;
    }
    if (strcmp(arg[1], "-free") == 0) {
        if (arg[2] == NULL)
            return;
        else {
            bool result = unmapFichero(arg[2], memBlocksList);
            if (!result) {
                printf("No se pudo hacer el unmap.\n");
            }
            return;
        }
    }
    if ((perm = arg[2]) != NULL && strlen(perm) < 4) {
        if (strchr(perm, 'r') != NULL)
            protection |= PROT_READ;
        if (strchr(perm, 'w') != NULL)
            protection |= PROT_WRITE;
        if (strchr(perm, 'x') != NULL)
            protection |= PROT_EXEC;
    }
    if (arg[2] == NULL)
        protection |= PROT_READ;

    if ((p = MapearFichero(arg[1], protection, memBlocksList)) == NULL)
        perror("Imposible mapear fichero");
    else
        printf("fichero %s mapeado en %p\n", arg[1], p);
}

const char *tipoToString(ALLOC_TYPE tipo) {
    switch (tipo) {
    case MALLOC:
        return "MALLOC";
    case MAPPED:
        return "MAPPED";
    case SHARED:
        return "SHARED";
    case ALL:
        return "ALL";
    default:
        return "DESCONOCIDO";
    }
}

void imprimirListaMem(tList *memoryList, ALLOC_TYPE tipoMem) {
    bool tieneOtraInfo;
    tPosL pos = first(*memoryList);
    printf(
        ORANGE
        "***LISTA DE BLOQUES ASIGNADOS %s PARA EL PROCESO %d***\n\n" RESET_COL,
        tipoToString(tipoMem), getpid());

    printf("%-18s %-10s %-12s %-25s %-15s %-15s %-10s\n", "Direccion", "Tamano",
           "Tipo", "Fecha", "Fichero", "Descritpor", "Clave");

    while (pos != LNULL) {
        tMemItemL *bloque = getItem(pos);
        tieneOtraInfo = bloque->otra_info != NULL;
        if (tipoMem != ALL) {
            if (bloque->tipo == tipoMem) {
                printf("%-18p %-10zu %-12s %-25s %-15s %-15d %-10d\n",
                       bloque->direccion, bloque->tamano,
                       tipoToString(bloque->tipo), bloque->fecha,
                       tieneOtraInfo ? bloque->otra_info->nomFichero : "-",
                       tieneOtraInfo ? bloque->otra_info->descriptor : -1,
                       tieneOtraInfo ? bloque->otra_info->shm_key
                                     : -1 // Ojo, puede tener un -1 como key por
                                          // no tener bloque info (malloc) o por
                                          // no tenener key (mapped)
                );
            }
        } else {
            printf("%-18p %-10zu %-12s %-25s %-15s %-15d %-10d\n",
                   bloque->direccion, bloque->tamano,
                   tipoToString(bloque->tipo), bloque->fecha,
                   tieneOtraInfo ? bloque->otra_info->nomFichero : "-",
                   tieneOtraInfo ? bloque->otra_info->descriptor : -1,
                   tieneOtraInfo ? bloque->otra_info->shm_key : -1);
        }

        pos = next(pos);
    }
    free(pos);
}

void *ObtenerMemoriaShmget(key_t clave, size_t tam, tList *memoryBlockList) {
    void *p;
    int aux, id,
        flags = 0777; /*los 9 bits menos significativos de los flags:permisos*/
    struct shmid_ds s;
    char date[TAM_FECHA + TAM_HORA];

    if (tam) /*tam distito de 0 indica crear */
        flags = flags | IPC_CREAT |
                IPC_EXCL;     /*cuando no es crear pasamos de tamano 0*/
    if (clave == IPC_PRIVATE) /*no nos vale*/
    {
        errno = EINVAL;
        return NULL;
    }
    if ((id = shmget(clave, tam, flags)) == -1)
        return (NULL);
    if ((p = shmat(id, NULL, 0)) == (void *)-1) {
        aux = errno;
        if (tam)
            shmctl(id, IPC_RMID, NULL);
        errno = aux;
        return (NULL);
    }
    shmctl(id, IPC_STAT,
           &s); /* si no es crear, necesitamos el tamano, que es s.shm_segsz*/
    tMemItemL *bloque = malloc(sizeof(tMemItemL));
    otraInfo *info = malloc(sizeof(otraInfo));

    bloque->direccion = p;
    bloque->tamano = s.shm_segsz;
    formatDate(s.shm_ctime, date);
    bloque->fecha = strdup(date);
    bloque->tipo = SHARED;
    info->descriptor = -1;
    info->nomFichero = strdup("-");
    info->shm_key = clave;
    bloque->otra_info = info;
    bool insertado = insertItem(bloque, LNULL, memoryBlockList);

    if (!insertado) {
        perror("No se ha podido insertar el SHARED en la lista");
    }
    return (p);
}
void do_SharedCreate(char *tr[], tList *memoryList) {
    key_t cl;
    size_t tam;
    void *p;

    if (tr[1] == NULL || tr[2] == NULL) {
        imprimirListaMem(memoryList, SHARED);
        return;
    }

    cl = (key_t)strtoul(tr[2], NULL, 10);
    tam = (size_t)strtoul(tr[3], NULL, 10);
    if (tam == 0) {
        printf("No se asignan bloques de 0 bytes\n");
        return;
    }
    if ((p = ObtenerMemoriaShmget(cl, tam, memoryList)) != NULL)
        printf("Asignados %lu bytes en %p\n", (unsigned long)tam, p);
    else
        printf("Imposible asignar memoria compartida clave %lu:%s\n",
               (unsigned long)cl, strerror(errno));
}

void do_Shared(char *tr[], tList *memoryList) {
    key_t cl;
    void *p;

    if (tr[1] == NULL) {
        imprimirListaMem(memoryList, SHARED);
        return;
    }

    cl = (key_t)strtoul(tr[1], NULL, 10);

    if ((p = ObtenerMemoriaShmget(cl, 0, memoryList)) != NULL)
        printf("Asignada memoria compartida de clave %lu en %p\n",
               (unsigned long)cl, p);
    else
        printf("Imposible asignar memoria compartida clave %lu:%s\n",
               (unsigned long)cl, strerror(errno));
}

void *buscarNodo(tList *memList, key_t clave, tPosL *p) {
    tPosL pos = first(*memList);

    while (pos != LNULL) {
        tMemItemL *nodo = getItem(pos);
        if (nodo->otra_info->shm_key != -1 &&
            (nodo->otra_info->shm_key == clave)) {
            if (p != NULL) {
                *p = pos;
            }
            return nodo->direccion;
        }
        pos = next(pos);
    }
    // No encontrado
    if (p != NULL) {
        *p = LNULL;
    }
    return NULL;
}

void doSharedFree(char *clave, tList *memList) {
    void *p; /*llamo a la funcion de la lista*/
             /*que devuelve la direccion donde*/
    tPosL posABorrar;
    key_t cl;

    cl = (key_t)strtoul(clave, NULL, 10);
    if (cl == IPC_PRIVATE || cl == 0) {
        printf("free necesita una clave accesible");
    } else {
        if ((p = buscarNodo(memList, cl, &posABorrar)) ==
            NULL) { /*esta la memoria de clave cl*/
            printf("No hay bloque de esa clave mapeado en el proceso\n");
            return;
        }
        shmdt(p);
        deleteAtPosition(posABorrar, memList);
    }
}

void do_SharedDelkey(char *key) {
    key_t clave;
    int id;

    if (key == NULL || (clave = (key_t)strtoul(key, NULL, 10)) == IPC_PRIVATE) {
        printf("      delkey necesita clave_valida\n");
        return;
    }
    if ((id = shmget(clave, 0, 0666)) == -1) {
        perror("shmget: imposible obtener memoria compartida");
        return;
    }
    if (shmctl(id, IPC_RMID, NULL) == -1)
        perror("shmctl: imposible eliminar memoria compartida\n");

    printf("La clave %s se ha borrado correctamente\n", key);
}

void LlenarMemoria(void *p, size_t cont, unsigned char byte) {
    unsigned char *arr = (unsigned char *)p;
    size_t i;

    for (i = 0; i < cont; i++)
        arr[i] = byte;
}

void memfill(void *addr, char *size, unsigned char byte, tList *memList) {
    size_t cont;
    tPosL memPos;
    tMemItemL *item;

    if ((cont = (size_t)strtoul(size, NULL, 10)) <= 0) {
        printf("Error en la cantidad de datos a llenar (memfill)\n");
        return;
    }

    memPos = getMemByAddress(addr, memList);
    if (memPos == LNULL) {
        printf(
            "No se puede llenar dicha dirección ya que no está disponible\n");
        return;
    }

    item = getItem(memPos);
    if (item != NULL && item->tamano < cont) {
        printf(ROJO_ERR
               "No se puede llenar más memoria de la reservada\n" RESET_COL);
        return;
    }

    LlenarMemoria(addr, cont, byte);
}

void memdump(void *addr, char *cont0) {
    size_t cont;
    if (addr == NULL || strcmp(cont0, "0") == 0) {
        printf("memdump: dirección nula o tamaño cero\n");
        return;
    }
    cont = (size_t)strtoul(cont0, NULL, 10);
    unsigned char *p = (unsigned char *)addr;
    size_t i;

    printf("memdump %p  (%zu bytes):\n", p, cont);

    for (i = 0; i < cont; i += 16) {
        // Offset
        printf("%08zx  ", i);

        // Parte hexadecimal (16 bytes)
        for (size_t j = 0; j < 16; j++) {
            if (i + j < cont)
                printf("%02x ", p[i + j]);
            else
                printf("   ");
            if (j == 7)
                printf(" "); // espacio entre grupos de 8
        }

        for (size_t j = 0; j < 16 && i + j < cont; j++) {
            unsigned char c = p[i + j];

            if (c == '\n')
                printf("\\n");
            else if (c == '\t')
                printf("\\t");
            else if (c == '\r')
                printf("\\r");
            else if (isprint(c))
                printf("%c", c);
            else
                printf(" ");
        }

        printf("\n");
    }
}

void Recursiva(int n) {
    char automatico[TAMANO];
    static char estatico[TAMANO];

    printf("parametro:%3d(%p) array %p, arr estatico %p\n", n, &n, automatico,
           estatico);

    if (n > 0)
        Recursiva(n - 1);
}

void printFuncs() {
    printf("Dirección de la función mem:        %p\n", (void *)mem);
    printf("Dirección de la función getShellPid: %p\n", (void *)getShellPid);
    printf("Dirección de la función mallocCmd:  %p\n", (void *)mallocCmd);
    printf("Dirección de malloc (libc):         %p\n", (void *)malloc);
    printf("Dirección de free (libc):           %p\n", (void *)free);
    printf("Dirección de open (libc):           %p\n", (void *)open);
}

void printVars() {
    // 3 funcs automáticas locales
    char auto1[100];
    int auto2 = 42;
    float auto3 = 3.14159f;

    // 3 externas no inicializadas
    printf("Externas no inicializadas:\n");
    printf("  listaFicheros: %p\n", (void *)listaFicheros);
    printf("  var2:          %p\n", (void *)&var2);
    printf("  var3:          %p\n", (void *)&var3);

    // 3 externas inicializadas (cadenas de color)
    printf("Externas inicializadas:\n");
    printf("  AZUL_BLD:      %p  -> \"%s\"\n", (void *)AZUL_BLD, AZUL_BLD);
    printf(RESET_COL "  VERDE_SCS:     %p  -> \"%s\"\n", (void *)VERDE_SCS,
           VERDE_SCS);
    printf(RESET_COL "  ROJO_ERR:      %p  -> \"%s\"\n", (void *)ROJO_ERR,
           ROJO_ERR);

    printf(RESET_COL "Estáticas no inicializadas:\n");
    printf("  estatico_global:     %p\n", (void *)estatico_global);
    printf("  permisos_global:     %p\n", (void *)permisos_global);
    printf("  contador_estatico:   %p\n", (void *)&contador_estatico);

    printf("Estáticas inicializadas:\n");
    printf("  contador_llamadas:   %p (valor = %d)\n",
           (void *)&contador_llamadas, contador_llamadas);
    printf("  version:             %p (valor = %.1f)\n", (void *)&version,
           version);
    printf("  mensaje:             %p (\"%s\")\n", (void *)mensaje, mensaje);

    printf("Automáticas (locales):\n");
    printf("  auto1 (array): %p\n", (void *)auto1);
    printf("  auto2 (int):   %p\n", (void *)&auto2);
    printf("  auto3 (float): %p\n", (void *)&auto3);
}

void mem(char *param, tList *memoryList) {

    if (strcmp(param, "-funcs") == 0) {
        printFuncs();
    } else if (strcmp(param, "-vars") == 0) {
        printVars();
    } else if (strcmp(param, "-blocks") == 0) {
        imprimirListaMem(memoryList, ALL);
    } else if (strcmp(param, "-all") == 0) {
        printFuncs();
        printVars();
        imprimirListaMem(memoryList, ALL);
    } else if (strcmp(param, "-pmap") == 0) {
        Do_pmap();
    } else {
    }
}

void mallocCmd(char *trozos[], tList *memList) {
    // malloc:  lista los bloques
    // malloc n: reserva n bytes
    // malloc -free n :  libera un bloque malloc de tamaño n
    char formattedDate[TAM_FECHA + TAM_HORA];
    time_t tiempo = time(NULL);

    if (trozos[1] == NULL) {
        // no argumentos =mostrar lista bloques malloc
        imprimirListaMem(memList, MALLOC);
        return;
    }

    if (strcmp(trozos[1], "-free") == 0) {
        if (trozos[2] == NULL) {
            printf("Uso: malloc -free tamaño\n");
            return;
        }
        size_t tam = (size_t)atoll(trozos[2]);
        if (tam == 0)
            return;
        tPosL p = first(*memList);
        while (p != LNULL) {
            tMemItemL *itemMem = getItem(p);
            if (itemMem->tipo == MALLOC && itemMem->tamano == tam) {
                liberarBloque(itemMem);
                deleteAtPosition(p, memList);
                printf(VERDE_SCS
                       "Bloque malloc de %ld bytes liberado\n" RESET_COL,
                       tam);
                return;
            } else {
                p = next(p);
            }
        }
        printf(AMARILLO_WARN
               "No se encontró bloque de %ld bytes en la lista\n" RESET_COL,
               tam);
        return;
    }

    // malloc n: reservar
    size_t tam = (size_t)strtoul(trozos[1], NULL, 10);
    if (tam == 0) {
        printf("No se pueden asignar 0 bytes\n");
        return;
    }

    void *p = malloc(tam);
    if (p == NULL) {
        perror("malloc");
        return;
    }

    formatDate(tiempo, formattedDate);
    // guardar el bloque en la lista
    tMemItemL *item = malloc(sizeof(tMemItemL));
    otraInfo *oi = malloc(sizeof(otraInfo));
    item->direccion = p;
    item->tamano = tam;
    item->fecha = strdup(formattedDate);
    item->tipo = MALLOC;
    oi->descriptor = -1;
    oi->nomFichero = strdup("-");
    oi->shm_key = -1;
    item->otra_info = oi;

    insertItem(item, LNULL, memList);
    printf(VERDE_SCS "Asignados %ld bytes en %p\n" RESET_COL, tam, p);
}

void freeCmd(char *trozos[], int, tList *memList) {
    tPosL p;
    tMemItemL *itemMem;

    if (trozos[1] == NULL) {
        printf("Uso: free addr\n");
        return;
    }

    void *addr = NULL;
    sscanf(trozos[1], "%p", &addr); // convertir texto a puntero

    if (addr == NULL) {
        printf(ROJO_ERR "Dirección no válida\n" RESET_COL);
        return;
    }
    // uscar en la lista de memoria el bloque
    p = first(*memList);
    while (p != LNULL) {
        itemMem = getItem(p);
        if (itemMem->direccion == addr) {
            liberarBloque(itemMem);
            deleteAtPosition(p, memList);
            printf(VERDE_SCS "Bloque en %p liberado\n" RESET_COL, addr);
            return;
        }
        p = next(p);
    }
    printf(AMARILLO_WARN "No se encontró el bloque en %p\n" RESET_COL, addr);
}

ssize_t LeerFichero(char *f, void *p, size_t cont) {
    struct stat s;
    ssize_t n;
    int df, aux;

    if (stat(f, &s) == -1 || (df = open(f, O_RDONLY)) == -1)
        return -1;
    if (cont == (long unsigned int)-1) /* si pasamos -1 como bytes a leer lo
                                          leemos entero*/
        cont = s.st_size;
    if ((n = read(df, p, cont)) == -1) {
        aux = errno;
        close(df);
        errno = aux;
        return -1;
    }
    close(df);
    return n;
}
void *CadenatoPointer(char *s) {
    void *p;
    sscanf(s, "%p", &p);
    if (p == NULL)
        errno = EFAULT;
    return p;
}

void readfileCmd(char *trozos[]) {
    void *p;
    size_t cont = (size_t)-1; // si no s indica tamaño se lee etnero
    ssize_t n;
    // comprobar parámetros
    if (trozos[1] == NULL || trozos[2] == NULL) {
        printf(ROJO_ERR "Faltan parámetros\n" RESET_COL);
        return;
    }

    p = CadenatoPointer(trozos[2]); // convertir cadena a punter real
    if (trozos[3] != NULL)          // si hay 3, se usa como num de bytes a leer
        cont = (size_t)atoll(trozos[3]);

    if ((n = LeerFichero(trozos[1], p, cont)) == -1) {
        perror(ROJO_ERR "Imposible leer fichero" RESET_COL);
    } else {

        printf(VERDE_SCS "Leídos %lld bytes de %s en %p\n" RESET_COL,
               (long long)n, trozos[1], p);
    }
}

void writefileCmd(char *trozos[]) {
    // writefile fichero addr cont
    if (trozos[1] == NULL || trozos[2] == NULL || trozos[3] == NULL) {
        printf("Uso: writefile fichero addr cont\n");
        return;
    }

    char *fich = trozos[1];
    void *addr = CadenatoPointer(trozos[2]); // convierte direciona puntero
    size_t cont = atoll(trozos[3]);          // num bytes a escribir
    // abre o crea fichero
    int df = open(fich, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (df == -1) {
        perror(ROJO_ERR "Error al abrir fichero" RESET_COL);
        return;
    }

    ssize_t escritos = write(df, addr, cont); // escribir datos en el fichero
    if (escritos == -1) {
        perror(ROJO_ERR "Error al escribir fichero" RESET_COL);
    } else {
        printf(VERDE_SCS "Escritos %zd bytes en %s desde %p\n" RESET_COL,
               escritos, fich, addr);
    }

    close(df);
}

void readCmd(char *tr[]) {
    int df;
    void *p;
    size_t cont;
    ssize_t n;

    if (tr[0] == NULL || tr[1] == NULL || tr[2] == NULL) {
        printf("Uso: read <df> <addr> <cont>\n");
        return;
    }

    df = atoi(tr[0]);            // descriptor fihcero
    p = CadenatoPointer(tr[1]);  // dirección de memoria destino
    cont = (size_t)atoll(tr[2]); // bytes. a leer

    if (p == NULL) {
        perror("Dirección de memoria no válida");
        return;
    }

    if ((n = read(df, p, cont)) == -1) { // lectura desde descriptor
        perror("Imposible leer del descriptor");
    } else {
        printf("leídos %lld bytes del descriptor %d en %p\n", (long long)n, df,
               p);
    }
}

void writeCmd(char *tr[]) {
    int df;
    void *p;
    size_t cont;
    ssize_t n;

    if (tr[0] == NULL || tr[1] == NULL || tr[2] == NULL) {
        printf("Uso: write <df> <addr> <cont>\n");
        return;
    }

    df = atoi(tr[0]);
    p = CadenatoPointer(tr[1]);
    cont = (size_t)atoll(tr[2]);

    if (p == NULL) {
        perror("Dirección de memoria no válida");
        return;
    }
    // escribir en el descriptor
    if ((n = write(df, p, cont)) == -1) {
        perror("Imposible escribir en el descriptor");
    } else {
        printf("escritos %lld bytes en descriptor %d desde %p\n", (long long)n,
               df, p);
    }
}

void Do_pmap(void) /*sin argumentos*/
{
    pid_t pid; /*hace el pmap (o equivalente) del proceso actual*/
    char elpid[32];
    char *argv[4] = {"pmap", elpid, NULL};

    sprintf(elpid, "%d", (int)getpid());
    if ((pid = fork()) == -1) {
        perror("Imposible crear proceso");
        return;
    }
    if (pid == 0) {
        if (execvp(argv[0], argv) == -1)
            perror("cannot execute pmap (linux, solaris)");

        argv[0] = "procstat";
        argv[1] = "vm";
        argv[2] = elpid;
        argv[3] = NULL;
        if (execvp(argv[0], argv) ==
            -1) /*No hay pmap, probamos procstat FreeBSD */
            perror("cannot execute procstat (FreeBSD)");

        argv[0] = "procmap", argv[1] = elpid;
        argv[2] = NULL;
        if (execvp(argv[0], argv) == -1) /*probamos procmap OpenBSD*/
            perror("cannot execute procmap (OpenBSD)");

        argv[0] = "vmmap";
        argv[1] = "-interleave";
        argv[2] = elpid;
        argv[3] = NULL;
        if (execvp(argv[0], argv) == -1) /*probamos vmmap Mac-OS*/
            perror("cannot execute vmmap (Mac-OS)");
        exit(1);
    }
    waitpid(pid, NULL, 0);
}

/*P3 - PROCESOS MULTIPROGRAMADOS */

int BuscarVariable(char *var,
                   char *e[]) { /*busca una variable en el entorno que se le
                                   pasa como parámetro devuelve la posicion de
                                   la variable en el entorno, -1 si no existe*/
    int pos = 0;
    char aux[MAXVAR];

    strcpy(aux, var);
    strcat(aux, "=");

    while (e[pos] != NULL)
        if (!strncmp(e[pos], aux, strlen(aux)))
            return (pos);
        else
            pos++;
    errno = ENOENT; /*no hay tal variable*/
    return (-1);
}

void envvar(char *tr[], char *envp[]) {
    int envpPos, environPos;
    char *env;
    char newEnv[MAXVAR];

    if (tr[1] == NULL) {
        helpCmd("envvar");
        return;
    } else {

        if (strcmp(tr[1], "-show") == 0 && tr[2] != NULL) {
            env = getenv(tr[2]);
            envpPos = BuscarVariable(tr[2], envp);
            environPos = BuscarVariable(tr[2], environ);

            if (env == NULL || envpPos == -1 || environPos == -1) {
                printf(
                    ROJO_ERR
                    "La variable %s no está definida en el entorno\n" RESET_COL,
                    tr[2]);
                return;
            }

            printf("Mediante arg3 main arg[%d] %s (%p) \n"
                   "Mediante environ %s=%s (%p)\n"
                   "Mediante getenv %s (%p)\n",
                   envpPos, envp[envpPos], &envp[envpPos], tr[2], env, &env,
                   environ[environPos], &environ[environPos]);
            return;
        }

        if (strcmp(tr[1], "-change") == 0 && tr[2] != NULL && tr[3] != NULL &&
            tr[4] != NULL) {
            env = getenv(tr[3]);
            envpPos = BuscarVariable(tr[3], envp);
            environPos = BuscarVariable(tr[3], environ);

            if (env == NULL || envpPos == -1 || environPos == -1) {
                printf(
                    ROJO_ERR
                    "La variable %s no está definida en el entorno\n" RESET_COL,
                    tr[3]);
                return;
            }

            char *aux = malloc(strlen(tr[3]) + strlen(tr[4]) + 2);
            sprintf(aux, "%s=%s", tr[3], tr[4]);

            if (strcmp(tr[2], "-a") == 0) {
                envp[envpPos] = aux;
            } else if (strcmp(tr[2], "-e") == 0) {
                environ[environPos] = aux;

            } else if (strcmp(tr[2], "-p") == 0) {
                strcpy(newEnv, tr[3]);
                strcat(newEnv, "=");
                strcat(newEnv, tr[4]);
                if (putenv(newEnv) != 0) {
                    perror(ROJO_ERR "Error en envvar -change" RESET_COL);
                    free(aux);
                }
            } else {
                helpCmd("envvar");
                free(aux);
                return;
            }
            return;
        }
    }
}

void fork(){
    
}

