# CSocket

A simple socket interface for c applications that supports both, client and server applications.

## How to set up a server application

The interface provides all common socket functionalities such as `bind`, `listen`, `accept` and more.
The server can operate in blocking-single-connection mode ([Simple Server](exmpl/simpleServer.c)) or in non-blocking-multi-connection mode ([Multi Server](exmpl/multiServer.c)).

The server can operate in IPv4 and IPv6 modes, which, depending on the OS, might overlap (*nix IPv6 Servers will accept IPv4 request, Windows Servers won't).

* ### Setting up the Multi Server

    1. initialize a Server : `csocket_initServerSocket(domain, type, protocol, addrc, port, csocket_t, specialAddr)`
    1. bind the Server : `csocket_bindServer(csocket_t)`
    1. set up the listen queue : `csocket_listen(csocket_t, maxQueue)`
    1. initialize a server handler : `csocket_setUpMultiServer(csocket_t, maxClient, onActivity, csocket_multiHandler_t)`
    1. run Server in a loop : `csocket_multiServer(csocket_multiHandler_t)`

* ### Setting up the Single Server

    1. initialize a Server : `csocket_initServerSocket(domain, type, protocol, addrc, port, csocket_t, specialAddr)`
    1. bind the Server : `csocket_bindServer(csocket_t)`
    1. set up the listen queue : `csocket_listen(csocket_t, maxQueue)`
    1. accept incoming client : `csocket_accept(csocket_t, csocket_activity_t)`

## How to set up a client application

## How to compile

* Windows:
    1. Download and install(adding the path variable) the most recent [WinLibs UCRT runtime](https://winlibs.com) and MinGW64(UCRT)
    1. Compile the code statically using gcc:

        ```bash
        gcc.exe -o <filename>.exe <filename>.c -l:libcsocket.lib -lws2_32 -L<libpath> -I<libpath>
        ```

* *nix:
    1. Verify the [Dependencies](#dependencies)
    1. Compile the code statically using gcc:

        ```bash
        gcc -o <filename>.o <filename>.c -l:libcsocket.a -lws2_32 -L<libpath> -I<libpath>
        ```

## Dependencies

* Windows:

    ```bash
    winsock2.h Ws2tcpip.h stdio.h io.h time.h
    ```

* *nix:

    ```bash
    arpa/inet.h sys/socket.h time.h fcntl.h string.h stdlib.h stdio.h unistd.h errno.h
    ```
