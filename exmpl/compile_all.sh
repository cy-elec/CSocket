#!/bin/bash
gcc -o bin/simpleClient.o simpleClient.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleServer.o simpleServer.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/multiServer.o multiServer.c -l:libcsocket.a -L../bin -I../bin

gcc -o bin/simpleClient6.o simpleClient6.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleServer6.o simpleServer6.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/multiServer6.o multiServer6.c -l:libcsocket.a -L../bin -I../bin

strip -s bin/*.o
