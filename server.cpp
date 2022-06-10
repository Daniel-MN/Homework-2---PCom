// Musuroi Daniel-Nicusor, 323CB
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <map>
#include <vector>
#include <algorithm>
#include "helpers.h"

int read_keyboard(int TCP_socket, int UDP_socket, fd_set read_fds, int fdmax) {
    char buf[5];
    fgets(buf, 5, stdin);

    // daca se citeste exit, atunci se inchid socket-urile 
    if (strncmp(buf, "exit", 4) == 0) {
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                close(i);
            }
        }
        close(TCP_socket);
        close(UDP_socket);
        return 0;
    }

    // a fost citit altceva de la tastatura
    return -1;
}

void send_tcp_message(char *buffer, int socket, int len) {
    int bytes_send = 0;
    int n;
    int bytes_remaining = len;

    n = send(socket, buffer, len, 0);
    
}


// primeste un mesaj UDP si trimite mesajul fiecarui client abonat la
// topicul respectiv
int receive_UDP(int UDP_socket, map<string, vector<string>> topics, 
                        map<string, struct TCP_client *> &clients) {
    int n;
    char buffer[BUFLEN_UDP];
    memset(buffer, 0, BUFLEN_UDP);
    struct sockaddr_in cli_addr;
    socklen_t cli_len;
    // primeste mesajul UDP
    n = recvfrom(UDP_socket, buffer, BUFLEN_UDP, 0, 
                (struct sockaddr *)&cli_addr, &cli_len);
    DIE(n < 0, "recvfrom");

    struct UDP_message *message = (struct UDP_message *)buffer;

    // creeaza un mesaj tcp pe care il va trimite la clientul TCP
    struct TCP_message tcp_message;
    tcp_message.ip_client_udp = cli_addr.sin_addr;
    tcp_message.port_client_udp = cli_addr.sin_port;
    tcp_message.message = *message;

    // cauta topicul si gaseste lista de clienti abonati la acest
    // topic
    auto topic_clients = topics.find(string(message->topic));
    struct TCP_client *client;

    // ia fiecare client din vectorul de ID-uri de clienti asociat
    // acestui topic
    for (string ID_client : topic_clients->second) {
        auto ID_Client_entry = clients.find(ID_client);
        client = ID_Client_entry->second;

        // daca clientul este online, atunci trimite mesajul
        if (client->online) {
            send_tcp_message((char *)&tcp_message, client->socket, 
                                                MAX_TCP_LEN_MESSAGE);
        } else {
            // daca clientul nu este online se verifica daca topicul
            // acesta este un topic la care clientul s-a abonat prin 
            // Store & forward
            bool notStored = true;
            for (int i = 0; i < client->nr_topics && notStored; i++) {
                if (strncmp(message->topic, client->topics[i], 
                                            strlen(message->topic)) == 0) {
                    notStored = false;
                    client->messages_stored[client->nr_messages] = tcp_message;
                    client->nr_messages++;
                }
            }
        }
    }
    return 0;
}

void receive_TCP_client(int TCP_socket, fd_set *read_fds, int *fdmax, 
                            map<string, struct TCP_client *> &clients, 
                            map<int, string> &socket_ID) {
    int clientfd;
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    clientfd = accept(TCP_socket, (struct sockaddr *)&cli_addr, &cli_len);
    DIE(clientfd < 0, "accept");

    char ID[10];
    int n = recv(clientfd, ID, sizeof(ID), 0);
    DIE(n < 0, "recv");

    ID[n] = '\0';

    auto ID_client = clients.find(string(ID));

    // Daca ID-ul clientului nu exista, inseamna ca se conecteaza un nou client
    // Daca ID-ul clientului exista deja, inseamna ca se reconecteaza un client
    // sau se conecteaza un client cu un ID al altui utilizator
    if (ID_client == clients.end()) {
        printf("New client %s connected from %s:%d.\n", ID, 
                    inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        // se creeaza un nou client
        struct TCP_client *client = (struct TCP_client *)malloc(sizeof(struct TCP_client));
        DIE(client == NULL, "malloc");

        client->nr_messages = 0;
        client->nr_topics = 0;
        client->online = true;
        client->socket = clientfd;

        string ID_string = string(ID);

        // clientul este retinut
        clients[ID_string] =  client;
        // se asociaza socketul acestui client nou creat
        socket_ID[clientfd] = ID_string;

        // se adauga socketul in multimea de descriptori
        FD_SET(clientfd, read_fds);
        if (clientfd > *fdmax) {
            *fdmax = clientfd;
        }

    } else {
        
        struct TCP_client *client = ID_client->second;
        if (client->online) {
            // Un client incearca sa se conecteze cu un ID al altui client
            close(clientfd);
            printf("Client %s already connected.\n", ID);
            return;
        } else {
            // Clientul se reconecteaza
            printf("New client %s connected from %s:%d.\n", ID, 
                    inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            client->online = true;
            client->socket = clientfd;

            socket_ID[clientfd] = string(ID);

            FD_SET(clientfd, read_fds);
            if (clientfd > *fdmax) {
                *fdmax = clientfd;
            }

            // La reconectare se trimit mesajele salvate (de la topicurile
            // cu subscribe store & forward)
            for (int i = 0; i < client->nr_messages; i++) {
                send_tcp_message((char *)(&(client->messages_stored[i])), 
                                        client->socket, MAX_TCP_LEN_MESSAGE);

                memset(&(client->messages_stored[i]), 0, MAX_TCP_LEN_MESSAGE);
            }

            client->nr_messages = 0;
        }
    }
}

void receive_sub_unsub(int socket, map<int, string> &socket_ID, 
                        map<string, struct TCP_client *> &clients, 
                    fd_set *read_fds, map<string, vector<string>> &topics) {
    
    char buf[BUFLEN_TCP];
    memset(buf, 0, BUFLEN_TCP);

    // se primeste un mesaj de la un client TCP
    int bytes_received = 0;
	int n = recv(socket, buf, BUFLEN_TCP, 0);
	DIE(n < 0, "recv");

	char ID[10];
    memcpy(ID, socket_ID[socket].c_str(), 10);
    string ID_string = socket_ID[socket];

    // clientul trebuie deconectat
    if (n == 0) {
        // se sterge socketul din multime si se sterge asocierea
        // socket_ID
        socket_ID.erase(socket);
        FD_CLR(socket, read_fds);

        // se cauta clientul si se afiseaza mesajul corespunzator
        auto it = clients.find(ID_string);
        if (it != clients.end()) {
            clients[ID_string]->online = false;
            printf("Client %s disconnected.\n", ID);
        }
        return;
    }

    

    // A fost primit un mesaj de subscribe sau de unsubscribe:
    if (strncmp(buf, "subscribe", 9) == 0) {
        // se extrage topicul si optiunea SF
        char *topic = strtok(buf + 10, " ");
        char *sf_str = strtok(NULL, " ");
        int SF = atoi(sf_str);
        DIE(SF < 0, "atoi");

        string topic_string = string(topic);

        // daca topicul exista deja atunci doar se adauga clientul
        // in vectorul de clienti abonati la acest topic
        auto it = topics.find(topic_string);
        if (it != topics.end()) {
            bool subscribed = false;
            // se verifica daca acest client nu este deja abonat 
            // la acest topic
            for (string ID_client : topics[topic_string]) {
                if (ID_client == ID_string) {
                    subscribed = true;
                    break;
                }
            }

            // daca nu este deja abonat, atunci se aboneaza
            if (!subscribed) {
                topics[topic_string].push_back(ID_string);
            }

        } else {
            // se creeaza acest topic la care s-a abonat clientul
            vector<string> v;
            v.push_back(ID_string);
            topics[topic_string] = v;
        }

        // daca clientul se aboneaza cu optiunea Store and Forward
        if (SF == 1) {
            bool SF_subscribed = false;
            // se verifica daca acest client nu este deja abonat cu 
            // optiunea Store and Forward
            for (int i = 0; i < clients[ID_string]->nr_topics; i++) {
                if (strcmp(clients[ID_string]->topics[i], topic) == 0) {
                    SF_subscribed = true;
                    break;
                }
            }

            // daca nu este deja abonat, atunci se aboneaza (SF = 1)
            if (!SF_subscribed) {
                memcpy(clients[ID_string]->topics[clients[ID_string]->nr_topics], 
                                                        topic, strlen(topic) + 1);
                clients[ID_string]->nr_topics++;
            }
        }
        return;
    }

    // daca se primeste un mesaj de unsubscribe
    if (strncmp(buf, "unsubscribe", 11) == 0) {
        // se extrahe topicul
        char *topic = strtok(buf + 12, " ");
        string topic_string = string(topic);

        // se cauta topicul si este sters clientul din
        // vectorul de clienti abonati la acest topic
        auto it = topics.find(topic_string);
        if (it != topics.end()) {
            auto itr = std::find(topics[topic_string].begin(), 
                                    topics[topic_string].end(), ID_string);
            if (itr != topics[topic_string].end()) {
                topics[topic_string].erase(itr);
            }
        }

        bool SF_on_topic = false;
        int i = 0;
        // se verifica daca clientul este abonat la acest topic cu optiunea
        // SF
        for (; i < clients[ID_string]->nr_topics && !SF_on_topic; i++) {
            if (strcmp(clients[ID_string]->topics[i], topic) == 0) {
                SF_on_topic = true;
            }
        }

        if (SF_on_topic) {
            // clientul este dezabonat de tot de la acest topic, fiind sters
            // si din optiunea de store and forward
            for (int j = i; j < clients[ID_string]->nr_topics; j++) {
                memset(clients[ID_string]->topics[j - 1], 0, 
                            strlen(clients[ID_string]->topics[j - 1]));
                memcpy(clients[ID_string]->topics[j - 1], clients[ID_string]->topics[j], 
                                            strlen(clients[ID_string]->topics[j]) + 1);
            }

            clients[ID_string]->nr_topics--;
        }
    }
}

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int UDP_sockfd, TCP_sockfd, portno;

    struct sockaddr_in serv_addr;

    if (argc < 2) {
        usage(argv[0]);
    }

    fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds
    
    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    // se creeaza socketul UDP
    UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(UDP_sockfd < 0, "socket");

    portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(UDP_sockfd, (const struct sockaddr *)&serv_addr, 
                                                sizeof(serv_addr));
    DIE(ret < 0, "bind");

    // se creeaza socketul TCP
    TCP_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(TCP_sockfd < 0, "socket");

    int flag = 1;
    ret = setsockopt(TCP_sockfd, IPPROTO_TCP, TCP_NODELAY, 
                                        (char *) &flag, sizeof(int));
    DIE(ret < 0, "setsockopt");

    ret = bind(TCP_sockfd, (struct sockaddr *) &serv_addr, 
                                sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(TCP_sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

    // Se adauga in multimea de descriptori intrarea stdin, 
    // socketul UDP si socketul TCP
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(UDP_sockfd, &read_fds);
    FD_SET(TCP_sockfd, &read_fds);

    if (UDP_sockfd > TCP_sockfd) {
        fdmax = UDP_sockfd;
    } else {
        fdmax = TCP_sockfd;
    }

    //key = ID, value = clientul
    map<string, struct TCP_client *> clients;
    //key = topic; value = vectori cu idurile clientilor abonati la aces topic;
    map<string, vector<string>> topics;
    //key = socket; value = ID-ul clientului
    map<int, string> socket_ID;

    int finish = 0;
    while (!finish) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == STDIN_FILENO) {
                    // se citeste ceva de la tastatura
                    ret = read_keyboard(TCP_sockfd, UDP_sockfd, read_fds, 
                                                                    fdmax);
                    DIE(ret < 0, "Only \"exit\" command is accepted!\n");

                    if (ret == 0) {
                        finish = 1;
                        break;
                    }

                } else if (i == UDP_sockfd) {
                    // trateaza pachet primit de la un client UDP
                    ret = receive_UDP(UDP_sockfd, topics, clients);
                    DIE(ret < 0, "receive_UDP");

                } else if (i == TCP_sockfd) {
                    // apare un nou client TCP sau se reconecteaza 
                    // un client TCP
                    receive_TCP_client(TCP_sockfd, &read_fds, 
                                                &fdmax, clients, socket_ID);
                } else {
                    // este un client TCP care trimite un mesaj de 
                    // subscribe sau unsubscribe
                    receive_sub_unsub(i, socket_ID, clients, &read_fds, topics);
                }
            }
        }
    }

    // se elibereaza memoria alocata dinamic
    for (auto i : clients) {
        free(i.second);
    }
    return 0;
}