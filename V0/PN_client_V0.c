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
            printf(" ------\n");
            printf(" |    |\n");
            printf(" |\n");
            printf(" |\n");
            printf(" |\n");
            printf(" |\n");
            break;
        case 1:
            printf(" ------\n");
            printf(" |    |\n");
            printf(" |    O\n");
            printf(" |\n");
            printf(" |\n");
            printf(" |\n");
            break;
        case 2:
            printf(" ------\n");
            printf(" |    |\n");
            printf(" |    O\n");
            printf(" |    |\n");
            printf(" |\n");
            printf(" |\n");
            break;
        case 3:
            printf(" ------\n");
            printf(" |    |\n");
            printf(" |    O\n");
            printf(" |   /|\n");
            printf(" |\n");
            printf(" |\n");
            break;
        case 4:
            printf(" ------\n");
            printf(" |    |\n");
            printf(" |    O\n");
            printf(" |   /|\\\n");
            printf(" |\n");
            printf(" |\n");
            break;
        case 5:
            printf(" ------\n");
            printf(" |    |\n");
            printf(" |    O\n");
            printf(" |   /|\\\n");
            printf(" |   /\n");
            printf(" |\n");
            break;
        case 6:
            printf(" ------\n");
            printf(" |    |\n");
            printf(" |    O\n");
            printf(" |   /|\\\n");
            printf(" |   / \\\n");
            printf(" |\n");
            break;
    }

    printf("=========\n\n");
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
    int essais_restants = 0;
    int nb_erreurs = 0;

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

    // recevoir le message de demarrage
    memset(buffer, 0, TAILLE_BUFFER);
    int nb_octets = recv(socket_client, buffer, TAILLE_BUFFER - 1, 0);
    if (nb_octets <= 0) {
        printf("pas de reponse du serveur.\n");
        close(socket_client);
        exit(1);
    }

    // extraire le nombre de lettres
    int longueur_mot = 0;
    if (sscanf(buffer, "start %d", &longueur_mot) != 1) {
        printf("message de demarrage invalide : %s\n", buffer);
        close(socket_client);
        exit(1);
    }

    printf("la partie commence !\n");
    printf("le mot contient %d lettres.\n\n", longueur_mot);

    // initialiser le mot avec des tirets
    for (int i = 0; i < longueur_mot; i++) {
        mot_affiche[i] = '-';
    }
    mot_affiche[longueur_mot] = '\0';

    int partie_terminee = 0;

    // boucle de jeu
    while (!partie_terminee) {
        
        // afficher l'etat du jeu
        dessiner_pendu(nb_erreurs);
        printf("mot : %s\n", mot_affiche);
        printf("entrez une lettre : ");
        fflush(stdout);

        // lire la saisie de l'utilisateur
        char ligne[64];
        if (fgets(ligne, sizeof(ligne), stdin) == NULL) {
            break;
        }

        // extraire la premiere lettre valide
        char lettre = 0;
        for (int i = 0; ligne[i] != '\0'; i++) {
            if (isalpha(ligne[i])) {
                lettre = tolower(ligne[i]);
                break;
            }
        }

        // si pas de lettre valide, on recommence
        if (lettre == 0) {
            printf("veuillez entrer une lettre valide (a-z).\n\n");
            continue;
        }

        // envoyer la lettre au serveur
        sprintf(buffer, "%c\n", lettre);
        send(socket_client, buffer, strlen(buffer), 0);

        // recevoir la reponse du serveur
        memset(buffer, 0, TAILLE_BUFFER);
        nb_octets = recv(socket_client, buffer, TAILLE_BUFFER - 1, 0);
        if (nb_octets <= 0) {
            printf("connexion perdue avec le serveur.\n");
            break;
        }
        buffer[nb_octets] = '\0';

        // analyser la reponse
        char nouveau_mot[TAILLE_BUFFER];
        char mot_secret[TAILLE_BUFFER];

        if (sscanf(buffer, "gagne %s", nouveau_mot) == 1) {
            // victoire
            strcpy(mot_affiche, nouveau_mot);
            dessiner_pendu(nb_erreurs);
            printf("\n=== BRAVO ! ===\n");
            printf("vous avez trouve le mot !\n");
            printf("le mot etait : %s\n\n", mot_affiche);
            partie_terminee = 1;
        }
        else if (sscanf(buffer, "perdu %s %s", nouveau_mot, mot_secret) == 2) {
            // defaite
            nb_erreurs = MAX_ERREURS;
            dessiner_pendu(nb_erreurs);
            printf("\n=== PERDU ! ===\n");
            printf("vous etes pendu...\n");
            printf("le mot etait : %s\n\n", mot_secret);
            partie_terminee = 1;
        }
        else if (sscanf(buffer, "oui %s %d", nouveau_mot, &essais_restants) == 2) {
            // bonne lettre
            strcpy(mot_affiche, nouveau_mot);
            printf("bonne lettre ! (essais restants : %d)\n\n", essais_restants);
        }
        else if (sscanf(buffer, "non %s %d", nouveau_mot, &essais_restants) == 2) {
            // mauvaise lettre
            strcpy(mot_affiche, nouveau_mot);
            nb_erreurs++;
            printf("mauvaise lettre ! (essais restants : %d)\n\n", essais_restants);
        }
        else if (sscanf(buffer, "deja %s %d", nouveau_mot, &essais_restants) == 2) {
            // lettre deja jouee
            printf("lettre deja utilisee ! (essais restants : %d)\n\n", essais_restants);
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