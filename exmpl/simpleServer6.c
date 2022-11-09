/**
 * @file simpleServer6.c
 * @author Felix Kröhnert (felix.kroehnert@online.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */


#include <stdio.h>
#include "csocket.h"

int main(void) {

	csocket_t socket = CSOCKET_EMPTY;
	printf("init: %d\n", csocket_initServerSocket(AF_INET6, SOCK_STREAM, 0, &in6addr_any, 4200, &socket, 1));
	char str[100];
	CSOCKET_NTOP(socket.domain, socket.mode.addr, str, 100);

	printf("binding: %d\n", csocket_bindServer(&socket));
	printf("listening: %d\n", csocket_listen(&socket, 3));

	csocket_activity_t activity = CSOCKET_EMPTY;
	char buf[12];

	printf("accepting: %d\n", csocket_accept(&socket, &activity));
	CSOCKET_NTOP(activity.domain, activity.addr, str, sizeof(str));
	printf("ACTIVITY_IP: %s\n", str);
	printf("ACTIVITY_LEN: %d\n", activity.addr_len);
	printf("ACTIVITY_TYPE: %d\n", activity.type);	
	csocket_recvA(&activity, buf, 12, 0);
	printf("recv: %s\n", buf);

	csocket_close(&socket);
	csocket_freeActivity(&activity);
	printf("closed\n");

	return 0;
}