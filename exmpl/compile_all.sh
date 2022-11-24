#!/bin/bash
gcc -o bin/simpleClient.o simpleClient.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleServer.o simpleServer.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/multiServer.o multiServer.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleTransmitter.o simpleTransmitter.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/multiTransmitter.o multiTransmitter.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleReceiver.o simpleReceiver.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleStation1.o simpleStation1.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleStation2.o simpleStation2.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleStation61.o simpleStation61.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleStation62.o simpleStation62.c -l:libcsocket.a -L../bin -I../bin

gcc -o bin/simpleClient6.o simpleClient6.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleServer6.o simpleServer6.c -l:libcsocket.a -L../bin -I../bin
gcc -o bin/multiServer6.o multiServer6.c -l:libcsocket.a -L../bin -I../bin

strip -s bin/*.o
