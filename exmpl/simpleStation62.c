/**
 * @file simpleStation62.c
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

	csocket_t listener = CSOCKET_EMPTY;
	csocket_addr_t dst_addr = CSOCKET_EMPTY;
	struct csocket_keepalive ka = CSOCKET_EMPTY;
	int rval;
	char str[100];
	struct timeval timeout = {
		.tv_sec = 0,
		.tv_usec = 500000,
	};

	rval = csocket_initServerSocket(AF_INET6, SOCK_DGRAM, 0, (void*)&in6addr_any, 4200, &listener, 1);
	printf("init Listener: %d\n", rval);
	if(rval) return 1;

	// enable default keep alive
	printf("Setting keepalive: %d %s\n", csocket_keepalive_create(0, NULL, 0, &ka, &listener), listener.last_err);
	csocket_keepalive_set(&ka, &listener);
	printf("Settings:\n\tEnabled: %d\n\tTimeout: %d\n\tMSG: %s\n\tType: %d\n\tTime: %ld\n", listener.ka->enabled, listener.ka->timeout, listener.ka->msg, listener.ka->msg_type, listener.ka->last_sig);

	rval = csocket_setAddressA(&dst_addr, AF_INET6, (void*)&in6addr_any, 4200, 1);
	printf("init Client: %d\n", rval);
	if(rval) return 1;

	CSOCKET_NTOP(listener.domain, listener.mode.addr, str, 100);

	rval = csocket_bindServer(&listener);
	printf("binding: %d\n", rval);
	if(rval) return 1;

	char buf[] = "halo CSKA%UNIX%1668967786%UNIX%-%HOST%FELIX%HOST%-%USER%fexkr%USER%\0Alpha";

	rval = csocket_recvfrom(&listener, &dst_addr, buf, sizeof(buf), 0);
	if(rval<1) {
		printf("Failed to receive [%d]\n", rval);
		return 1;
	}
	printf("recv[%d]: '%s'\n", rval, buf);

	csocket_close(&listener);
	printf("closed\n");

	return 0;
}