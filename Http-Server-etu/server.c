#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "main_http.h"

// Le buffer circulaire

typedef struct
{
	int * buffer;
	int   buf_in;
	int   buf_out;
	int   buffer_max;
}bounded_buffer_t;

bounded_buffer_t bb;

void init_buffer(int buffer_max)
{
	bb.buffer = (int*)malloc(sizeof(int)*buffer_max);
	bb.buffer_max =buffer_max;
}

int buffer_add(int item)
{
	if(bb.buf_in == INT_MAX) bb.buf_in = 0;
	if((bb.buf_in - bb.buf_out) >= 0 && abs(bb.buf_in - bb.buf_out) < bb.buffer_max)
	{
		bb.buffer[ bb.buf_in % bb.buffer_max ] = item;
		bb.buf_in ++;
		return 1;
	}
	return 0;
}

int buffer_get()
{
	if(bb.buf_out == INT_MAX) bb.buf_out = 0;
	if((bb.buf_in - bb.buf_out) > 0 && abs(bb.buf_in - bb.buf_out) < bb.buffer_max)
	{
		int peer_sfd = bb.buffer[ bb.buf_out % bb.buffer_max ];
		bb.buf_out++;
		return peer_sfd;
	}
	return -1;
}

int is_buffer_full()
{
	if(abs(bb.buf_out - bb.buf_in) == bb.buffer_max-1)
		return 1;
	return 0;
}

int is_buffer_empty()
{
	if(bb.buf_in == bb.buf_out)
		return 1;
	return 0;
}

void destroy_buffer()
{
	free(bb.buffer);
}

// La fonction d'affichage de messages

int print(char * fmt, ...)
{
        int print_count;
        va_list myargs;
        va_start(myargs,fmt);
        print_count = vfprintf(stderr,fmt,myargs);
        va_end(myargs);
        return print_count;
}

// Des fonctions utilitaires

int check_file_exists(const char * path)
{
	FILE * fp;
	fp=fopen(path,"r");
	if (fp)
	{
		fclose(fp);
		return 1;
	}
	return 0;
}

int file_size(const char *path)
{
	if(!check_file_exists(path)) exit(0);

	struct stat st;
	stat(path,&st);
	return st.st_size;
}

int check_folder_exists(const char *path)
{
	struct stat st;
	if(lstat(path,&st)<0)
	{	print("ROOT DIRECTORY DOES NOT EXISTS");
		return 0;
	}
	if(S_ISDIR(st.st_mode))
		return 1;
	return 0;
}

int set_index(char *path)
{
	struct stat st;
	if(lstat(path,&st)<0)
	{	perror("");
		return -1;
	}
	if(S_ISDIR(st.st_mode))  strcat(path,"/index.html");
	return 1;
}

void trim_resource(char * resource_location)
{
	if(strstr(resource_location,"#")) strcpy(strpbrk(resource_location,"#"),"");
	if(strstr(resource_location,"?")) strcpy(strpbrk(resource_location,"?"),"");
}

// Les variables globales

static  char  	      path_root[PATH_MAX] = CURRENT_DIRECTORY;
static  int   	      port_number         = DEFAULT_PORT_NUMBER;
typedef void 	      (*strategy_t)(int);
static  int   	      buffer_max          = DEFAULT_BUFFER_SIZE;
static  int   	      worker_max          = DEFAULT_WORKER_SIZE;
static  sig_atomic_t  status_on           = True;
char  	      strategy_name[STRATEGY_MAX];
pthread_mutex_t buffer_lock_mtx          = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  buffer_not_full_cond      = PTHREAD_COND_INITIALIZER;
pthread_cond_t  buffer_not_empty_cond     = PTHREAD_COND_INITIALIZER;

const char* get_extension(const char *path)
{
	if(strstr(path,".HTML") || strstr(path,".html")) return "text/html";
	if(strstr(path,".JPEG") || strstr(path,".jpeg")) return "image/jpeg";
	if(strstr(path,".PNG" ) || strstr(path,".png" )) return "image/png";
	if(strstr(path,".TXT" ) || strstr(path,".txt" )) return "text";
	if(strstr(path,".JPG" ) || strstr(path,".jpg" )) return "image/jpg";
	if(strstr(path,".CSS" ) || strstr(path,".css" )) return "text/css";
	if(strstr(path,".JS"  ) || strstr(path,".js"  )) return "application/javascript";
	if(strstr(path,".XML" ) || strstr(path,".xml" )) return "application/xml";
	if(strstr(path,".MP3" ) || strstr(path,".mp3" )) return "audio/mpeg";
	if(strstr(path,".MPEG") || strstr(path,".mpeg")) return "video/mpeg";
	if(strstr(path,".MPG" ) || strstr(path,".mpg" )) return "video/mpeg";
	if(strstr(path,".MP4" ) || strstr(path,".mp4" )) return "video/mp4";
	if(strstr(path,".MOV" ) || strstr(path,".mov" )) return "video/quicktime";
	return "text/html";
}

static int next_request(int fd,http_request_t *request)
{
	char   command_line  [MAX_HEADER_LINE_LENGTH];
	char   method        [MAX_METHODS];
	char   payload_source[PATH_MAX];
	int    minor_version;
	int    major_version;
	char   head_name     [MAX_HEADER_LINE_LENGTH];
	char   head_value    [MAX_HEADER_VALUE_LENGTH];
	int    head_count = 0;
	FILE * fr;
	fr = fdopen(dup(fd),"r");

puts("next request");

	if(fr)
	{
		fgets(command_line,MAX_HEADER_LINE_LENGTH,fr);
		sscanf(command_line, "%s %s HTTP/%d.%d%*s",method,payload_source,&major_version,&minor_version);

		if(strcmp(method,"GET")==0)
		{
			request->method        = HTTP_METHOD_GET;
			trim_resource(payload_source);
			strcpy(request->uri,payload_source);
			request->major_version = major_version;
			request->minor_version = minor_version;
		}
		else
		{
			request->method        = HTTP_STATUS_NOT_IMPLEMENTED;
		}
	}
	while(head_count < MAX_HEADERS)
	{
		fgets(command_line,MAX_HEADER_LINE_LENGTH,fr);

		if(strstr(command_line,":"))
		{
			sscanf(command_line,"%s: %s",head_name,head_value);

			strcpy(request->headers[head_count].field_name,head_name);
			strcpy(request->headers[head_count++].field_value,head_value);
		}
		else
			break;
	}

	request->header_count=head_count;
	fclose(fr);
	return 1;
}

static int set_response_field_name_and_value(http_response_t *response,const char *name,const char *value)
{
	strcpy(response->headers[response->header_count].field_name,name);
	strcpy(response->headers[response->header_count++].field_value,value);
	return 1;
}

static int handle_error(http_status_t status,char * error_resource)
{
	if(status.code == HTTP_STATUS_LOOKUP[HTTP_STATUS_OK].code)	return 1;
	if(status.code == HTTP_STATUS_LOOKUP[HTTP_STATUS_NOT_FOUND].code)
	{
		strcpy(error_resource,path_root);
		strcat(error_resource,ERROR_NOT_FOUND_404);

		if(!check_file_exists(error_resource))
		{
			strcpy(error_resource,path_root);
			strcat(error_resource,ERROR_BAD_REQUEST_400);
		}
		if(!check_file_exists(error_resource))
		{
			strcpy(error_resource,DEFUALT_ERROR_NOT_FOUND_404);
		}
	}
	return 0;
}

static http_status_t check_response_status(const int status,const char * path)
{
	if(status == HTTP_STATUS_NOT_IMPLEMENTED) return HTTP_STATUS_LOOKUP[status];
	if(!check_file_exists(path))              return HTTP_STATUS_LOOKUP[HTTP_STATUS_NOT_FOUND];
	return HTTP_STATUS_LOOKUP[HTTP_STATUS_OK];
}

static int build_response(const http_request_t *request,http_response_t *response)
{
	char buffer[MAX_HEADER_VALUE_LENGTH];
	int head_count=0;
	time_t now=0;
	struct tm *t;
	strcat(response->resource_path,request->uri);
	set_index(response->resource_path);

	response->status        = check_response_status(request->method,response->resource_path);
	handle_error(response->status, response->resource_path);

	response->major_version = request->major_version;
	response->minor_version = request->minor_version;

	now = time(NULL);
	t = gmtime(&now);
	strftime(buffer,30,"%a, %d %b %Y %H:%M:%S %Z",t);

	set_response_field_name_and_value(response,"Date",buffer);
	set_response_field_name_and_value(response,"Server","MAIN HTTP");
	set_response_field_name_and_value(response,"Content-Type",get_extension(response->resource_path));
	printf("response=%s\n",response->resource_path);
	sprintf(buffer,"%d",file_size(response->resource_path));
	set_response_field_name_and_value(response,"Content-Length",buffer);

	return 1;
}

static int send_response(int fd,const http_response_t *response)
{
	time_t now;
	struct tm t;
	FILE   *fr;
	char   buf[MAX_HEADER_VALUE_LENGTH];
	FILE   *fp = fdopen(dup(fd),"w");
	size_t size;
	int    head_no;
	int    ch;
	fprintf(fp,"HTTP/%d.%d %d %s\r\n",response->major_version,response->minor_version,response->status.code,response->status.reason);
	for(head_no = 0; head_no<response->header_count; head_no++)
	{
		fprintf(fp,"%s: %s\r\n",response->headers[head_no].field_name,response->headers[head_no].field_value);
	}
	fprintf(fp,"\n");
	fr=fopen(response->resource_path,"r");  //print payload
	if(fr)
	{
		while((ch=getc(fr))!=EOF)  fprintf(fp,"%c",ch);
		fclose(fr);
	}
	fclose(fp);
	return 1;
}

static int clear_responses(http_response_t *response)//clearing responses avoids possiblity of duplicate headers err
{
	int head_no;
	for(head_no =0; head_no<response->header_count; head_no++)
	{
		strcpy(response->headers[head_no].field_name,"");
		strcpy(response->headers[head_no].field_value,"");
	}
	response->header_count = 0;
	return 1;
}

static void manage_single_request(int peer_sfd)//gestion d'une requete
{
	http_request_t  *request  = (http_request_t*)malloc(sizeof(http_request_t));
	http_response_t *response = (http_response_t*)malloc(sizeof(http_response_t));
	strcpy(response->resource_path,path_root);

	next_request(peer_sfd, request);//on recupere la requete
	build_response(request, response);//construire la reponse
	send_response(peer_sfd, response);//envoyer la reponse

	clear_responses(response);
	free(request);
	free(response);
}

static void perform_process_operation(int sfd)
{
	print("PERFORMING USING FORK");
	while(status_on)
	{
        //while(1)
		// Rajouter du code ici
		//accept (la fonction accept envoie une copie de sfd)
		//fork
		//fils va faire manage request sur le resulat d'accept
	}
}
static void perform_serially(int sfd)
{
	print("\nPERFORMING SERIALLY");
	while(status_on)
	{
		// Rajouter du code ici
		//faire accept
		//manage request
		//ferme le socket
		int peer_fd=accept(sfd,NULL,NULL);
		if(peer_fd==-1)
        {
            perror("ACCEPT error during perform serially");
            return;
        }
        manage_single_request(peer_fd);
        close(peer_fd);
	}
}

static void producer_operation(int sfd)
{
	while(status_on)
	{
		// Rajouter du code ici
	}
	destroy_buffer();
}

static void* consumer_operation()
{
	while(status_on)
	{
		// Rajouter du code ici
	}
	pthread_exit(pthread_self);
}

static void perform_thread_pool_operation(int sfd)
{
	print("PERFORMING THREAD POOL");
	int worker_no = 0;
	pthread_t tid;
	init_buffer(buffer_max);

	for(worker_no = 0 ; worker_no < worker_max; worker_no++)
	{
		pthread_create(&tid,NULL,consumer_operation,NULL);
	}

	producer_operation(sfd);
}

static void* manage_request_response_per_thread(void *peer_sfd)
{
	manage_single_request(*(int*)peer_sfd);
	close(*(int*)peer_sfd);
	free((int*)peer_sfd);
	pthread_exit(pthread_self);
}

static void perform_thread_operation(int sfd)
{
     	print("PERFORMING USING THREADS");
	pthread_t tid;
	while(status_on)
	{
		// Rajouter du code ici
	}
}

void graceful_shutdown(int sig)
{
	status_on = False;
}

int initialize_server()
{
	struct sockaddr_in myaddr;
	int    sfd;
	int    optval = 1;
	//sfd => socket file descriptor

	sfd = socket(AF_INET,SOCK_STREAM,0);    //creating socket

	if(sfd == -1)
	{
		print("socket");
		exit(0);
	}

	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)// setsockopt nous permet de reutiliser le meme port
		print("\nsetsockopt");

	memset(&myaddr,0,sizeof(myaddr));
	myaddr.sin_family      = AF_INET;
	myaddr.sin_port        = htons(port_number);
	myaddr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sfd, (struct sockaddr*) &myaddr, sizeof(myaddr)) == -1)
	{
		print("PORT NO NOT FOUND GLOBAL ERROR STATUS IS:");
		exit(0);
	}
	if(listen(sfd,BACKLOG)==-1)
		print("\nLISTEN FAILED");

	return sfd;
}

strategy_t configure_server(int argc,char *argv[])
{
	int option,option_count=0;
	strategy_t operation;

	while((option = getopt(argc,argv,"p:d:w:q:ft:"))!=-1)
	{
		switch (option)
		{
			case 'p':
				port_number = atoi(optarg);//recuperer le numero de port
				break;
			case 'd':
				strcpy(path_root,optarg);//dossier de fichiers html
				break;
			case 't'://mode thread
				//strcpy(strategy_name,"Threads");
				operation = &perform_thread_operation;
				option_count++;
				break;
			case 'f'://mode fork
				//strcpy(strategy_name,"Fork(Using processes)");
				operation = perform_process_operation;
				option_count++;
				break;
			case 'w'://mode thread pull
				//strcpy(strategy_name,"Thread Pool");
				worker_max = atoi(optarg);
				operation = &perform_thread_pool_operation;
				option_count++;
				break;
			case 'q':
				buffer_max = atoi(optarg);
				break;
				break;
			default:
				print("Please enter the right arguments\n");
				break;
		}
	}
	if(option_count > 1)
	{
		print("\nDon't pass arguments to use more than one strategy.\n");
		exit(0);
	}

	if(!check_folder_exists(path_root)) exit(0);

	if(option_count ==0)
	{
		operation = &perform_serially;
		strcpy(strategy_name,"Serial Operation");
	}
	return operation;
}
int main(int argc,char *argv[])
{
	strategy_t server_operation;
	int sfd;
	server_operation = (strategy_t)configure_server(argc,argv);
	sfd              = initialize_server();
	server_operation(sfd);  				//start server
	if(close(sfd)==-1)					//close server
		print("\nError while closing");

	return 0;
}
