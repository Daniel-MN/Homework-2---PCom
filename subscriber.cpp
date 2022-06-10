// Musuroi Daniel-Nicusor, 323CB
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cmath>
#include "helpers.h"

// afiseaza un mesaj de tip int
void print_int(char *payload) {
    uint8_t *sign = (uint8_t *)payload;
	if (*sign == 1) {
		printf("-");
	}

    uint32_t *nr = (uint32_t *)(payload + 1);
    printf("%d\n", ntohl(*nr));
}

// afiseaza un mesaj de tip short_Real
void print_short_real(char *payload) {
	uint16_t *nr = (uint16_t *)payload;
	uint16_t nr_host = ntohs(*nr);
	printf("%d.%d%d\n", nr_host / 100, (nr_host % 100) / 10, nr_host % 10);
}

// afiseaza un mesaj de tip float
void print_float(char *payload) {
	uint8_t *sign = (uint8_t *)payload;
	if (*sign == 1) {
		printf("-");
	}
	uint32_t *nr = (uint32_t *)(payload + 1);
	uint32_t nr_host = ntohl(*nr);

	uint8_t *exp = (uint8_t *)(payload + 5);

	float power10 = pow(10, *exp);

	printf("%f\n", nr_host / power10);
}

// afiseaza tipul mesajului si mesajul efectiv
void print_type_payload(int type, char *payload) {
	switch(type) {
		case 0:
			printf("INT - ");
			print_int(payload);
			break;
		case 1:
			printf("SHORT_REAL - ");
			print_short_real(payload);
			break;
		case 2:
			printf("FLOAT - ");
			print_float(payload);
			break;
		case 3:
			printf("STRING - %s\n", payload);
			break;
		default:
			printf("Unknown type: %d\n", type);
			DIE(1, "Unknown type");
	}
}


// primeste informatia si afiseaza mesajul
int receive_print_info(int sockfd) {
	char buffer[MAX_TCP_LEN_MESSAGE];

	// primeste mesajul de la server
	int bytes_received = 0;
	int n = recv(sockfd, buffer, MAX_TCP_LEN_MESSAGE, 0);
	DIE(n < 0, "recv");

	// daca se pierde conexiunea
	if (n == 0) {	
		return -1;
	}

	struct TCP_message *m = (struct TCP_message *)buffer;

	printf("%s:%d - %s - ", inet_ntoa(m->ip_client_udp), m->port_client_udp, 
													m->message.topic);

	print_type_payload(m->message.type, m->message.payload);
	return 0;
}

// trimite un mesaj de tip TCP
void send_tcp_message(char *buffer, int socket, int len) {
    int bytes_send = 0;
    int n;
    int bytes_remaining = len;

    n = send(socket, buffer, len, 0);
    DIE(n < 0, "send");
}

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int sockfd, n, ret;
	struct sockaddr_in serv_addr;

	if (argc < 4) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

    int flag = 1;
    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, 
                                        (char *) &flag, sizeof(int));
    DIE(ret < 0, "setsockopt");

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar

	// se goleste multimea de descriptori de citire (read_fds) si 
	// multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// se trimite ID-ul acestui client
    ret = send(sockfd, argv[1], strlen(argv[1]), 0);
    DIE(ret < 0, "send");

	char buf[BUFLEN_TCP];
    memset(buf, 0, BUFLEN_TCP);

	FD_SET(STDIN_FILENO, &read_fds);
	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) 
	// in multimea read_fds
	FD_SET(sockfd, &read_fds);

    while (1) {
		tmp_fds = read_fds;

		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		// Daca se citeste de la tastatura:
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			memset(buf, 0, BUFLEN_TCP);
			fgets(buf, BUFLEN_TCP, stdin);

            if (buf[strlen(buf) - 1] == '\n') {
                buf[strlen(buf) - 1] = '\0';
            }

			if (strncmp(buf, "exit", 4) == 0) {
				break;
			} else if (strncmp(buf, "subscribe", 9) == 0) {
				send_tcp_message(buf, sockfd, BUFLEN_TCP);

				printf("Subscribed to topic.\n");
            } else if (strncmp(buf, "unsubscribe", 11) == 0) {
				send_tcp_message(buf, sockfd, BUFLEN_TCP);

				printf("Unsubscribed to topic.\n");
            } else {
                printf("Wrong command. Try:\n");
				printf("subscribe <topic> <SF>\n");
				printf("or\n");
				printf("unsubscribe <topic>\n");
            }
		}

        // Daca primesc un mesaj de la server
		if (FD_ISSET(sockfd, &tmp_fds)) {
			ret = receive_print_info(sockfd);
			if (ret < 0) {
				break;
			}
		}
	}

	close(sockfd);

	return 0;
}