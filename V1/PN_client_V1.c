#include <stdio.h>    
#include <stdlib.h>     
#include <string.h>    
#include <unistd.h>    
#include <ctype.h>       
#include <sys/socket.h>  
#include <arpa/inet.h>   

// constantes du programme
#define TAILLE_BUFFER 256
#define MAX_ERREURS 6

// fonction pour dessiner le pendu
void dessiner_pendu(int erreurs) {
    printf("\n");
    switch(erreurs) {
        case 0:
            printf(" ------\n |    |\n |\n |\n |\n |\n");
            break;
        case 1:
            printf(" ------\n |    |\n |    O\n |\n |\n |\n");
            break;
        case 2:
            printf(" ------\n |    |\n |    O\n |    |\n |\n |\n");
            break;
        case 3:
            printf(" ------\n |    |\n |    O\n |   /|\n |\n |\n");
            break;
        case 4:
            printf(" ------\n |    |\n |    O\n |   /|\\\n |\n |\n");
            break;
        case 5:
            printf(" ------\n |    |\n |    O\n |   /|\\\n |   /\n |\n");
            break;
        case 6:
            printf(" ------\n |    |\n |    O\n |   /|\\\n |   / \\\n |\n");
            break;
    }
    printf("=========\n\n");
}

// fonction pour afficher les lettres utilisees
void afficher_lettres(char *lettres) {
    printf("lettres utilisees : ");
    if (lettres[0] == '-') {
        printf("aucune");
    } else {
        for (int i = 0; lettres[i] != '\0'; i++) {
            printf("%c ", lettres[i]);
        }
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    
    // verifier les arguments (ip et port)
    if (argc != 3) {
        printf("usage : %s <adresse_ip> <port>\n", argv[0]);
        printf("exemple : %s 127.0.0.1 5000\n", argv[0]);
        exit(1);
    }

    // recuperer ip et port
    char *serveur_ip = argv[1];
    int port = atoi(argv[2]);

    // declaration des variables
    int socket_client;
    struct sockaddr_in adresse;
    char buffer[TAILLE_BUFFER];
    char mot_affiche[TAILLE_BUFFER];
    char lettres_utilisees[27] = "-";
    int essais_restants = MAX_ERREURS;
    int nb_erreurs = 0;
    int mon_tour = 0;

    // creation du socket TCP/IPv4
    socket_client = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_client < 0) {
        perror("impossible de creer le socket");
        exit(1);
    }

    // configuration de l'adresse du serveur
    memset(&adresse, 0, sizeof(adresse));
    adresse.sin_family = AF_INET;
    adresse.sin_port = htons(port);

    // convertir l'adresse ip en binaire
    if (inet_pton(AF_INET, serveur_ip, &adresse.sin_addr) <= 0) {
        perror("adresse IP invalide");
        close(socket_client);
        exit(1);
    }

    // connexion au serveur
    printf("connexion a %s:%d...\n", serveur_ip, port);
    if (connect(socket_client, (struct sockaddr*)&adresse, sizeof(adresse)) < 0) {
        perror("connexion au serveur impossible");
        close(socket_client);
        exit(1);
    }
    printf("connecte au serveur !\n\n");

    // recevoir le premier message du serveur
    memset(buffer, 0, TAILLE_BUFFER);
    int nb_octets = recv(socket_client, buffer, TAILLE_BUFFER - 1, 0);
    if (nb_octets <= 0) {
        printf("pas de reponse du serveur.\n");
        close(socket_client);
        exit(1);
    }

    int longueur_mot = 0;

    // analyser le message : "attente X" ou "start X"
    if (sscanf(buffer, "attente %d", &longueur_mot) == 1) {
        //  joueur 1 attendre
        printf("en attente d'un autre joueur...\n");
        printf("le mot contient %d lettres.\n\n", longueur_mot);
        mon_tour = 0;
    }
    else if (sscanf(buffer, "start %d", &longueur_mot) == 1) {
        // joueur 2 commence
        printf("la partie commence ! c'est votre tour.\n");
        printf("le mot contient %d lettres.\n\n", longueur_mot);
        mon_tour = 1;
    }
    else {
        printf("message invalide : %s\n", buffer);
        close(socket_client);
        exit(1);
    }

    // initialiser le mot avec des tirets
    for (int i = 0; i < longueur_mot; i++) {
        mot_affiche[i] = '-';
    }
    mot_affiche[longueur_mot] = '\0';

    int partie_terminee = 0;

    // boucle de jeu
    while (!partie_terminee) {
        
        if (mon_tour) {
            // c'est mon tour : afficher et jouer
            dessiner_pendu(nb_erreurs);
            printf("mot : %s\n", mot_affiche);
            printf("erreurs restantes : %d\n", essais_restants);
            afficher_lettres(lettres_utilisees);
            printf("\nentrez une lettre ou le mot complet : ");
            fflush(stdout);

            // lire la saisie de l'utilisateur
            char ligne[64];
            if (fgets(ligne, sizeof(ligne), stdin) == NULL) {
                break;
            }

            // extraire toutes les lettres valides
            char proposition[64];
            int len = 0;
            for (int i = 0; ligne[i] != '\0' && ligne[i] != '\n'; i++) {
                if (isalpha(ligne[i])) {
                    proposition[len++] = tolower(ligne[i]);
                }
            }
            proposition[len] = '\0';

            // si pas de lettre valide, on recommence
            if (len == 0) {
                printf("veuillez entrer une lettre ou un mot valide.\n\n");
                continue;
            }

            // envoyer la proposition au serveur (lettre ou mot)
            sprintf(buffer, "%s\n", proposition);
            send(socket_client, buffer, strlen(buffer), 0);
        }
        else {
            // pas mon tour : attendre l'autre joueur
            dessiner_pendu(nb_erreurs);
            printf("mot : %s\n", mot_affiche);
            printf("erreurs restantes : %d\n", essais_restants);
            afficher_lettres(lettres_utilisees);
            printf("\nen attente de l'autre joueur...\n");
        }

        // recevoir la reponse du serveur
        memset(buffer, 0, TAILLE_BUFFER);
        nb_octets = recv(socket_client, buffer, TAILLE_BUFFER - 1, 0);
        if (nb_octets <= 0) {
            printf("connexion perdue avec le serveur.\n");
            break;
        }
        buffer[nb_octets] = '\0';

        // variables pour analyser la reponse
        char nouveau_mot[TAILLE_BUFFER];
        char nouvelles_lettres[27];

        // victoire : j'ai trouve le mot
        if (sscanf(buffer, "gagne_mot %s", nouveau_mot) == 1) {
            strcpy(mot_affiche, nouveau_mot);
            dessiner_pendu(nb_erreurs);
            printf("\n=== VICTOIRE ! ===\n");
            printf("vous avez trouve le mot : %s\n\n", mot_affiche);
            partie_terminee = 1;
        }
        // victoire : l'adversaire a fait trop d'erreurs
        else if (sscanf(buffer, "gagne_erreurs %s", nouveau_mot) == 1) {
            dessiner_pendu(nb_erreurs);
            printf("\n=== VICTOIRE ! ===\n");
            printf("votre adversaire a fait trop d'erreurs !\n");
            printf("le mot etait : %s\n\n", nouveau_mot);
            partie_terminee = 1;
        }
        // defaite : j'ai fait trop d'erreurs
        else if (sscanf(buffer, "perdu_erreurs %s", nouveau_mot) == 1) {
            nb_erreurs = MAX_ERREURS;
            dessiner_pendu(nb_erreurs);
            printf("\n=== DEFAITE ! ===\n");
            printf("vous avez fait trop d'erreurs !\n");
            printf("le mot etait : %s\n\n", nouveau_mot);
            partie_terminee = 1;
        }
        // defaite : l'adversaire a trouve le mot
        else if (sscanf(buffer, "perdu_autre %s", nouveau_mot) == 1) {
            dessiner_pendu(nb_erreurs);
            printf("\n=== DEFAITE ! ===\n");
            printf("votre adversaire a trouve le mot !\n");
            printf("le mot etait : %s\n\n", nouveau_mot);
            partie_terminee = 1;
        }
        // bonne lettre
        else if (sscanf(buffer, "oui %s %d %s", nouveau_mot, &essais_restants, nouvelles_lettres) == 3) {
            strcpy(mot_affiche, nouveau_mot);
            strcpy(lettres_utilisees, nouvelles_lettres);
            nb_erreurs = MAX_ERREURS - essais_restants;
            printf("bonne lettre ! c'est au tour de l'autre joueur.\n\n");
            mon_tour = 0;
        }
        // mauvaise lettre
        else if (sscanf(buffer, "non %s %d %s", nouveau_mot, &essais_restants, nouvelles_lettres) == 3) {
            strcpy(mot_affiche, nouveau_mot);
            strcpy(lettres_utilisees, nouvelles_lettres);
            nb_erreurs = MAX_ERREURS - essais_restants;
            printf("mauvaise lettre ! c'est au tour de l'autre joueur.\n\n");
            mon_tour = 0;
        }
        // lettre deja jouee
        else if (sscanf(buffer, "deja %s %d %s", nouveau_mot, &essais_restants, nouvelles_lettres) == 3) {
            strcpy(lettres_utilisees, nouvelles_lettres);
            printf("lettre deja utilisee ! rejouez.\n\n");
            mon_tour = 1;
        }
        // c'est mon tour
        else if (sscanf(buffer, "tour %s %d %s", nouveau_mot, &essais_restants, nouvelles_lettres) == 3) {
            strcpy(mot_affiche, nouveau_mot);
            strcpy(lettres_utilisees, nouvelles_lettres);
            nb_erreurs = MAX_ERREURS - essais_restants;
            printf("c'est votre tour !\n\n");
            mon_tour = 1;
        }
        else {
            printf("reponse serveur : %s\n", buffer);
        }
    }

    // fermer la connexion
    close(socket_client);
    printf("deconnecte du serveur.\n");

    return 0;
}