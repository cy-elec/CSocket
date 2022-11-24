#!/bin/bash
gcc.exe -o bin/simpleClient.exe simpleClient.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleServer.exe simpleServer.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/multiServer.exe multiServer.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleTransmitter.exe simpleTransmitter.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/multiTransmitter.exe multiTransmitter.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleReceiver.exe simpleReceiver.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleStation1.exe simpleStation1.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleStation2.exe simpleStation2.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleStation61.exe simpleStation61.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleStation62.exe simpleStation62.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin

gcc.exe -o bin/simpleClient6.exe simpleClient6.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleServer6.exe simpleServer6.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/multiServer6.exe multiServer6.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin

strip.exe -s bin/*.exe
