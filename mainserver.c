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
int compteurMissions; // pour savoir à quelle mission on est
int compteurVotes; // combien de votes ont été éffectués
int compteurReussites; // combien de vote reussite
int compteurRebelles; // combien de rebelles
int compteurEspions;  // combien d'Espions
int participantsMissions[5]={2,2,3,3,2};	// pour savoir combien de participant à la mission
int Nbparticipants = 1;
char serverbuffer[256];

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

    int envoie_roles = 0;
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
                sprintf(mess,"6 %d Je suis un rebelle", i);
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

void sendMeneur()//il envoie le meneur
{
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
