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

## SOCKET SETUP

* ### `csocket_setAddress(struct sockaddr **out_addr, socklen_t *addr_len, int domain, void *addrc, int port, int specialAddr)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_setAddressA(csocket_addr_t *out_addr, int domain, void *addrc, int port, int specialAddr)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_initServerSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_initClientSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

## RECV/SEND

* ### `csocket_hasRecvData(csocket_t *src_socket)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_hasRecvFromData(csocket_t *src_socket, csocket_addr_t *dst_addr)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_recv(csocket_t *src_socket, void *buf, size_t len, int flags)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_recvfrom(csocket_t *src_socket, csocket_addr_t *dst_addr, void *buf, size_t len, int flags)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_send(csocket_t *src_socket, void *buf, size_t len, int flags)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_sendto(csocket_t *src_socket, void *buf, size_t len, int flags)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

## ACTIVITY

* ### `csocket_updateA(csocket_activity_t *activity)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_updateFromA(csocket_activity_t *activity)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_printActivity(FILE *fp, csocket_activity_t *activity)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_hasRecvDataA(csocket_activity_t *activity)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_hasRecvFromDataA(csocket_activity_t *activity)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_recvA(csocket_activity_t *activity, void *buf, size_t len, int flags)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_recvfromA(csocket_activity_t *activity, void *buf, size_t len, int flags)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_sendA(csocket_activity_t *activity, void *buf, size_t len, int flags)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_sendtoA(csocket_activity_t *activity, void *buf, size_t len, int flags)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_sockToAct(csocket_t *src_socket)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_sockToActA(csocket_t *src_socket, csocket_addr_t *dst_addr)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

## KEEPALIVE

* ### `csocket_keepalive_create(int timeout, char *msg, size_t msg_len, csocket_keepalive_t *ka, csocket_t *src_socket)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_keepalive_modify(int timeout, char *msg, size_t msg_len, csocket_keepalive_t *ka, csocket_t *src_socket)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_keepalive_set(csocket_keepalive_t *ka, csocket_t *src_socket)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_keepalive_unset(csocket_t *src_socket)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_keepalive_copy(csocket_keepalive_t **dst, const csocket_keepalive_t *src)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_isAlive(csocket_keepalive_t *ka)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_keepAlive(csocket_t *src_socket)`

  |||
  --|--
  |**description**||
  |**params**||
  |**return**||

* ### `csocket_resolveKeepAliveMsg(char *src, size_t *size, size_t *offset)`

  |||
  --|--
  |**description**|Resolves the replacement string of a specified variable and writes the relative positioning to the referenced variable `offset`.|
  |**params**|_pointer to a char buffer that holds the variable_ `char *src`, _size of the buffer_ `size_t size`, _relative positioning_ `size_t *offset`|
  |**return**|`char *` - On success, returns pointer to replacement string on heap, otherwise NULL.|

* ### `csocket_getKeepAliveVariable(char *dst, size_t *dst_len, char *query, csocket_keepalive_t *ka)`

  |||
  --|--
  |**description**|Resolves a Query `query` from the keepalive variables and places it in the `dst` buffer of size `dst_len`.|
  |**params**|_pointer to destination buffer_ `char *dst`, _size of the buffer_ `size_t *dst_len`, _pointer to string query_ `char *query`, _pointer to a keepalive type_ `csocket_keepalive_t *ka`|
  |**return**|`int` - On success return 0, otherwise -1.|

* ### `csocket_updateKeepAlive(csocket_keepalive_t *ka, int fd)`

  |||
  --|--
  |**description**|Updates keepalive info for `connected` sockets.|
  |**params**|_pointer to a keepalive type_ `csocket_keepalive_t *ka`, _socket fd_ `int fd`|
  |**return**|`int` - On success return 0, otherwise -1.|

* ### `csocket_updateKeepAliveFrom(csocket_keepalive_t *ka, int fd, csocket_addr_t *dst_addr)`

  |||
  --|--
  |**description**|Updates keepalive info for `connectionless` sockets.|
  |**params**|_pointer to a keepalive type_ `csocket_keepalive_t *ka`, _socket fd_ `int fd`, _pointer to an address type_ `csocket_addr_t *dst_addr`|
  |**return**|`int` - On success return 0, otherwise -1.|

* ### `csocket_printKeepAlive(FILE *fp, csocket_keepalive_t *ka)`

  |||
  --|--
  |**description**|Writes the keepalive type to the specified FILE.|
  |**params**|_FILE_ `FILE *fp`, _pointer to a keepalive type_ `csocket_keepalive_t *ka`|
  |**return**|`void`|

## SERVER

* ### `csocket_bindServer(csocket_t *src_socket)`

  |||
  --|--
  |**description**|Bind a name to a csocket.|
  |**params**|_pointer to a csocket_ `csocket_t *src_socket`|
  |**return**|`int` - On success return 0, otherwise return -1 and set the last error in last_err.|

* ### `csocket_listen(csocket_t *src_socket, int maxQueue)`

  |||
  --|--
  |**description**|Listen for connections on csocket with a provided Queue size.|
  |**params**|_pointer to a csocket_ `csocket_t *src_socket`, _Queue size_ `int maxQueue`|
  |**return**|`int` - On success return 0, otherwise return -1 and set the last error in last_err.|

* ### `csocket_accept(csocket_t *src_socket, csocket_activity_t *activity)`

  |||
  --|--
  |**description**|Block code execution until a client connects and set the corresponding activity flags.|
  |**params**|_pointer to a csocket_ `csocket_t *src_socket`, _pointer to an activity type_ `csocket_activity_t *activity`|
  |**return**|`int` - On success, return 0, otherwise return -1 and set the last error in last_err.|

* ### `csocket_setUpMultiServer(csocket_t *src_socket, int maxClient, void (*onActivity)(csocket_multiHandler_t *, csocket_activity_t), csocket_multiHandler_t *handler)`

  |||
  --|--
  |**description**|Creates a multiHandler `handler` that can be used in [csocket_multiServer()](#csocket_multiservercsocket_multihandler_t-handler).|
  |**params**|_pointer to a csocket_ `csocket_t *src_socket`, _Size of client buffer_ `int maxClient`, _pointer to an activity handler_ `void (*onActivity)(csocket_multiHandler_t *, csocket_activity_t)`, _pointer to a multiHandler type_ `csocket_multiHAndler_t *handler`|
  |**return**|`int` - On success, return 0, otherwise return -1 and set the last error in last_err.|

* ### `csocket_multiServer(csocket_multiHandler_t *handler)`

  |||
  --|--
  |**description**|Runs the multiServer.|
  |**params**|_pointer to a mutliHandler type_ `csocket_multiHandler_t *handler`|
  |**return**|`int` - On success, return 0, otherwise return -1 and set the last error in last_err.|

* ### `csocket_shutdownClient(csocket_multiHandler_t *handler, struct csocket_clients *client)`

  |||
  --|--
  |**description**|Marks Client `client` for shutdown. If `client` is NULL, marks all connected clients for shutdown.|
  |**params**|_pointer to a multiHandler type_ `csocket_multiHandler_t *handler`, _pointer to a clients struct_ `struct csocket_clients *client`|
  |**return**|`int` - On success, return n > 0 number of marked clients, otherwise return <= 0.|

## CLIENT

* ### `csocket_connectClient(csocket_t *src_socket, struct timeval *timeout)`

  |||
  --|--
  |**description**|Connects a csocket. If timeout is not NULL, return after max. timeout.|
  |**params**|_pointer to a csocket_ `csocket_t *src_socket`, _pointer to a timeval structure_ `struct timeval *timeout`|
  |**return**|`int` - On success, return 0, otherwise return -1 and set the last error in last_err.|

## UTIL

* ### `csocket_ntop(int domain, const void *addr, char *dst, socklen_t len)`

  |||
  --|--
  |**description**|Automatically calls the arpa/inet.h function ntop with appropriate address casting by providing the domain.|
  |**params**|_address family_ `int domain`, _pointer to a network address structure_ `const void *addr`, _pointer to a output buffer_ `char *dst`, _size of the provided buffer_ `socklen_t len`|
  |**return**|`const char *` - On success, return pointer to dst, otherwise NULL.|
  |**alias**|`CSOCKET_NTOP(domain, addr, str, strlen)`|

## FREE

* ### `csocket_free(csocket_t *src_socket)`

  |||
  --|--
  |**description**|Frees by csocket allocated heap memory.|
  |**params**|_pointer to a csocket_ `csocket_t *src_socket`|
  |**return**|`void`|

* ### `csocket_freeActivity(csocket_activity_t *activity)`

  |||
  --|--
  |**description**|Frees by activity type allocated heap memory.|
  |**params**|_pointer to an activity type_ `csocket_activity_t *activity`|
  |**return**|`void`|

* ### `csocket_freeMultiHandler(csocket_multiHandler_t *handler)`

  |||
  --|--
  |**description**|Frees by multiHander type allocated heap memory.|
  |**params**|_pointer to a multiHandler type_ `csocket_multiHandler_t *handler`|
  |**return**|`void`|

* ### `csocket_freeClients(struct csocket_clients *client)`

  |||
  --|--
  |**description**|Frees by clients structure allocated heap memory.|
  |**params**|_pointer to a clients structure_ `struct csocket_clients *client`|
  |**return**|`void`|

* ### `csocket_freeKeepalive(csocket_keepalive_t *ka)`

  |||
  --|--
  |**description**|Frees by keepalive allocated heap memory.|
  |**params**|_pointer to a keepalive type_ `csocket_keepalive_t *ka`|
  |**return**|`void`|

## CLOSE

* ### c`socket_close(csocket_t *src_socket)`

  |||
  --|--
  |**description**|Closes the connection and frees the socket/csocket.|
  |**params**|_pointer to a csocket_ `csocket *src_socket`|
  |**return**|`void`|
