#include "../message.h"
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
//for select
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


//globals here
const char* clients[] = {"Tom", "Jack", "Albert", "Mom", "Dad"};

int main(int argc, char *argv[]){
    //called in the format server <UDP listen port>
    if(argc != 2){
        printf("usage: server <UDP listen port>\n");
        exit(1);
    }

    //find available ports
    char available_host[20];
    gethostname(available_host, sizeof(available_host));
    int rv;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);    //using memset to initialize struct
    hints.ai_family = AF_INET;          //IPv4 internet protocols
    hints.ai_socktype = SOCK_STREAM;    //socket is TCP
    if((rv = getaddrinfo(available_host, argv[1], &hints, &res)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    //socket initialization
    int socketFd = 0;
    socketFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); //create socket
    //allow reuse of local addresses for bind
    int yes = 1;
    if(setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
        printf("Fails to set socket options\n");
        exit(1);
    }
    //bind to port
    if(bind(socketFd, res->ai_addr, res->ai_addrlen) < 0){
        printf("Fails to bind to this port\n");
        exit(1);
    }

}