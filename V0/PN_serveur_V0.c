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


int main() {

    // on declare les sockets serveur et client
    int socket_serveur;   
    int socket_client;     
    
    // cette structure contient l'adresse du serveur (IP + port)
    struct sockaddr_in adresse;
    int taille_adresse = sizeof(adresse);
    
    // buffer = zone memoire pour stocker les messages envoyes/recus
    char buffer[TAILLE_MESSAGE];
    
    // le mot a deviner et sa longueur
    const char *mot = MOT_SECRET;
    int longueur_mot = strlen(mot);
    
    // creation d'un socket pour le serveur , on utilise IPV4 et TCP
    socket_serveur = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_serveur < 0) {
        perror("impossible de creer le socket");
        exit(1); 
    }

    // option pour reutiliser le port immediatement apres arret du serveur
    int option = 1;
    setsockopt(socket_serveur, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // configuration de l'adresse du serveur,on met tous a zero, puis on remplit les champs
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

    // listen = le serveur commence a ecouter les connexions entrantes
    if (listen(socket_serveur, 1) < 0) {
        perror("echec du listen");
        close(socket_serveur);
        exit(1);
    }

    // Le serveur attend un client, joue une partie a chaque connexion et recommence
    while (1) {
        
        printf("\nattente d'un client...\n");

        // accept = on attend qu'un client se connecte
        socket_client = accept(socket_serveur, (struct sockaddr*)&adresse, (socklen_t*)&taille_adresse);
        if (socket_client < 0) {
            perror("echec de accept");
            continue;
        }

        printf("client connecte \n");

        // mot_affiche = le mot avec des tirets pour les lettres pas trouvees
        char mot_affiche[TAILLE_MESSAGE];
        
        // tableau pour savoir quelles lettres ont deja ete jouees
        int lettres_utilisees[26] = {0};
        
        int nb_erreurs = 0;    
        int lettres_trouvees = 0; 

        // on remplit mot_affiche avec des tirets
        for (int i = 0; i < longueur_mot; i++) {
            mot_affiche[i] = '-';
        }
        mot_affiche[longueur_mot] = '\0';

        
        // on envoie "start X" au client, ou X = nombre de lettres
        
        sprintf(buffer, "start %d\n", longueur_mot);
        send(socket_client, buffer, strlen(buffer), 0);
        printf("envoye : start %d\n", longueur_mot);

        int partie_terminee = 0;
        while (!partie_terminee) {
            
            // on vide le buffer avec memset
            // recv = on recoit un message du client 
            memset(buffer, 0, TAILLE_MESSAGE);
            int nb_octets = recv(socket_client, buffer, TAILLE_MESSAGE - 1, 0);
            if (nb_octets <= 0) {
                printf("client deconnecte.\n");
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

            // si ce n'est pas un lettre on ignore
            if (lettre == 0) {
                continue;
            }

            printf("lettre recue : %c\n", lettre);

            // index_lettre = position dans le tableau (a=0, b=1, c=2, etc.)
            int index_lettre = lettre - 'a';

            // si la lettre a deja ete jouee, on demande de rejouer
            if (lettres_utilisees[index_lettre]) {
                sprintf(buffer, "deja %s %d\n", mot_affiche, MAX_ERREURS - nb_erreurs);
                send(socket_client, buffer, strlen(buffer), 0);
                printf("lettre deja utilisee.\n");
                continue;
            }

            // on marque la lettre comme utilisee
            lettres_utilisees[index_lettre] = 1;
            
            int lettre_trouvee = 0;

            // on parcourt chaque lettre du mot secret si la lettre est presente on l'affiche
            for (int i = 0; i < longueur_mot; i++) {
                if (tolower(mot[i]) == lettre) {
                    mot_affiche[i] = mot[i];
                    lettre_trouvee = 1;
                    lettres_trouvees++;
                }
            }
        
            // si mauvaise lettre, on augmente le compteur d'erreurs
            if (!lettre_trouvee) {
                nb_erreurs++;
                printf("mauvaise lettre ! erreurs : %d/%d\n", nb_erreurs, MAX_ERREURS);
            } else {
                printf("bonne lettre ! mot : %s\n", mot_affiche);
            }
            
            //defaite du joueur
            if (nb_erreurs >= MAX_ERREURS) {
                sprintf(buffer, "perdu %s %s\n", mot_affiche, mot);
                send(socket_client, buffer, strlen(buffer), 0);
                printf("partie perdue ! le mot etait : %s\n", mot);
                partie_terminee = 1;
            }
            //victoire du joueur
            else if (lettres_trouvees == longueur_mot) {
                sprintf(buffer, "gagne %s\n", mot_affiche);
                send(socket_client, buffer, strlen(buffer), 0);
                printf("partie gagnee !\n");
                partie_terminee = 1;
            }
            else {
                // la partie continue
                if (lettre_trouvee) {
                    sprintf(buffer, "oui %s %d\n", mot_affiche, MAX_ERREURS - nb_erreurs);
                } else {
                    sprintf(buffer, "non %s %d\n", mot_affiche, MAX_ERREURS - nb_erreurs);
                }
                send(socket_client, buffer, strlen(buffer), 0);
            }
        }
        
        close(socket_client);
        printf("connexion fermee. pret pour une nouvelle partie.\n");
    }
    
    close(socket_serveur);
    return 0;
}