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


#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);

void error_on_server(int connfd, char* err_msg){
        char error_msg[]="HTTP/1.1 500 Internal Server Error";
        printf("Error occurred on the server side from %s!\n",err_msg);
        write(connfd, error_msg,strlen(error_msg));
}

int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
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
    int file_fd;
    char filename[MAXBUF];   
    char file_buf[MAXBUF]; 
    char err_msg[100]={0};
    bool con_status=false;
    char* return_case=NULL;
    //char filetype_ret[MAXLINE];

    //     //check if the request has connection:keep alive message
        return_case = strcasestr(buf,"Keep-alive");
        //printf("```````````````````````````````````````````````````````````````````````RETURN CASE:%s\n",return_case);
        if(return_case!=NULL){
            con_status = true;
        }

            if((strncmp(buf, "GET", 3))==0){

                strcpy(err_msg,"GET\n");
                /*version extract*/
                ver_ret=strstr(buf,"HTTP");
                strncpy(version, ver_ret+5,3);
                //printf("************************************************\n");
                //printf("VERSION: %s\n", version);

                //error check
                if((strncmp(version,"1.1",3)!=0)&&(strncmp(version,"1.0",3)!=0))
                    goto ERROR;

                //appending with www
                strncpy(filename,"www",3);
                /*filename extract*/
                filename_ret=strstr(buf+4," ");
                strncpy(filename+3,buf+4,(filename_ret-(buf+4)));
                printf("FILENAME: %s\n",filename);

                /*open file*/
                file_fd = open(filename, O_RDONLY, 0444);
                
                //error check
                if(file_fd<0)
                    goto ERROR;

                int fstatret=fstat(file_fd, &finfo);
                //fstat to get file data
                if(fstatret<0){
                    perror("File does not exist!\n");
                    goto ERROR;
                }

                /*find file type*/
                char* filetype_ret= strrchr(filename,'.');
                //strncpy(filetype_ret,ret_filetype,strlen(ret_filetype));
                printf("FILETYPE_RET:%s\n",filetype_ret);

                if(con_status==true){
                    if((strncmp(filetype_ret,".html",5))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:text/html\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".txt",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:text/plain\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".png",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:image/png\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".gif",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:image/gif\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".jpg",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:image/jpg\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".css",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:text/css\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".js",3))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:application/javascript\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,(finfo.st_size));
                    }                    
                }
                else{
                    if((strncmp(filetype_ret,".html",5))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:text/html\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".txt",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:text/plain\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".png",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:image/png\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".gif",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:image/gif\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".jpg",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:image/jpg\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".css",4))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:text/css\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,(finfo.st_size));
                    }
                    else if((strncmp(filetype_ret,".js",3))==0){
                        sprintf(msg, "HTTP/%s 200 Document Follows\r\n Content-Type:application/javascript\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,(finfo.st_size));
                    }
                }
                    printf("server returning a http message with the following content.\n%s\n",msg);
                write(connfd, msg, strlen(msg));


                while((readret = read(file_fd, file_buf, MAXLINE))>0){
                    n=write(connfd, file_buf, readret);
                    if(n<0){
                        perror("Error in write\n");
                        goto ERROR;
                    }
                }

            }

            else if((strncmp(buf, "POST", 4))==0){

                strcpy(err_msg,"POST");
                /*version extract*/
                ver_ret=strstr(buf,"HTTP");
                strncpy(version, ver_ret+5,3);
                //printf("************************************************\n");
                //printf("VERSION: %s\n", version);

                //error check
                if((strncmp(version,"1.1",3)!=0)&&(strncmp(version,"1.0",3)!=0))
                    goto ERROR;        

                //appending with www
                strncpy(filename,"www",3);
                /*filename extract*/
                filename_ret=strstr(buf+5," ");
                strncpy(filename+3,buf+5,(filename_ret-(buf+5)));
                printf("FILENAME: %s\n",filename);

                post_ret=strrchr(buf,'\n');
                post_ret++;
                strncpy(post_data,post_ret,strlen(post_ret));

                /*open file*/
                file_fd = open(filename, O_RDONLY, 0444);

                //error check
                if(file_fd<0)
                    goto ERROR;        

                int fstatret=fstat(file_fd, &finfo);
                //fstat to get file data
                if(fstatret<0){
                perror("File does not exist!\n");
                goto ERROR;
                }

                /*find file type*/
                char* filetype_ret= strrchr(filename,'.');
                //strncpy(filetype_ret,ret_filetype,strlen(ret_filetype));
                if(con_status==true){
                    if((strncmp(filetype_ret,".html",5))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:text/html\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".txt",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:text/plain\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".png",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:image/png\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".gif",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:image/gif\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".jpg",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:image/jpg\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".css",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:text/css\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".js",3))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:application/javascript\r\n Content-Length: %ld\r\n Connection: Keep-alive\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }                    
                }
                else{
                    if((strncmp(filetype_ret,".html",5))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:text/html\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".txt",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:text/plain\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".png",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:image/png\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".gif",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:image/gif\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".jpg",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:image/jpg\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".css",4))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:text/css\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                    else if((strncmp(filetype_ret,".js",3))==0){
                        sprintf(msg, "HTTP/%s 200 OK\r\n Content-Type:application/javascript\r\n Content-Length: %ld\r\n Connection: Close\r\n\r\n",version,((finfo.st_size)+33+strlen(post_ret)));
                    }
                }
                    printf("server returning a http message with the following content.\n%s\n",msg);
                write(connfd, msg, strlen(msg));

                sprintf(post_msg,"<html><body><pre><h1>%s </h1></pre>",post_data);
                write(connfd, post_msg, strlen(post_msg));

                while((readret = read(file_fd, file_buf, MAXLINE))>0){
                    n=write(connfd, file_buf, readret);
                    if(n<0){
                        perror("Error in write\n");
                        goto ERROR;
                    }
                }

            }

            else{
ERROR:          error_on_server(connfd,msg);
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
    bool con_status=false;
    fd_set rfds;
    struct timeval tv;
    int retval;
    int select_flag=0;
    int timeout_flag=0;

//     // strcpy(buf,httpmsg);
//    printf("server returning a http message with the following content.\n%s\n",buf);
   // write(connfd, buf,strlen(httpmsg));

    n = read(connfd, buf, MAXLINE);
    //printf("server received the following request:\n%s\n",buf);
    con_status = serve_request(buf,connfd);

    bzero(buf,MAXLINE);
    //make a while loop here there keeps on checking "connection:keep alive" in the incoming request 
    while(con_status==true){
           printf("In while!\n");

           /* Watch stdin (fd 0) to see when it has input. */

           FD_ZERO(&rfds);
           FD_SET(connfd, &rfds);

           /* Wait up to five seconds. */

           tv.tv_sec = 10;
           tv.tv_usec = 0;

           retval = select(connfd+1, &rfds, NULL, NULL, &tv);
           /* Don't rely on the value of tv now! */

           if (retval == -1){
               printf("Negative return value on select!!\n");
           }
           else if (retval && FD_ISSET(connfd,&rfds)){
               /* FD_ISSET(0, &rfds) will be true. */

                n = read(connfd, buf, MAXLINE);
                printf("server received the following request:\n%s\n",buf);
                                
                con_status = serve_request(buf,connfd);
           }
           else{
               printf("Timed out!!!!!!!\n");
               break;
           }
           bzero(buf,MAXLINE);
    }


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

