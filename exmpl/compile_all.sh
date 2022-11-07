#!/bin/bash

gcc -o simpleClient.o simpleClient.c -l:libcsocket.a -L../bin -I../bin
gcc -o simpleServer.o simpleServer.c -l:libcsocket.a -L../bin -I../bin
gcc -o multiServer.o multiServer.c -l:libcsocket.a -L../bin -I../bin