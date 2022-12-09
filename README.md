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

## Structures, Types and Constants

```c
// empty structure initializer
#define CSOCKET_EMPTY {0}
```

### CSOCKET

```c
// type that holds the csocket
typedef struct csocket {
    // sys/socket fields
    int domain;
    int type;
    int protocol;
    // server/client
    struct csocket_mode mode;

    // keepalive (global settings)
    struct csocket_keepalive *ka;

    // error string
    char last_err[32];
} csocket_t;
```

```c
// placeholder with set size to hold server or client information
struct csocket_mode {
    // common
    int fd;
    struct sockaddr *addr;
    socklen_t addr_len;
    int data[2];
    int sc;
};
```

```c
// structure that holds server information -- can be stored in a mode structure
struct csocket_server {
    // server socket
    int server_fd;
    // bind
    struct sockaddr *addr;
    socklen_t addr_len;
    // listen
    int backlog;
    // accept
    int client_fd;
    int sc;
};
```

```c
// structure that holds client information -- can be stored in a mode structure
struct csocket_client {
    // client socket
    int client_fd;
    // server addr
    struct sockaddr *addr;
    socklen_t addr_len;
    int zero[1];
    int sc;
};
```

```c
// type to hold address information
typedef struct csocket_addr {
    int domain;
    struct sockaddr *addr;
    socklen_t addr_len;
} csocket_addr_t;
```

```c
// structure that holds client information
struct csocket_clients {
    // incoming socket
    int fd;
    int domain;
    struct sockaddr *addr;
    socklen_t addr_len;
    time_t connection_time;
    // keepalive
    struct csocket_keepalive *ka;
    // manual shutdown
    char shutdown;
};
```

### Keepalive System

```c
// default keepalive timeout
#define CSKA_TIMEOUT 120
// default keepalive message
#define CSKA_DEFAULTMSG "CSKA%UNIX%-%HOST%-%USER%\0"
#define CSKA_DEFAULTMSGLEN 25
```

```c
// type to hold keepalive information
typedef struct csocket_keepalive {
    int enabled;
    // keepalive timeout in seconds
    int timeout;
    // keepalive buffer = RCVBUF
    char *buffer;
    socklen_t buffer_len;
    socklen_t buffer_usage;
    // keepalive message and length
    char *msg;
    size_t msg_len;
    /**
     * keepalive message types:
     * 1 - custom message stored in msg with length msg_len
     * other values: CSKA_DEFAULTMSG
     * 
     * variables for custom messages:
     * - %UNIX% -> current time in unix format
     * - %HOST% -> hostname of the current machine
     * - %USER% -> username of the current machine
    **/
    int msg_type;
    // resolved paramters
    char *params;
    socklen_t params_len;
    socklen_t params_usage;
    // last keepalive timestamp
    time_t last_sig;
    // handler function
    void (*onActivity)(struct csocket_keepalive *);

    // file descriptor and address information
    int fd;
    csocket_addr_t address;

    // socket connection timestamp
    time_t connection_time;
} csocket_keepalive_t;
```

### MultiHandler

```c
// type that holds multiHandler information and all the managed clients
typedef struct csocket_multiHandler {
    // socket
    csocket_t *src_socket;
    // handler function
    void (*onActivity)(struct csocket_multiHandler *, csocket_activity_t);
    // client stash
    int maxClients;
    struct csocket_clients *client_sockets;
} csocket_multiHandler_t;
```

### Activities

```c
// Activity Types
#define CSACT_TYPE_CONN 1        // Connected
#define CSACT_TYPE_DISCONN 2     // Disconnected
#define CSACT_TYPE_READ 4        // Read Available
#define CSACT_TYPE_WRITE 8       // Write Available
#define CSACT_TYPE_EXT 16        // Extra Available
#define CSACT_TYPE_DECLINED 32   // Connection Declined -> usually due to maxClients
```

```c
// type to hold an activity
typedef struct csocket_activity {
    // incoming socket/client handle
    struct csocket_clients client_socket;
    // action type
    int type;
    // timestamps
    time_t time;
    time_t update_time;
} csocket_activity_t;
```

## Functions

* ## name

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||
