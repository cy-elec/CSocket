/**
 * @file csocket.c
 * @author Felix Kröhnert (felix.kroehnert@online.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-04
 * 
 * @copyright Copyright (c) 2022
 * 
**/


#include "csocket.h"

const struct in_addr inaddr_any = {.s_addr = INADDR_ANY};

static int _initSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr) {
	if(!src_socket) return -1;

	csocket_free(src_socket);

	if(!addrc && !specialAddr) {
		strcpy(src_socket->last_err, "missing: addr");
		return -1;
	}

	if(domain!=AF_INET&&domain!=AF_INET6) {
		strcpy(src_socket->last_err, "invalid domain");
		return -1;
	}

	// set vars in struct
	src_socket->domain = domain;
	src_socket->type = type;
	src_socket->protocol = protocol;

	// create socket
	if((src_socket->mode.fd = socket(domain, type, protocol)) < 0) {
		strcpy(src_socket->last_err, "create socket");
		return -1;
	}

	return csocket_setAddress(&src_socket->mode.addr, &src_socket->mode.addr_len, domain, addrc, port, specialAddr);
}

static int _hasRecvData(int fd) {
	char buf;
	int res = 0;
	// disable block and test recv
	#ifdef _WIN32
		{
			u_long iMode = 1;
			ioctlsocket(fd, FIONBIO, &iMode);
			res = (recv(fd, &buf, 1, MSG_PEEK)==1);
		}
	#else
		fcntl(fd, F_SETFL, O_NONBLOCK);
		res = (recv(fd, &buf, 1, MSG_PEEK|MSG_DONTWAIT)==1);
	#endif
	

	// enable block
	#ifdef _WIN32
		{
			u_long iMode = 0;
			ioctlsocket(fd, FIONBIO, &iMode);
		}
	#else
		int flg = fcntl(fd, F_GETFL);
		if(flg!=-1)
			fcntl(fd, F_SETFL, flg & ~O_NONBLOCK);
	#endif

	return res;
}


// setup
int csocket_setAddress(struct sockaddr **out_addr, socklen_t *addr_len, int domain, void *addrc, int port, int specialAddr) {
	if(!out_addr || !addr_len || (!addrc && !specialAddr)) return -1;
	// set address
	if(domain == AF_INET) {
		struct sockaddr_in *addr = calloc(1, sizeof(struct sockaddr_in));
		if(!addr) {
			return -1;
		}
		addr->sin_family = AF_INET;
		if(!specialAddr) {
			if(1!=inet_pton(AF_INET, (char*)addrc, &addr->sin_addr)) {
				free(addr);
				return -1;
			}
		}
		else {
			addr->sin_addr = *(struct in_addr*)addrc;
		}
		addr->sin_port = htons(port);
		
		*out_addr = (struct sockaddr*) addr;
		*addr_len = sizeof(struct sockaddr_in);

	}
	else if(domain == AF_INET6) {
		struct sockaddr_in6 *addr = calloc(1, sizeof(struct sockaddr_in6));
		if(!addr) {
			return -1;
		}
		addr->sin6_family = AF_INET6;
		if(!specialAddr) {
			if(1!=inet_pton(AF_INET6, (char*)addrc, &addr->sin6_addr)) {
				free(addr);
				return -1;
			}
		}
		else {
			addr->sin6_addr = *(struct in6_addr*)addrc;
		}
		addr->sin6_port = htons(port);

		*out_addr = (struct sockaddr*) addr;
		*addr_len = sizeof(struct sockaddr_in6);
	}
	else {
		return -1;
	}

	return 0;
}
int csocket_initServerSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr) {

	if(_initSocket(domain, type, protocol, addrc, port, src_socket, specialAddr)) return -1;
	src_socket->mode.sc = 1;
	// set reusable ports
	{
		#ifdef _WIN32
			char
		#else 
			int
		#endif 
			opt = 1;
		if(setsockopt(src_socket->mode.fd, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &opt, sizeof(opt))) {
			strcpy(src_socket->last_err, "setsockopt");
			return -1;
		}
	}

	return 0;
}
int csocket_initClientSocket(int domain, int type, int protocol, void *addrc, int port, csocket_t *src_socket, int specialAddr) {
	int rv = _initSocket(domain, type, protocol, addrc, port, src_socket, specialAddr);
	src_socket->mode.sc = 2;
	return rv;
}

// establish connection/bind socket
int csocket_bindServer(csocket_t *src_socket){
	if(!src_socket || src_socket->mode.sc!=1) return -1;

	// bind
	struct csocket_server server = *((struct csocket_server*)&src_socket->mode);
	if(bind(server.server_fd, server.addr, server.addr_len)) {
		strcpy(src_socket->last_err, "bind");
		return -1;
	}

	return 0;
}
int csocket_connectClient(csocket_t *src_socket, struct timeval *timeout) {
	if(!src_socket || src_socket->mode.sc!=2) return -1;

	struct csocket_client client = *((struct csocket_client*)&src_socket->mode);
	
	// disable block
	#ifdef _WIN32
		{
			u_long iMode = 0;
			ioctlsocket(client.client_fd, FIONBIO, &iMode);
		}
	#else
		fcntl(client.client_fd, F_SETFL, O_NONBLOCK);
	#endif
	
	// connect
	if(connect(client.client_fd, client.addr, client.addr_len) == -1) {
		if(errno != CS_EINPROGRESS) {
			strcpy(src_socket->last_err, "connect err");
			return -1;
		}
	}
	// verify connection
	fd_set wr, rd;
	FD_ZERO(&wr);
	FD_ZERO(&rd);
	FD_SET(client.client_fd, &wr);
	FD_SET(client.client_fd, &rd);
	int ret = select(client.client_fd+1, &rd, &wr, NULL, timeout);
	if(ret == 0) {
		strcpy(src_socket->last_err, "connect timeout");
		return -1;
	}
	if(ret < 0) {
		strcpy(src_socket->last_err, "connect err");
		return -1;
	}
	#ifdef _WIN32
		char
	#else 
		int
	#endif 
		err = 1;
	socklen_t len = sizeof(int);
	if((!FD_ISSET(client.client_fd, &rd)&&!FD_ISSET(client.client_fd, &wr)) || getsockopt(client.client_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
		strcpy(src_socket->last_err, "connect none");
		return -1;
	}

	#ifdef _WIN32
		{
			u_long iMode = 0;
			ioctlsocket(client.client_fd, FIONBIO, &iMode);
		}
	#else
		// enable block
		int flg = fcntl(client.client_fd, F_GETFL);
		if(flg!=-1)
			fcntl(client.client_fd, F_SETFL, flg & ~O_NONBLOCK);
	#endif

	return -(err != 0);
}


const char * csocket_ntop(int domain, const void *addr, char *dst, socklen_t len) {
	return inet_ntop(domain, domain==AF_INET?(void*)&(((struct sockaddr_in*)addr)->sin_addr):(void*)&(((struct sockaddr_in6*)addr)->sin6_addr), dst, len);
}


// one client at a time
int csocket_listen(csocket_t *src_socket, int maxQueue) {
	if(!src_socket || src_socket->mode.sc!=1) return -1;
	if(src_socket->type != SOCK_STREAM && src_socket->type != SOCK_SEQPACKET) {
		strcpy(src_socket->last_err, "invalid type");
		return -1;
	}
	if(listen(src_socket->mode.fd, maxQueue) < 0) {
		strcpy(src_socket->last_err, "listen");
		return -1;
	}
	return 0;
}
int csocket_accept(csocket_t *src_socket, csocket_activity_t *activity) {
	if(!src_socket || !activity || src_socket->mode.sc!=1) return -1;

	// free activity, just in case
	csocket_freeActivity(activity);

	struct csocket_server server = *((struct csocket_server*)&src_socket->mode);


	// update size
	if(src_socket->domain == AF_INET)
		activity->addr_len = sizeof(struct sockaddr_in);
	else
		activity->addr_len = sizeof(struct sockaddr_in6);

	if(activity->addr)
		free(activity->addr);
	activity->addr = calloc(1, activity->addr_len);

	if(!activity->addr) {
		strcpy(src_socket->last_err, "calloc");
		return -1;
	}

	// accept
	if((server.client_fd = accept(server.server_fd, activity->addr, &activity->addr_len)) < 0 ) {
		strcpy(src_socket->last_err, "accept");
		return -1;
	}

	if(activity->addr_len == sizeof(struct sockaddr_in))
		activity->domain = AF_INET;
	else if(activity->addr_len == sizeof(struct sockaddr_in6))
		activity->domain = AF_INET6;
	else
		activity->domain = -1;
	activity->type = CSACT_TYPE_CONN;
	activity->fd = server.client_fd;

	// set time
	activity->time = time(NULL);
	activity->update_time = activity->time;

	// poll action
	fd_set wr, rd, ex;
	FD_ZERO(&wr);
	FD_ZERO(&rd);
	FD_ZERO(&ex);
	FD_SET(activity->fd, &wr);
	FD_SET(activity->fd, &rd);
	FD_SET(activity->fd, &ex);

	struct timeval TIMEVAL_ZERO = {0};
	int ret = select(activity->fd+1, &rd, &wr, &ex, &TIMEVAL_ZERO);
	if(ret < 0) {
		strcpy(src_socket->last_err, "accept");
		return -1;
	}
	if(FD_ISSET(activity->fd, &wr))
		activity->type |= CSACT_TYPE_WRITE;
	if(FD_ISSET(activity->fd, &rd))
		activity->type |= CSACT_TYPE_READ;
	if(FD_ISSET(activity->fd, &ex))
		activity->type |= CSACT_TYPE_EXT;

	return 0;
}
void csocket_updateA(csocket_activity_t *activity) {
	// poll action
	fd_set wr, rd, ex;
	FD_ZERO(&wr);
	FD_ZERO(&rd);
	FD_ZERO(&ex);
	FD_SET(activity->fd, &wr);
	FD_SET(activity->fd, &rd);
	FD_SET(activity->fd, &ex);
	struct timeval TIMEVAL_ZERO = {0};
	int ret = select(activity->fd+1, &rd, &wr, &ex, &TIMEVAL_ZERO);
	if(ret < 0) {
		return;
	}

	// set time
	activity->update_time = time(NULL);

	if(FD_ISSET(activity->fd, &wr))
		activity->type |= CSACT_TYPE_WRITE;
	if(FD_ISSET(activity->fd, &rd))
		activity->type |= CSACT_TYPE_READ;
	if(FD_ISSET(activity->fd, &ex))
		activity->type |= CSACT_TYPE_EXT;
}
// handling all clients - should be called in a while(true)
int csocket_setUpMultiServer(csocket_t *src_socket, int maxClient, void (*onActivity)(csocket_activity_t), csocket_multiHandler_t *handler) {
	if(!src_socket || !handler || src_socket->mode.sc!=1) return -1;
	
	// free handler, just in case
	csocket_freeMultiHandler(handler);

	handler->src_socket = src_socket;
	handler->onActivity = onActivity;
	if(maxClient<1) {
		strcpy(src_socket->last_err, "multiServer maxClient<1");
		return -1;
	} 
	handler->maxClients = maxClient;

	handler->client_sockets = calloc(maxClient, sizeof(struct csocket_clients));
	if(!handler->client_sockets) {
		strcpy(src_socket->last_err, "multiServer calloc");
		return -1;
	}

	return 0;
}
int csocket_multiServer(csocket_multiHandler_t *handler) {
	if(!handler) return -1;

	/**
	 * 1. SET FDS
	 * 2. test root socket for read/write -> new client
	 * 3. check other sockets on read and trigger onAction event
	 * 
	**/

	struct csocket_server server= *((struct csocket_server*)&handler->src_socket->mode);

	int maxfd = server.server_fd;

	fd_set rd;
	FD_ZERO(&rd);
	FD_SET(server.server_fd, &rd);

	// add other clients
	for(int i=0; i<handler->maxClients; ++i) {
		if(handler->client_sockets[i].fd>0) {
			FD_SET(handler->client_sockets[i].fd, &rd);
		}
		maxfd = handler->client_sockets[i].fd>maxfd?handler->client_sockets[i].fd:maxfd;
	}
	struct timeval TIMEVAL_ZERO = {0};
	if(select(maxfd+1, &rd, NULL, NULL, &TIMEVAL_ZERO)<0) {
		return 0;
	}

	/*
		HANDLE SELECTED EVENTS
	*/

	// action on server.server_fd -> new client
	if(FD_ISSET(server.server_fd, &rd)) {

		// new client
		struct csocket_clients client = {0}; 
		// set size
		if(handler->src_socket->domain == AF_INET)
			client.addr_len = sizeof(struct sockaddr_in);
		else
			client.addr_len = sizeof(struct sockaddr_in6);
		
		// alloc address
		client.addr = calloc(1, client.addr_len);

		if(!client.addr) {
			strcpy(handler->src_socket->last_err, "multiServer calloc");
			return -1;
		}
	
		// accept
		if((server.client_fd = accept(server.server_fd, client.addr, &client.addr_len)) < 0 ) {
			free(client.addr);
			strcpy(handler->src_socket->last_err, "multiServer accept");
			return -1;
		}

		/**
		 * 
		 * FORM ACTIVITY AND CLIENT
		 * 
		**/
		client.fd = server.client_fd;
		
		// verify size and set domain
		if(client.addr_len == sizeof(struct sockaddr_in))
			client.domain = AF_INET;
		else if(client.addr_len == sizeof(struct sockaddr_in6))
			client.domain = AF_INET6;
		else
			client.domain = -1;
		
		// set activity
		csocket_activity_t activity = CSOCKET_EMPTY;
		activity.fd = client.fd;
		activity.domain = client.domain;
		activity.addr = client.addr;
		activity.addr_len = client.addr_len;
		activity.type = CSACT_TYPE_CONN | CSACT_TYPE_DECLINED;
		// set time
		activity.time = time(NULL);
		activity.update_time = activity.time;

		// poll action
		fd_set wr2, rd2, ex2;
		FD_ZERO(&wr2);
		FD_ZERO(&rd2);
		FD_ZERO(&ex2);
		FD_SET(activity.fd, &wr2);
		FD_SET(activity.fd, &rd2);
		FD_SET(activity.fd, &ex2);
		struct timeval TIMEVAL_ZERO = {0};
		int ret = select(activity.fd+1, &rd2, &wr2, &ex2, &TIMEVAL_ZERO);
		if(ret < 0) {
			strcpy(handler->src_socket->last_err, "multiServer accept");
			return -1;
		}
		if(FD_ISSET(activity.fd, &wr2))
			activity.type |= CSACT_TYPE_WRITE;
		if(FD_ISSET(activity.fd, &rd2))
			activity.type |= CSACT_TYPE_READ;
		if(FD_ISSET(activity.fd, &ex2))
			activity.type |= CSACT_TYPE_EXT;

		// add to list
		for(int i=0; i<handler->maxClients; ++i) {
			if(handler->client_sockets[i].fd == 0) {
				handler->client_sockets[i] = client;
				activity.type &= ~CSACT_TYPE_DECLINED;
				break;
			}
		}
		
		// trigger action
		if(handler->onActivity)
			handler->onActivity(activity);
	}

	// action on any socket
	for(int i=0; i<handler->maxClients; ++i) {
		// client
		struct csocket_clients client = handler->client_sockets[i]; 
		if(FD_ISSET(client.fd, &rd)) {
			// no data: disconnected
			if(!_hasRecvData(client.fd)) {
				/**
				 * 
				 * FORM ACTIVITY
				 * 
				**/
				csocket_activity_t activity = CSOCKET_EMPTY;
				activity.fd = client.fd;
				activity.domain = client.domain;
				activity.addr = client.addr;
				activity.addr_len = client.addr_len;
				activity.type = CSACT_TYPE_DISCONN;
				// set time
				activity.time = time(NULL);
				activity.update_time = activity.time;
				
				shutdown(client.fd, SHUT_RDWR);
				
				// trigger action
				if(handler->onActivity) {
					handler->onActivity(activity);
					handler->client_sockets[i] = (const struct csocket_clients)CSOCKET_EMPTY;
				}
				else {
					// free
					csocket_freeActivity(&activity);
					// write addr = NULL to prevent duplicate free
					handler->client_sockets[i].addr = NULL;
					csocket_freeClients(&handler->client_sockets[i]);
				}
			}
			else {
				/**
				 * 
				 * FORM ACTIVITY
				 * 
				**/
				csocket_activity_t activity = CSOCKET_EMPTY;
				activity.fd = client.fd;
				activity.domain = client.domain;
				activity.addr = client.addr;
				activity.addr_len = client.addr_len;
				// set time
				activity.time = time(NULL);
				activity.update_time = activity.time;

				// poll action
				fd_set wr2, rd2, ex2;
				FD_ZERO(&wr2);
				FD_ZERO(&rd2);
				FD_ZERO(&ex2);
				FD_SET(activity.fd, &wr2);
				FD_SET(activity.fd, &rd2);
				FD_SET(activity.fd, &ex2);
				struct timeval TIMEVAL_ZERO = {0};
				int ret = select(activity.fd+1, &rd2, &wr2, &ex2, &TIMEVAL_ZERO);
				if(ret < 0) {
					strcpy(handler->src_socket->last_err, "multiServer socketchange");
					return -1;
				}
				if(FD_ISSET(activity.fd, &wr2))
					activity.type |= CSACT_TYPE_WRITE;
				if(FD_ISSET(activity.fd, &rd2))
					activity.type |= CSACT_TYPE_READ;
				if(FD_ISSET(activity.fd, &ex2))
					activity.type |= CSACT_TYPE_EXT;

				// trigger action
				if(handler->onActivity)
					handler->onActivity(activity);
			}
		}
	}

	return 0;
}

// activity
void csocket_printActivity(int fd, csocket_activity_t *activity) {
	int fd2 = dup(fd);
	if(!activity) return;
	FILE *fp = fdopen(fd2, "w");
	if(!fp) return;

	char addrs[40];
	CSOCKET_NTOP(activity->domain, activity->addr, addrs, 40);

	fprintf(fp, "[%.24s] >> %s", ctime(&activity->time), addrs);
	fprintf(fp, "\tLast update: %.24s\n", ctime(&activity->update_time));
	fprintf(fp, "\tType: %s%s%s%s%s%s\n", activity->type&CSACT_TYPE_CONN?"CONN ":"", activity->type&CSACT_TYPE_DISCONN?"DISCONN ":"", activity->type&CSACT_TYPE_READ?"READ ":"", activity->type&CSACT_TYPE_WRITE?"WRITE ":"", activity->type&CSACT_TYPE_EXT?"EXT ":"", activity->type&CSACT_TYPE_DECLINED?"DECLINED ":"");
	fprintf(fp, "\t\n");

	fclose(fp);
}


// read/write
int csocket_hasRecvData(csocket_t *src_socket) {
	if(!src_socket) return -1;
	return _hasRecvData(src_socket->mode.fd);
}
int csocket_recv(csocket_t *src_socket, void *buf, size_t len, int flags) {
	if(!src_socket) return -1;
	return recv(src_socket->mode.fd, buf, len, flags);
}
int csocket_recvfrom(csocket_t *src_socket, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t addr_len) {
	if(!src_socket) return -1;
	return recvfrom(src_socket->mode.fd, buf, len, flags, addr, &addr_len);
}

int csocket_send(csocket_t *src_socket, void *buf, size_t len, int flags) {
	if(!src_socket) return -1;
	return send(src_socket->mode.fd, buf, len, flags);
}
int csocket_sendto(csocket_t *src_socket, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t addr_len) {
	if(!src_socket) return -1;
	return sendto(src_socket->mode.fd, buf, len, flags, addr, addr_len);
}

int csocket_hasRecvDataA(csocket_activity_t *activity) {
	if(!activity) return -1;
	return _hasRecvData(activity->fd);
}
int csocket_recvA(csocket_activity_t *activity, void *buf, size_t len, int flags) {
	if(!activity) return -1;
	return recv(activity->fd, buf, len, flags);
}
int csocket_recvfromA(csocket_activity_t *activity, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t addr_len) {
	if(!activity) return -1;
	return recvfrom(activity->fd, buf, len, flags, addr, &addr_len);
}

int csocket_sendA(csocket_activity_t *activity, void *buf, size_t len, int flags) {
	if(!activity) return -1;
	return send(activity->fd, buf, len, flags);
}
int csocket_sendtoA(csocket_activity_t *activity, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t addr_len) {
	if(!activity) return -1;
	return sendto(activity->fd, buf, len, flags, addr, addr_len);
}


// close connection
void csocket_close(csocket_t *src_socket) {
	if(!src_socket) return;

	// free (close remaining fds)
	csocket_free(src_socket);
}

// free
void csocket_free(csocket_t *src_socket) {
	if(!src_socket) return;
	
	// free
	if(src_socket->mode.sc>0&&src_socket->mode.addr) {
		free(src_socket->mode.addr);
		src_socket->mode.addr = NULL;
	}

	// close fd
	if(src_socket->mode.fd>=0)
		shutdown(src_socket->mode.fd, SHUT_RDWR);
	if(src_socket->mode.sc==1&&((struct csocket_server*)&src_socket->mode)->client_fd>=0)
		shutdown(((struct csocket_server*)&src_socket->mode)->client_fd, SHUT_RDWR);
	// reset
	*src_socket = (const csocket_t)CSOCKET_EMPTY;
}
void csocket_freeActivity(csocket_activity_t *activity) {
	if(!activity) return;

	// free
	if(activity->addr) {
		free(activity->addr);
		activity->addr = NULL;
	}

	// close fd
	if(activity->fd>=0)
		shutdown(activity->fd, SHUT_RDWR);
	// reset
	*activity = (const csocket_activity_t)CSOCKET_EMPTY;
}

void csocket_freeMultiHandler(csocket_multiHandler_t *handler) {
	if(!handler) return;

	// free
	if(handler->client_sockets) {
		free(handler->client_sockets);
		handler->client_sockets = NULL;
	}

	// reset
	*handler = (const csocket_multiHandler_t)CSOCKET_EMPTY;
}
void csocket_freeClients(struct csocket_clients *client) {
	if(!client) return;

	// free
	if(client->addr) {
		free(client->addr);
		client->addr = NULL;
	}

	// reset
	*client = (const struct csocket_clients)CSOCKET_EMPTY;
}


// non callable funtions
static void __attribute__((constructor)) csocket_constructor() {
	#ifdef _WIN32
		WSADATA wsa;
		WSAStartup(MAKEWORD(2,2), &wsa);
	#endif
	printf("%s\n", CSOCKET_START_MSG);
}

static void __attribute__((destructor)) csocket_destructor() {
	#ifdef _WIN32
		WSACleanup();
	#endif
	printf("%s\n", CSOCKET_CLOSE_MSG);
}
