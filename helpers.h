#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace std;
/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)


#define BUFLEN_TCP	65 // dimensiunea maxima a calupului de date
							//primit de la un client TCP
#define BUFLEN_UDP 1551 //dimensiunea maxima a calupului de date primit
						// de la un client UDP
#define MAX_NR_TOPICS 20 // numarul maxim de mesaje la care poate fi abonat
						// un client cu store&forward
#define MAX_NR_MESSAGES 100 // numarul maxim de mesaje stocate
#define MAX_CLIENTS	100	// numarul maxim de clienti in asteptare
#define MAX_TCP_LEN_MESSAGE 1557

// mesajul transmis de un client UDP la server
struct UDP_message {
	char topic[50];
	uint8_t type;
	char payload[1500];
}__attribute__((packed));

// mesajul transmis de server la un client TCP
struct TCP_message {
	struct in_addr ip_client_udp; // adresa IP a clientului UDP de la care a
								  // fost primit mesajul
	in_port_t port_client_udp; // portul clientului UDP de pe care a fost 
							   // transmis mesajul
	struct UDP_message message; // mesajul UDP transmis
}__attribute__((packed));

struct TCP_client {
	int socket; // socketul prin care serverul comunica cu acest client
	bool online; // retine daca un client este in prezent online sau nu
	char topics[MAX_NR_TOPICS][50]; // topicurile la care este abonat un client
									// prin store&forward
	int nr_topics; // numarul de topicuri la care este abonat un client prin
				   // prin store%forward
	struct TCP_message messages_stored[MAX_NR_MESSAGES]; // mesajele stocate
					// pentru a fi transmise la reconectarea clientului
	int nr_messages; // numarul de mesaje stocate
};

#endif
