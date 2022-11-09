#!/bin/bash

#add header
cp src/*.h bin/

## static

#compile
gcc.exe -c -Wall -Wextra src/*.c
#archive
ar -rcs bin/libcsocket.lib *.o

#post-cleanup
rm *.o
