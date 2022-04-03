/* 
 * tcpechosrv.c - A concurrent TCP echo server using threads
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <stdbool.h>
#include <strings.h>
#include <netinet/tcp.h>



#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
int input_port=0;

void error_on_server(int connfd, char* err_msg){
        char error_msg[]="HTTP/1.1 500 Internal Server Error";
        printf("Error occurred on the server side from %s!\n",err_msg);
        write(connfd, error_msg,strlen(error_msg));
}

void bad_request(int connfd){
    printf("HTTP 400 Bad Request\n\r");
    char bad_request_msg[]="HTTP 400 Bad Request";
    write(connfd, bad_request_msg,strlen(bad_request_msg));
}

void hostname_not_found(int connfd){
    printf("HTTP 404 Not Found\n\r");
    char hostname_not_resolved_msg[]="HTTP 404 Not Found";
    write(connfd, hostname_not_resolved_msg, strlen(hostname_not_resolved_msg));
}

int socket_connect(char *host, int connfd){
	struct hostent *hp;
	struct sockaddr_in addr;
	int on = 1, sock;     
    int sockopt_ret,connect_ret;

	if((hp = gethostbyname(host)) == NULL){
		herror("gethostbyname");
		hostname_not_found(connfd);
	}
	bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
	addr.sin_port = htons(80);                                                  //USED THE DEFAULT PORT 80
	addr.sin_family = AF_INET;
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    printf("SOCK:%d\n SOCKOPT_RET:%d\n",sock,sockopt_ret);

	if(sock == -1){
		perror("setsockopt");
	}
	
    connect_ret = connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    printf("CONNECT_RET:%d\n",connect_ret);
    
    if(connect_ret == -1){
        perror("connect");
        exit(1);
    }
	return sock;
}

int main(int argc, char **argv) 
{
    int listenfd, *connfdp, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    input_port = atoi(argv[1]);

    listenfd = open_listenfd(input_port);
    while (1) {
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, (socklen_t * restrict)&clientlen);
        pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp) 
{  
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}

bool serve_request(char buf[MAXBUF], int connfd)
{
    size_t n;
    char msg[MAXLINE];
    char post_msg[MAXLINE];
    struct stat finfo;
    char post_data[1024]={0};
    char* ver_ret=NULL;
    char* filename_ret=NULL; 
    char* post_ret=NULL;
    char version[20]={0};
    ssize_t readret;
    int filename_index; 
    int filename_size=0;
    int file_fd;
    char filename[MAXBUF];   
    char file_buf[MAXBUF]; 
    char err_msg[100]={0};
    bool con_status=false;
    char* return_case=NULL;
    char serve_request_msg[100];
    char* return_for_server=NULL;
    char server_data[MAXBUF];
    char* hostname_ret=NULL;
    char* find_slash=NULL;
    char hostname[100];
    int hostname_size=0;
    char server_path[1024];
    char blacklist[MAXBUF];
    char* blacklist_ret=NULL;
    int blacklist_fd=0;
    int rret=0;

    int server_sock=0;
    //char filetype_ret[MAXLINE];

    //     //check if the request has connection:keep alive message
        return_case = strcasestr(buf,"Keep-alive");
        //printf("```````````````````````````````````````````````````````````````````````RETURN CASE:%s\n",return_case);
        if(return_case!=NULL){
            con_status = true;
        }

            if((strncmp(buf, "GET", 3))==0){

                int error_cnt=0;

                strncpy(err_msg,"GET\n",4);

                // /*version extract*/
                ver_ret=strstr(buf,"HTTP");

                // strncpy(version, ver_ret+5,3);
                
                // //error check
                // if((strncmp(version,"1.1",3)!=0)&&(strncmp(version,"1.0",3)!=0))
                //     error_cnt++;

                //Find the domain name
                hostname_ret = strstr(buf, "http");

                //find link and then extract domain name
                if(hostname_ret == NULL){

                    filename_size = (ver_ret - (buf+4))-1;
                    strncpy(filename, buf+4, filename_size);
                }

                else{

                    filename_size = (ver_ret - (hostname_ret+7))-1;
                    strncpy(filename, hostname_ret+7, filename_size);
                }

                printf("FILENAME: %s\n",filename);

                //just to extract domain name
                find_slash = strstr(filename, "/");

                if(find_slash == NULL){
                    strncpy(hostname,filename,strlen(filename));
                }
                else{
                    hostname_size = find_slash - filename;
                    strncpy(hostname, filename, hostname_size);
                }

                printf("HOSTNAME:%s\n", hostname);

                // //Check if hostname in blacklist
                // blacklist_fd = open("blacklist.txt", O_RDONLY, 0444);

                // while((rret=read(blacklist_fd,blacklist,MAXBUF))>0){
                //     //printf("BLACKLIST_FILE_DATA: %s\n\r",blacklist);
                // }

                // blacklist_ret = strstr(blacklist,hostname);
                // if(blacklist_ret == NULL){

                        //get the message to be sent to server
                        return_for_server = strstr(buf, "\r\n");

                        strncpy(serve_request_msg, buf, (return_for_server - buf)+2);

                        printf("SERVER_REQUEST_MSG:%s\n",serve_request_msg);

                        server_sock = socket_connect(hostname, connfd);
                        printf("SERVER_SOCKET_FS=%d\n",server_sock);
                        
                        sprintf(server_path, "%sHost: %s\r\n\r\n",serve_request_msg,hostname);

                        write(server_sock, server_path, strlen(server_path));
                        //write(server_sock,"GET /\r\n",strlen("GET /\r\n"));

                        bzero(server_data, MAXBUF);

                        while((readret = read(server_sock, server_data, 4000))>0){
                            fprintf(stderr, "%s", server_data);
                            printf("%s\n",server_data);
                            
                            //write to client
                            n = write(connfd, server_data, readret);
                            if(n<0){
                                perror("Error in write\n\r");
                                printf("Write error!\n\r");
                            }

                            bzero(server_data, MAXBUF);
                        }
                        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CAME OUT~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\r");
                // }

                // else{
                //         printf("ERROR 403 Forbidden\n\r");
                //         write(connfd, "ERROR 403 Forbidden\n\r",strlen("ERROR 403 Forbidden\n\r"));
                // }

                //shutdown(server_sock, SHUT_RDWR);
                //close(server_sock);
            }

            else{
                  bad_request(connfd);
            }            

        return con_status;
}
/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd) 
{
    size_t n;
    char buf[MAXLINE]; 
    bool connection_status=false;
    fd_set rfds;
    struct timeval tv;
    int retval;
    int select_flag=0;
    int timeout_flag=0;



    n = read(connfd, buf, MAXLINE);
    printf("server received the following request:\n%s\n",buf);
    connection_status = serve_request(buf,connfd);

    bzero(buf,MAXLINE);
    //make a while loop here there keeps on checking "connection:keep alive" in the incoming request 
    // while(connection_status == true){
    //        //printf("In while!\n");

    //        /* Watch stdin (fd 0) to see when it has input. */

    //        FD_ZERO(&rfds);
    //        FD_SET(connfd, &rfds);

    //        /* Wait up to five seconds. */

    //        tv.tv_sec = 10;
    //        tv.tv_usec = 0;

    //        retval = select(connfd+1, &rfds, NULL, NULL, &tv);
    //        /* Don't rely on the value of tv now! */

    //        if (retval == -1){
    //            printf("Negative return value on select!!\n");
    //        }
    //        else if (retval && FD_ISSET(connfd,&rfds)){
    //            /* FD_ISSET(0, &rfds) will be true. */

    //             n = read(connfd, buf, MAXLINE);
    //             printf("server received the following request:\n%s\n",buf);
                                
    //             connection_status = serve_request(buf,connfd);
    //        }
    //        else{
    //            printf("Timed out!!!!!!!\n");
    //            break;
    //        }
    //        bzero(buf,MAXLINE);
    // }


}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */

