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
#define MSG_DONTWAIT 0x0008

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
extern struct timespec csocket_timeout; 


#define CSOCKET_EMPTY {0}

struct csocket_mode {
	// common
	int fd;
	struct sockaddr *addr;
	socklen_t addr_len;
	int data[2];
	int sc;
};

typedef struct csocket_addr {
	int domain;
	struct sockaddr *addr;
	socklen_t addr_len;
} csocket_addr_t;

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
	int zero[1];
	int sc;
};

// default timeout of 2 min (timeout in seconds)
#define CSKA_TIMEOUT 120
#define CSKA_DEFAULTMSG "CSKA%UNIX%-%HOST%-%USER%\0"
#define CSKA_DEFAULTMSGLEN 25

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
	 * other values: DEFAULT - "CSKA%UNIX%-%HOST%-%USER%\0"
	 * 
	 * variables for custom messages:
	 * - %UNIX% -> current time in unix format
	 * - %HOST% -> hostname of the current machine
	 * - %USER% -> username of the current machine
	**/
	int msg_type;
	// last keepalive
	char *params;
	socklen_t params_len;
	socklen_t params_usage;
	time_t last_sig;
	// handler function
	void (*onActivity)(struct csocket_keepalive *);

	int fd;
	csocket_addr_t address;
} csocket_keepalive_t;

typedef struct csocket {
	// sys/socket fields
	int domain;
	int type;
	int protocol;
	// server/client
	struct csocket_mode mode;

	// keepalive (global settings)
	struct csocket_keepalive *ka;

	// error
	char last_err[32];
} csocket_t;

/*
	multiHandler
*/
struct csocket_clients {
	// incoming socket
	int fd;
	int domain;
	struct sockaddr *addr;
	socklen_t addr_len;
	// keepalive
	struct csocket_keepalive *ka;
	// manual shutdown
	char shutdown;
};

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
	// incoming socket/client handle
	struct csocket_clients client_socket;
	// action type
	int type;
	// timestamps
	time_t time;
	time_t update_time;
} csocket_activity_t;


typedef struct csocket_multiHandler {
	// socket
	csocket_t *src_socket;
	// handler function
	void (*onActivity)(struct csocket_multiHandler *, csocket_activity_t);
	// client stash
	int maxClients;
	struct csocket_clients *client_sockets;

} csocket_multiHandler_t;


/*
	SOCKET SETUP
*/
#pragma region SOCKET SETUP

static int _initSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr);

int csocket_setAddress(struct sockaddr **out_addr, socklen_t *addr_len, int domain, void *addrc, int port, int specialAddr);

int csocket_setAddressA(csocket_addr_t *out_addr, int domain, void *addrc, int port, int specialAddr);

int csocket_initServerSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr);

int csocket_initClientSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr);

#pragma endregion
/*
	RECV/SEND
*/
#pragma region RECV/SEND

static int _hasRecvData(int fd);

static int _hasRecvFromData(int fd, struct sockaddr *addr, socklen_t *addr_len);

int csocket_hasRecvData(csocket_t *src_socket);

int csocket_hasRecvFromData(csocket_t *src_socket, csocket_addr_t *dst_addr);

ssize_t csocket_recv(csocket_t *src_socket, void *buf, size_t len, int flags);

ssize_t csocket_recvfrom(csocket_t *src_socket, csocket_addr_t *dst_addr, void *buf, size_t len, int flags);

ssize_t csocket_send(csocket_t *src_socket, void *buf, size_t len, int flags);

ssize_t csocket_sendto(csocket_t *src_socket, void *buf, size_t len, int flags);

#pragma endregion
/*
	RECV/SEND BUFFER
*/
#pragma region RECV/SEND BUFFER

static int _hasRecvDataBuffer(struct csocket_keepalive *ka, int fd);

static int _hasRecvFromDataBuffer(struct csocket_keepalive *ka, int fd, struct sockaddr *addr, socklen_t *addr_len);

static ssize_t _updateBuffer(struct csocket_keepalive *ka, int fd, int flags);

static ssize_t _updateFromBuffer(struct csocket_keepalive *ka, int fd, int flags, struct sockaddr *addr, socklen_t *addr_len);

static ssize_t _readBuffer(csocket_t *src_socket, void *buf, size_t len, int flags);

static ssize_t _readBufferA(csocket_activity_t *activity, void *buf, size_t len, int flags);

static ssize_t _readFromBuffer(csocket_t *src_socket, csocket_addr_t *dst_addr, void *buf, size_t len, int flags);

static ssize_t _readFromBufferA(csocket_activity_t *activity, void *buf, size_t len, int flags);

static int _searchKeyKeepAlive(const char *query, size_t qlength, const char *msg, size_t *msg_offset, const char *buffer, socklen_t *buffer_offset, socklen_t buffer_usage, socklen_t *var_len);

static int _findKeepAliveMsg(char *msg, size_t msg_len, char *buffer, socklen_t *buffer_usage, char *params, socklen_t *params_usage);

#pragma endregion
/*
	ACTIVITY
*/
#pragma region ACTIVITY

void csocket_updateA(csocket_activity_t *activity);

void csocket_updateFromA(csocket_activity_t *activity);

void csocket_printActivity(FILE *fp, csocket_activity_t *activity);

int csocket_hasRecvDataA(csocket_activity_t *activity);

int csocket_hasRecvFromDataA(csocket_activity_t *activity);

ssize_t csocket_recvA(csocket_activity_t *activity, void *buf, size_t len, int flags);

ssize_t csocket_recvfromA(csocket_activity_t *activity, void *buf, size_t len, int flags);

ssize_t csocket_sendA(csocket_activity_t *activity, void *buf, size_t len, int flags);

ssize_t csocket_sendtoA(csocket_activity_t *activity, void *buf, size_t len, int flags);

csocket_activity_t * csocket_sockToAct(csocket_t *src_socket);

csocket_activity_t * csocket_sockToActA(csocket_t *src_socket, csocket_addr_t *dst_addr);

#pragma endregion
/*
	KEEPALIVE
*/
#pragma region KEEPALIVE

int csocket_keepalive_create(int timeout, char *msg, size_t msg_len, struct csocket_keepalive *ka, csocket_t *src_socket);

int csocket_keepalive_modify(int timeout, char *msg, size_t msg_len, struct csocket_keepalive *ka, csocket_t *src_socket);

int csocket_keepalive_set(struct csocket_keepalive *ka, csocket_t *src_socket);

int csocket_keepalive_unset(csocket_t *src_socket);

int csocket_keepalive_copy(struct csocket_keepalive **dst, const struct csocket_keepalive *src);

int csocket_isAlive(struct csocket_keepalive *ka);

int csocket_keepAlive(csocket_t *src_socket);

char * csocket_resolveKeepAliveMsg(char *src, size_t *size, size_t *offset);

int csocket_getKeepAliveVariable(char *dst, size_t *dst_len, char *query, struct csocket_keepalive *ka);

int csocket_updateKeepAlive(struct csocket_keepalive *ka, int fd);

int csocket_updateKeepAliveFrom(struct csocket_keepalive *ka, int fd, csocket_addr_t *dst_addr);

void csocket_printKeepAlive(FILE *fp, csocket_keepalive_t *ka);

#pragma endregion
/*
	SERVER
*/
#pragma region SERVER

int csocket_bindServer(csocket_t *src_socket);

int csocket_listen(csocket_t *src_socket, int maxQueue);

	/*
		ONE CLIENT AT A TIME
	*/

	int csocket_accept(csocket_t *src_socket, csocket_activity_t *activity);

	/*
		MULTI SERVER
	*/
	// handling all clients - should be called in a while(true)
	int csocket_setUpMultiServer(csocket_t *src_socket, int maxClient, void (*onActivity)(csocket_multiHandler_t *, csocket_activity_t), csocket_multiHandler_t *handler);

	int csocket_multiServer(csocket_multiHandler_t *handler);

int csocket_shutdownClient(csocket_multiHandler_t *handler, struct csocket_clients *client);

#pragma endregion
/*
	CLIENT
*/
#pragma region CLIENT

int csocket_connectClient(csocket_t *src_socket, struct timeval *timeout);

#pragma endregion
/*
	UTIL
*/
#pragma region UTIL

const char * csocket_ntop(int domain, const void *addr, char *dst, socklen_t len);
#define CSOCKET_NTOP(domain, addr, str, strlen) csocket_ntop(domain, addr, str, strlen)

#pragma endregion
/*
	FREE
*/
#pragma region FREE

// free
void csocket_free(csocket_t *src_socket);

void csocket_freeActivity(csocket_activity_t *activity);

void csocket_freeMultiHandler(csocket_multiHandler_t *handler);

void csocket_freeClients(struct csocket_clients *client);

void csocket_freeKeepalive(struct csocket_keepalive *ka);

#pragma endregion


// close connection
void csocket_close(csocket_t *src_socket);

// non callable funtions
static void __attribute__((constructor)) csocket_constructor();
static void __attribute__((destructor)) csocket_destructor();

#endif
