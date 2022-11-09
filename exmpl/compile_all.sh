#!/bin/bash

gcc -o simpleClient.o simpleClient.c -l:libcsocket.a -L../bin -I../bin
gcc -o simpleServer.o simpleServer.c -l:libcsocket.a -L../bin -I../bin
gcc -o multiServer.o multiServer.c -l:libcsocket.a -L../bin -I../bin

gcc -o simpleClient6.o simpleClient6.c -l:libcsocket.a -L../bin -I../bin
gcc -o simpleServer6.o simpleServer6.c -l:libcsocket.a -L../bin -I../bin
gcc -o multiServer6.o multiServer6.c -l:libcsocket.a -L../bin -I../bin