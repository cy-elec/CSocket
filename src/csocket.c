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
	return -(recv(fd, &buf, 1, MSG_PEEK|MSG_DONTWAIT)!=1);
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
				return -1;
			}
		}
		else {
			
			addr->sin_addr.s_addr = (in_addr_t*)addrc;
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
		int opt = 1;
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
	fcntl(client.client_fd, F_SETFL, O_NONBLOCK);
	
	// connect
	if(connect(client.client_fd, client.addr, client.addr_len) == -1) {
		if(errno != EINPROGRESS) {
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
	int err = 0;
	socklen_t len = sizeof(int);
	if((!FD_ISSET(client.client_fd, &rd)&&!FD_ISSET(client.client_fd, &wr)) || getsockopt(client.client_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
		strcpy(src_socket->last_err, "connect none");
		return -1;
	}

	// enable block
	int flg = fcntl(client.client_fd, F_GETFL);
	if(flg!=-1)
		fcntl(client.client_fd, F_SETFL, flg & ~O_NONBLOCK);

	return -(err != 0);
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
	if(src_socket->type != SOCK_STREAM && src_socket->type != SOCK_SEQPACKET) {
		strcpy(src_socket->last_err, "invalid type");
		return -1;
	}

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

	if(activity->addr == NULL) {
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

	// poll action
	fd_set wr, rd, ex;
	FD_ZERO(&wr);
	FD_ZERO(&rd);
	FD_ZERO(&ex);
	FD_SET(activity->fd, &wr);
	FD_SET(activity->fd, &rd);
	FD_SET(activity->fd, &ex);
	int ret = select(activity->fd+1, &rd, &wr, &ex, NULL);
	if(ret < 0) {
		strcpy(src_socket->last_err, "accept err");
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
	int ret = select(activity->fd+1, &rd, &wr, &ex, NULL);
	if(ret < 0) {
		return;
	}
	if(FD_ISSET(activity->fd, &wr))
		activity->type |= CSACT_TYPE_WRITE;
	if(FD_ISSET(activity->fd, &rd))
		activity->type |= CSACT_TYPE_READ;
	if(FD_ISSET(activity->fd, &ex))
		activity->type |= CSACT_TYPE_EXT;
}
// handling all clients - should be called in a while(true)
int csocket_setUpMultiClient(csocket_t *src_socket, int maxClient, void (*onActivity)(csocket_activity_t), csocket_multiHandler_t *handler) {
	if(!src_socket || src_socket->mode.sc!=1) return -1;

	return 0;
}
int csocket_multiClient(csocket_multiHandler_t *handler) {
	if(!handler) return -1;

	return 0;
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
	// reset
	*handler = (const csocket_multiHandler_t)CSOCKET_EMPTY;
}