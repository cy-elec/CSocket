#!/bin/bash
gcc -o bin/simpleClient.o simpleClient.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleServer.o simpleServer.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/multiServer.o multiServer.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleTransmitter.o simpleTransmitter.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/multiTransmitter.o multiTransmitter.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleReceiver.o simpleReceiver.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleStation1.o simpleStation1.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleStation2.o simpleStation2.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleStation61.o simpleStation61.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleStation62.o simpleStation62.c -static -l:libcsocket.a -L../bin -I../bin

gcc -o bin/simpleClient6.o simpleClient6.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/simpleServer6.o simpleServer6.c -static -l:libcsocket.a -L../bin -I../bin
gcc -o bin/multiServer6.o multiServer6.c -static -l:libcsocket.a -L../bin -I../bin

strip -s bin/*.o
