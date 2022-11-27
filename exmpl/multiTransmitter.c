/**
 * @file multiTransmitter.c
 * @author Felix Kröhnert (felix.kroehnert@online.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-13
 * 
 * @copyright Copyright (c) 2022
 * 
**/

#include <signal.h>
#include <stdio.h>
#include "csocket.h"


void onActivity(csocket_multiHandler_t *handler, csocket_activity_t act) {
	csocket_printActivity(fileno(stdout), &act);
	// if data, print
	if(act.type&CSACT_TYPE_WRITE) {
		char buf[] = "HELLOW SERVER";
		int rval = csocket_sendA(&act, buf, strlen(buf)+1, 0);
		if(rval!=strlen(buf)+1) {
			printf("Failed to transmit [%d]\n", rval);
			return;
		}
		printf("transmitted successfully: %s\n", buf);
	}
}

int loop = 1;


void onkill(int signum) {
	loop = 0;
	printf("\r");
}

int main(void) {

	signal(SIGINT, onkill);

	csocket_t socket = CSOCKET_EMPTY;
	csocket_multiHandler_t handler = CSOCKET_EMPTY;
	char str[100];
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

	rval = csocket_setUpMultiServer(&socket, 64, &onActivity, &handler);
	printf("setting up multiServer: %d\n", rval);
	if(rval) return 1;

	while(loop) {
		if(csocket_multiServer(&handler)) {
			printf("multiServer failed: %s\n", socket.last_err);
			break;
		}
	}

	csocket_close(&socket);
	printf("closed\n");

	return 0;
}
