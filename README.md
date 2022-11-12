# CSocket

## How to compile
* Windows:
    1) Download and install(adding the path variable) the most recent [WinLibs UCRT runtime](https://winlibs.com) and MinGW64(UCRT)
    2) Compile the code statically using gcc: 
	    ```
	    gcc.exe -o <filename>.exe <filename>.c -l:libcsocket.lib -lws2_32 -L<libpath> -I<libpath>
	    ```
* *nix:
    1) Verify the [Dependencies](#dependencies)
	2) Compile the code statically using gcc:
	    ```
	    gcc -o <filename>.o <filename>.c -l:libcsocket.a -lws2_32 -L<libpath> -I<libpath>
	    ```

## Dependencies
* Windows:
    ```
	winsock2.h Ws2tcpip.h stdio.h io.h
	```
* *nix:
	```
    arpa/inet.h	sys/socket.h time.h fcntl.h string.h stdlib.h stdio.h unistd.h errno.h
	```