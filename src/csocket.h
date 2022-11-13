/**
 * @file csocket.h
 * @author Felix Kröhnert (felix.kroehnert@online.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-04
 * 
 * @copyright Copyright (c) 2022
 * 
**/


#ifndef _CSOCKET_H
#define _CSOCKET_H

#ifdef _WIN32
#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_VISTA
#endif
#ifndef WINVER
#define WINVER _WIN32_WINNT_VISTA
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif

#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <io.h>
#include <time.h>

// socket 
#define socklen_t int
typedef struct in_addr in_addr_t;
#define SO_REUSEPORT 0

// shutdown
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH

// MESSAGES
#define CSOCKET_START_MSG "Running CSocket for Windows by Felix Kroehnert"
#define CSOCKET_CLOSE_MSG "Stopping CSocket for Windows by Felix Kroehnert..."

// errno
#define CS_EINPROGRESS WSAEINPROGRESS

#else 

// MESSAGES
#define CSOCKET_START_MSG "Running CSocket for *nix by Felix Kroehnert"
#define CSOCKET_CLOSE_MSG "Stopping CSocket for *nix by Felix Kroehnert..."

#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

// errno
#define CS_EINPROGRESS EINPROGRESS

#endif

extern const struct in_addr inaddr_any;

/**
 * 
 * MACROS AND CORRESPONDING FUNCTIONS
 * 
**/
const char * csocket_ntop(int domain, const void *addr, char *dst, socklen_t len);
#define CSOCKET_NTOP(domain, addr, str, strlen) csocket_ntop(domain, addr, str, strlen)
#define CSOCKET_EMPTY {0}

struct csocket_mode {
	// common
	int fd;
	struct sockaddr *addr;
	socklen_t addr_len;
	int data[2];
	int sc;
};

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

struct csocket_client {
	// client socket
	int client_fd;
	// server addr
	struct sockaddr *addr;
	socklen_t addr_len;
	int zero[2];
	int sc;
};

typedef struct csocket {
	// sys/socket fields
	int domain;
	int type;
	int protocol;
	// server
	struct csocket_mode mode;
	
	// error
	char last_err[32];
} csocket_t;


/*
	ACTIVITY
*/

#define CSACT_TYPE_CONN 1
#define CSACT_TYPE_DISCONN 2
#define CSACT_TYPE_READ 4
#define CSACT_TYPE_WRITE 8
#define CSACT_TYPE_EXT 16
#define CSACT_TYPE_DECLINED 32

typedef struct csocket_activity {
	// incoming socket
	int fd;
	int domain;
	struct sockaddr *addr;
	socklen_t addr_len;
	// action type
	int type;
	// timestamps
	time_t time;
	time_t update_time;
} csocket_activity_t;

/*
	multiHandler
*/
struct csocket_clients {
	// incoming socket
	int fd;
	int domain;
	struct sockaddr *addr;
	socklen_t addr_len;
};

typedef struct csocket_multiHandler {
	// socket
	csocket_t *src_socket;
	// handler function
	void (*onActivity)(csocket_activity_t);
	// client stash
	int maxClients;
	struct csocket_clients *client_sockets;

} csocket_multiHandler_t;


// setup
int csocket_setAddress(struct sockaddr **out_addr, socklen_t *addr_len, int domain, void *addrc, int port, int specialAddr);
int csocket_initServerSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr);
int csocket_initClientSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr);

// establish connection/bind socket
int csocket_bindServer(csocket_t *src_socket);
int csocket_connectClient(csocket_t *src_socket, struct timeval *timeout);

// server side

// one client at a time
int csocket_listen(csocket_t *src_socket, int maxQueue);
int csocket_accept(csocket_t *src_socket, csocket_activity_t *activity);
void csocket_updateA(csocket_activity_t *activity);

// handling all clients - should be called in a while(true)
int csocket_setUpMultiServer(csocket_t *src_socket, int maxClient, void (*onActivity)(csocket_activity_t), csocket_multiHandler_t *handler);
int csocket_multiServer(csocket_multiHandler_t *handler);

// activity
void csocket_printActivity(int fd, csocket_activity_t *activity);

// read/write
int csocket_hasRecvData(csocket_t *src_socket);
int csocket_recv(csocket_t *src_socket, void *buf, size_t len, int flags);
int csocket_recvfrom(csocket_t *src_socket, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t addr_len);

int csocket_send(csocket_t *src_socket, void *buf, size_t len, int flags);
int csocket_sendto(csocket_t *src_socket, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t addr_len);

int csocket_hasRecvDataA(csocket_activity_t *activity);
int csocket_recvA(csocket_activity_t *activity, void *buf, size_t len, int flags);
int csocket_recvfromA(csocket_activity_t *activity, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t addr_len);

int csocket_sendA(csocket_activity_t *activity, void *buf, size_t len, int flags);
int csocket_sendtoA(csocket_activity_t *activity, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t addr_len);

// close connection
void csocket_close(csocket_t *src_socket);

// free
void csocket_free(csocket_t *src_socket);
void csocket_freeActivity(csocket_activity_t *activity);
void csocket_freeMultiHandler(csocket_multiHandler_t *handler);
void csocket_freeClients(struct csocket_clients *client);


// non callable funtions
static void __attribute__((constructor)) csocket_constructor();
static void __attribute__((destructor)) csocket_destructor();

#endif
