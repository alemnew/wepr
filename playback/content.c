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
#include <assert.h>
#include <sys/resource.h>
#include <leone_tools.h>
#include "playback.h"
#include <hurl_core.h>
#include <fcntl.h>
#include <jansson.h>
#include <sys/mman.h>
#include<time.h>

#define THREADED_SERVER
#define min(a,b) (a < b ? a : b)
#define PORT 2083
void *content_handler(void *client_sock);

int *content_server() { // arg -void *arg
	int sock, *client_sock;
	struct sockaddr_in addr, *client_addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int reuse_addr = 1;
	struct rlimit max_fds;
	pthread_t thread;

	pthread_attr_t attr;
	pthread_attr_t *attrp; /* NULL or &attr */
	int s;

	printf("Content Server started!\n");
	/* Create socket for IPv4 only */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		hurl_debug(__func__, "Failed to create socket.");
		return 0;
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

	/* Bind socket to localhost: */
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t) PORT);
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

	for (;;) {
		client_addr = malloc(sizeof(struct sockaddr_in));
		client_sock = malloc(sizeof(int));
		if ((*client_sock = accept(sock, (struct sockaddr *) client_addr,
				&addr_len)) == -1) {
			hurl_debug(__func__, "Failed to accept client: %s.",
					strerror(errno));
			/**
			 * if error =  too many open files, then exit(1)
			 */
			if (errno == 24) {
				exit(1);
			}
			continue;
		}

		hurl_debug(__func__, "Client connected...");
		/* Handle Content request in separate thread. */
		attrp = &attr;
		s = pthread_attr_init(&attr);
		if (s != 0) {
			hurl_debug(__func__, "Error: pthread_attr_init");
		}
		s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if (s != 0) {
			hurl_debug(__func__, "Error: pthread_attr_setdetachstate");
		}

		s = pthread_create(&thread, attrp, content_handler, client_sock);
		if (s != 0) {
			printf("Error: pthread_create - content_handler %d", s);
			//hurl_debug(__func__,  "Error: pthread_create - content_handler");
		}
	}
	return 0;
}

void *content_handler(void *client_sock) {
	char recvbuf[4096];
	int sock = *((int *) client_sock);
	int recv_len;
	int send_len = -2;
	int offset = 0;
	//char *eof_header;
	char *dir, *jsonfn;
//	int request_received = 0;
	//int header_len;
	char fin[] = "DONE";

	/* Free socket number after copying it. */
	free(client_sock);

	/* Reset receive buffer */
	bzero(recvbuf, sizeof(recvbuf));
	if ((recv_len = recv(sock, &recvbuf[offset], sizeof(recvbuf) - offset, 0))
			< 0) {
		hurl_debug(__func__, " Path Not Received.");
		return 0;
	} else if (recv_len == 0) {
		/* Client closed the connection */
		hurl_debug(__func__,
				"recv(): client closed the connection before sending body directory.");
		//pthread_exit(NULL);
	}

	//header_len = recv_len;
	dir = malloc(sizeof(char) * recv_len + 1);
	memcpy(dir, recvbuf, recv_len + 1);
	//path received and send okay
	if ((send_len = send(sock, fin, strlen(fin), 0)) < 0) {
		hurl_debug(__func__, " Waiting to receive JSON. %d", send_len);
	}
	send_len = -2;

	/* Reset receive buffer */
	bzero(recvbuf, sizeof(recvbuf));
	if ((recv_len = recv(sock, &recvbuf[offset], sizeof(recvbuf) - offset, 0))
			< 0) {
		hurl_debug(__func__, " JSON filename Not Received.");
		return 0;
	} else if (recv_len == 0) {
		// Client closed the connection
		hurl_debug(__func__,
				"recv(): client closed the connection before sending JSON path.");
		pthread_exit(NULL);
	}

	jsonfn = malloc(sizeof(char) * recv_len + 1);
	memcpy(jsonfn, recvbuf, recv_len + 1);

	if ((send_len = send(sock, fin, strlen(fin), 0)) < 0) {
		hurl_debug(__func__, " Confirmation sent. %d", send_len);
	}

	hurl_debug(__func__, "Path = %s", dir);
	hurl_debug(__func__, "Json file name = %s", jsonfn);

	if (!load_webperf(jsonfn, dir)) {
		hurl_debug(__func__, "Problem to load JSON file %s", jsonfn);
		exit(1);
	}
	//start http server
	http_server(80);
	/* Close connection. */
	//shutdown(sock, SHUT_RDWR);
	close(sock); //close the file discriptor the client connection
	pthread_exit(NULL);

}

/**
 * load JSON file to memory and  map downloaded content to memory
 */
int load_webperf(char *path, char *search_path) {
	json_t *root, *elements, *element, *obj, *dns, *http, *headers;
	json_error_t error;
	int nrof_elements;
	int i, j;
	const char *tag, *url;
	HURLParsedURL *parsed_url;
	PlaybackElement *pbe, *playback_search;
	char *data_path;
	int data_path_len;
	int fp;
	const char *header_key;
	json_t *header_value;

	// debugging
	/*
	time_t ltime;
	ltime = time(NULL);
	write_log(__func__, "\n %s", asctime(localtime(&ltime)));
	*/
	// debug end 

	/* Load JSON file */
	root = json_load_file(path, 0, &error);
	if (!root) {
		log_debug(__func__, "Failed to load JSON from '%s'", path);
		log_debug(__func__, "%s", error.text);
		log_debug(__func__, "Line: %d, Column: %d", error.line, error.column);
		if (errno == 2 || errno == 24) {
			log_debug(__func__, "ERROR:  %s", strerror(errno));
			//exit(0);
		}
		write_log(__func__, "Failed to load JSON from '%s.\n Error: %s'", path,
				error.text);
		return 0;
	}

	if (!json_is_object(root)) {
		log_debug(__func__,
				"Parsing error: Document root should be an object.");
		return 0;
	}

	/* Get test tag */
	if ((obj = json_object_get(root, "tag")) == NULL) {
		log_debug(__func__, "Parsing error: Attribute 'tag' is missing.");
		return 0;
	}
	if (!json_is_string(obj)) {
		log_debug(__func__, "Parsing error: Expected 'tag' to be a string.");
		return 0;
	}
	tag = json_string_value(obj);
	log_debug(__func__, "Tag: %s", tag);

	/* Get array of elements. */
	if ((elements = json_object_get(root, "elements")) == NULL) {
		log_debug(__func__, "Parsing error: Attribute 'elements' is missing.");
	}
	if (!json_is_array(elements)) {
		log_debug(__func__,
				"Parsing error: Expected 'elements' to be an array.");
		return 0;
	}

	/* Get number of elements */
	nrof_elements = json_array_size(elements);

	/* Initialize playback elements (PBEs) */
	app.elements_len = nrof_elements;
	app.elements = calloc(nrof_elements, sizeof(PlaybackElement));
	//printf("App elemtns length at initial: %d, \n", app.elements_len);
	/* Read metrics from all measured elements. */
	for (i = 0; i < nrof_elements; i++) {
		pbe = &app.elements[i];

		/* Get element from array. */
		element = json_array_get(elements, i);
		if (!json_is_object(element)) {
			log_debug(__func__, "Parsing error");
			return 0;
		}

		/* Get element URL */
		if ((obj = json_object_get(element, "url")) == NULL) {
			log_debug(__func__, "Parsing error: URL tag not found.");
			return 0;
		}
		if (json_is_string(obj)) {
			url = json_string_value(obj);
			log_debug(__func__, "URL: %s", url);

			/* Parse URL and extract domain and path */
			if (!hurl_parse_url((char *) url, &parsed_url)) {
				log_debug(__func__, "Failed to parse URL.");
				return 0;
			}
			/* Set domain, path, and port of PBE */
			pbe->http.domain = strdup(parsed_url->hostname);
			pbe->http.path = strdup(parsed_url->path);
			pbe->http.port = parsed_url->port;

			hurl_parsed_url_free(parsed_url);

		} else {
			log_debug(__func__,
					"Parsing error: Expected element URL to be a string.");
			return 0;
		}

		/* Get element URL hash (this is the unique identifier of an element) */
		if ((obj = json_object_get(element, "hash")) == NULL) {
			log_debug(__func__, "Parsing error: Hash tag not found.");
			return 0;
		}
		if (json_is_string(obj)) {
			pbe->http.hash = strdup(json_string_value(obj));
			log_debug(__func__, "hash: %s", pbe->http.hash);

			/* Load data... map file to memory */
			data_path_len = strlen(search_path) + strlen("/")
					+ strlen(pbe->http.hash) + strlen(".body");
			data_path = malloc(sizeof(char) * (data_path_len + 1));
			snprintf(data_path, data_path_len + 1, "%s/%s.body", search_path,
					pbe->http.hash);
			if ((fp = open(data_path, O_RDONLY)) == -1) {
				log_debug(__func__, "Failed to open '%s': %s", data_path,
						strerror(errno));
				/* TODO: Failed to open file */
			} else {
				/* Map file to memory */
				pbe->http.data_len = lseek(fp, 0, SEEK_END);
				if ((pbe->http.data = mmap(NULL, pbe->http.data_len, PROT_READ,
				MAP_PRIVATE, fp, 0)) == NULL) {
					log_debug(__func__, "Failed to memory map '%s': %s",
							pbe->http.hash, strerror(errno));
				} else {
					log_debug(__func__, "Memory mapped '%s'", pbe->http.hash);
				}
			}

		} else {
			log_debug(__func__,
					"Parsing error: Expected hash tag to be string.");
			return 0;
		}

		/* Get DNS metrics */
		if ((dns = json_object_get(element, "dns")) == NULL) {
			log_debug(__func__, "Parsing error: dns tag is missing.");
			return 0;
		}

		/* Get DNS trigger (the hash of the element that contains the DNS metrics for the domain shared with this element) */
		if ((obj = json_object_get(dns, "trigger")) != NULL) {
			/* This is a reference to the element that triggered DNS resolution */
			if (json_is_string(obj)) {
				pbe->dns_trigger = strdup(json_string_value(obj));
				log_debug(__func__, "trigger: %s", pbe->dns_trigger);
			} else {
				log_debug(__func__, "Parsing error");
				return 0;
			}
		} else {
			/* This is an element that triggered DNS resolution */
			pbe->dns = calloc(1, sizeof(DNSPlayback));

			/* Read networkTime */
			/* TODO: execTime captures both network and application layer delays and will be more appropriate for replay */
			if ((obj = json_object_get(dns, "networkTime")) == NULL) {
				log_debug(__func__, "Parsing error");
				return 0;
			}
			if (json_is_number(obj)) {
				pbe->dns->network_time = (unsigned int) json_number_value(obj);
				log_debug(__func__, "DNS network time: %u",
						pbe->dns->network_time);
			} else {
				log_debug(__func__,
						"Parsing error: DNS networkTime is not a number.");
				return 0;
			}

			/* Read query name */
			if ((obj = json_object_get(dns, "queryName")) == NULL) {
				log_debug(__func__, "Parsing error");
				return 0;
			}
			if (json_is_string(obj)) {
				pbe->dns->domain = strdup(json_string_value(obj));
				log_debug(__func__, "Query name: %s", pbe->dns->domain);
			} else {
				log_debug(__func__,
						"Parsing error: Domain name was not a string");
				return 0;
			}

		}

		/* Get HTTP metrics */
		if ((http = json_object_get(element, "http")) == NULL) {
			log_debug(__func__,
					"Parsing error: Element is missing HTTP metrics.");
			return 0;
		}

		/* Read responseCode (HTTP repsonse code, e.g. 200, 404) */
		if ((obj = json_object_get(http, "responseCode")) == NULL) {
			log_debug(__func__,
					"Parsing error: HTTP response code is missing.");
			return 0;
		}
		if (json_is_number(obj)) {
			pbe->http.response_code = (unsigned int) json_number_value(obj);
			log_debug(__func__, "HTTP response code: %u",
					pbe->http.response_code);
		} else {
			log_debug(__func__,
					"Parsing error: HTTP response code it not a number.");
			return 0;
		}

		/* Read connectTime */
		if ((obj = json_object_get(http, "connectTime")) == NULL) {
			log_debug(__func__, "Parsing error");
			return 0;
		}
		if (json_is_number(obj)) {
			pbe->http.connect_time = (unsigned int) json_number_value(obj);
			log_debug(__func__, "HTTP connect time: %u",
					pbe->http.connect_time);
		} else {
			log_debug(__func__, "Parsing error");
			return 0;
		}

		/* Read downloadTime */
		if ((obj = json_object_get(http, "downloadTime")) == NULL) {
			log_debug(__func__, "Parsing error");
			return 0;
		}
		if (json_is_number(obj)) {
			pbe->http.download_time = (float) json_number_value(obj);
			log_debug(__func__, "Download time: %f", pbe->http.download_time);
		} else {
			log_debug(__func__,
					"Parsing error: HTTP download time is missing.");
			return 0;
		}

		/* Read downloadSize */
		/* TODO: The actual number of transferred bytes should also include header and encoding overhead,
		 * 			and this value should be used for calculating bandwidth limit. */
		if ((obj = json_object_get(http, "downloadSize")) == NULL) {
			log_debug(__func__, "Parsing error");
			return 0;
		}
		if (json_is_number(obj)) {
			pbe->http.content_len = (float) json_number_value(obj);
			log_debug(__func__, "Download size: %f", pbe->http.content_len);
		} else {
			log_debug(__func__,
					"Parsing error: HTTP download size is missing.");
			return 0;
		}

		/* Was the transfer successfull? */
		if (pbe->http.response_code == 200) {

			/* Get HTTP headers */
			if ((headers = json_object_get(http, "headers")) == NULL) {
				log_debug(__func__,
						"1: Parsing error: Element is missing HTTP headers.");
				//return 0;
				continue;
			}

			/* Find content type */

			json_object_foreach(headers, header_key, header_value)
			{
				if (strcasecmp(header_key, "content-type") == 0) {
					if (json_is_string(header_value)) {
						pbe->http.content_type = strdup(
								json_string_value(header_value));
						log_debug(__func__, "Content type: %s",
								pbe->http.content_type);
					} else {
						log_debug(__func__, "Parsing error");
						return 0;
					}
				}
			}
			if (pbe->http.content_len > 0 && pbe->http.content_type == NULL) {
				log_debug(__func__,
						"Parsing error 2: Element is missing content type.... but brwoser still works");
				/* TODO: I suppose it is possible the some servers dont use this header although they should.
				 *       Web browsers will probably still work even if the header is missing. */
				//return 0;
			}
		}

	} //for

	/* Link elements without DNS metrics to elements with DNS metrics. */
	for (i = 0; i < app.elements_len; i++) {
		pbe = &app.elements[i];
		if (pbe->dns_trigger) {
			/* Let's find the trigger elements */
			for (j = 0; j < app.elements_len; j++) {
				if (j == i)
					continue;
				playback_search = &app.elements[j];
				if(playback_search->http.hash == NULL) {
					continue;
				}
				if (strcmp(playback_search->http.hash, pbe->http.hash) == 0) {
					/* Match found */
					assert(playback_search->dns != NULL);
					pbe->dns = playback_search->dns;
					break;
				}
			}
			if (pbe->dns_trigger == NULL) {
				log_debug(__func__, "ERROR: Trigger element NOT found!");
				return 0;
			}
		}
	}
	close(fp); //close the file..

	return 1;
}
