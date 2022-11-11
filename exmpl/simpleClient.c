/**
 * @file simpleClient.c
 * @author Felix Kröhnert (felix.kroehnert@online.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-04
 * 
 * @copyright Copyright (c) 2022
 * 
**/


#include <stdio.h>
#include "csocket.h"

int main(void) {

	csocket_t socket = CSOCKET_EMPTY;
	int rval;
	char str[100];
	struct timeval timeout = {
		.tv_sec = 0,
		.tv_usec = 500000,
	};

	rval = csocket_initClientSocket(AF_INET, SOCK_STREAM, 0, "127.0.0.1", 4200, &socket, 0);
	printf("init: %d\n", rval);
	if(rval) return 1;

	CSOCKET_NTOP(socket.domain, socket.mode.addr, str, 100);

	rval = csocket_connectClient(&socket, &timeout);
	printf("connect[%s--%d]: %d\n", str, ntohs(socket.domain==AF_INET?((struct sockaddr_in*)socket.mode.addr)->sin_port:((struct sockaddr_in6*)socket.mode.addr)->sin6_port), 
	rval);
	if(rval) return 1;

	char buf[12] = "hello world";
	printf("sending: \"%s\"\n", buf);
	rval = csocket_send(&socket, buf, 12, 0);
	if(12!=rval) {
		printf("Failed to send data [%d]\n", rval);
		return 1;
	}

	csocket_close(&socket);
	printf("closed\n");

	return 0;
}