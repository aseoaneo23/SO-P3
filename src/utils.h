#include "lista.h"
#include <dirent.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_NOMBRE 128

typedef enum
{
    N,
    LAST_N,
    COUNT,
    CLEAR,
    NO_MOD
} MOD;
typedef enum
{
    FINISHED,
    STOPPED,
    SIGNALED,
    ACTIVE
} procStatus;

typedef struct
{
    int df;
    char name[MAX_NOMBRE];
    int modo;
    int ocupado;
} Fichero;

struct dirOps
{
    int dLong;
    int dLink;
    int dHide;
    int dRec;
};

typedef enum
{
    MALLOC,
    SHARED,
    MAPPED,
    ALL
} ALLOC_TYPE;

typedef struct otraInfo
{
    char *nomFichero;
    int descriptor;
    int shm_key;
} otraInfo;

typedef struct tMemItemL
{
    void *direccion;
    size_t tamano;
    ALLOC_TYPE tipo;
    char *fecha;
    otraInfo *otra_info;
} tMemItemL;

//TODO: implementar punteros a función para la entrada del comando 
typedef struct cmd
{
    char *nombre;
    void (*funcion)(char **);
} cmd;

typedef struct tProcess
{
    pid_t pid;
    char *fecha;
    procStatus status;
    char *cmdLine;
    int priority;
} tProcess;

void imprimirPrompt();
void leerEntrada(char *comando);
void inicializarFicherosEstandar();
void *MapearFichero(char *fichero, int protection, tList *memBlockList);
void imprimirListaMem(tList *memoryList, ALLOC_TYPE tipo);
void do_Mmap(char *arg[], tList *memBlockList);
bool unmapFichero(char *fichero, tList *memList);
void *ObtenerMemoriaShmget(key_t clave, size_t tam, tList *memoryBlockList);
void do_SharedDelkey(char *key);
void LlenarMemoria(void *p, size_t cont, unsigned char byte);
void memfill(void *addr, char *size, unsigned char byte, tList *memList);
void Recursiva(int n);
void memdump(void *addr, char *cont);
void do_SharedCreate(char *tr[], tList *memoryList);
void do_Shared(char *tr[], tList *memoryList);
void doSharedFree(char *cl, tList *memList);
void mem(char *param, tList *memList);
void formatDate(time_t time, char *formattedDate);
char *extractRealPath(char *path);
bool procesarEntrada(char *comando, tList *historical, tList *memList, struct dirOps *ops, char *envp[]);
int TrocearCadena(char *cadena, char *trozos[]);
void authors(char *mod);
void getShellPid(char *mod);
void changeDir(char *path);
void printCurrentDir();
void createList();
bool updateHistorical(tList *historical, char *command);
void printHistorical(tList historical);
void customHistoricalPrint(MOD type, char *mod, tList historical, tList memList, struct dirOps *ops, char *envp[]);
void infosys(char *mod);
void helpCmd(char *mod);
void dateCmd(char *mod);
void closeCmd(char *mod);
void dupCmd(char *mod);
void ListaFichAbiertos(void);
void EliminarFichAbiertos(int fd);
char *NameFicheroDescriptor(int fd);
void AnadirFicherosAbiertos(int fd, const char *nombre, int flags);
void cleanListFromMemory(tList *L);
MOD identifyModifier(char *mod);
void manageHistoricalWMods(char *mod, tList *historical, tList *memList, struct dirOps *ops, char *envp[]);
int extractDigit(char *mod, MOD modifierType);
void Cmd_open(char *tr[], int num_trozos);
void create(char *trozos[]);
struct dirOps setDirParams(char *trozos[], int numTrozos, struct dirOps *ops);
void getDirParams(struct dirOps *ops);
void erase(char *trozos[], int numTrozos);
void dirCmd(char *trozos[], int numTrozos, struct dirOps *ops);
void lseekCmd(char *trozos[], int num_trozos);
void writestrCmd(char *trozos[], int num_trozos);
void dirPrintCustom(struct dirOps *ops, struct stat *dirInfo,
                    struct dirent *entrada, char *ruta);
char *ConvierteModo2(mode_t m);
void formatLastEditDate(struct stat *dirInfo, char *fecha, char *hora);
void recDir(const char *path, struct dirOps *ops, int recursionMode,
            bool modoDir);
void dirCmd2(char *trozos[], int numTrozos, struct dirOps *ops);
void *buscarNodo(tList *memList, key_t clave, tPosL *p);
int borrarNodoPorAddr(tList memList, void *addr);
void mallocCmd(char *trozos[], tList *memList);
void freeCmd(char *trozos[], int numTrozos, tList *memList);
void *CadenatoPointer(char *s);
ssize_t LeerFichero(char *f, void *p, size_t cont);
void readfileCmd(char *trozos[]);
void writefileCmd(char *trozos[]);
void readCmd(char *tr[]);
void writeCmd(char *tr[]);
void printFuncs();
void printVars();
void Do_pmap(void);
int BuscarVariable(char *var, char *e[]);
void envvar(char *tr[], char *envp[]);
// void fork();
// void delRec();