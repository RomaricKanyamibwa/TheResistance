  #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include <time.h> //Ne pas oublier d'inclure le fichier time.h

# define END 3

int portno;
int nbj;  // compteur joueur == nbj pour commencer
int nbespions;
int meneurCourant;
int compteurJoueurs; // pour compter les joueurs
int compteurMissions = 0; // pour savoir à quelle mission on est
int compteurVotes = 1; // combien de votes ont été éffectués
int compteurVotes_oui = 1; //le meneur vote oui
int compteurVotes_mission = 0; // combien de votes ont été éffectués
int compteurVotes_oui_mission = 0; //le meneur vote oui
int compteurReussites = 0; // combien de mission reussite
int compteurRebelles; // combien de rebelles
int compteurEspions;  // combien d'Espions
int participantsMissions[5]={2,2,3,3,2};	// pour savoir combien de participant à la mission
int Nbparticipants = 1;
char serverbuffer[256];
int voteMeneur_voteMission = 1; //si vote pour le meneur alors variable = 1 sinon variable = 0

char com;
char adrip[20];
int dportno;
char name[20];
char team[10];
//int equipe[10];


/* PRINCIPE :
		attente 'C'
		remplir la structure
		renvoyer le joueur aux autres joueurs
		si compteur == nbj
		fsmstate = 101 ;

*/
struct joueur
{
	char nom[20];
	char ipaddress[20];
	int portno;
	int equipe; // 0=pas dans equipe, 1=dans equipe
	int role; // 0=rebelle, 1=espion
	int vote; // 0=refus, 1=accept
	int reussite; // 0=echec, 1=reussite
} tableauJoueurs[5];
int roles[5];

void sendMessage(int j, char *mess);
void broadcast(char *message);
void sendMeneur();//il envoie le meneur

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void initRoles()
{
}

void *server(void *ptr)
{
     int sockfd, newsockfd;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     int n,i;

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
      /*attribution des roles*/
     int envoie_roles = 0;
     /*envoie meneur*/
    int meneur_bool = 0;

    int nums_equipe_proposee_meneur[3] = {-1,-1,-1};

    while (1)
    {
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);//accept est un appel system bloquant
  	if (newsockfd < 0)
       	error("ERROR on accept");
  	bzero(serverbuffer,256);
  	n = read(newsockfd,serverbuffer,255);
  	if (n < 0) error("ERROR reading from socket");
  	printf("Here is a message from a client: '%s' '%c'\n",serverbuffer,serverbuffer[0]);

	if ( serverbuffer[0] == 'C' && compteurJoueurs < nbj)
	{
		char connect;
		char mess[100];

		printf("Commande C\n");
		sscanf ( serverbuffer , "%c %s %d %s " , &connect ,
			tableauJoueurs[compteurJoueurs].ipaddress ,
			&tableauJoueurs[compteurJoueurs].portno ,
			tableauJoueurs[compteurJoueurs].nom ) ;
        n=compteurJoueurs;
        for(i=0;i<=n;i++)
        {
            sprintf(mess,"C %s %d",tableauJoueurs[i].nom,i);
            compteurJoueurs++;
            broadcast(mess);
            compteurJoueurs--;
        }
		compteurJoueurs++;
	}

    if ( serverbuffer[0] == 'E' && compteurJoueurs == nbj)
    {
        char connect;
        char mess[100];


        int j; //nb joueurs seletionnes par le meneur

        printf("Commande E\n");
        sscanf ( serverbuffer , "%c %d %d %d %d" , &connect , &j, &nums_equipe_proposee_meneur[0], &nums_equipe_proposee_meneur[1], &nums_equipe_proposee_meneur[2]) ;

        sprintf(mess,"M %d ", j);
        int i;

        for(i=0;i<j;i++)
        {
            strcat(mess, tableauJoueurs[nums_equipe_proposee_meneur[i]].nom);
            strcat(mess, " ");
        }
        broadcast(mess);

    }

    if ( serverbuffer[0] == 'V' && voteMeneur_voteMission==1) //vote de l equipe
    {
        char connect;
        int vote;

        printf("Commande V\n");
        sscanf ( serverbuffer , "%c %d" , &connect , &vote);

        compteurVotes++;
        printf("%d %d %d\n",compteurVotes,compteurVotes_oui, nbj );

        if(vote)
        {
            compteurVotes_oui++;
        }
        if(compteurVotes==nbj) //tout le monde à voté
        {
            if(compteurVotes_oui > nbj/2) //le oui gagne
            {
                meneur_bool = 1; //on change pas encore de meneur
                compteurVotes_oui = 1;
                compteurVotes = 1;
                voteMeneur_voteMission = 0; // on passe au vote a l interieur de la mission

                int i;
                char mess[256];
                for(i=0;i<3;i++)
                {
                  printf("%d%d BLABLA\n", nums_equipe_proposee_meneur[0], nums_equipe_proposee_meneur[1]);
                    if(nums_equipe_proposee_meneur[i]!=-1)
                    {
                        broadcast("N Mission en cours...\n");
                        sprintf(mess, "Z \n");
                        sendMessage(nums_equipe_proposee_meneur[i],mess);
                        nums_equipe_proposee_meneur[i] = -1; //on reinitialise pour le vote au tout suivant
                    }
                }
            }
            else //le non gagne
            {
                meneur_bool =0;
                compteurVotes_oui = 1;
                compteurVotes = 1;

            }
        }
        serverbuffer[0] = ' '; //sinon rentre dans V d'après aloirs que pas encore eu de vote
    }


    if ( serverbuffer[0] == 'V' && voteMeneur_voteMission==0) // vote à l interieur de la mission
    {
        char connect;
        int vote;

        printf("Commande V mission\n");
        sscanf ( serverbuffer , "%c %d" , &connect , &vote);

        compteurVotes_mission++;
        printf("%d%d%d\n",compteurVotes_mission,participantsMissions[compteurMissions],compteurVotes_oui_mission );
        if(vote)
        {
            compteurVotes_oui_mission++;
            printf("%d%d%d\n",compteurVotes_mission,participantsMissions[compteurMissions],compteurVotes_oui_mission );
            printf("VOTE\n");
        }
        if(compteurVotes_mission==participantsMissions[compteurMissions]) //tout le monde à voté
        {
            printf("TOUT LE MONDE A VOTE\n");
            printf("%d%d%d\n",compteurVotes_mission,participantsMissions[compteurMissions],compteurVotes_oui_mission );
            if(compteurVotes_oui_mission == participantsMissions[compteurMissions]) //le oui gagne
            {
                printf("LE OUI GANGE\n");
                meneur_bool = 0; //on change de meneur
                compteurVotes_oui_mission = 0;
                compteurVotes_mission = 0;
                voteMeneur_voteMission = 1; // on passe au vote de lequipe
                compteurReussites++;
                compteurMissions++;

                broadcast("N La mission a réussi !\n");
            }
            else //le non gagne
            {
                meneur_bool = 0;//on change de meneur
                compteurVotes_oui_mission = 0;
                compteurVotes_mission = 0;
                voteMeneur_voteMission = 1;
                compteurMissions++;

                broadcast("N La mission a échoué !\n");

            }
        }

    }

    if(compteurJoueurs == nbj && envoie_roles == 0)
    {
        int num_espion[2];
        srand(time(NULL)); // initialisation de rand
        num_espion[0]= rand()%nbj;
        num_espion[1]= rand()%nbj;
        while(num_espion[1]==num_espion[0])
        {
            num_espion[1]= rand()%nbj;
        }

        printf("%d%d\n", num_espion[0], num_espion[1] );

        /* on met le role des gentils*/
        for(i=0;i<nbj;i++)
            tableauJoueurs[i].role = 0;

        /* on met le role des espions*/
        tableauJoueurs[num_espion[0]].role = 1;
        tableauJoueurs[num_espion[1]].role = 1;

        for(i=0;i<nbj;i++)
        {
            char mess[100];
            if(tableauJoueurs[i].role ==0) //gentils
            {
                sprintf(mess,"6 %d Je suis un rebel", i);
                sendMessage(i,mess);
            }
            if(tableauJoueurs[i].role ==1) //espion
            {
                int autre_espion;
                if(num_espion[0]== i)
                    autre_espion = num_espion[1];
                else
                    autre_espion = num_espion[0];
                sprintf(mess,"7 %d %d Je suis un espion et %s l'est aussi", i, autre_espion, tableauJoueurs[autre_espion].nom);
                sendMessage(i,mess);
            }

        }

        envoie_roles = 1;

    }


    if(compteurJoueurs == nbj && meneur_bool==0)
    {
        char message[100];
        sendMeneur();
        meneur_bool =1;
        for (i=0;i<compteurJoueurs;i++)
        {
            if(i!=(meneurCourant-1+nbj)%nbj) //on fait meneurcourant++ dans sendmeneur
            {
                sprintf(message,"3");
                sendMessage(i,message);
            }
        }

    }

    if(compteurReussites == 3)
    {
      broadcast("F Les Rebels ont gagné !");
      close(newsockfd);
      close(sockfd);
      return;


    }
    if(compteurMissions - compteurReussites == 3)
    {
      broadcast("F Les Espions ont gagné !");
      close(newsockfd);
      close(sockfd);
      return;
    }


  	close(newsockfd);
     }
     close(sockfd);
    }


void sendMessage(int j, char *mess)
{
        int sockfd, n;
        struct sockaddr_in serv_addr;
        struct hostent *playerserver;
        char buffer[256];

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
                error("ERROR opening socket");
        playerserver = gethostbyname(tableauJoueurs[j].ipaddress);
        if (playerserver == NULL) {
                fprintf(stderr,"ERROR, no such host\n");
                exit(0);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)playerserver->h_addr, (char *)&serv_addr.sin_addr.s_addr,
                                playerserver->h_length);
        serv_addr.sin_port = htons(tableauJoueurs[j].portno);
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
                        error("ERROR connecting");

        n = write(sockfd,mess,strlen(mess));
        if (n < 0)
                error("ERROR writing to socket");
        close(sockfd);
}

void broadcast(char *message)
{
        int i;

        printf("broadcast %s\n",message);
        for (i=0;i<compteurJoueurs;i++)
                sendMessage(i,message);
}

void sendRoles()//il envoie le
{
}

void sendMeneur()//il envoie le meneur et le nombre de joueur qu'il devra choisir pour la mission
{
    char message[100];
    sprintf(message,"8 %d %d", meneurCourant, participantsMissions[compteurMissions]);
    broadcast(message);
    meneurCourant = (meneurCourant+1)%nbj;

}


void sendEquipe()//forme une equipe
{
}

void sendChosenOnes()//envoyer les elus
{
}

int main(int argc, char *argv[])
{
     pthread_t thread1, thread2;
     int  iret1, iret2;
     meneurCourant = 0;

	if (argc!=3)
	{
		printf("Usage : ./mainserver nbjoueurs numport\n");
		exit(1);
	}

     com='0';
     nbj=atoi(argv[1]);
     printf("Nombre de joueurs=%d\n",nbj);
     portno=atoi(argv[2]);
     printf("Serveur ecoute sur port %d\n",portno);
     compteurJoueurs=0;

     compteurRebelles=0;
     compteurEspions=0;
     compteurMissions=0;
     meneurCourant = 0;
     nbespions=1;
     initRoles();

    /* Create independent threads each of which will execute function */

     iret1 = pthread_create( &thread1, NULL, server, NULL);
     if(iret1)
     {
         fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
         exit(EXIT_FAILURE);
     }

     printf("pthread_create() for thread 1 returns: %d\n",iret1);

     /* Wait till threads are complete before main continues. Unless we  */
     /* wait we run the risk of executing an exit which will terminate   */
     /* the process and all threads before the threads have completed.   */

     pthread_join( thread1, NULL);

     exit(0);
}
