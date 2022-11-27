/**
 * @file multiServer.c
 * @author Felix Kröhnert (felix.kroehnert@online.de)
 * @brief 
 * @version 0.1
 * @date 2022-11-06
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
	if(act.type&CSACT_TYPE_READ) {
		printf("\tReceived: ");
		char buf, arr[1024];
		int rval = 0, srval = 0;
		while(csocket_hasRecvDataA(&act)==1 && rval<sizeof(arr)) {
			csocket_recvA(&act, &buf, 1, 0);
			arr[rval] = buf;
			rval++;
			printf("%c", buf);
		}
		arr[rval] = 0;
		char resolvedparam[11];
		size_t size = sizeof resolvedparam - 1;
		csocket_getKeepAliveVariable(resolvedparam, &size, "%HOST%", act.client_socket.ka);
		resolvedparam[10] = 0;
		printf("\t\n\tResolved[%ld]: %.*s\n", size, (int)size, resolvedparam);
		srval = csocket_sendA(&act, arr, rval, 0);
		if(srval!=rval)
			printf("\tFailed to send [%d]\n\n", srval);
		else
			printf("\tsent[%d]: '%s'\n\n", rval, arr);
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
	struct csocket_keepalive ka = CSOCKET_EMPTY;

	rval = csocket_initServerSocket(AF_INET, SOCK_STREAM, 0, (void*)&inaddr_any, 4200, &socket, 1);
	printf("init: %d\n", rval);
	if(rval) return 1;

	CSOCKET_NTOP(socket.domain, socket.mode.addr, str, 100);

	// enable default keep alive
	printf("Setting keepalive: %d %s\n", csocket_keepalive_create(0, NULL, 0, &ka, &socket), socket.last_err);
	csocket_keepalive_set(&ka, &socket);
	printf("Settings:\n\tEnabled: %d\n\tTimeout: %d\n\tMSG: %s\n\tType: %d\n\tTime: %ld\n", socket.ka->enabled, socket.ka->timeout, socket.ka->msg, socket.ka->msg_type, socket.ka->last_sig);


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
	csocket_freeMultiHandler(&handler);
	printf("closed\n");

	return 0;
}
