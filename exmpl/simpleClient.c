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
	struct csocket_keepalive ka = CSOCKET_EMPTY;
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

	// enable default keep alive
	printf("Setting keepalive: %d %s\n", csocket_keepalive_create(0, NULL, 0, &ka, &socket), socket.last_err);
	csocket_keepalive_set(&ka, &socket);
	printf("Settings:\n\tEnabled: %d\n\tTimeout: %d\n\tMSG: %s\n\tType: %d\n\tTime: %ld\n", socket.ka->enabled, socket.ka->timeout, socket.ka->msg, socket.ka->msg_type, socket.ka->last_sig);


	rval = csocket_connectClient(&socket, &timeout);
	printf("connect[%s--%d]: %d\n", str, ntohs(socket.domain==AF_INET?((struct sockaddr_in*)socket.mode.addr)->sin_port:((struct sockaddr_in6*)socket.mode.addr)->sin6_port), rval);
	if(rval) return 1;

	char buf[] = "halo CSKA%UNIX%1668967786%UNIX%-%HOST%FELIX%HOST%-%USER%fexkr%USER%\0Alpha";
	rval = csocket_send(&socket, buf, sizeof(buf), 0);
	if(sizeof(buf)!=rval) {
		printf("Failed to send data [%d]\n", rval);
		return 1;
	}
	printf("sent[%d]: \"%s\"\n", rval, buf);
	
	rval = csocket_recv(&socket, buf, sizeof(buf), 0);
	if(rval<1) {
		printf("Failed to receive [%d]\n", rval);
		return 1;
	}
	printf("recv[%d]: '%s'\n", rval, buf);

	if(csocket_keepAlive(&socket))
		printf("Failed to send KeepAlive Signal\n");

	csocket_close(&socket);
	printf("closed\n");

	return 0;
}