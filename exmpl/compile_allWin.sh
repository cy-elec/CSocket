#!/bin/bash

gcc.exe -o bin/simpleClient.exe simpleClient.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleServer.exe simpleServer.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/multiServer.exe multiServer.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin

gcc.exe -o bin/simpleClient6.exe simpleClient6.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/simpleServer6.exe simpleServer6.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o bin/multiServer6.exe multiServer6.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin