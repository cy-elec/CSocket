#!/bin/bash

gcc.exe -o simpleClient.exe simpleClient.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o simpleServer.exe simpleServer.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o multiServer.exe multiServer.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin

gcc.exe -o simpleClient6.exe simpleClient6.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o simpleServer6.exe simpleServer6.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin
gcc.exe -o multiServer6.exe multiServer6.c -l:libcsocket.lib -lws2_32 -L../bin -I../bin