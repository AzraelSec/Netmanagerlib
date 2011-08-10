#Netmanagerlib

VERSION = 0.5
SRC = src/Netmanager.c
BIN = bin/libnetmanager
LIB = lib/libnetmanager.a
OBJ = obj/libnetmanager.o
CFLAGS = -DVERSION=\"${VERSION}\" -I include/ 

all:
	mkdir bin/
	mkdir obj/
	mkdir lib/
	gcc -o $(BIN) $(SRC) $(CFLAGS) -c
	gcc -c $(SRC) -o $(OBJ) $(CFLAGS)
	ar rcs $(LIB) $(OBJ)

install:all
	cp -f $(LIB) /usr/lib/
	cp -f include/Netmanager.h /usr/include/Netmanager.h
clean:
	rm -rf $(BIN) $(LIB) $(OBJ)

disinstall:
	rm -f /usr/include/Netmanager.h
	rm -f /usr/lib/libnetmanager.a
