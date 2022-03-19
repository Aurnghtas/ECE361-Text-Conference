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
const char* passwords[] = {"123", "234", "345", "456", "567"}; //corresponding client passwords
//record whether the corresponding client is connected, same functionality as master
bool connected[] = {false, false, false, false, false}; 
int clientFds[] = {-1, -1, -1, -1, -1}; //record the corresponding client fds


//this is the helper function that checks the type of command recieved and do corresponding things
void handle_message_type(Message* msg, int cFd){
    int Type = msg->type; //the recieved message type
    Message replyMsg; //the struct message of reply 
    char reply_buffer[MAX_MSG_TO_STRING]; //the converted buffer for reply stream

    if(Type == LOGIN){
        bool inlist = false;
        bool pwdright = false;
        //the request is to login
        for(int i = 0; i < 5;i++){
            //find out who is trying to login
            if((strcmp(clients[i], msg->source) == 0) && (strcmp(passwords[i], msg->data) == 0)){
                if(strcmp(clients[i], msg->source) == 0) inlist = true;
                if(strcmp(passwords[i], msg->data) == 0) pwdright = true;
                //identified client and correct password
                if(connected[i]){
                    //this client is already connected
                    char* replydata = "You are already logged in!\n";
                    strcpy(replyMsg.data, replydata);
                    replyMsg.size = strlen(replydata);
                    strcpy(replyMsg.source, msg->source);
                    replyMsg.type = LO_NAK;
                    messageToStrings(replyMsg, reply_buffer);
                    if(send(cFd, reply_buffer, strlen(reply_buffer), 0) == -1){
                        printf("Error in sending the Message to the client\n");
                        exit(1);
                    }
                }else{
                    //not already connected
                    clientFds[i] = cFd; //register the corresponding socket
                    connected[i] = true; //mark it as connected
                    char* replydata = "Log in success!\n";
                    strcpy(replyMsg.data, replydata);
                    replyMsg.size = strlen(replydata);
                    strcpy(replyMsg.source, msg->source);
                    replyMsg.type = LO_ACK;
                    messageToStrings(replyMsg, reply_buffer);
                    if(send(cFd, reply_buffer, strlen(reply_buffer), 0) == -1){
                        printf("Error in sending the Message to the client\n");
                        exit(1);
                    }
                }
            }
            char* replydata;
            if(!inlist){
                replydata = "Not identified in the client list!\n";
                strcpy(replyMsg.data, replydata);
            }
            if(!pwdright){
                replydata = "Incorrect password!\n";
                strcpy(replyMsg.data, replydata);
            }
            replyMsg.size = strlen(replydata);
            strcpy(replyMsg.source, msg->source);
            replyMsg.type = LO_NAK;
            messageToStrings(replyMsg, reply_buffer);
            if(send(cFd, reply_buffer, strlen(reply_buffer), 0) == -1){
                printf("Error in sending the Message to the client\n");
                exit(1);
            }   
        }
    }else if(Type == EXIT){
        char *cId = msg->source;
        //check to remove this client from connected list
        for(int i = 0;i < 5;i++){
            if(strcmp(clients[i], cId) == 0){
                connected[i] = false;
                //sessions[i] = NULL;
            }
        }
        close(cFd); //simply delete this client Fd from the set
    }

}


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
    hints.ai_flags = AI_PASSIVE;
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
    //*****notice now, my initialFd becomes the listneer, so initialFd handles new*******************************
    //*****connections while FD_sets records which fds(already connected) are ready to listen********************

    //start serving, uses select to do sychronization for set of FDs
    //the fd set
    fd_set FD_sets;
    int maxFd = -1;
    //the recieve buffer
    char buffer[MAX_MSG_TO_STRING];
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

        //find the one ready for reading(won't block) and modify FD_sets to only those ready for reading
        if(select(maxFd + 1, &FD_sets, NULL, NULL, NULL) <= 0){
            printf("Fails to select a appropriate socket:");
            exit(1);
        }

        if(!FD_ISSET(initialFd, &FD_sets)){
            //indicating no new connections, so loop through to find which of the already
            //connected fds are ready to read
            int num_Byte_recieved = 0;
            for(int i = 0;i < 5;i++){
                int cFd = clientFds[i];
                //if ready, then read
                if(FD_ISSET(cFd, &FD_sets)){
                    //printf("%d\n",cFd);
                    num_Byte_recieved = recv(cFd, buffer, MAX_MSG_TO_STRING, 0); //read
                    if(num_Byte_recieved == -1){
                        printf("Fails to receive message\n");
                        exit(1);
                    }

                    buffer[num_Byte_recieved] = '\0';

                    Message msg;
                    msg = stringsToMessage(buffer); //parse the buffer(char *) into struct message

                    handle_message_type(&msg, cFd);
                }  
            }
        }else{
            //listener on meaning there's new connection, then accept it
            int newFd = 0;
            struct sockaddr_storage cAddr;
            socklen_t addrSize = sizeof(cAddr);
            if(newFd = accept(initialFd, (struct sockaddr *)&cAddr, &addrSize) < 0){
                perror("Fails to accept new connection\n");
                exit(1);
            }

            //after connection, recieve message
            int num_Byte_recieved = 0;
            num_Byte_recieved = recv(newFd, buffer, MAX_MSG_TO_STRING, 0); //read
            if(num_Byte_recieved == -1){
                printf("Fails to receive message\n");
                exit(1);
            }

            buffer[num_Byte_recieved] = '\0';

            Message msg;
            msg = stringsToMessage(buffer); //parse the buffer(char *) into struct message

            handle_message_type(&msg, newFd);
        }

    }
}