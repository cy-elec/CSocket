/**
 * @file simpleClient6.c
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
	printf("init: %d\n", csocket_initClientSocket(AF_INET6, SOCK_STREAM, 0, "::1", 4200, &socket, 0));

	char str[100];
	CSOCKET_NTOP(socket.domain, socket.mode.addr, str, 100);

	struct timeval timeout = {
		.tv_sec = 0,
		.tv_usec = 500000,
	};
	printf("connect[%s--%d]: %d\n", str, ntohs(socket.domain==AF_INET?((struct sockaddr_in*)socket.mode.addr)->sin_port:((struct sockaddr_in6*)socket.mode.addr)->sin6_port), 
	csocket_connectClient(&socket, &timeout));

	printf("sending: \"hello world\"\n");
	char buf[12] = "hello world";
	csocket_send(&socket, buf, 12, 0);

	csocket_close(&socket);
	printf("closed\n");

	return 0;
}