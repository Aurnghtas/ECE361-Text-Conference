#include "../message.h"
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
//for select
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>


//globals here, allows 5 users at a time
const char* clients[] = {"Tom", "Jack", "Albert", "Mom", "Dad"}; //client names
bool connected[] = {false, false, false, false, false}; //record weather the corresponding client is connected
int clientFds[] = {-1, -1, -1, -1, -1}; //record the corresponding client fds

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
    int initialFd = 0;
    initialFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); //create socket
    //allow reuse of local addresses for bind
    int yes = 1;
    if(setsockopt(initialFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
        printf("Fails to set socket options\n");
        exit(1);
    }
    //bind to port
    if(bind(initialFd, res->ai_addr, res->ai_addrlen) < 0){
        printf("Fails to bind to this port\n");
        exit(1);
    }

    if(listen(initialFd, 10) == -1){ //maximum 10 pending requests
        printf("Fails to listen to connections\n");
        exit(1);
    }

    //start serving, uses select to do sychronization for set of FDs
    int maxFd = -1;
    fd_set FD_sets;
    while(1){
        FD_ZERO(&FD_sets); //clear set, initialize
        FD_SET(initialFd, &FD_sets); //add the current fd to set
        maxFd = initialFd;

        //add in newly connected clients to set
        for(int i = 0;i < 5;i++){ 
            if(connected[i]){ 
                int clientfd = clientFds[i];
                if(clientfd > 0)
                    FD_SET(clientfd, &FD_sets);
                
                if(clientfd > maxFd) maxFd = clientfd;
            }
        }

        //
        if(select(max_sd+1, &sock_set, NULL, NULL, NULL) <= 0){
            perror("Fails to select a appropriate socket:");
            exit(1);
        }


    }
}