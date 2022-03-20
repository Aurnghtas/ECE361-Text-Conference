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

typedef struct session{
    char *name;
}Session; 

//globals here, allows 5 users at a time
const char* clients[] = {"Tom", "Jack", "Albert", "Mom", "Dad"}; //client names
const char* passwords[] = {"123", "234", "345", "456", "567"}; //corresponding client passwords
//record whether the corresponding client is connected, same functionality as master
bool connected[] = {false, false, false, false, false}; 
int clientFds[] = {-1, -1, -1, -1, -1}; //record the corresponding client fds
char *joined[] = {NULL, NULL, NULL, NULL, NULL}; //points to the session the client joins, NULL for not joining any


//this is the helper function that checks the type of command recieved and do corresponding things
void handle_message_type(Message* msg, int cFd){
    int Type = msg->type; //the recieved message type
    Message replyMsg; //the struct message of reply 
    char reply_buffer[MAX_MSG_TO_STRING]; //the converted buffer for reply stream

    if(Type == LOGIN){
        bool inlist = false;
        bool pwdright = false;
        //the request is to login
        for(int i = 0;i < 5;i++){
            //find out who is trying to login
            if((strcmp(clients[i], msg->source) == 0) && (strcmp(passwords[i], msg->data) == 0)){
                if(strcmp(clients[i], msg->source) == 0) inlist = true;
                if(strcmp(passwords[i], msg->data) == 0) pwdright = true;
                //identified client and correct password
                if(connected[i]){
                    //this client is already connected
                    char* replydata = "You are already logged in!";
                    strcpy(replyMsg.data, replydata);
                    replyMsg.size = strlen(replyMsg.data);
                    strcpy(replyMsg.source, msg->source);
                    replyMsg.type = LO_NAK;
                    messageToStrings(replyMsg, reply_buffer);
                    if(send(cFd, reply_buffer, strlen(reply_buffer), 0) == -1){ //+1 needed?
                        printf("Error in sending the Message to the client\n");
                        exit(1);
                    }
                }else{
                    //not already connected
                    clientFds[i] = cFd; //register the corresponding socket
                    connected[i] = true; //mark it as connected
                    char* replydata = "Log in success!";
                    strcpy(replyMsg.data, replydata);
                    replyMsg.size = strlen(replyMsg.data);
                    strcpy(replyMsg.source, msg->source);
                    replyMsg.type = LO_ACK;
                    messageToStrings(replyMsg, reply_buffer);
                    printf("line 61: %s\n", reply_buffer);
                    if(send(cFd, reply_buffer, strlen(reply_buffer) + 1, 0) == -1){ 
                        printf("Error in sending the Message to the client\n");
                        exit(1);
                    }
                }
            }
            // char* replydata;
            // if(!inlist){
            //     replydata = "Not identified in the client list!\n";
            //     strcpy(replyMsg.data, replydata);
            // }
            // if(!pwdright){
            //     replydata = "Incorrect password!\n";
            //     strcpy(replyMsg.data, replydata);
            // }
            // replyMsg.size = strlen(replydata);
            // strcpy(replyMsg.source, msg->source);
            // replyMsg.type = LO_NAK;
            // messageToStrings(replyMsg, reply_buffer);
            // if(send(cFd, reply_buffer, strlen(reply_buffer), 0) == -1){
            //     printf("Error in sending the Message to the client\n");
            //     exit(1);
            // }   
        }
    }else if(Type == EXIT){
        char *cId = msg->source;
        //check to remove this client from connected list
        for(int i = 0;i < 5;i++){
            if(strcmp(clients[i], cId) == 0){
                connected[i] = false;
                clientFds[i] = -1;
                //sessions[i] = NULL;
            }
        }
        close(cFd); //simply delete this client Fd from the set
        printf("Successfully close the cFd at line 96\n");
    }else if(Type == JOIN){
        char *cId = msg->source;
        char *name = msg->data;
        bool found = false;
        int whichOne = -1;
        for(int i = 0;i < 5;i++){
            if(joined[i] && (strcmp(name, joined[i]) == 0)){
                found = true; //find that there's already someone joined in the session
                whichOne = i;
                break;
            }
        }

        //session is there, join in
        if(found){
            strcpy(joined[whichOne], name);

            char* replydata = name; //data field is the session id joined
            strcpy(replyMsg.data, replydata);
            replyMsg.size = strlen(replyMsg.data);
            strcpy(replyMsg.source, msg->source);
            replyMsg.type = JN_ACK;
            messageToStrings(replyMsg, reply_buffer);
            if(send(cFd, reply_buffer, strlen(reply_buffer) + 1, 0) == -1){ //+1 needed?
                printf("Error in sending the Message to the client\n");
                exit(1);
            }
        }else{
            char* replydata = ("Session %s does not exit!", name);
            strcpy(replyMsg.data, replydata);
            replyMsg.size = strlen(replyMsg.data);
            strcpy(replyMsg.source, msg->source);
            replyMsg.type = JN_NAK;
            messageToStrings(replyMsg, reply_buffer);
            if(send(cFd, reply_buffer, strlen(reply_buffer) + 1, 0) == -1){ //+1 needed?
                printf("Error in sending the Message to the client\n");
                exit(1);
            }
        }
    }else if(Type == LEAVE_SESS){
        char *clientId = msg->source;
        for(int i = 0;i < 5;i++){
            if(strcmp(clients[i], clientId) == 0){
                if(joined[i]){
                    joined[i] = NULL;
                    /*char* replydata = "Session left";
                    strcpy(replyMsg.data, replydata);
                    replyMsg.size = strlen(replyMsg.data);
                    strcpy(replyMsg.source, msg->source);
                    replyMsg.type = LEAVE_SESS;
                    messageToStrings(replyMsg, reply_buffer);
                    if(send(cFd, reply_buffer, strlen(reply_buffer) + 1, 0) == -1){ //+1 needed?
                        printf("Error in sending the Message to the client\n");
                        exit(1);
                    }*/
                }
            }
        }
    }else if(Type == NEW_SESS){
        printf("now we are in create new session\n");
        char *sessionId = (char *)msg->data;
        char *clientId = (char *)msg->source;
        // Session *newSession = (Session *)malloc(sizeof(Session));
        // strcpy(newSession->name, sessionId);

        //check if session already exits
        printf("now we are in line 171 to check for the client and session: %s and %s\n", clientId, sessionId);
        bool exists = false;
        for(int i = 0;i < 5;i++){
            if(joined[i] != NULL){
                printf("line 174: %s\n", joined[i]);
            }
            if(joined[i]!=NULL && strcmp(sessionId, joined[i]) == 0){
                exists = true;
                break;
            }
        }
        printf("line 180: %d\n",exists);
        if(exists){
            char* replydata = ("Session %s already exist!", sessionId);        
            strcpy(replyMsg.data, replydata);
            replyMsg.size = strlen(replyMsg.data);
            strcpy(replyMsg.source, msg->source);
            replyMsg.type = NS_NAK;
            messageToStrings(replyMsg, reply_buffer);
            printf("line 186: %s\n", reply_buffer);
            if(send(cFd, reply_buffer, strlen(reply_buffer) + 1, 0) == -1){ //+1 needed?
                printf("Error in sending the Message to the client\n");
                exit(1);
            }
            return;
        }
        for(int i = 0;i < 5;i++){
            //find the client to start new session
            if(strcmp(clients[i], clientId) == 0){
                //if session does not exit yet
                printf("line 199\n");
                if(joined[i] == NULL){
                    strcpy(joined[i], sessionId);
                    printf("line 202: %s\n", joined[i]);
                    char* replydata = ("Created new session %s!", sessionId);
                    strcpy(replyMsg.data, replydata);
                    replyMsg.size = strlen(replyMsg.data);
                    strcpy(replyMsg.source, msg->source);
                    replyMsg.type = NS_ACK;
                    messageToStrings(replyMsg, reply_buffer);
                    if(send(cFd, reply_buffer, strlen(reply_buffer) + 1, 0) == -1){ //+1 needed?
                        printf("Error in sending the Message to the client\n");
                        exit(1);
                    }
                }
            }
        }
    }else if(Type == MESSAGE){
        char *clientId = msg->source;
        char *wantedsession = NULL;
        //first, find out which session the message need to go
        for(int i = 0;i < 5;i++){
            if(connected[i] && strcmp(clients[i], clientId) == 0){
                strcpy(wantedsession, joined[i]);
                break;
            }
        }
        //send to all clients in this session
        for(int i = 0;i < 5;i++){
            if(connected[i] && (strcmp(joined[i], wantedsession)==0)) {
                strcpy(replyMsg.data, msg->data);
                replyMsg.size = strlen(replyMsg.data);
                strcpy(replyMsg.source, msg->source);
                replyMsg.type = MESSAGE;
                messageToStrings(replyMsg, reply_buffer);
                if(send(cFd, reply_buffer, strlen(reply_buffer) + 1, 0) == -1){ //+1 needed?
                    printf("Error in sending the Message to the client\n");
                    exit(1);
                }
                //send the message to this client
            } 
        }
    }else if(Type == QUERY){
        printf("Now the message type is QUERY\n");
        char replydata[MAX_MSG_TO_STRING];
        memset(replydata, 0, sizeof(replydata));
        for(int i = 0;i < 5;i++){ 
            if(connected[i]){
                if(joined[i] == NULL){
                    sprintf(replydata, "%s%s:No Session||",replydata, clients[i]);
                }else{
                    sprintf(replydata, "%s%s:%s||", replydata, clients[i], joined[i]);
                }
            }
        }
        strcpy(replyMsg.data, replydata);
        replyMsg.size = strlen(replyMsg.data);
        strcpy(replyMsg.source, msg->source);
        replyMsg.type = QU_ACK;
        messageToStrings(replyMsg, reply_buffer);
        printf("line 242: %s\n", reply_buffer);
        if(send(cFd, reply_buffer, strlen(reply_buffer) + 1, 0) == -1){ //+1 needed?
            printf("Error in sending the Message to the client\n");
            exit(1);
        }
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

    //loop through the results and bind to the first available one
    struct addrinfo *ptr;
    int yes = 1;
    int initialFd = 0;
    for(ptr = res;ptr != NULL;ptr = ptr->ai_next){
        //find available port num
        if((initialFd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
            perror("Server: socket");
            continue;
        }
        /*if(setsockopt(initialFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if(bind(initialFd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
            close(initialFd);
            perror("Server: bind");
            continue;
        }*/

        //here we already find an available port num
        break;
    }

    //socket initialization
    //int initialFd = 0;
    //initialFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); //create socket
    //allow reuse of local addresses for bind
    //int yes = 1;
    if(setsockopt(initialFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
        printf("Fails to set socket options\n");
        exit(1);
    }
    //bind to port
    if(bind(initialFd, res->ai_addr, res->ai_addrlen) < 0){
        printf("Fails to bind to this port\n");
        exit(1);
    }
    printf("Server at port number %d", initialFd);
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
    while(1){
        char buffer[MAX_MSG_TO_STRING];
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
                    memset(buffer, '\0', MAX_MSG_TO_STRING);
                    num_Byte_recieved = recv(cFd, buffer, MAX_MSG_TO_STRING, 0); //read
                    printf("line 181: %s\n", buffer);
                    if(num_Byte_recieved == -1){
                        printf("Fails to receive message 1\n");
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
            newFd = accept(initialFd, (struct sockaddr *)&cAddr, &addrSize);
            if(newFd < 0){
                perror("Fails to accept new connection\n");
                exit(1);
            }

            //after connection, recieve message
            int num_Byte_recieved = 0;
            memset(buffer, '\0', MAX_MSG_TO_STRING);
            num_Byte_recieved = recv(newFd, buffer, MAX_MSG_TO_STRING, 0); //read
            printf("line 213: %d\n", newFd);
            printf("line 214: %s\n", buffer);
            if(num_Byte_recieved == -1){
                printf("Fails to receive message 2\n");
                exit(1);
            }

            buffer[num_Byte_recieved] = '\0';

            Message msg;
            msg = stringsToMessage(buffer); //parse the buffer(char *) into struct message
            printf("line 220: %d, %d, %s, %s\n", msg.type, msg.size, msg.source, msg.data);

            handle_message_type(&msg, newFd);
            printf("now we are at line 223\n");
        }

    }
}