#ifndef NETWORK_H
#define NETWORK_H

#include <netinet/in.h>
#include <stdint.h>

#define MAX_JOUEURS 4
#define PORT        12345
#define TAILLE_DATA 1024
#define TAILLE_NOM  64

typedef enum {
    MSG_NOM,
    MSG_BIENVENUE,
    MSG_QUESTION,
    MSG_REPONSE,
    MSG_RESULTAT,
    MSG_SCORES
} TypeMessage;

typedef struct {
    uint8_t  type;
    uint16_t longueur;
    char     data[TAILLE_DATA];
} Message;

int  reseau_creer_serveur(int port);
int  reseau_accepter_client(int fd_serveur, struct sockaddr_in *addr_out);
int  reseau_connecter_serveur(const char *ip, int port);
int  reseau_envoyer(int fd, TypeMessage type, const char *data);
int  reseau_recevoir(int fd, Message *msg_out);
void reseau_fermer(int fd);
int  reseau_parser_reponse(const char *data);

#endif
