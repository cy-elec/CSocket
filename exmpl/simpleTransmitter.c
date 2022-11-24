/**
 * @file simpleTransmitter.c
 * @author Felix Kröhnert (felix.kroehnert@online.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-13
 * 
 * @copyright Copyright (c) 2022
 * 
**/


#include <stdio.h>
#include "csocket.h"

int main(void) {

	csocket_t socket = CSOCKET_EMPTY;
	csocket_activity_t activity = CSOCKET_EMPTY;
	char str[100];
	char buf[] = "HELLOW SERVER";
	int rval;

	rval = csocket_initServerSocket(AF_INET, SOCK_STREAM, 0, (void*)&inaddr_any, 4200, &socket, 1);
	printf("init: %d\n", rval);
	if(rval) return 1;
	CSOCKET_NTOP(socket.domain, socket.mode.addr, str, 100);

	rval = csocket_bindServer(&socket);
	printf("binding: %d\n", rval);
	if(rval) return 1;
	rval = csocket_listen(&socket, 3);
	printf("listening: %d\n", rval);
	if(rval) return 1;


	rval = csocket_accept(&socket, &activity);
	printf("accepting: %d\n", rval);
	if(rval) return 1;

	CSOCKET_NTOP(activity.client_socket.domain, activity.client_socket.addr, str, sizeof(str));

	printf("ACTIVITY_IP: %s\n", str);
	printf("ACTIVITY_LEN: %d\n", activity.client_socket.addr_len);
	printf("ACTIVITY_TYPE: %d\n", activity.type);	

	rval = csocket_sendA(&activity, buf, strlen(buf), 0);
	if(rval!=strlen(buf)) {
		printf("Failed to transmit [%d]\n", rval);
		return 1;
	}
	printf("transmitted successfully: %s\n", buf);

	csocket_close(&socket);
	csocket_freeActivity(&activity);
	printf("closed\n");

	return 0;
}