/*
 * playback.h
 *
 *  Created on: Mar 17, 2014
 *      Author: boyem1
 */

#ifndef PLAYBACK_H_
#define PLAYBACK_H_

typedef struct dns_playback DNSPlayback;
struct dns_playback {
	char *domain;
	unsigned int network_time, execution_time;
	int has_v4, has_v6; /* Did the message contain an A or AAAA record. */
	int v6_first; /* The v6 address came first. */
	unsigned int packet_len;
};


struct playback_element {
	DNSPlayback *dns;
	char *dns_trigger;
	struct {
		char *domain;
		char *path;
		int port;
		char *hash;
		unsigned int connect_time; /* Time to establish connection. */
		unsigned int connect_time_ssl; /* Time to establish TLS connection */
		unsigned int first_bytes; /* Time to first byte received. */
		unsigned int header_len; /* Size of HTTP header. */
		unsigned int content_len; /* Content length */
		int encoding_overhead; /* Bytes received - header_len - content_len */
		float download_time; /* Total time of transfer */
		char *data;
		unsigned int data_len;
		char *content_type;
		unsigned int response_code;
	} http;
};
typedef struct playback_element PlaybackElement;

struct playback_globals {
	PlaybackElement *elements;
	int elements_len;
};

struct playback_globals app;
void *dns_server(void *arg); //arg -
int http_server(int port);
int *content_server(); // arg -void *arg

#endif /* PLAYBACK_H_ */
