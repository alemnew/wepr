#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "hurl_core.h"
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <math.h>
#include <sys/resource.h>
#include "playback.h"
#include <fcntl.h>
#include <stdarg.h>

#define THREADED_SERVER
#define min(a,b) (a < b ? a : b)

void *http_handler(void *client_sock);
void send_404(int sock);

int http_server(int port) {
	int sock, *client_sock;
	struct sockaddr_in addr, *client_addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int reuse_addr = 1;
	struct rlimit max_fds;
	pthread_t thread;

	pthread_attr_t attr;
	pthread_attr_t *attrp; /* NULL or &attr */
	int s;

	/* Create socket for IPv4 only */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		hurl_debug(__func__, "Failed to create socket:  %s", strerror(errno));
		/**
		 * if error =  too many open files, then exit(1)
		 */
		if (errno == 24) {
			exit(0);
		}
		//return;
	}

	/* Enable reuse of port number */
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));

	/* Change max number of file descriptors */
	getrlimit(RLIMIT_NOFILE, &max_fds);
	if (max_fds.rlim_cur < max_fds.rlim_max) {
		/* Change limit */
		max_fds.rlim_cur = max_fds.rlim_max;
		hurl_debug(__func__, "Changing max FDs to %d", max_fds.rlim_max);
		setrlimit(RLIMIT_NOFILE, &max_fds);
	}

	/* Bind socket to localhost:53 */
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t ) port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(sock, (struct sockaddr *) &addr, addr_len) == -1) {
		hurl_debug(__func__, "Failed to bind socket: %s", strerror(errno));
		return 0;
	}

	/* Put socket into listening mode */
	if (listen(sock, 1024) == -1) {
		hurl_debug(__func__, "Failed to listen on socket.");
		return 0;
	}

	/**
	 * there is no way to exit from this loop even if the client disconneted...
	 *time_t endTime = time(NULL) + 30;
	 while (time(NULL) < endTime) { // loop only for 30 second
	 */
	while (1) { // this make sure that all requests from the browser are served.
		client_addr = malloc(sizeof(struct sockaddr_in));
		client_sock = malloc(sizeof(int));
		if ((*client_sock = accept(sock, (struct sockaddr *) client_addr,
				&addr_len)) == -1) {
			hurl_debug(__func__, "... Failed to accept client: %s.",
					strerror(errno));
			/**
			 * if error =  too many open files, then exit(1)
			 */
			if (errno == 24) {
				exit(0);
			}
			//continue;
		}

		hurl_debug(__func__, "Client connected.");

		//http_handler(client_sock); // to be removed...the function argument change from void to int
		/* Handle HTTP request in separate thread. */
		attrp = &attr;
		s = pthread_attr_init(&attr);
		if (s != 0) {
			hurl_debug(__func__, "Error: pthread_attr_init");
		}
		s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if (s != 0) {
			hurl_debug(__func__, "Error: pthread_attr_setdetachstate");
		}

		s = pthread_create(&thread, attrp, http_handler, client_sock);
		if (s != 0) {
			printf("Error: pthread_create - http_handler %d", s);
			//hurl_debug(__func__,  "Error: pthread_create - content_handler");
		}

	}
	close(sock); // close the http_server socket and make it ready for next request
	return 0;
}

/* Convert milliseconds to timespec */
void ms_to_timespec(int ms, struct timespec *ts) {
	if (ms > 1000) {
		ts->tv_sec = roundf((float) ms / 1000.0f);

	}
	ts->tv_nsec = (ms - ts->tv_sec * 1000) * 10e5;
}

void *http_handler(void *client_sock) { //arg void
	char recvbuf[4096];
	int offset = 0; /* TODO: This was probably meant for keeping track of offsets in persistent connections. */
	int request_received = 0;
	int recv_len;
	int reuse_addr = 1;
	char *respbuf;
	int respbuf_offset, respbuf_len;
	//int timing_skew = 4;
	//int delay_ms = 50;
	int sock = *((int *) client_sock);

	struct timespec shutdown_delay;
	int i;
	char *request_domain, *request_path;
	char *eof_header, *header;
	int header_len;
	char *tok, *header_line;
	int line_count = 0;
	int value_len;
	int match_found = 0;
	PlaybackElement *pbe;
	unsigned int data_sent = 0;
	int send_len;
	struct response {
		char *header, *body;
		unsigned int header_len, body_len;
		char *content_type;
		float body_bwlimit;
	} response;
	struct timespec throttle_delay;
	float delay;
	int chunk_size = 128;
	char *header_split_ptr;

	/* TODO: Add support for persistent connections. */

	/* TODO: Depending on the systme it may be necessary to skew response delays such that the correct delay is perceived by clients.
	 * 		 Example: The system is too slow to process requests, so induced delays should be less because the system is already adding delay.
	 * 		 This has not been a problem so far.
	 */
	/* Enable reuse of port number */
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));
	/* Free socket number after copying it. */
	free(client_sock);

	throttle_delay.tv_sec = 0;
	throttle_delay.tv_nsec = 1000000;

	/* Set shutdown delay. */
	/* TODO: This might have to be adjusted. */
	shutdown_delay.tv_sec = 0;
	shutdown_delay.tv_nsec = 100000;

	/* Reset receive buffer */
	bzero(recvbuf, sizeof(recvbuf));
	/* Receive request */
	while (!request_received) {
		/* Check if request is biffer than buffer. */
		if (sizeof(recvbuf) - offset <= 0) {
			hurl_debug(__func__, "Request buffer overflow.");
			pthread_exit(NULL);
		}
		if ((recv_len = recv(sock, &recvbuf[offset], sizeof(recvbuf) - offset,
				0)) < 0) {
			/* Receive error */
			hurl_debug(__func__, "recv() failed: %s", strerror(errno));
			close(sock); // not sure
			pthread_exit(NULL);
		} else if (recv_len == 0) {
			/* Client closed the connection */
			hurl_debug(__func__, "recv(): client closed the connection.");
			//shutdown(sock, SHUT_RDWR);
			//close(sock);
			pthread_exit(NULL);
		}

		/* Has the entire request been received? */
		if ((eof_header = strstr(recvbuf, "\r\n\r\n")) != NULL) {
			hurl_debug(__func__, "Request received.");
			request_received = 1;
		}
	}

	/* Parse request */
	header_len = eof_header - recvbuf; // header_len is always 0

	header = malloc(sizeof(char) * header_len + 1);
	memcpy(header, recvbuf, header_len + 1);
	header[header_len] = '\0';
	tok = header;
	/* For each header line */
	while ((header_line = strtok_r(tok, "\r\n", &header_split_ptr)) != NULL) {
		line_count++;
		tok = NULL;
		hurl_debug(__func__, "Header line #%d: %s", line_count, header_line);

		/* Get requested path from first header line. */
		if (line_count == 1) {
			if (strncasecmp(header_line, "GET ", strlen("GET ")) == 0) {
				value_len = strlen(header_line) - strlen("GET ")
						- strlen(" HTTP/1.1");
				request_path = malloc(sizeof(char) * (value_len + 1));
				memcpy(request_path, header_line + strlen("GET "), value_len);
				request_path[value_len] = '\0';
				hurl_debug(__func__, "Request path: %s", request_path);
			} else {
				/* This is not a GET request */
				send_404(sock);
			}
		} else if (strncasecmp(header_line, "Host: ", strlen("Host: ")) == 0) {
			/* Get 'Host' header */
			value_len = strlen(header_line) - strlen("Host: ");
			request_domain = malloc(sizeof(char) * (value_len + 1));
			memcpy(request_domain, header_line + strlen("Host: "), value_len);
			request_domain[value_len] = '\0';
			hurl_debug(__func__, "Request domain: %s", request_domain);
		}
	}

	/* Did the request contain a 'Host' header? */
	if (!request_domain) {
		/* NO, and then we cannot identify the correct PBE. */
		send_404(sock);
	}
	//printf("1: The value of appl leng in http server: %u \n", app.elements_len);
	/* Find requested element */
	int nullcount = 0;
	for (i = 0; i < app.elements_len; i++) {
		pbe = &app.elements[i];
		if(app.elements[i].http.domain == NULL) {
			nullcount++;
			continue;
		}
	//printf("2: App elemn domain: %s, at iteration %d \n", app.elements[i].http.domain, i);
	//printf("22: Path : %s at iteration %d \n ", pbe->http.path, i);
	/* Does the domain and path match? */
		/* TODO: There might be some issues here if the length of the path tag is limited in the Webperf test. */
		if (strcmp(pbe->http.domain, request_domain) == 0
				&& strcmp(pbe->http.path, request_path) == 0) {
			/* Match found */
			//printf("3: PBE value: %s", pbe->http.domain);
			hurl_debug(__func__, "Matching element found.");
			match_found = 1;
			break;
		}
	}
	if(nullcount > 0){
		hurl_debug(__func__, "%d elements are found NULL\n", nullcount);
	}
	
	/* Was a matching PBE found? */
	if (!match_found) {
		/* NO, so send error message. */
		hurl_debug(__func__, "File not found: %s%s", request_domain,
				request_path);
	printf("PE have: %s, %s\n", pbe->http.domain, pbe->http.path);
		send_404(sock);
	}

	/* Check response code.
	 * If the code is zero, then the download failed with no response from the server. */
	if (pbe->http.response_code == 0) {
		/* Download failed, so kill connection */
		hurl_debug(__func__,
				"Download of element failed - closing connection failure.");
		//shutdown(sock, SHUT_RDWR);
		close(sock);
		pthread_exit(NULL);
	}

	/* Check if .body file was successfully loaded */
	if (!pbe->http.data) {
		/* Send error message. */
		send_404(sock);
	}

	response.body = pbe->http.data;
	response.body_len = pbe->http.data_len;
	response.content_type = pbe->http.content_type;

	/* Create response header */
	/* TODO: Add additional headers from Webperf results file to response header.
	 * 		 The header could also simply be filled with junk data. */
	response.header = malloc(sizeof(char) * 1024 + 1); //does this result malloc problem ???
	response.header_len =
			snprintf(response.header, 1024,
					"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nConnection: close\r\nContent-Length: %u\r\n\r\n",
					response.content_type, response.body_len);

	/* Calculate bandwidth throttling */
	/* Add encoding overhead to bandwidth calculation. */
	response.body_bwlimit = (float) (response.body_len + response.header_len)
			/ ((float) pbe->http.download_time / 1000.0f);
	hurl_debug(__func__, "Bandwidth limit is %f bps", response.body_bwlimit);

	while (data_sent < response.header_len + response.body_len) {
		/* Has the header been sent? */
		if (data_sent < response.header_len) {
			/* Send response header */
			respbuf = response.header;
			respbuf_offset = data_sent;
			respbuf_len = response.header_len - data_sent;

			/* TODO: Delay header according to 'time to first header byte' in webperf result. */
		} else {
			/* Send response body */
			respbuf = response.body;
			respbuf_offset = data_sent - response.header_len;
			/* respbuf_len = data_sent - response.header_len - response.body_len; */
			respbuf_len = min(chunk_size,
					response.header_len + response.body_len - data_sent);
			/*hurl_debug(__func__, "Send size is %d", respbuf_len);*/
		}

		/* Calculate appropriate delay for the number of bytes being transmitted. */
		delay = (float) respbuf_len / response.body_bwlimit;
		throttle_delay.tv_sec = floor(delay);
		throttle_delay.tv_nsec = (delay - throttle_delay.tv_sec) * 10e8;
		/* hurl_debug(__func__, "Throttle delay is s=%d, ns=%d", throttle_delay.tv_sec, throttle_delay.tv_nsec); */

		/* Wait until it is time to send next block of data */
		nanosleep(&throttle_delay, NULL);

		/* Send data */
		if ((send_len = send(sock, respbuf + respbuf_offset, respbuf_len,
				MSG_NOSIGNAL)) > 0) {
			data_sent += send_len;
		} else {
			/* Transmission error */
			hurl_debug(__func__, "Transmission error: %s", strerror(errno));
			break;
		}

	}

	/* Wait for client to close connection */
	nanosleep(&shutdown_delay, NULL);

	/* Close connection. */
	//shutdown(sock, SHUT_RDWR);
	//what if closing the client socket here??
	close(*((int *) client_sock)); //new one...
	close(sock);
	pthread_exit(NULL);

}

/* Send 404 Not found response and close connection. */
void send_404(int sock) {
	char *respbuf;
	int respbuf_len;
	struct timespec shutdown_delay;

	shutdown_delay.tv_nsec = 10 * 10e5;
	shutdown_delay.tv_sec = 0;

	/* Send error message. */
	respbuf =
			"HTTP/1.1 404 Not found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n\0";
	respbuf_len = strlen(respbuf);
	send(sock, respbuf, respbuf_len, MSG_NOSIGNAL);
	nanosleep(&shutdown_delay, NULL);
	shutdown(sock, SHUT_RDWR);
	pthread_exit(NULL);
}
