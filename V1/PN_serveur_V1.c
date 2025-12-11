#include <stdio.h>       
#include <stdlib.h>      
#include <string.h>      
#include <unistd.h>   
#include <ctype.h>      
#include <sys/socket.h>  
#include <netinet/in.h>  


#define MOT_SECRET "test"

#define PORT 5000
#define MAX_ERREURS 6
#define TAILLE_MESSAGE 256

// fonction pour creer la liste des lettres utilisees
void creer_liste_lettres(int lettres_utilisees[], char *liste) {
    int pos = 0;
    for (int i = 0; i < 26; i++) {
        if (lettres_utilisees[i]) {
            liste[pos++] = 'a' + i;
        }
    }
    if (pos == 0) {
        liste[0] = '-';
        pos = 1;
    }
    liste[pos] = '\0';
}

int main() {

    // on declare les sockets serveur et les deux joueurs
    int socket_serveur;   
    int socket_joueur1;
    int socket_joueur2;
    
    // cette structure contient l'adresse du serveur (IP + port)
    struct sockaddr_in adresse;
    int taille_adresse = sizeof(adresse);
    
    // buffer = zone memoire pour stocker les messages envoyes/recus
    char buffer[TAILLE_MESSAGE];
    
    // le mot a deviner et sa longueur
    const char *mot = MOT_SECRET;
    int longueur_mot = strlen(mot);
    
    // creation d'un socket pour le serveur, on utilise IPV4 et TCP
    socket_serveur = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_serveur < 0) {
        perror("impossible de creer le socket");
        exit(1); 
    }

    // option pour reutiliser le port immediatement apres arret du serveur
    int option = 1;
    setsockopt(socket_serveur, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // configuration de l'adresse du serveur, on met tous a zero, puis on remplit les champs
    memset(&adresse, 0, sizeof(adresse));
    adresse.sin_family = AF_INET;        
    adresse.sin_addr.s_addr = INADDR_ANY; 
    adresse.sin_port = htons(PORT);       

    // bind = on attache le socket a l'adresse (IP + port)
    if (bind(socket_serveur, (struct sockaddr*)&adresse, sizeof(adresse)) < 0) {
        perror("echec du bind");
        close(socket_serveur);
        exit(1);
    }

    // listen = le serveur commence a ecouter les connexions entrantes (2 joueurs max)
    if (listen(socket_serveur, 2) < 0) {
        perror("echec du listen");
        close(socket_serveur);
        exit(1);
    }

    printf("serveur demarre sur le port %d\n", PORT);

    // le serveur attend 2 clients, joue une partie et recommence
    while (1) {
        
        // accept = on attend que le joueur 1 se connecte
        printf("\nattente du joueur 1...\n");
        socket_joueur1 = accept(socket_serveur, (struct sockaddr*)&adresse, (socklen_t*)&taille_adresse);
        if (socket_joueur1 < 0) {
            perror("echec accept joueur 1");
            continue;
        }
        printf("joueur 1 connecte\n");

        // on envoie "attente X" au joueur 1 
        sprintf(buffer, "attente %d\n", longueur_mot);
        send(socket_joueur1, buffer, strlen(buffer), 0);

        // accept = on attend que le joueur 2 se connecte
        printf("attente du joueur 2...\n");
        socket_joueur2 = accept(socket_serveur, (struct sockaddr*)&adresse, (socklen_t*)&taille_adresse);
        if (socket_joueur2 < 0) {
            perror("echec accept joueur 2");
            close(socket_joueur1);
            continue;
        }
        printf("joueur 2 connecte\n");

        // on envoie "start X" au joueur 2 
        sprintf(buffer, "start %d\n", longueur_mot);
        send(socket_joueur2, buffer, strlen(buffer), 0);

        // mot_affiche = le mot avec des tirets pour les lettres pas trouvees
        char mot_affiche[TAILLE_MESSAGE];
        
        // tableau pour savoir quelles lettres ont deja ete jouees
        int lettres_utilisees[26] = {0};
        
        int lettres_trouvees = 0;
        char liste_lettres[27];
        
        // erreurs separees pour chaque joueur
        int erreurs_j1 = 0;
        int erreurs_j2 = 0;

        // on remplit mot_affiche avec des tirets
        for (int i = 0; i < longueur_mot; i++) {
            mot_affiche[i] = '-';
        }
        mot_affiche[longueur_mot] = '\0';

        printf("la partie commence !\n");

        int partie_terminee = 0;
        int tour = 2;  // le joueur 2 commence

        // boucle de jeu
        while (!partie_terminee) {
            
            // on choisit le socket du joueur actif
            int socket_actif;
            if (tour == 1) {
                socket_actif = socket_joueur1;
            } else {
                socket_actif = socket_joueur2;
            }
            
            // on choisit le socket de l'autre joueur
            int socket_autre;
            if (tour == 1) {
                socket_autre = socket_joueur2;
            } else {
                socket_autre = socket_joueur1;
            }
            
            // on recupere les erreurs du joueur actif
            int *erreurs_actif;
            if (tour == 1) {
                erreurs_actif = &erreurs_j1;
            } else {
                erreurs_actif = &erreurs_j2;
            }
            
            // on recupere les erreurs de l'autre joueur
            int *erreurs_autre;
            if (tour == 1) {
                erreurs_autre = &erreurs_j2;
            } else {
                erreurs_autre = &erreurs_j1;
            }
            
            printf("tour du joueur %d", tour);
            printf("\n \n");

            // on vide le buffer avec memset
            // recv = on recoit un message du joueur actif
            memset(buffer, 0, TAILLE_MESSAGE);
            int nb_octets = recv(socket_actif, buffer, TAILLE_MESSAGE - 1, 0);
            
            // joueur deconnecte
            if (nb_octets <= 0) {
                printf("joueur %d deconnecte\n", tour);
                break;
            }

            // on cherche la premiere lettre valide dans le message et on la met en minuscule
            char lettre = 0;
            for (int i = 0; buffer[i] != '\0'; i++) {
                if (isalpha(buffer[i])) {
                    lettre = tolower(buffer[i]);
                    break;
                }
            }

            // si ce n'est pas une lettre on ignore
            if (lettre == 0) {
                continue;
            }

            printf("joueur %d joue : %c\n", tour, lettre);

            // index_lettre = position dans le tableau (a=0, b=1, c=2, etc.)
            int index_lettre = lettre - 'a';

            // si la lettre a deja ete jouee, on demande de rejouer
            if (lettres_utilisees[index_lettre]) {
                creer_liste_lettres(lettres_utilisees, liste_lettres);
                sprintf(buffer, "deja %s %d %s\n", mot_affiche, MAX_ERREURS - *erreurs_actif, liste_lettres);
                send(socket_actif, buffer, strlen(buffer), 0);
                printf("lettre deja utilisee\n");
                continue;  // meme joueur rejoue
            }

            // on marque la lettre comme utilisee
            lettres_utilisees[index_lettre] = 1;
            creer_liste_lettres(lettres_utilisees, liste_lettres);

            int lettre_trouvee = 0;

            // on parcourt chaque lettre du mot secret, si la lettre est presente on l'affiche
            for (int i = 0; i < longueur_mot; i++) {
                if (tolower(mot[i]) == lettre) {
                    mot_affiche[i] = mot[i];
                    lettre_trouvee = 1;
                    lettres_trouvees++;
                }
            }

            // si mauvaise lettre, on augmente le compteur d'erreurs du joueur
            if (!lettre_trouvee) {
                (*erreurs_actif)++;
                printf("joueur %d : mauvaise lettre ! erreurs : %d/%d\n", tour, *erreurs_actif, MAX_ERREURS);
            } else {
                printf("joueur %d : bonne lettre ! mot : %s\n", tour, mot_affiche);
            }

            // victoire : le joueur a trouve le mot
            if (lettres_trouvees == longueur_mot) {
                // on envoie victoire au joueur actif
                sprintf(buffer, "gagne_mot %s\n", mot_affiche);
                send(socket_actif, buffer, strlen(buffer), 0);
                
                // on envoie defaite a l'autre
                sprintf(buffer, "perdu_autre %s\n", mot);
                send(socket_autre, buffer, strlen(buffer), 0);
                
                printf("joueur %d gagne !\n", tour);
                partie_terminee = 1;
            }
            // defaite : le joueur a fait trop d'erreurs
            else if (*erreurs_actif >= MAX_ERREURS) {
                // on envoie defaite au joueur actif
                sprintf(buffer, "perdu_erreurs %s\n", mot);
                send(socket_actif, buffer, strlen(buffer), 0);
                printf("joueur %d elimine !\n", tour);
                
                // l'autre gagne par defaut
                sprintf(buffer, "gagne_erreurs %s\n", mot);
                send(socket_autre, buffer, strlen(buffer), 0);
                printf("joueur %d gagne par defaut !\n", (tour == 1) ? 2 : 1);
                
                partie_terminee = 1;
            }
            // la partie continue
            else {
                if (lettre_trouvee) {
                    sprintf(buffer, "oui %s %d %s\n", mot_affiche, MAX_ERREURS - *erreurs_actif, liste_lettres);
                } else {
                    sprintf(buffer, "non %s %d %s\n", mot_affiche, MAX_ERREURS - *erreurs_actif, liste_lettres);
                }
                send(socket_actif, buffer, strlen(buffer), 0);

                // on dit a l'autre que c'est son tour
                sprintf(buffer, "tour %s %d %s\n", mot_affiche, MAX_ERREURS - *erreurs_autre, liste_lettres);
                send(socket_autre, buffer, strlen(buffer), 0);

                // on change de joueur
                tour = (tour == 1) ? 2 : 1;
            }
        }
        
        // on ferme les connexions des deux joueurs
        close(socket_joueur1);
        close(socket_joueur2);
        printf("partie terminee. pret pour une nouvelle partie.\n");
    }
    
    close(socket_serveur);
    return 0;
}