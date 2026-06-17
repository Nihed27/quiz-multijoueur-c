#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../common/protocol.h"
#include "network.h"

#define MSG_START    2
#define MSG_QUESTION 3
#define MSG_RANKING  6
#define MSG_END      7

#define NB_JOUEURS   2
#define MAX_QUESTIONS 50

typedef struct {
    char texte[256];
    char options[4][100];
    char bonne_reponse;
} Question;

typedef struct {
    int idx_joueur;
    int idx_question;
} ThreadArg;

Question questions[MAX_QUESTIONS];
int nb_questions = 0;

pthread_mutex_t players_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  all_answered_cond = PTHREAD_COND_INITIALIZER;
int players_answered_count = 0;
int sockets[NB_JOUEURS];
int scores[NB_JOUEURS];

void load_questions(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("Erreur ouverture questions.txt"); exit(1); }

    char ligne[512];
    nb_questions = 0;

    while (nb_questions < MAX_QUESTIONS && fgets(ligne, sizeof(ligne), f)) {
        ligne[strcspn(ligne, "\n")] = '\0';
        if (strlen(ligne) == 0) continue;

        char *token;
        Question *q = &questions[nb_questions];

        token = strtok(ligne, "|");
        if (!token) continue;
        strncpy(q->texte, token, sizeof(q->texte) - 1);

        for (int i = 0; i < 4; i++) {
            token = strtok(NULL, "|");
            if (!token) goto next;
            strncpy(q->options[i], token, sizeof(q->options[i]) - 1);
        }

        token = strtok(NULL, "|");
        if (!token) goto next;
        q->bonne_reponse = toupper(token[0]);

        nb_questions++;
        next:;
    }
    fclose(f);
    printf("[QUIZ] %d questions chargées\n", nb_questions);
}

void broadcast(QuizMessage *msg) {
    for (int i = 0; i < NB_JOUEURS; i++)
        send(sockets[i], msg, sizeof(QuizMessage), 0);
}

void vider_buffers() {
    char tmp[2048];
    for (int i = 0; i < NB_JOUEURS; i++) {
        int flags = fcntl(sockets[i], F_GETFL, 0);
        fcntl(sockets[i], F_SETFL, flags | O_NONBLOCK);
        while (recv(sockets[i], tmp, sizeof(tmp), 0) > 0);
        fcntl(sockets[i], F_SETFL, flags);
    }
}

void envoyer_classement() {
    int ordre[NB_JOUEURS];
    for (int i = 0; i < NB_JOUEURS; i++) ordre[i] = i;

    for (int i = 0; i < NB_JOUEURS - 1; i++)
        for (int j = i+1; j < NB_JOUEURS; j++)
            if (scores[ordre[j]] > scores[ordre[i]]) {
                int tmp = ordre[i]; ordre[i] = ordre[j]; ordre[j] = tmp;
            }

    char classement[1024] = "=== CLASSEMENT ===\n";
    for (int i = 0; i < NB_JOUEURS; i++) {
        char ligne[100];
        sprintf(ligne, "%d. Joueur %d : %d pts\n", i+1, ordre[i]+1, scores[ordre[i]]);
        strcat(classement, ligne);
    }

    QuizMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_RANKING;
    strcpy(msg.data, classement);
    broadcast(&msg);
    printf("%s", classement);
}

void* handle_player(void* arg) {
    ThreadArg *targ = (ThreadArg*)arg;
int idx = targ->idx_joueur;
int q   = targ->idx_question;
    int sock = sockets[idx];

    QuizMessage msg;
    memset(&msg, 0, sizeof(msg));
    recv(sock, &msg, sizeof(msg), 0);

    char reponse = toupper(msg.data[0]);
    char bonne   = questions[q].bonne_reponse;

    printf("Joueur %d a répondu : %c (bonne: %c)\n", idx+1, reponse, bonne);

    pthread_mutex_lock(&players_mutex);
    if (reponse == bonne) {
        scores[idx]++;
        printf("Joueur %d : CORRECT! score=%d\n", idx+1, scores[idx]);
    } else {
        printf("Joueur %d : FAUX\n", idx+1);
    }
    players_answered_count++;
    pthread_mutex_unlock(&players_mutex);

    pthread_cond_signal(&all_answered_cond);
    return NULL;
}

int main() {
    printf("Serveur démarré\n");
    memset(scores, 0, sizeof(scores));

    load_questions("questions.txt");

    int fd_serveur = reseau_creer_serveur(12345);
    if (fd_serveur < 0) return -1;

    printf("En attente de %d joueurs...\n", NB_JOUEURS);
    for (int i = 0; i < NB_JOUEURS; i++) {
    sockets[i] = reseau_accepter_client(fd_serveur, NULL);

    // Lire le pseudo envoyé par le client (MSG_CONNECT)
    QuizMessage tmp;
    memset(&tmp, 0, sizeof(tmp));
    recv(sockets[i], &tmp, sizeof(tmp), 0);
    printf("Joueur %d connecté! Pseudo: %s\n", i+1, tmp.data);
}
    printf("Tous connectés! Le quiz commence!\n");

    QuizMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_START;
    strcpy(msg.data, "Le quiz commence!");
    broadcast(&msg);
    sleep(1);

    for (int q = 0; q < nb_questions; q++) {
        Question *qst = &questions[q];

        vider_buffers();

        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_QUESTION;
        snprintf(msg.data, sizeof(msg.data),
            "Q%d: %s\nA) %s  B) %s  C) %s  D) %s",
            q+1, qst->texte,
            qst->options[0], qst->options[1],
            qst->options[2], qst->options[3]);
        broadcast(&msg);
        printf("Question %d envoyée\n", q+1);

        sleep(1);

        players_answered_count = 0;
        pthread_t threads[NB_JOUEURS];
        ThreadArg args[NB_JOUEURS];
        for (int i = 0; i < NB_JOUEURS; i++) {
            args[i].idx_joueur   = i;
            args[i].idx_question = q;
            pthread_create(&threads[i], NULL, handle_player, &args[i]);
        }

        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += 15;

        pthread_mutex_lock(&players_mutex);
        while (players_answered_count < NB_JOUEURS) {
            int ret = pthread_cond_timedwait(&all_answered_cond, &players_mutex, &deadline);
            if (ret == ETIMEDOUT) { printf("Temps écoulé!\n"); break; }
        }
        pthread_mutex_unlock(&players_mutex);

        for (int i = 0; i < NB_JOUEURS; i++)
            pthread_join(threads[i], NULL);

        envoyer_classement();
        sleep(2);
    }

    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_END;
    strcpy(msg.data, "Quiz terminé! Merci d'avoir joué!");
    broadcast(&msg);

    for (int i = 0; i < NB_JOUEURS; i++)
        close(sockets[i]);
    pthread_mutex_destroy(&players_mutex);
    pthread_cond_destroy(&all_answered_cond);
    close(fd_serveur);
    return 0;
}
