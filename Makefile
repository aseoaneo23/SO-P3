CC = gcc
CFLAGS =  -Wall -Wextra 
DEB = gdb
DEBFLAGS = -g -O0 
VALGRIND = valgrind --leak-check=full --show-reachable=yes ./a.out

SRC = ./src/p3.c
OBJ = a.out
LIB = ./src/lista.c

COM = ./src/utils.c

# Se compila y se ejecuta el programa
all: $(OBJ)
	./$(OBJ)

# Se compila el programa
compile: $(OBJ)

# Se limpia el directorio de archivos compilados
clean:
	rm -f *.out

debug:
	$(CC) $(DEBFLAGS) $(SRC) $(COM) $(LIB)
	$(DEB) a.out

valgrind:
	 $(compile)
	 $(VALGRIND)

# Compilacion del programa
$(OBJ): $(SRC) $(COM) $(LIB) 
	$(CC) $(CFLAGS) -o $(OBJ) $(SRC) $(COM) $(LIB) 