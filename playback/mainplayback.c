#include <stdlib.h>
#include <stdio.h>
#include <jansson.h>
#include <string.h>
#include <leone_tools.h>
#include "playback.h"
#include <hurl_core.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>


int main(int argc, char *argv[]) {
	pthread_t thread;


	/* Start DNS server in separate thread */
	pthread_create(&thread, NULL, dns_server, NULL);

	/* Start the content handler sever in separate thread*/
	printf("DNS Server started...!\n");
	content_server();
	//pthread_create(&content_thread, NULL, content_server, NULL);
	/* Start HTTP server */
	//http_server(80);

	/* TODO: Add SSL wrapper for HTTP server.
	 *       Use self-signed certificate and disable certificate verification in browser when testing.
	 */

	exit(EXIT_SUCCESS);
}



