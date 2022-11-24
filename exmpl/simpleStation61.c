/**
 * @file simpleStation61.c
 * @author Felix Kröhnert (felix.kroehnert@online.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-23
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

	rval = csocket_initClientSocket(AF_INET6, SOCK_DGRAM, 0, "::1", 4200, &socket, 0);
	printf("init: %d\n", rval);
	if(rval) return 1;

	CSOCKET_NTOP(socket.domain, socket.mode.addr, str, 100);

	char buf[] = "halo CSKA%UNIX%1668967786%UNIX%-%HOST%FELIX%HOST%-%USER%fexkr%USER%\0Alpha";
	rval = csocket_sendto(&socket, buf, sizeof(buf), 0);
	if(sizeof(buf)!=rval) {
		printf("Failed to send data [%d]\n", rval);
		return 1;
	}
	printf("sent[%d]: \"%s\"\n", rval, buf);

	csocket_close(&socket);
	printf("closed\n");

	return 0;
}