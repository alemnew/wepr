#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "leone_tools.h"
#include <string.h>
#include <netinet/in.h>
#include "dns_support.h"
#include "dns_core.h"
#include <assert.h>
#include "playback.h"
#include <math.h>

#define DNS_SERVER_PORT 53 // should be set to default 53
#define DNS_TTL 60

DNSMessage *parse_dns_query_message(char *data, unsigned int data_len);
void dns_create_response_packet(DNSRecord *question, char **packet,
		unsigned short *packet_len, unsigned short transaction_id);
void dns_create_error_response(DNSRecord *question, char **packet,
		unsigned short *packet_len, unsigned short transaction_id);
void *handle_dns_query(void *msg);

struct query_message {
	char recvbuf[4096];
	unsigned int recvbuf_len;
	struct sockaddr client_addr;
	unsigned int client_addr_len;
	int sock;
};

/**
 *  Creates a DNS server listening on localhost:53 for UDP packets
 */
void *dns_server(void *arg) { //arg - void *arg
	int sock;
	struct sockaddr_in addr;
	unsigned int addr_len = sizeof(struct sockaddr_in);

	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_t *attrp; /* NULL or &attr */
	int s;
	struct query_message *query;

	/* Create UDP socket */
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		log_debug(__func__, "Failed to create socket: %s", strerror(errno));
		return NULL;
	}

	/* Bind it to port 53 */
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET; /* IPv4 support only */
	addr.sin_port = htons(DNS_SERVER_PORT);
	if (bind(sock, (struct sockaddr *) &addr, addr_len) == -1) {
		log_debug(__func__, "Failed to bind socket: %s", strerror(errno));
		return NULL;
	}

	log_debug(__func__, "Listinging for requests on port %d...",
	DNS_SERVER_PORT);
	
	printf("This is silly ");
	for (;;) {
		query = malloc(sizeof(struct query_message));
		query->client_addr_len = sizeof(struct sockaddr);
		query->sock = sock;
		if ((query->recvbuf_len = recvfrom(sock, query->recvbuf,
				sizeof(query->recvbuf), MSG_NOSIGNAL, &query->client_addr,
				&query->client_addr_len)) > 0) {
			// log_debug(__func__, "Received message: %d bytes", query->recvbuf_len);

			/* TODO: Check that DNS message is at least the minimum length before parsing. */
			/* Parse request in separate thread. */

			attrp = &attr;
			s = pthread_attr_init(&attr);
			if (s != 0) {
				hurl_debug(__func__, "Error: pthread_attr_init");
			}
			s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			if (s != 0) {
				hurl_debug(__func__, "Error: pthread_attr_setdetachstate");
			}

			s = pthread_create(&thread, attrp, handle_dns_query,
					(void *) query);
			if (s != 0) {
				printf("Error: pthread_create - handle_dns_query %d", s);
				//hurl_debug(__func__,  "Error: pthread_create - content_handler");
			}
		}
	}
	return NULL;

}

void *handle_dns_query(void *msg) {
	DNSMessage *query;
	char *packet;
	unsigned short packet_len;
	int i;
	char *qname;
	PlaybackElement *pbe;
	int match_found = 0;
	struct timespec delay;
	float delay_s;
	char *recvbuf;
	unsigned int recvbuf_len;
	struct query_message *query_msg = (struct query_message *) msg;
	recvbuf = query_msg->recvbuf;
	recvbuf_len = query_msg->recvbuf_len;
	/* Parse DNS request */
	if ((query = parse_dns_query_message(recvbuf, recvbuf_len)) != NULL) {
		if (query->nrof_questions == 1) {
			/* Find query name */
			//printf("!!! 2 The number of elements in dns server %u", (unsigned int) app.elements_len);
			qname = query->questions[0]->name;
			//	log_debug(__func__, "Query: %s", query->questions[0]->name);
			//printf("!!! 22 The number of elements in dns server %u", (unsigned int) app.elements_len);

			/* Find requested element: PBE with matching domain name. */
			for (i = 0; i < app.elements_len; i++) {
				pbe = &app.elements[i];
				if (pbe->dns != NULL && strcmp(pbe->dns->domain, qname) == 0) {
					//Match found 
			//printf("!!! 3 The number of elements in dns server %u", (unsigned int) app.elements_len);
					log_debug(__func__, "Matching element found.");
					match_found = 1;
					break;
				}
			}

			/* Was a matching PBE found? */
			if (!match_found) {
				//	log_debug(__func__, "Element not found. Sending NXDOMAIN response...");
				/* Send error response without delay. */
				dns_create_error_response(query->questions[0], &packet,
						&packet_len, query->id);
				sendto(query_msg->sock, packet, packet_len, MSG_NOSIGNAL,
						&query_msg->client_addr, query_msg->client_addr_len);
			} else {
				dns_create_response_packet(query->questions[0], &packet,
						&packet_len, query->id);

				/* Calculate response delay. */
				/* TODO: Use DNS execTime instead of networkTime */
				delay_s = (float) pbe->dns->network_time / 1000.0f;
				delay.tv_sec = (__time_t ) floor(delay_s);
				delay.tv_nsec = (long int) ((delay_s - delay.tv_sec) * 10e8);
				log_debug(__func__, "Delaying DNS response with %ld s, %ld ns",
						delay.tv_sec, delay.tv_nsec);

				/* Delay transmission */
				nanosleep(&delay, NULL);
				/* TODO Handle interrupts and remaining sleep time */
				sendto(query_msg->sock, packet, packet_len, MSG_NOSIGNAL,
						&query_msg->client_addr, query_msg->client_addr_len);
			}
		} else {
			log_debug(__func__, "Query did not contain any questions.");
		}
	} else {
		log_debug(__func__, "Failed to parse DNS query.");
	}

	/* free(query_msg); */
	pthread_exit(NULL);

}

void dns_packet_insert_record(Buffer *msgbuf, DNSRecord *record) {
	char *domain, *domain_tmp, *token, *domain_split_ptr;
	unsigned int token_len;

	domain = allocstrcpy(record->name, strlen(record->name), 1); /* Copy qname before calling strtok(). */
	domain_tmp = domain;
	/* Full label detected. */
	while ((token = strtok_r(domain_tmp, ".", &domain_split_ptr)) != NULL) {
		token_len = (unsigned char) strlen(token);
		/* Insert token length. */
		buffer_insert(msgbuf, (char *) &token_len, 1);
		/* Insert token */
		buffer_insert(msgbuf, token, token_len);
		/* Set token input to NULL. */
		if (domain_tmp != NULL)
			domain_tmp = NULL;
	}
	/* Terminate question with \0. */
	buffer_insert(msgbuf, "\0", 1);

	/* Free domain buffer. */
	free(domain);

	/* Insert class and type. */
	buffer_insert_short(msgbuf, htons(record->type));
	buffer_insert_short(msgbuf, htons(IN));

	if (record->section != QUESTIONS) {

		/* Insert TTL */
		buffer_insert_int(msgbuf, htonl(DNS_TTL));

		/* Insert RDATA length and RDATA */
		buffer_insert_short(msgbuf, htons(record->data_len));
		if (record->data_len > 0) {
			buffer_insert(msgbuf, record->data, record->data_len);
		}
	}
}

void dns_create_error_response(DNSRecord *question, char **packet,
		unsigned short *packet_len, unsigned short transaction_id) {
	struct buffer *msgbuf;
	unsigned short flags = 0;
	assert(question != NULL);

	/* Initialize message buffer. */
	buffer_init(&msgbuf, 1024, 1024);

	/* Modify flags. ONLY create query messages. */
	dns_message_flag(&flags, DNS_FLAG_TYPE, RESPONSE);
	dns_message_flag(&flags, DNS_FLAG_RESP_CODE, DNS_ERROR_NXDOMAIN);

	/* Insert header values into message buffer. */
	buffer_insert_short(msgbuf, htons(transaction_id));
	buffer_insert_short(msgbuf, htons(flags));
	buffer_insert_short(msgbuf, htons(1)); /* Nrof questions. DNS servers only support one query per message. */
	buffer_insert_short(msgbuf, htons(0)); /* Nrof answers. */
	buffer_insert_short(msgbuf, htons(0));/* Nrof authorities. */
	buffer_insert_short(msgbuf, htons(0)); /* Nrof addtionals. */

	/* Insert original question */
	dns_packet_insert_record(msgbuf, question);

	*packet = msgbuf->head;
	*packet_len = msgbuf->data_len;
	free(msgbuf);
}

void dns_create_response_packet(DNSRecord *question, char **packet,
		unsigned short *packet_len, unsigned short transaction_id) {
	struct buffer *msgbuf;
	unsigned short flags = 0;
	DNSRecord answer;
	char rdata[4] = { 127, 0, 0, 1 }; /* ALWAYS respond with 127.0.0.1 no matter the query name */
	assert(question != NULL);

	bzero(&answer, sizeof(DNSRecord));

	/* Initialize message buffer. */
	buffer_init(&msgbuf, 1024, 1024);

	/* Modify flags. ONLY create query messages. */
	dns_message_flag(&flags, DNS_FLAG_TYPE, RESPONSE);

	/* Insert header values into message buffer. */
	buffer_insert_short(msgbuf, htons(transaction_id));
	buffer_insert_short(msgbuf, htons(flags));
	buffer_insert_short(msgbuf, htons(1)); /* Nrof questions. DNS servers only support one query per message. */
	buffer_insert_short(msgbuf, htons(1)); /* Nrof answers. */
	buffer_insert_short(msgbuf, htons(0));/* Nrof authorities. */
	buffer_insert_short(msgbuf, htons(0)); /* Nrof addtionals. */

	/* Insert original question */
	dns_packet_insert_record(msgbuf, question);

	/* Insert fake answer */
	answer.class = IN;
	answer.type = A;
	answer.name = question->name;
	answer.data = rdata; /* 127.0.0.1 */
	answer.data_len = 4;
	answer.section = ANSWERS;

	dns_packet_insert_record(msgbuf, &answer);

	*packet = msgbuf->head;
	*packet_len = msgbuf->data_len;
	free(msgbuf);
}

DNSMessage *parse_dns_query_message(char *data, unsigned int data_len) {
	char *cursor = data, *cursor_max;
	unsigned short r, nrof_records;
	struct buffer *namebuf;
	int label_len;
	unsigned short flags;
	unsigned short *cursor_short;
	DNSMessage *message;
	DNSRecord *record, **record_ptr = NULL;
	DNSSection section = 0;

	cursor_short = (unsigned short *) data;
	message = calloc(1, sizeof(DNSMessage));

	/* Get transaction ID. */
	message->id = ntohs(chars_to_short(cursor));
	cursor += 2;

	cursor_short = (unsigned short *) cursor;
	flags = ntohs(*cursor_short);

	/* Parse flags. */
	message->type = dns_message_flag(&flags, DNS_FLAG_TYPE, DNS_FLAG_READ);
	message->authoritative = dns_message_flag(&flags,
			DNS_FLAG_AUTHORITATIVE_ANS, DNS_FLAG_READ);
	message->truncation = dns_message_flag(&flags, DNS_FLAG_TRUNCATION,
			DNS_FLAG_READ);
	message->recursion_desired = dns_message_flag(&flags,
			DNS_FLAG_RECURSION_DESIRED, DNS_FLAG_READ);
	message->recursion_avail = dns_message_flag(&flags,
			DNS_FLAG_RECURSION_AVAIL, DNS_FLAG_READ);
	message->response_code = dns_message_flag(&flags, DNS_FLAG_RESP_CODE,
			DNS_FLAG_READ);

	/* Ignore responses */
	if (message->type != QUERY) {
		free(message);
		log_debug(__func__, "Message is not a query. Ignoring it...");
		return NULL;
	}

	/* Check message code. */
	if (message->response_code != 0) {
		/* message failed. */
		switch (message->response_code) {
		case 1:
			log_debug(__func__, "message code: Format error.");
			break;
		case 2:
			log_debug(__func__, "message code: Server failure.");
			break;
		case 3:
			log_debug(__func__, "message code: Non-existant domain.");
			break;
		case 4:
			log_debug(__func__, "message code: Not implemented.");
			break;
		case 5:
			log_debug(__func__, "message code: Query refused.");
			break;
		default:
			log_debug(__func__, "ERROR: message code %u",
					message->response_code);
			break;
		}
	}

	/* Update cursors. */
	cursor += 2;
	cursor_short++;

	/* Read number of questions, answers, name servers, and additional records. */
	message->nrof_questions = ntohs(*cursor_short++);
	message->nrof_answers = ntohs(*cursor_short++);
	message->nrof_authorities = ntohs(*cursor_short++);
	message->nrof_additionals = ntohs(*cursor_short++);

	/* Allocate memory for record pointers. */
	nrof_records = message->nrof_questions + message->nrof_answers
			+ message->nrof_authorities + message->nrof_additionals;

	/* Update char cursor. */
	cursor = (char *) cursor_short;
	cursor_max = data + data_len;

	/* Read records. */
	for (r = 0; r < nrof_records; r++) {
		/* Determine current section of message. */
		if (r < message->nrof_questions) {
			section = QUESTIONS;
			record_ptr = &message->questions[r];
		} else if (r < message->nrof_questions + message->nrof_answers) {
			section = ANSWERS;
			record_ptr = &message->answers[r - message->nrof_questions];
		} else if (r
				< message->nrof_questions + message->nrof_answers
						+ message->nrof_authorities) {
			section = AUTHORITIES;
			record_ptr = &message->authorities[r - message->nrof_questions
					- message->nrof_answers];
		} else if (r
				< message->nrof_questions + message->nrof_answers
						+ message->nrof_authorities
						+ message->nrof_additionals) {
			section = ADDITIONALS;
			record_ptr = &message->additionals[r - message->nrof_questions
					- message->nrof_answers - message->nrof_authorities];
		}
		*record_ptr = calloc(1, sizeof(DNSRecord));
		record = *record_ptr;

		/* Read name. */
		buffer_init(&namebuf, 1024, 1024);
		while ((label_len = dns_parse_rr_label(data, &cursor, cursor_max,
				namebuf)) > 0) {
			/* Do nothing. */
		}

		/* Check return value of label reader. */
		if (label_len == 0) {
			/* Labels read successfully. */
			/* Remove trailing dot. */
			if (namebuf->data_len > 0) { /* Handle records for root servers. */
				buffer_rewind(namebuf, 1);
				buffer_trim(namebuf);
			}
			record->section = section;
			record->name = namebuf->head;
			free(namebuf);
		} else {
			/* Failed to read labels. */
			buffer_free(namebuf);
			/* TODO: Free memory */
			log_debug(__func__, "Failed to read labels.");
			return NULL;
		}

		/* Read type and class. */
		record->type = ntohs(chars_to_short(cursor));
		cursor += 2;
		record->class = ntohs(chars_to_short(cursor));
		cursor += 2;

	}

	/* Check if end of message has been reached. */
	if (cursor != data + data_len) {
		log_debug(__func__,
				"WARNING: Parsing ended before end of DNS message (possibly padded).");
		/* TODO: Do proper memory clean up */
		return NULL;
	}
	return message;
}
