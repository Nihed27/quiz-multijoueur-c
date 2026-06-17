#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <netinet/in.h>

#define PORT 12345
#define MAX_DATA 1024
#define MAX_JOUEURS 4

// Types de messages
#define MSG_WAIT       1
#define MSG_START      2
#define MSG_QUESTION   3
#define MSG_ANSWER     4
#define MSG_FEEDBACK   5
#define MSG_RANKING    6
#define MSG_END        7
#define MSG_CONNECT    8

typedef struct {
    int type;
    int payload_size;
    char data[MAX_DATA];
} QuizMessage;

// Fonctions réseau
int reseau_creer_serveur(int port);
int reseau_accepter_client(int fd_serveur, struct sockaddr_in *addr_out);

#endif
