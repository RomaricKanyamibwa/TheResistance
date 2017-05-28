#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include <gtk/gtk.h>

pthread_t server_thread;
int server_thread_ret;
int localServerThreadPortno;
char server_thread_buffer[1000];
int num_du_meneur;
int nb_joueur_participant;
int nom_proposition_meneur[5] = {-1,-1,-1,-1,-1};
char *nom_joueur[5];

char mainServerAddr[100];
char mainServerPort[100];
char localServerAddr[100];
char localServerPort[100];
char username[100];

GtkWidget *labelAddrServer;
GtkWidget *labelPortServer;
GtkWidget *labelPlayer[8];
GtkWidget *rolePlayer[8];
GtkWidget *checkboxPlayer[8];
GtkWidget *votePlayer[8];
GtkWidget *radiovotePlayer[2];
GtkWidget *radiosucceedPlayer[2];
GtkWidget *boutonProposition;
GtkWidget *Meneur[8];

GtkTextBuffer *buffer;
GtkWidget *text_view;
GtkWidget *scrolled_window;

volatile gboolean return_fct_gdk;

/*gtk pas thread safe corriger le machin*/

typedef struct data
{
  GtkWidget *n1;
  GtkWidget *n2;
  gboolean n3;
  char *c1;
  GtkLabel* l1;

}gp;

static gboolean gdk_toggle_button_set_active(gpointer data0)
{
  gp* data = (gp*) data0;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->n1), data->n3);
  free(data);
  return G_SOURCE_REMOVE;
}


static gboolean gdk_toggle_button_get_active(gpointer data0)
{
  gp* data = (gp*) data0;
  return_fct_gdk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->n1));
  free(data0);
  if(return_fct_gdk)
    //printf("TRUETURurteiter\n");
  //printf("GDK Thread zerzerzer\n");
  return G_SOURCE_REMOVE;
}

static gboolean gdk_widget_set_sensitive(gpointer data0)
{
  gp* data = (gp*) data0;
  gtk_widget_set_sensitive (data->n1, data->n3);
  free(data);
  return G_SOURCE_REMOVE;
}

static gboolean gdk_label_set_text(gpointer data0)
{
  gp* data = (gp*) data0;
  char *text=calloc(sizeof(char),100);;
  strcpy(text,data->c1);
  GtkLabel* label=data->l1;
  gtk_label_set_text (label,text);
  free(data);
  return G_SOURCE_REMOVE;

}

static gboolean gdk_ecrire(gpointer data0)
{
  gp* data = (gp*) data0;
  GtkTextIter iter;
  char *phrase_entree=calloc(sizeof(char),256);
  phrase_entree = data->c1;
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
  gtk_text_buffer_insert (buffer, &iter, phrase_entree, -1);
  free(data);
  return G_SOURCE_REMOVE;
}



/*Prototype fonction*/
void sendMessageToMainServer(char *mess);

void *server_func(void *ptr)
{
     int sockfd, newsockfd;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     int n;

     // Cree le socket

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
     {
        printf("ERROR opening socket\n");
	exit(1);
     }

     // Bind le socket avec server_thread_portno

     bzero((char *) &serv_addr, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(localServerThreadPortno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              error("ERROR on binding");

     // Ecoute sur la socket

     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     while (1)
     {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
	{
          printf("ERROR on accept\n");
	  exit(1);
        }

        bzero(server_thread_buffer,256);
        n = read(newsockfd,server_thread_buffer,255);
        if (n < 0)
	{
	  printf("ERROR reading from socket\n");
	  exit(1);
        }

        printf("received from main_server '%s'\n",server_thread_buffer);

	if (server_thread_buffer[0]=='1')
  {
    gp *data = calloc(sizeof(gp),1);
    data->n1 = checkboxPlayer[0];
    data->n3 = TRUE;
    gdk_threads_add_idle (gdk_toggle_button_set_active, (gpointer) data);
  }
	else if (server_thread_buffer[0]=='0')
  {
    gp *data = calloc(sizeof(gp),1);
    data->n1 = checkboxPlayer[0];
    data->n3 = FALSE;
    gdk_threads_add_idle (gdk_toggle_button_set_active, (gpointer) data);
  }
	else if (server_thread_buffer[0]=='2')
  {
		gp *data = calloc(sizeof(gp),1);
        data->n1 = checkboxPlayer[0];
        data->n3 = TRUE;
        gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data);
  }
	else if (server_thread_buffer[0]=='3')
  {
    int connect;
    char phrase[100];

    sscanf ( server_thread_buffer , "%d" , &connect);
    int i;
    for(i=0;i<5;i++)
    {
      gp *data = calloc(sizeof(gp),1);
      data->n1 = checkboxPlayer[i];
      data->n3 = FALSE;
      gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data);
    }

    gp *data = calloc(sizeof(gp),1);
    data->n1 = boutonProposition;
    data->n3 = FALSE;
    gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data);
  }
	else if (server_thread_buffer[0]=='4')
  {
		gp *data = calloc(sizeof(gp),1);
        data->n1 = boutonProposition;
        data->n3 = TRUE;
        gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data);
  }
	else if (server_thread_buffer[0]=='5')
  {
		gp *data = calloc(sizeof(gp),1);
        data->n1 = boutonProposition;
        data->n3 = FALSE;
        gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data);
  }
	else if (server_thread_buffer[0]=='6')
  {
    int connect;
    int num_du_rebelle;
    char phrase[100];

    sscanf ( server_thread_buffer , "%d %d %s" , &connect, &num_du_rebelle, phrase);

    gp *data = calloc(sizeof(gp),1);
    data->l1 = (GtkLabel*)rolePlayer[num_du_rebelle];
    data->c1 = "Rebelle";
    gdk_threads_add_idle(gdk_label_set_text, (gpointer) data);
  }
	else if (server_thread_buffer[0]=='7')
  {
    int connect;
    int num_du_rebelle;
    int num_autre_rebelle;
    char phrase[100];

    sscanf ( server_thread_buffer , "%d %d %d %s" , &connect, &num_du_rebelle, &num_autre_rebelle, phrase);

    gp *data = calloc(sizeof(gp),1);
    data->l1 = (GtkLabel*)rolePlayer[num_du_rebelle];
    data->c1 = "Espion";
    gdk_threads_add_idle(gdk_label_set_text, (gpointer) data);

    gp *data1 = calloc(sizeof(gp),1);
    data1->l1 = (GtkLabel*)rolePlayer[num_autre_rebelle];
    data1->c1 = "Espion";
    gdk_threads_add_idle(gdk_label_set_text, (gpointer) data1);

  }
	else if (server_thread_buffer[0]=='C')
	{
		char connect;
		char nom[100];
		int index;

        printf("Commande C\n");
        sscanf ( server_thread_buffer , "%c %s %d" , &connect, nom, &index);

        printf("nom=%s index=%d\n",nom, index);
        strcpy(nom_joueur[index],nom);

        gp *data = calloc(sizeof(gp),1);
        data->c1=calloc(sizeof(char),100);
        data->l1 = (GtkLabel*)labelPlayer[index];
        strcpy(data->c1,nom);
        gdk_threads_add_idle(gdk_label_set_text, (gpointer) data);
	}
  else if (server_thread_buffer[0]=='8')
  {
    int connect;

    sscanf ( server_thread_buffer , "%d %d %d" , &connect, &num_du_meneur, &nb_joueur_participant);

    int i;
    for(i=0;i<5;i++)
    {
      if(i==num_du_meneur)
      {
         gp *data = calloc(sizeof(gp),1);
         data->l1 = (GtkLabel*)Meneur[i];
         data->c1 = "Meneur";
         gdk_threads_add_idle(gdk_label_set_text, (gpointer) data);
      }

      else
      {
         gp *data = calloc(sizeof(gp),1);
         data->l1 = (GtkLabel*)Meneur[i];
         data->c1 = "";
         gdk_threads_add_idle(gdk_label_set_text, (gpointer) data);
      }

    }

    gp *data = calloc(sizeof(gp),1); //enleve l activation des boutons oui non pour tout le monde
    data->n1 = radiovotePlayer[0];
    data->n3 = FALSE;
    gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data);
    gp *data1 = calloc(sizeof(gp),1);
    data1->n1 = radiovotePlayer[1];
    data1->n3 = FALSE;
    gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data1);


    if(strcmp(nom_joueur[num_du_meneur],username)==0) //si le joueur est le meneur
    {
      gp *data2 = calloc(sizeof(gp),1);
      data2->n1 = boutonProposition;
      data2->n3 = TRUE;
      gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data2);

      int j;

      for(j=0;j<5;j++)
      {
        gp *data3 = calloc(sizeof(gp),1);
        data3->n1 = checkboxPlayer[j];
        data3->n3 = TRUE;
        gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data3);
      }

      char phrase_meneur[256];
      gp *data = calloc(sizeof(gp),1);
      data->c1=calloc(sizeof(char),256);
      sprintf(phrase_meneur,"Cher meneur, c'est à vous de jouer ! Vous devez sélectionner %d joueurs pour la prochaine mission.\n\n", nb_joueur_participant);

      strcpy(data->c1,phrase_meneur);
      gdk_threads_add_idle(gdk_ecrire, (gpointer) data);

    }
  }


  else if (server_thread_buffer[0]=='M' && strcmp(nom_joueur[num_du_meneur],username)!=0) //si le joueur n'est pas le meneur
  {
    char connect;
    char mess[500];
    char *gens[3];
    char gens_cancatenes[256];
    int j;

    int i;
    for(i=0;i<3;i++)
    {
      gens[i] = calloc(sizeof(char),256);

    }



    gp *data = calloc(sizeof(gp),1); //active des boutons oui non
    data->n1 = radiovotePlayer[0];
    data->n3 = TRUE;
    gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data);
    gp *data1 = calloc(sizeof(gp),1);
    data1->n1 = radiovotePlayer[1];
    data1->n3 = TRUE;
    gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data1);


    printf("Commande M\n");
    sscanf ( server_thread_buffer , "%c %d %s %s %s" , &connect, &j, gens[0], gens[1], gens[2]);

    for(i=0;i<j;i++)
    {
        strcat(gens_cancatenes, gens[i]);
        strcat(gens_cancatenes, " ");
    }


    sprintf(mess,"Voici les personnes sélectionnées par le meneur pour la prochaine mission %s. \n Etes-vous d'accord avec son choix ? A vous de voter !\n\n", gens_cancatenes);

    gp *data3 = calloc(sizeof(gp),1);
    data3->c1=calloc(sizeof(char),500);
    strcpy(data3->c1,mess);
    gdk_threads_add_idle(gdk_ecrire, (gpointer) data3);
  }

  else if (server_thread_buffer[0]=='Z')
  {
    char connect;
    char mess[500];

    gp *data = calloc(sizeof(gp),1); //active des boutons oui non
    data->n1 = radiovotePlayer[0];
    data->n3 = TRUE;
    gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data);
    gp *data1 = calloc(sizeof(gp),1);
    data1->n1 = radiovotePlayer[1];
    data1->n3 = TRUE;
    gdk_threads_add_idle(gdk_widget_set_sensitive, (gpointer) data1);

    printf("Commande Zs\n");
    sprintf (mess, "Bravo vous être sélectionné. Si vous voulez que la mission réussisse tapez oui, sinon non.\n\n");
    gp *data2 = calloc(sizeof(gp),1);
    data2->c1=calloc(sizeof(char),500);
    strcpy(data2->c1,mess);
    gdk_threads_add_idle(gdk_ecrire, (gpointer) data2);

  }


  else if(server_thread_buffer[0]=='N') // envoie un message à tout le monde
  {
    char connect;
    GtkTextIter iter;
    char *content=malloc(sizeof(char)*256);

    printf("Commande N\n");
    content = server_thread_buffer;
    content++;
    content++;
    gp *data = calloc(sizeof(gp),1);
    data->c1=calloc(sizeof(char),256);
    strcpy(data->c1,content);
    gdk_threads_add_idle(gdk_ecrire, (gpointer) data);

  }

        close(newsockfd);
     }
     close(sockfd);
}

/*fonction pour proposition d equipe par le meneur*/
void click_boutonProposition(GtkWidget *widget, gpointer window)
{
  GtkTextMark *mark;
  GtkTextIter iter;

	printf("click_boutonProposition\n");
  int i;
  int j=0;
  for(i=0;i<5;i++)
  {
    /*gp *data = calloc(sizeof(gp),1);
    data->n1 = checkboxPlayer[i];
    gdk_threads_add_idle(gdk_toggle_button_get_active, (gpointer) data);
    printf("JOuer Thread 2\n");
    if(return_fct_gdk)*/
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkboxPlayer[i])))
    {
      //printf("TRUTRTURTURTURTURTURTURTURTURTURT\n");
      nom_proposition_meneur[j]=i;
      j++;
    }

  }

  char phrase_entree[256];

  if(j!=nb_joueur_participant)
  {
    if(j==1)
    {
      sprintf(phrase_entree,"Attention tu as rentré %d joueur alors qu'il fallait en rentrer %d. Recommence !\n\n", j ,nb_joueur_participant);
    }
    else
    {
      sprintf(phrase_entree,"Attention tu as rentré %d joueurs alors qu'il fallait en rentrer %d. Recommence !\n\n", j ,nb_joueur_participant);
    }


    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
    gtk_text_buffer_insert (buffer, &iter, phrase_entree, -1);
    return;
  }

  char gens_choisis_par_meneur_pour_server[256];
  sprintf(gens_choisis_par_meneur_pour_server, "E %d ", j);
  char anex[3];

    sprintf(phrase_entree,"\n\n");
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
    gtk_text_buffer_insert (buffer, &iter, phrase_entree, -1);

  for(i=0;i<j;i++)
  {
    sprintf(phrase_entree,"%s\n", nom_joueur[nom_proposition_meneur[i]]);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
    gtk_text_buffer_insert (buffer, &iter, phrase_entree, -1);

    /*message de la listes de gens choisi par le meneur à envoyer au mainserver*/
    sprintf(anex, "%d", nom_proposition_meneur[i]);
    strcat(gens_choisis_par_meneur_pour_server, anex);
    strcat(gens_choisis_par_meneur_pour_server, " ");
  }

 sendMessageToMainServer(gens_choisis_par_meneur_pour_server);

  sprintf(phrase_entree,"Les %d joueurs que %s a seclectionné sont :\n", nb_joueur_participant, nom_joueur[num_du_meneur]);

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
  gtk_text_buffer_insert (buffer, &iter, phrase_entree, -1);

  gtk_widget_set_sensitive (boutonProposition, FALSE);
}



void voteOui(GtkWidget *widget, gpointer window) {
		printf("Oui\n");
    gtk_widget_set_sensitive (  radiovotePlayer[0], FALSE);
    gtk_widget_set_sensitive (  radiovotePlayer[1], FALSE);

    char mess[5];
    sprintf(mess,"V 1");
    sendMessageToMainServer(mess);


}

void voteNon(GtkWidget *widget, gpointer window) {
		printf("Non\n");
        gtk_widget_set_sensitive (  radiovotePlayer[0], FALSE);
    gtk_widget_set_sensitive (  radiovotePlayer[1], FALSE);

    char mess[5];
    sprintf(mess,"V 0");
    sendMessageToMainServer(mess);
}

void sendMessageToMainServer(char *mess)
{
    int sockfd, mainServerPortno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(mainServerAddr);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", mainServerAddr);
        exit(1);
    }

    mainServerPortno=atoi(mainServerPort);

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(mainServerPortno);

    /* connect: create a connection with the server */
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
      error("ERROR connecting");

    /* send the message line to the server */
    n = write(sockfd, mess, strlen(mess));
    if (n < 0)
      error("ERROR writing to socket");

    close(sockfd);

}

int main(int argc, char** argv) {

  GtkWidget *window;
  GtkWidget *table;
  GtkWidget *fixed;

  GtkWidget *entry1;
  GtkWidget *entry2;
  GtkWidget *entry3;
  int i;
  char addr_server_text[50];
  char port_server_text[50];
  char com_connexion[200];

if (argc!=6)
{
	printf("Usage : .joueur @ip_server numport_server @ip_joueur numport_joueur prenom\n");
	exit(1);
}

  printf("mainserver   addr: %s\n", argv[1]);
  printf("mainserver   port: %s\n", argv[2]);
  printf("local server addr: %s\n", argv[3]);
  printf("local server port: %s\n", argv[4]);
  printf("         username: %s\n", argv[5]);

  strcpy(mainServerAddr,argv[1]);
  strcpy(mainServerPort,argv[2]);
  strcpy(localServerAddr,argv[3]);
  strcpy(localServerPort,argv[4]);
  strcpy(username,argv[5]);

  localServerThreadPortno=atoi(argv[4]);

  server_thread_ret = pthread_create( &server_thread, NULL, server_func, NULL);
  if (server_thread_ret)
     {
         fprintf(stderr,"Error - pthread_create() return code: %d\n",server_thread_ret);
         exit(EXIT_FAILURE);
     }

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(window), "The Resistance");
  gtk_container_set_border_width(GTK_CONTAINER(window), 15);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  fixed = gtk_fixed_new();
  gtk_container_add(GTK_CONTAINER(window), fixed);

  sprintf(addr_server_text,"Adresse serveur: %s",argv[1]);
  labelAddrServer = gtk_label_new(addr_server_text);
  gtk_fixed_put(GTK_FIXED(fixed), labelAddrServer, 0, 0);
  gtk_widget_set_size_request(labelAddrServer,200,20);

  sprintf(port_server_text,"Port serveur: %s",argv[2]);
  labelPortServer = gtk_label_new(port_server_text);
  gtk_fixed_put(GTK_FIXED(fixed), labelPortServer, 200, 0);
  gtk_widget_set_size_request(labelPortServer,200,20);


  sprintf(port_server_text,"Nom joueur: %s",argv[5]);
  labelPortServer = gtk_label_new(port_server_text);
  gtk_fixed_put(GTK_FIXED(fixed), labelPortServer, 400, 0);
  gtk_widget_set_size_request(labelPortServer,200,20);

  for (i=0;i<5;i++)
  {
    nom_joueur[i]=calloc(sizeof(char),256); //pour initialiser nom des joueurs
  	labelPlayer[i] = gtk_label_new("Inconnu");
  	gtk_fixed_put(GTK_FIXED(fixed), labelPlayer[i], 0, 100+i*20);
  	gtk_widget_set_size_request(labelPlayer[i],100,20);

        checkboxPlayer[i] = gtk_check_button_new();
	GTK_WIDGET_UNSET_FLAGS(checkboxPlayer[i], GTK_CAN_FOCUS);
  	gtk_fixed_put(GTK_FIXED(fixed), checkboxPlayer[i], 100, 100+i*20);
  	gtk_widget_set_size_request(checkboxPlayer[i],30,20);

  	rolePlayer[i] = gtk_label_new("?");
  	gtk_fixed_put(GTK_FIXED(fixed), rolePlayer[i], 150, 100+i*20);
  	gtk_widget_set_size_request(rolePlayer[i],60,20);

        votePlayer[i] = gtk_label_new("--------");
  	gtk_fixed_put(GTK_FIXED(fixed), votePlayer[i], 210, 100+i*20);
  	gtk_widget_set_size_request(votePlayer[i],60,20);

    Meneur[i] = gtk_label_new("");
    gtk_fixed_put(GTK_FIXED(fixed), Meneur[i], 270, 100+i*20);
    gtk_widget_set_size_request(Meneur[i],60,20);
  }

  boutonProposition = gtk_button_new_with_label("Proposition");//create button
  gtk_fixed_put(GTK_FIXED(fixed), boutonProposition, 400, 100);//position du button
  gtk_widget_set_size_request(votePlayer[i],100,20);//
  g_signal_connect(G_OBJECT(boutonProposition), "clicked",
      G_CALLBACK(click_boutonProposition), G_OBJECT(window));//fonction de call back,

  text_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_fixed_put(GTK_FIXED(fixed), scrolled_window, 10, 400);
  gtk_widget_set_size_request(scrolled_window,400,100);

  radiovotePlayer[0] = gtk_button_new_with_label("Oui");
  gtk_fixed_put(GTK_FIXED(fixed), radiovotePlayer[0], 210, 200);
  gtk_widget_set_size_request(radiovotePlayer[0],60,30);
  g_signal_connect(radiovotePlayer[0], "clicked", G_CALLBACK(voteOui), (gpointer) window);

  radiovotePlayer[1] = gtk_button_new_with_label("Non");
  gtk_fixed_put(GTK_FIXED(fixed), radiovotePlayer[1], 310, 200);
  gtk_widget_set_size_request(radiovotePlayer[1],60,30);
  g_signal_connect(radiovotePlayer[1], "clicked", G_CALLBACK(voteNon), (gpointer) window);

  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  gtk_widget_show_all(window);

  sprintf(com_connexion,"C %s %s %s",localServerAddr,localServerPort,username);
  sendMessageToMainServer(com_connexion);

  gtk_main();

  return 0;
}
