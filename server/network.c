#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "network.h"

int reseau_creer_serveur(int port) {
    int fd;
    struct sockaddr_in adresse;
    int opt = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("[RESEAU] socket()"); return -1; }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&adresse, 0, sizeof(adresse));
    adresse.sin_family      = AF_INET;
    adresse.sin_addr.s_addr = INADDR_ANY;
    adresse.sin_port        = htons(port);

    if (bind(fd, (struct sockaddr *)&adresse, sizeof(adresse)) < 0) {
        perror("[RESEAU] bind()"); close(fd); return -1;
    }

    if (listen(fd, MAX_JOUEURS) < 0) {
        perror("[RESEAU] listen()"); close(fd); return -1;
    }

    printf("[RESEAU] Serveur prêt sur le port %d\n", port);
    return fd;
}

int reseau_accepter_client(int fd_serveur, struct sockaddr_in *addr_out) {
    struct sockaddr_in adresse_client;
    socklen_t taille = sizeof(adresse_client);

    int fd_client = accept(fd_serveur,
                           (struct sockaddr *)&adresse_client,
                           &taille);
    if (fd_client < 0) {
        if (errno != EINTR) perror("[RESEAU] accept()");
        return -1;
    }

    if (addr_out) *addr_out = adresse_client;

    printf("[RESEAU] Client connecté : %s:%d (fd=%d)\n",
           inet_ntoa(adresse_client.sin_addr),
           ntohs(adresse_client.sin_port),
           fd_client);
    return fd_client;
}

int reseau_connecter_serveur(const char *ip, int port) {
    int fd;
    struct sockaddr_in adresse;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("[RESEAU] socket()"); return -1; }

    memset(&adresse, 0, sizeof(adresse));
    adresse.sin_family = AF_INET;
    adresse.sin_port   = htons(port);
    inet_pton(AF_INET, ip, &adresse.sin_addr);

    if (connect(fd, (struct sockaddr *)&adresse, sizeof(adresse)) < 0) {
        perror("[RESEAU] connect()"); close(fd); return -1;
    }

    printf("[RESEAU] Connecté au serveur %s:%d\n", ip, port);
    return fd;
}

int reseau_envoyer(int fd, TypeMessage type, const char *data) {
    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = (uint8_t)type;
    if (data) {
        strncpy(msg.data, data, TAILLE_DATA - 1);
        msg.longueur = (uint16_t)strlen(msg.data);
    }
    return send(fd, &msg, sizeof(msg), 0);
}

int reseau_recevoir(int fd, Message *msg_out) {
    return recv(fd, msg_out, sizeof(Message), 0);
}

void reseau_fermer(int fd) {
    close(fd);
}

int reseau_parser_reponse(const char *data) {
    return atoi(data);
}
