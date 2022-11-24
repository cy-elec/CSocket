/**
 * @file simpleServer.c
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
	csocket_activity_t activity = CSOCKET_EMPTY;
	char str[100];
	char buf[100];
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


	rval = csocket_accept(&socket, &activity);
	printf("accepting: %d\n", rval);
	if(rval) return 1;

	CSOCKET_NTOP(activity.client_socket.domain, activity.client_socket.addr, str, sizeof(str));

	printf("ACTIVITY_IP: %s\n", str);
	printf("ACTIVITY_LEN: %d\n", activity.client_socket.addr_len);
	printf("ACTIVITY_TYPE: %d\n", activity.type);

	rval = csocket_recvA(&activity, buf, 100, 0);
	if(rval<1) {
		printf("Failed to receive [%d]\n", rval);
		return 1;
	}
	printf("recv[%d]: %s\n", rval, buf);
	
	#ifdef _WIN32
	Sleep(2000);
	#else
	sleep(2);
	#endif
	
	int srval = csocket_sendA(&activity, buf, rval, 0);
	if(srval!=rval) {
		printf("Failed to send [%d]\n", srval);
		return 1;
	}
	printf("sent[%d]: %s\n", rval, buf);

	#ifdef _WIN32
	Sleep(2000);
	#else
	sleep(2);
	#endif

	printf("updating keepalive: %d\n", csocket_updateKeepAlive(activity.client_socket.ka, activity.client_socket.fd));
	char resolvedparam[100];
	size_t size = sizeof resolvedparam - 1;
	csocket_getKeepAliveVariable(resolvedparam, &size, "%UNIX%", activity.client_socket.ka);
	resolvedparam[size] = 0;
	printf("Resolved Unix[%ld]: %s\n", size, resolvedparam);

	size = sizeof resolvedparam - 1;
	csocket_getKeepAliveVariable(resolvedparam, &size, "%HOST%", activity.client_socket.ka);
	resolvedparam[size] = 0;
	printf("Resolved Host[%ld]: %s\n", size, resolvedparam);
	
	size = sizeof resolvedparam - 1;
	csocket_getKeepAliveVariable(resolvedparam, &size, "%USER%", activity.client_socket.ka);
	resolvedparam[size] = 0;
	printf("Resolved User[%ld]: %s\n", size, resolvedparam);

	csocket_close(&socket);
	csocket_freeActivity(&activity);
	printf("closed\n");

	return 0;
}