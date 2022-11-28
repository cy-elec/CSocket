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
struct timespec csocket_timeout = {0};

#pragma region SOCKET SETUP

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


int csocket_setAddressA(csocket_addr_t *out_addr, int domain, void *addrc, int port, int specialAddr) {
	if(!out_addr) return -1;
	return csocket_setAddress(&out_addr->addr, &out_addr->addr_len, domain, addrc, port, specialAddr);
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

#pragma endregion

#pragma region RECV/SEND

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

static int _hasRecvFromData(int fd, struct sockaddr *addr, socklen_t *addr_len) {
	char buf;
	int res = 0;
	// disable block and test recv
	#ifdef _WIN32
		{
			u_long iMode = 1;
			ioctlsocket(fd, FIONBIO, &iMode);
			res = (recvfrom(fd, &buf, 1, MSG_PEEK, addr, addr_len)==1);
		}
	#else
		fcntl(fd, F_SETFL, O_NONBLOCK);
		res = (recvfrom(fd, &buf, 1, MSG_PEEK|MSG_DONTWAIT, addr, addr_len)==1);
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

// read/write
int csocket_hasRecvData(csocket_t *src_socket) {
	if(!src_socket) return -1;
	if(src_socket->ka && src_socket->ka->enabled) {
		return _hasRecvDataBuffer(src_socket->ka, src_socket->mode.fd);
	}
	return _hasRecvData(src_socket->mode.fd);
}

int csocket_hasRecvFromData(csocket_t *src_socket, csocket_addr_t *dst_addr) {
	if(!src_socket||!dst_addr) return -1;
	if(src_socket->ka && src_socket->ka->enabled) {
		return _hasRecvFromDataBuffer(src_socket->ka, src_socket->mode.fd, dst_addr->addr, &dst_addr->addr_len);
	}
	return _hasRecvFromData(src_socket->mode.fd, dst_addr->addr, &dst_addr->addr_len);
}

ssize_t csocket_recv(csocket_t *src_socket, void *buf, size_t len, int flags) {
	
	if(!src_socket) return -1;
	if(src_socket->ka && src_socket->ka->enabled) {
		return _readBuffer(src_socket, buf, len, flags);
	}
	return recv(src_socket->mode.fd, buf, len, flags);
}

ssize_t csocket_recvfrom(csocket_t *src_socket, csocket_addr_t *dst_addr, void *buf, size_t len, int flags) {
	if(!src_socket||!dst_addr) return -1;
	if(src_socket->ka && src_socket->ka->enabled) {
		return _readFromBuffer(src_socket, dst_addr, buf, len, flags);
	}
	return recvfrom(src_socket->mode.fd, buf, len, flags, dst_addr->addr, &dst_addr->addr_len);
}

ssize_t csocket_send(csocket_t *src_socket, void *buf, size_t len, int flags) {
	if(!src_socket) return -1;
	return send(src_socket->mode.fd, buf, len, flags);
}

ssize_t csocket_sendto(csocket_t *src_socket, void *buf, size_t len, int flags) {
	if(!src_socket) return -1;
	return sendto(src_socket->mode.fd, buf, len, flags, src_socket->mode.addr, src_socket->mode.addr_len);
}

#pragma endregion

#pragma region RECV/SEND BUFFER

static int _hasRecvDataBuffer(csocket_keepalive_t *ka, int fd) {
	if(!ka) return -1;
	_updateBuffer(ka, fd, MSG_PEEK|MSG_DONTWAIT);
	if(ka->buffer_usage>0) return 1;
	return 0;
}

static int _hasRecvFromDataBuffer(csocket_keepalive_t *ka, int fd, struct sockaddr *addr, socklen_t *addr_len) {
	if(!ka) return -1;
	_updateFromBuffer(ka, fd, MSG_PEEK|MSG_DONTWAIT, addr, addr_len);
	if(ka->buffer_usage>0) return 1;
	return 0;
}

// update internal buffer
static ssize_t _updateBuffer(csocket_keepalive_t *ka, int fd, int flags) {
	if(!ka || !ka->enabled) return -1;

	if(ka->buffer_usage==ka->buffer_len) {
		return 0;
	}

	socklen_t offset = ka->buffer_usage;

	ssize_t res = 0;
	int lasterr = 0;
	// disable block and test recv
	#ifdef _WIN32
		{
			if(flags&MSG_DONTWAIT) {
				u_long iMode = 1;
				ioctlsocket(fd, FIONBIO, &iMode);
			}
			res = recv(fd, ka->buffer+offset, ka->buffer_len-offset, flags&~MSG_DONTWAIT&~MSG_PEEK);
			lasterr = errno;
		}
	#else
		if(flags&MSG_DONTWAIT) {
			fcntl(fd, F_SETFL, O_NONBLOCK);
		}
		res = recv(fd, ka->buffer+offset, ka->buffer_len-offset, flags&~MSG_PEEK);
		lasterr = errno;
	#endif

	// enable block
	if(flags&MSG_DONTWAIT) {
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
	}
	
	if(res<=0 && (ka->buffer_usage > 0 || lasterr == EAGAIN || lasterr == EWOULDBLOCK))return 0;
	if(res<=0 && ka->buffer_usage <= 0) return -1;
	ka->buffer_usage = offset+res;

	// search buffer for keepalive and set ka->last_sig
	size_t length = ka->buffer_usage;

	/*
		RESOLVE ka->msg / CSKA_DEFAULTMSG
		SEARCH FOR RESOLVED QUERY and remove from buffer + update ka->last_sig
	*/
	if(!_findKeepAliveMsg(ka->msg, ka->msg_len, ka->buffer, &ka->buffer_usage, ka->params, &ka->params_usage)) {
		ka->last_sig = time(NULL);
		if(ka->onActivity) ka->onActivity(ka);
	}

	// if cropped length is smaller than previous length, call update again with offset of current length
	if(ka->buffer_usage==ka->buffer_len&&(socklen_t)length<ka->buffer_usage)
		return _updateBuffer(ka, fd, flags);

	if(ka->buffer_usage <= 0) return -1;
	return 0;
}

// update internal buffer (from)
static ssize_t _updateFromBuffer(csocket_keepalive_t *ka, int fd, int flags, struct sockaddr *addr, socklen_t *addr_len) {
	if(!ka || !ka->enabled) return -1;

	if(ka->buffer_usage==ka->buffer_len) {
		return 0;
	}

	socklen_t offset = ka->buffer_usage;

	ssize_t res = 0;
	int lasterr = 0;
	// disable block and test recv
	#ifdef _WIN32
		{
			if(flags&MSG_DONTWAIT) {
				u_long iMode = 1;
				ioctlsocket(fd, FIONBIO, &iMode);
			}
			res = recvfrom(fd, ka->buffer+offset, ka->buffer_len-offset, flags&~MSG_DONTWAIT&~MSG_PEEK, addr, addr_len);
			lasterr = errno;
		}
	#else
		if(flags&MSG_DONTWAIT) {
			fcntl(fd, F_SETFL, O_NONBLOCK);
		}
		res = recvfrom(fd, ka->buffer+offset, ka->buffer_len-offset, flags&~MSG_PEEK, addr, addr_len);
		lasterr = errno;
	#endif
	

	// enable block
	if(flags&MSG_DONTWAIT) {
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
	}

	if(res<=0 && (ka->buffer_usage > 0 || lasterr == EAGAIN || lasterr == EWOULDBLOCK))return 0;
	if(res<=0 && ka->buffer_usage <= 0) return -1;
	ka->buffer_usage = offset+res;

	// search buffer for keepalive and set ka->last_sig
	size_t length = ka->buffer_usage;

	/*
		RESOLVE ka->msg / CSKA_DEFAULTMSG
		SEARCH FOR RESOLVED QUERY and remove from buffer + update ka->last_sig
	*/
	if(!_findKeepAliveMsg(ka->msg, ka->msg_len, ka->buffer, &ka->buffer_usage, ka->params, &ka->params_usage)) {
		ka->last_sig = time(NULL);
		if(ka->onActivity) ka->onActivity(ka);
	}

	// if cropped length is smaller than previous length, call update again with offset of current length
	if(ka->buffer_usage==ka->buffer_len&&(socklen_t)length<ka->buffer_usage)
		return _updateFromBuffer(ka, fd, flags, addr, addr_len);

	if(ka->buffer_usage <= 0) return -1;
	return 0;
}

static ssize_t _readBuffer(csocket_t *src_socket, void *buf, size_t len, int flags) {
	
	csocket_activity_t *activity = csocket_sockToAct(src_socket);
	if(activity) {
		return _readBufferA(activity, buf, len, flags);
	}
	return -1;
}

static ssize_t _readBufferA(csocket_activity_t *activity, void *buf, size_t len, int flags) {
	int res = 0;

	// timeout
	struct timespec ts, cs;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_sec+=csocket_timeout.tv_sec;
	ts.tv_nsec+=csocket_timeout.tv_nsec;

	if(!buf || len==0) return -1;
	do {
		csocket_updateA(activity);

		// timeout 
		clock_gettime(CLOCK_MONOTONIC, &cs);
		if((csocket_timeout.tv_sec>0||csocket_timeout.tv_nsec>0)&&((ts.tv_sec == cs.tv_sec && ts.tv_nsec < cs.tv_nsec) || ts.tv_sec < cs.tv_sec)) {
			break;
		}
	} while(!(activity->type&CSACT_TYPE_READ));
	
	while(!_hasRecvDataBuffer(activity->client_socket.ka, activity->client_socket.fd)) {
		// timeout
		clock_gettime(CLOCK_MONOTONIC, &cs);
		if((csocket_timeout.tv_sec>0||csocket_timeout.tv_nsec>0)&&((ts.tv_sec == cs.tv_sec && ts.tv_nsec < cs.tv_nsec) || ts.tv_sec < cs.tv_sec)) {
			break;
		}
	}
	if(_updateBuffer(activity->client_socket.ka, activity->client_socket.fd, flags|MSG_DONTWAIT))
		return -1;
	
	if(res<0) return -1;

	ssize_t resolved = len;

	if((socklen_t)len>activity->client_socket.ka->buffer_usage)
		resolved = activity->client_socket.ka->buffer_usage;

	memcpy(buf, activity->client_socket.ka->buffer, resolved);

	if(!(flags&MSG_PEEK)) {
		memmove(activity->client_socket.ka->buffer, activity->client_socket.ka->buffer+resolved, activity->client_socket.ka->buffer_usage-resolved);
		activity->client_socket.ka->buffer_usage-=resolved;
	}

	return resolved;
}

static ssize_t _readFromBuffer(csocket_t *src_socket, csocket_addr_t *dst_addr, void *buf, size_t len, int flags) {
	
	csocket_activity_t *activity = csocket_sockToActA(src_socket, dst_addr);
	if(activity) {
		int rv = _readFromBufferA(activity, buf, len, flags);
		dst_addr->addr_len = activity->client_socket.addr_len;
		return rv;
	}
	return -1;
}

static ssize_t _readFromBufferA(csocket_activity_t *activity, void *buf, size_t len, int flags) {
	int res = 0;

	// timeout
	struct timespec ts, cs;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_sec+=csocket_timeout.tv_sec;
	ts.tv_nsec+=csocket_timeout.tv_nsec;

	if(!buf || len==0) return -1;
	do {
		csocket_updateFromA(activity);

		// timeout
		clock_gettime(CLOCK_MONOTONIC, &cs);
		if((csocket_timeout.tv_sec>0||csocket_timeout.tv_nsec>0)&&((ts.tv_sec == cs.tv_sec && ts.tv_nsec < cs.tv_nsec) || ts.tv_sec < cs.tv_sec)) {
			break;
		}
	} while(!(activity->type&CSACT_TYPE_READ));
	
	while(!_hasRecvFromDataBuffer(activity->client_socket.ka, activity->client_socket.fd, activity->client_socket.addr, &activity->client_socket.addr_len)) {
		// timeout
		clock_gettime(CLOCK_MONOTONIC, &cs);
		if((csocket_timeout.tv_sec>0||csocket_timeout.tv_nsec>0)&&((ts.tv_sec == cs.tv_sec && ts.tv_nsec < cs.tv_nsec) || ts.tv_sec < cs.tv_sec)) {
			break;
		}
	}
	if(_updateFromBuffer(activity->client_socket.ka, activity->client_socket.fd, flags|MSG_DONTWAIT, activity->client_socket.addr, &activity->client_socket.addr_len))
		return -1;
	
	if(res<0) return -1;

	ssize_t resolved = len;

	if((socklen_t)len>activity->client_socket.ka->buffer_usage)
		resolved = activity->client_socket.ka->buffer_usage;

	memcpy(buf, activity->client_socket.ka->buffer, resolved+1);
	
	if(!(flags&MSG_PEEK)) {
		memmove(activity->client_socket.ka->buffer, activity->client_socket.ka->buffer+resolved, activity->client_socket.ka->buffer_usage-resolved);
		activity->client_socket.ka->buffer_usage-=resolved;
	}

	return resolved;
}

static int _searchKeyKeepAlive(const char *query, size_t qlength, const char *msg, size_t *msg_offset, const char *buffer, socklen_t *buffer_offset, socklen_t buffer_usage, socklen_t *var_len) {
	
	if(strncmp(query, msg+*msg_offset, qlength)==0&&strncmp(query, buffer+*buffer_offset, qlength)==0) {
		socklen_t starting_at = *buffer_offset;
		socklen_t ending_at = 0;
		for(socklen_t o=starting_at+qlength; o<buffer_usage; ++o) {
			if(strncmp(query, buffer+o, qlength)==0) {
				ending_at = o+qlength-1;
				break;
			}
		}
		if(ending_at==0) {
			*msg_offset+=1;
		}
		else {
			*msg_offset+=qlength;
			*var_len += ending_at-starting_at-qlength+1;
			*buffer_offset = ending_at;
		}
		return 0;
	}
	return -1;
}

static int _findKeepAliveMsg(char *msg, size_t msg_len, char *buffer, socklen_t *buffer_usage, char *params, socklen_t *params_usage) {
	size_t msg_offset = 0;

	socklen_t var_len = msg_len;

	int foundKA = 0;


	for(socklen_t buffer_offset = 0; buffer_offset < *buffer_usage; ++buffer_offset) {
		
		if(*(msg+msg_offset)==*(buffer+buffer_offset)) {
			
			if(msg_offset==msg_len-1) {
				
				memcpy(params, buffer+(buffer_offset)-(var_len-1), var_len);
				
				*params_usage = var_len;
				params[var_len+1] = 0;
				
				memmove(buffer+(buffer_offset)-(var_len-1), buffer+(buffer_offset+1), *buffer_usage-(buffer_offset));
				*buffer_usage-=var_len;

				msg_offset = 0;
				buffer_offset = (buffer_offset)-(var_len-2);
				foundKA = 1;
			}
			else {
				// search keywords
				if(_searchKeyKeepAlive("%UNIX%", 6, msg, &msg_offset, buffer, &buffer_offset, *buffer_usage, &var_len) +
				_searchKeyKeepAlive("%HOST%", 6, msg, &msg_offset, buffer, &buffer_offset, *buffer_usage, &var_len) +
				_searchKeyKeepAlive("%USER%", 6, msg, &msg_offset, buffer, &buffer_offset, *buffer_usage, &var_len) <= -3) {
					++msg_offset;
				}
			}
		}
		else {
			msg_offset = 0;
			var_len = msg_len;
		}
	}

	return !foundKA;
}

#pragma endregion

#pragma region ACTIVITY

void csocket_updateA(csocket_activity_t *activity) {
	if(!activity) return;

	// poll action
	fd_set wr, rd, ex;
	FD_ZERO(&wr);
	FD_ZERO(&rd);
	FD_ZERO(&ex);
	FD_SET(activity->client_socket.fd, &wr);
	FD_SET(activity->client_socket.fd, &rd);
	FD_SET(activity->client_socket.fd, &ex);
	struct timeval TIMEVAL_ZERO = {0};
	int ret = select(activity->client_socket.fd+1, &rd, &wr, &ex, &TIMEVAL_ZERO);
	if(ret < 0) {
		return;
	}

	// set time
	activity->update_time = time(NULL);

	if(FD_ISSET(activity->client_socket.fd, &wr))
		activity->type |= CSACT_TYPE_WRITE;
	if(FD_ISSET(activity->client_socket.fd, &rd) || (activity->client_socket.ka && activity->client_socket.ka->buffer_usage>0))
		activity->type |= CSACT_TYPE_READ;
	else if(csocket_hasRecvDataA(activity)==0)
		activity->type &= ~CSACT_TYPE_READ;

	if(FD_ISSET(activity->client_socket.fd, &ex))
		activity->type |= CSACT_TYPE_EXT;
}

void csocket_updateFromA(csocket_activity_t *activity) {
	if(!activity) return;

	// poll action
	fd_set wr, rd, ex;
	FD_ZERO(&wr);
	FD_ZERO(&rd);
	FD_ZERO(&ex);
	FD_SET(activity->client_socket.fd, &wr);
	FD_SET(activity->client_socket.fd, &rd);
	FD_SET(activity->client_socket.fd, &ex);
	struct timeval TIMEVAL_ZERO = {0};
	int ret = select(activity->client_socket.fd+1, &rd, &wr, &ex, &TIMEVAL_ZERO);
	if(ret < 0) {
		return;
	}

	// set time
	activity->update_time = time(NULL);

	if(FD_ISSET(activity->client_socket.fd, &wr))
		activity->type |= CSACT_TYPE_WRITE;
	if(FD_ISSET(activity->client_socket.fd, &rd) || (activity->client_socket.ka && activity->client_socket.ka->buffer_usage>0))
		activity->type |= CSACT_TYPE_READ;
	else if(csocket_hasRecvFromDataA(activity)==0)
		activity->type &= ~CSACT_TYPE_READ;

	if(FD_ISSET(activity->client_socket.fd, &ex))
		activity->type |= CSACT_TYPE_EXT;
}

void csocket_printActivity(FILE *fp, csocket_activity_t *activity) {
	if(!fp) return;

	char addrs[40];
	CSOCKET_NTOP(activity->client_socket.domain, activity->client_socket.addr, addrs, 40);

	fprintf(fp, "[%.24s] >> %s", ctime(&activity->time), addrs);
	fprintf(fp, "\tLast update: %.24s\n", ctime(&activity->update_time));
	fprintf(fp, "\tType: %s%s%s%s%s%s\n", activity->type&CSACT_TYPE_CONN?"CONN ":"", activity->type&CSACT_TYPE_DISCONN?"DISCONN ":"", activity->type&CSACT_TYPE_READ?"READ ":"", activity->type&CSACT_TYPE_WRITE?"WRITE ":"", activity->type&CSACT_TYPE_EXT?"EXT ":"", activity->type&CSACT_TYPE_DECLINED?"DECLINED ":"");
	fprintf(fp, "\t\n");
}

int csocket_hasRecvDataA(csocket_activity_t *activity) {
	if(!activity) return -1;
	if(activity->client_socket.ka && activity->client_socket.ka->enabled) {
		return _hasRecvDataBuffer(activity->client_socket.ka, activity->client_socket.fd);
	}
	return _hasRecvData(activity->client_socket.fd);
}

int csocket_hasRecvFromDataA(csocket_activity_t *activity) {
	if(!activity) return -1;
	if(activity->client_socket.ka && activity->client_socket.ka->enabled) {
		return _hasRecvFromDataBuffer(activity->client_socket.ka, activity->client_socket.fd, activity->client_socket.addr, &activity->client_socket.addr_len);
	}
	return _hasRecvFromData(activity->client_socket.fd, activity->client_socket.addr, &activity->client_socket.addr_len);
}

ssize_t csocket_recvA(csocket_activity_t *activity, void *buf, size_t len, int flags) {
	if(!activity) return -1;
	if(activity->client_socket.ka && activity->client_socket.ka->enabled) {
		return _readBufferA(activity, buf, len, flags);
	}
	return recv(activity->client_socket.fd, buf, len, flags);
}

ssize_t csocket_recvfromA(csocket_activity_t *activity, void *buf, size_t len, int flags) {
	if(!activity) return -1;
	if(activity->client_socket.ka && activity->client_socket.ka->enabled) {
		return _readFromBufferA(activity, buf, len, flags);
	}
	return recvfrom(activity->client_socket.fd, buf, len, flags, activity->client_socket.addr, &activity->client_socket.addr_len);
}

ssize_t csocket_sendA(csocket_activity_t *activity, void *buf, size_t len, int flags) {
	if(!activity) return -1;
	return send(activity->client_socket.fd, buf, len, flags);
}

ssize_t csocket_sendtoA(csocket_activity_t *activity, void *buf, size_t len, int flags) {
	if(!activity) return -1;
	return sendto(activity->client_socket.fd, buf, len, flags, activity->client_socket.addr, activity->client_socket.addr_len);
}

csocket_activity_t * csocket_sockToAct(csocket_t *src_socket) {
	if(!src_socket) return NULL;

	csocket_activity_t *activity = calloc(1, sizeof(csocket_activity_t));
	if(!activity) return NULL;

	activity->time = time(NULL);
	activity->update_time = activity->time;
	activity->client_socket = (const struct csocket_clients){0};
	activity->client_socket.fd = src_socket->mode.fd;
	activity->client_socket.domain = src_socket->domain;
	activity->client_socket.addr = src_socket->mode.addr;
	activity->client_socket.addr_len = src_socket->mode.addr_len;
	activity->client_socket.ka = src_socket->ka;
	csocket_updateA(activity);

	return activity; 
}

csocket_activity_t * csocket_sockToActA(csocket_t *src_socket, csocket_addr_t *dst_addr) {
	if(!src_socket||!dst_addr) return NULL;

	csocket_activity_t *activity = calloc(1, sizeof(csocket_activity_t));
	if(!activity) return NULL;

	activity->time = time(NULL);
	activity->update_time = activity->time;
	activity->client_socket = (const struct csocket_clients){0};
	activity->client_socket.fd = src_socket->mode.fd;
	activity->client_socket.domain = src_socket->domain;
	activity->client_socket.addr = dst_addr->addr;
	activity->client_socket.addr_len = dst_addr->addr_len;
	activity->client_socket.ka = src_socket->ka;
	csocket_updateA(activity);

	return activity; 
}

#pragma endregion

#pragma region KEEPALIVE

int csocket_keepalive_create(int timeout, char *msg, size_t msg_len, csocket_keepalive_t *ka, csocket_t *src_socket) {
	if(!src_socket || !ka) return -1;

	if(timeout<1) ka->timeout = CSKA_TIMEOUT;
	else ka->timeout = timeout;
	
	#ifdef _WIN32
		int
	#else 
		socklen_t
	#endif
		rcvfb;
	socklen_t len = sizeof(rcvfb);
	if(getsockopt(src_socket->mode.fd, SOL_SOCKET, SO_RCVBUF, 
	#ifdef _WIN32 
		(char*)
	#endif
	&rcvfb, &len)) {
		strcpy(src_socket->last_err, "kacreate getsockopt");
		return -1;
	}
	ka->buffer = malloc(rcvfb+1);
	ka->buffer_len = rcvfb;
	if(!ka->buffer) {
		strcpy(src_socket->last_err, "kacreate malloc");
		return -1;
	}
	ka->params = malloc(rcvfb+1);
	ka->params_len = rcvfb;
	if(!ka->params) {
		strcpy(src_socket->last_err, "kacreate malloc2");
		return -1;
	}
	
	if(msg_len>0) {
		ka->msg_len = msg_len;
		ka->msg_type = 1;
	}
	else {
		ka->msg_len = CSKA_DEFAULTMSGLEN;
		ka->msg_type = 2;
	}
	ka->msg = malloc(ka->msg_len+1);
	if(!ka->msg) {
		strcpy(src_socket->last_err, "kacreate malloc3");
		return -1;
	}
	memcpy(ka->msg, msg_len>0?msg:CSKA_DEFAULTMSG, ka->msg_len);

	ka->enabled = 1;
	
	return 0;
}

int csocket_keepalive_modify(int timeout, char *msg, size_t msg_len, csocket_keepalive_t *ka, csocket_t *src_socket) {
	if(!src_socket || !ka) return -1;

	if(timeout<1) ka->timeout = CSKA_TIMEOUT;
	else ka->timeout = timeout;

	if(msg_len>0) {
		ka->msg_len = msg_len;
		ka->msg_type = 1;
	}
	else {
		ka->msg_len = CSKA_DEFAULTMSGLEN;
		ka->msg_type = 2;
	}
	if(ka->msg) free(ka->msg);
	ka->msg = malloc(ka->msg_len+1);
	if(!ka->msg) {
		strcpy(src_socket->last_err, "kamodify malloc2");
		return -1;
	}
	memcpy(ka->msg, msg_len>0?msg:CSKA_DEFAULTMSG, ka->msg_len);

	ka->enabled = 1;

	return 0;
}

int csocket_keepalive_set(csocket_keepalive_t *ka, csocket_t *src_socket) {
	if(!src_socket || !ka) return -1;
	src_socket->ka = ka;
	return 0;
}

int csocket_keepalive_unset(csocket_t *src_socket) {
	if(!src_socket || !src_socket->ka) return -1;
	*src_socket->ka = (const csocket_keepalive_t)CSOCKET_EMPTY;
	return 0;
}

int csocket_keepalive_copy(csocket_keepalive_t **dst, const csocket_keepalive_t *src) {
	if(!dst||!src) return 0;

	csocket_freeKeepalive(*dst);

	if(!*dst) {
		*dst = calloc(1, sizeof(csocket_keepalive_t));
		if(!*dst) return -1;
	}

	(*dst)->enabled = src->enabled;
	(*dst)->timeout = src->timeout;
	(*dst)->buffer = malloc(src->buffer_len);
	(*dst)->buffer_len = src->buffer_len;
	if(!(*dst)->buffer)
		return -1;
	(*dst)->params = malloc(src->params_len);
	(*dst)->params_len = src->params_len;
	if(!(*dst)->params)
		return -1;
	(*dst)->msg_len = src->msg_len;
	if((*dst)->msg_len>0) {
		(*dst)->msg = malloc((*dst)->msg_len);
		if(!(*dst)->msg)
			return -1;
		memcpy((*dst)->msg, src->msg, (*dst)->msg_len);
	}
	(*dst)->msg_type = src->msg_type;
	(*dst)->last_sig = time(NULL);
	(*dst)->onActivity = src->onActivity;

	(*dst)->address.domain = src->address.domain;
	(*dst)->address.addr = src->address.addr;
	(*dst)->address.addr_len = src->address.addr_len;


	return 0;
}

int csocket_isAlive(csocket_keepalive_t *ka) {
	if(!ka || !ka->enabled) return -1;
	if(ka->timeout == 0) return 1;

	return (ka->last_sig>(time(NULL)-ka->timeout));
}

int csocket_keepAlive(csocket_t *src_socket) {
	if(!src_socket || src_socket->mode.sc!=2 || !src_socket->ka) return -1;
	
	if(!src_socket->ka->enabled) {
		strcpy(src_socket->last_err, "keepAlive unavailable: not enabled");
		return -1;
	}
	if(src_socket->ka->last_sig>time(NULL)-src_socket->ka->timeout+src_socket->ka->timeout/4) {
		strcpy(src_socket->last_err, "keepAlive unavailable: timeout");
		return 0;
	}	

	size_t toSendSize = src_socket->ka->msg_len;
	char *toSend = malloc(toSendSize);
	if(!toSend) {
		strcpy(src_socket->last_err, "keepAlive malloc");
		return -1;
	}
	memcpy(toSend, src_socket->ka->msg, toSendSize);

	char *dst = NULL;
	size_t size = 0, offset = 0;
	for(size_t i=0; i<toSendSize; ++i) {
		if((dst = csocket_resolveKeepAliveMsg(toSend+i, &size, &offset))) {
			toSendSize += size;
			char *n = realloc(toSend, toSendSize);
			if(!n) {
				strcpy(src_socket->last_err, "keepAlive realloc");
				return -1;
			}
			toSend = n;
			memmove(toSend+i+offset+size, toSend+i+offset, toSendSize-(i+offset+size));
			memcpy(toSend+i+offset, dst, size);
			i+=size+offset-1;
			free(dst);
		}
	}

	src_socket->ka->last_sig = time(NULL);
	
	ssize_t r = csocket_send(src_socket, toSend, toSendSize, 0);
	if(r<=0 || (size_t)r!=toSendSize) {
		strcpy(src_socket->last_err, "keepAlive send");
		return -1;
	}

	return 0;
}

char * csocket_resolveKeepAliveMsg(char *src, size_t *size, size_t *offset) {
	if(!src) return NULL;

	*size = 0;
	*offset = 0;
	char *dst = NULL;

	if(strncmp(src, "%UNIX%", 6)==0) {
		char aunix[32];
		memset(aunix, 0, sizeof aunix);
		if((size_t)snprintf(aunix, sizeof aunix, "%ld", (unsigned long)time(NULL))>=(sizeof aunix))
			return NULL;
		*offset = 6;
		*size = strlen(aunix)+*offset;
		dst = malloc(*size);
		if(!dst) return NULL;
		memcpy(dst, aunix, *size-*offset);
		memcpy(dst+*size-*offset, "%UNIX%", *offset);
	}
	else if(strncmp(src, "%HOST%", 6)==0) {
		char host[64];
		memset(host, 0, sizeof host);
		if(gethostname(host, sizeof host))
			strcpy(host, "csckUnknown");
		*offset = 6;
		*size = strlen(host)+*offset;
		dst = malloc(*size);
		if(!dst) return NULL;
		memcpy(dst, host, *size-*offset);
		memcpy(dst+*size-*offset, "%HOST%", *offset);
	}
	else if(strncmp(src, "%USER%", 6)==0) {
		char user[1024];
		size_t user_len = sizeof user;
		memset(user, 0, user_len);
		#ifdef _WIN32
		if(!GetUserName(user, (long unsigned int*)&user_len)) {
		#else
		if(getlogin_r(user, user_len)) {
		#endif
			strcpy(user, "csckUnknown");
		}
		*offset = 6;
		*size = strlen(user)+*offset;
		dst = malloc(*size);
		if(!dst) return NULL;
		memcpy(dst, user, *size-*offset);
		memcpy(dst+*size-*offset, "%USER%", *offset);
	}

	return dst;
}

int csocket_getKeepAliveVariable(char *dst, size_t *dst_len, char *query, csocket_keepalive_t *ka) {
	if(!dst || !query || !ka || !ka->enabled || !ka->params || !ka->params_usage) return -1;

	size_t qoff = 0;
	socklen_t var_len = 0;

	for(socklen_t params_offset = 0; params_offset < ka->params_usage; ++params_offset) {
		if(_searchKeyKeepAlive(query, strlen(query), query, &qoff, ka->params, &params_offset, ka->params_usage, &var_len)==0) {
			
			socklen_t recs = (socklen_t) *dst_len;
			*dst_len = var_len-strlen(query);
			if((size_t)recs>*dst_len)
				recs = *dst_len;

			memcpy(dst, ka->params+params_offset-var_len+1, recs);	

			return 0;
		}
	}
	
	return -1;
}

int csocket_updateKeepAlive(csocket_keepalive_t *ka, int fd) {
	if(!ka) return -1;
	if(!ka->enabled) return 0;

	if(_updateBuffer(ka, fd, MSG_PEEK|MSG_DONTWAIT)<0) {
		return -1;
	}
	
	return 0;
}

int csocket_updateKeepAliveFrom(csocket_keepalive_t *ka, int fd, csocket_addr_t *dst_addr) {
	if(!ka||!dst_addr) return -1;
	if(!ka->enabled) return 0;

	if(_updateFromBuffer(ka, fd, MSG_PEEK|MSG_DONTWAIT, dst_addr->addr, &dst_addr->addr_len)<0)
		return -1;
	
	return 0;
}

void csocket_printKeepAlive(FILE *fp, csocket_keepalive_t *ka) {
	if(!fp) return;

	char addrs[40];
	CSOCKET_NTOP(ka->address.domain, ka->address.addr, addrs, 40);

	fprintf(fp, "[%.24s] >> %s", ctime(&ka->last_sig), addrs);
	fprintf(fp, "\tTimeout: %d[s]\n\tType: KEEPALIVE\n\tHandlerEnabled: %d\n", ka->timeout, ka->onActivity!=0);
	fprintf(fp, "\t\n");
}

#pragma endregion

#pragma region SERVER

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

/*
	ONE CLIENT AT A TIME
*/

int csocket_accept(csocket_t *src_socket, csocket_activity_t *activity) {
	if(!src_socket || !activity || src_socket->mode.sc!=1) return -1;

	// free activity, just in case
	csocket_freeActivity(activity);

	// keepalive
	if(csocket_keepalive_copy(&activity->client_socket.ka, src_socket->ka)) return -1;

	struct csocket_server server = *((struct csocket_server*)&src_socket->mode);


	// update size
	if(src_socket->domain == AF_INET)
		activity->client_socket.addr_len = sizeof(struct sockaddr_in);
	else
		activity->client_socket.addr_len = sizeof(struct sockaddr_in6);

	if(activity->client_socket.addr)
		free(activity->client_socket.addr);
	activity->client_socket.addr = calloc(1, activity->client_socket.addr_len);

	if(!activity->client_socket.addr) {
		strcpy(src_socket->last_err, "calloc");
		return -1;
	}

	// accept
	if((server.client_fd = accept(server.server_fd, activity->client_socket.addr, &activity->client_socket.addr_len)) < 0 ) {
		strcpy(src_socket->last_err, "accept");
		return -1;
	}

	if(activity->client_socket.addr_len == sizeof(struct sockaddr_in))
		activity->client_socket.domain = AF_INET;
	else if(activity->client_socket.addr_len == sizeof(struct sockaddr_in6))
		activity->client_socket.domain = AF_INET6;
	else
		activity->client_socket.domain = -1;
	activity->type = CSACT_TYPE_CONN;
	activity->client_socket.fd = server.client_fd;

	if(activity->client_socket.ka) {
		activity->client_socket.ka->address.domain = activity->client_socket.domain;
		activity->client_socket.ka->address.addr = activity->client_socket.addr;
		activity->client_socket.ka->address.addr_len = activity->client_socket.addr_len;
	}

	// set time
	activity->time = time(NULL);
	activity->update_time = activity->time;

	// poll action
	fd_set wr, rd, ex;
	FD_ZERO(&wr);
	FD_ZERO(&rd);
	FD_ZERO(&ex);
	FD_SET(activity->client_socket.fd, &wr);
	FD_SET(activity->client_socket.fd, &rd);
	FD_SET(activity->client_socket.fd, &ex);

	struct timeval TIMEVAL_ZERO = {0};
	int ret = select(activity->client_socket.fd+1, &rd, &wr, &ex, &TIMEVAL_ZERO);
	if(ret < 0) {
		strcpy(src_socket->last_err, "accept");
		return -1;
	}
	if(FD_ISSET(activity->client_socket.fd, &wr))
		activity->type |= CSACT_TYPE_WRITE;
	if(FD_ISSET(activity->client_socket.fd, &rd))
		activity->type |= CSACT_TYPE_READ;
	if(FD_ISSET(activity->client_socket.fd, &ex))
		activity->type |= CSACT_TYPE_EXT;

	return 0;
}

/*
	MULTI SERVER
*/
// handling all clients - should be called in a while(true)
int csocket_setUpMultiServer(csocket_t *src_socket, int maxClient, void (*onActivity)(csocket_multiHandler_t *, csocket_activity_t), csocket_multiHandler_t *handler) {
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
	if(!handler || handler->src_socket->mode.sc!=1) return -1;

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
		struct csocket_clients client = CSOCKET_EMPTY;
		if(csocket_keepalive_copy(&client.ka, handler->src_socket->ka)) return -1;
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

		client.ka->address.domain = client.domain;
		client.ka->address.addr = client.addr;
		client.ka->address.addr_len = client.addr_len;
	
		
		// set activity
		csocket_activity_t activity = CSOCKET_EMPTY;
		activity.client_socket = client;
		activity.type = CSACT_TYPE_CONN | CSACT_TYPE_DECLINED;
		// set time
		activity.time = time(NULL);
		activity.update_time = activity.time;

		// poll action
		fd_set wr2, rd2, ex2;
		FD_ZERO(&wr2);
		FD_ZERO(&rd2);
		FD_ZERO(&ex2);
		FD_SET(activity.client_socket.fd, &wr2);
		FD_SET(activity.client_socket.fd, &rd2);
		FD_SET(activity.client_socket.fd, &ex2);
		struct timeval TIMEVAL_ZERO = {0};
		int ret = select(activity.client_socket.fd+1, &rd2, &wr2, &ex2, &TIMEVAL_ZERO);
		if(ret < 0) {
			strcpy(handler->src_socket->last_err, "multiServer accept");
			return -1;
		}
		if(FD_ISSET(activity.client_socket.fd, &wr2))
			activity.type |= CSACT_TYPE_WRITE;
		if(FD_ISSET(activity.client_socket.fd, &rd2))
			activity.type |= CSACT_TYPE_READ;
		if((!csocket_hasRecvDataA(&activity) && !csocket_hasRecvFromDataA(&activity)))
			activity.type &= ~CSACT_TYPE_READ;
		if(FD_ISSET(activity.client_socket.fd, &ex2))
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
			handler->onActivity(handler, activity);
	}

	// action on any socket
	for(int i=0; i<handler->maxClients; ++i) {
		// client
		struct csocket_clients client = handler->client_sockets[i]; 

		int fdset = FD_ISSET(client.fd, &rd);

		csocket_updateKeepAlive(client.ka, client.fd);
		csocket_addr_t kaadr;
		kaadr.domain = client.domain;
		kaadr.addr = client.addr;
		kaadr.addr_len = client.addr_len;
		csocket_updateKeepAliveFrom(client.ka, client.fd, &kaadr);

		fdset |= _hasRecvDataBuffer(client.ka, client.fd)==1||_hasRecvFromDataBuffer(client.ka, client.fd, client.addr, &client.addr_len)==1;

		if(fdset||!csocket_isAlive(client.ka)||client.shutdown) {

			/**
			 * 
			 * FORM ACTIVITY
			 * 
			**/
			csocket_activity_t activity = CSOCKET_EMPTY;
			activity.client_socket.fd = client.fd;
			activity.client_socket.domain = client.domain;
			activity.client_socket.addr = client.addr;
			activity.client_socket.addr_len = client.addr_len;

			// keepalive
			activity.client_socket.ka = client.ka;

			// set time
			activity.time = time(NULL);
			activity.update_time = activity.time;
				
			// disconnect
			// DGRAMs allow 0 width data -> recv = 0
			// options: 1) manual shutdown by user 2) timeout

			// no data: disconnected
			if(csocket_isAlive(activity.client_socket.ka)==0 || client.shutdown) {

				activity.type = CSACT_TYPE_DISCONN;
				
				shutdown(client.fd, SHUT_RDWR);
				
				// trigger action
				if(handler->onActivity) {
					handler->onActivity(handler, activity);
					handler->client_sockets[i] = (const struct csocket_clients)CSOCKET_EMPTY;
					client.ka = handler->src_socket->ka;
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

				// poll action
				fd_set wr2, ex2;
				FD_ZERO(&wr2);
				FD_ZERO(&ex2);
				FD_SET(activity.client_socket.fd, &wr2);
				FD_SET(activity.client_socket.fd, &ex2);
				struct timeval TIMEVAL_ZERO = {0};
				int ret = select(activity.client_socket.fd+1, NULL, &wr2, &ex2, &TIMEVAL_ZERO);
				if(ret < 0) {
					strcpy(handler->src_socket->last_err, "multiServer socketchange");
					return -1;
				}
				if(FD_ISSET(activity.client_socket.fd, &wr2))
					activity.type |= CSACT_TYPE_WRITE;
				if(fdset)
					activity.type |= CSACT_TYPE_READ;
				if(FD_ISSET(activity.client_socket.fd, &ex2))
					activity.type |= CSACT_TYPE_EXT;

				// trigger action on data
				if((csocket_hasRecvDataA(&activity)==1 || csocket_hasRecvFromDataA(&activity)==1) && handler->onActivity)
					handler->onActivity(handler, activity);
			}
		}
	}

	return 0;
}

int csocket_shutdownClient(csocket_multiHandler_t *handler, struct csocket_clients *client) {
	if(!handler) return -1;

	int n = 0;

	for(int i=0; i<handler->maxClients; ++i) {
		if(handler->client_sockets[i].fd>0 && (!client || handler->client_sockets[i].fd==client->fd)) {
			handler->client_sockets[i].shutdown = 1;
			if(client) client->shutdown = 1;
			n++;
		}
	}
	
	return n;
}

#pragma endregion

#pragma region CLIENT

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

#pragma endregion

#pragma region UTIL

const char * csocket_ntop(int domain, const void *addr, char *dst, socklen_t len) {
	return inet_ntop(domain, domain==AF_INET?(void*)&(((struct sockaddr_in*)addr)->sin_addr):(void*)&(((struct sockaddr_in6*)addr)->sin6_addr), dst, len);
}

#pragma endregion

#pragma region FREE

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

	csocket_freeKeepalive(src_socket->ka);
	
	// reset
	*src_socket = (const csocket_t)CSOCKET_EMPTY;
}

void csocket_freeActivity(csocket_activity_t *activity) {
	if(!activity) return;

	// free
	if(activity->client_socket.addr) {
		free(activity->client_socket.addr);
		activity->client_socket.addr = NULL;
	}

	// close fd
	if(activity->client_socket.fd>=0)
		shutdown(activity->client_socket.fd, SHUT_RDWR);
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

	csocket_freeKeepalive(client->ka);

	// reset
	*client = (const struct csocket_clients)CSOCKET_EMPTY;
}

void csocket_freeKeepalive(csocket_keepalive_t *ka) {
	if(!ka) return;

	// free
	if(ka->buffer) {
		free(ka->buffer);
		ka->buffer = NULL;
	}
	// free
	if(ka->msg) {
		free(ka->msg);
		ka->msg = NULL;
	}
	// free
	if(ka->params) {
		free(ka->params);
		ka->params = NULL;
	}


	*ka = (const csocket_keepalive_t)CSOCKET_EMPTY;
}

#pragma endregion


// close connection
void csocket_close(csocket_t *src_socket) {
	if(!src_socket) return;

	// free (close remaining fds)
	csocket_free(src_socket);
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
