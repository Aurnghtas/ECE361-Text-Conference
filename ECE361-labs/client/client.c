#include "../message.h"
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_INPUT_LEN 1024

/*******************
 * Client Commands *
 *******************/
const char *login = "/login";
const char *logout = "/logout";
const char *joinsession = "/joinsession";
const char *leavesession = "/leavesession";
const char *createsession = "/createsession";
const char *list = "/list";
const char *quit = "/quit";

/*************************************************************************** 
 * some parts of the code are cited from https://beej.us/guide/bgnet/html/ *
 ***************************************************************************/
int main(int argc, char *argv[]) {
    if(argc != 1) {
        printf("Usage: client\n");
        exit(1);
    }

    char user_input[MAX_INPUT_LEN];
    char *command;
    int socketfd = -2;
    bool logged_in = false; // whether the client logs in or not
    bool in_session = false; // whether the client joins a conference session or not
    char *client_ID; // current client's id

    while(true){
        Message msg;
        char msg_buffer[MAX_MSG_TO_STRING]; // buffer for converting Message to strings
        Message recv_msg;
        char recv_buffer[MAX_MSG_TO_STRING]; // buffer for receiving strings from the server
        memset(user_input, '\0', sizeof(char)*MAX_INPUT_LEN);
        fgets(user_input, MAX_INPUT_LEN, stdin); // gets the user input
        command = strtok(user_input, " ");

        /* login command */
        if(strcmp(command, login)==0) {
            if(socketfd!=-2) {
                printf("An client ID is already in use, please quit the previous client ID first\n");
                continue;
            }

            char *pwd, *server_IP, *server_port;

            /* invalid usage of login */
            if((client_ID = strtok(NULL, " "))==NULL){
                printf("Usage for login: /login <client ID> <password> <server-IP> <server-port>\n");
                continue;
            }
            if((pwd = strtok(NULL, " "))==NULL){
                printf("Usage for login: /login <client ID> <password> <server-IP> <server-port>\n");
                continue;
            }
            if((server_IP = strtok(NULL, " "))==NULL){
                printf("Usage for login: /login <client ID> <password> <server-IP> <server-port>\n");
                continue;
            }
            if((server_port = strtok(NULL, "\n"))==NULL){
                printf("Usage for login: /login <client ID> <password> <server-IP> <server-port>\n");
                continue;
            }

            /*!!!!!!!!!!!!!!!! Only For Debug Purpose !!!!!!!!!!!!!!!!*/
            printf("inside login command, we have %s %s %s %s\n", client_ID, pwd, server_IP, server_port);

            /* establish the connection */
            int rv;
            struct addrinfo hints, *res;
            memset(&hints, 0, sizeof hints); // using memset to initialize struct
            hints.ai_family = AF_INET; // IPv4 internet protocols
            hints.ai_socktype = SOCK_STREAM; // socket is TCP
            hints.ai_flags = AI_PASSIVE; // fill in my IP for me

            /* get IP address */
            if((rv = getaddrinfo(server_IP, server_port, &hints, &res)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                exit(1);
            }

            /* get the socketfd */
            if((socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
                printf("Failed to create a socket in login\n");
                exit(1);
            }

            /* connect to the server */
            if(connect(socketfd, res->ai_addr, res->ai_addrlen)==-1) {
                printf("Failed to connect to the server\n");
                exit(1);
            }
            
            /* send LOGIN Message to the server */
            msg.type = LOGIN;
            strcpy(msg.source, client_ID);
            strcpy(msg.data, pwd);
            msg.size = strlen(msg.data);
            messageToStrings(msg, msg_buffer);

            if(send(socketfd, msg_buffer, strlen(msg_buffer), 0)==-1){
                printf("Error in sending the LOGIN Message to the server\n");
                continue;
            }

            /* receive ack from the server */
            int num_bytes_recv = recv(socketfd, recv_buffer, strlen(recv_buffer), 0);
            if(num_bytes_recv==-1) {
                printf("Error in receiving ack for LOGIN Message from the server\n");
            }

            recv_buffer[num_bytes_recv] = '\0'; // add the string terminator to the buffer

            recv_msg = stringsToMessage(recv_buffer);
            
            if(recv_msg.type==LO_ACK) {
                printf("Successfully log in with %s\n", client_ID);
                logged_in = true;
            } else if(recv_msg.type=LO_NAK) {
                printf("Failed to log in with %s, reason: %s\n", client_ID, recv_msg.data);
                close(socketfd);
                socketfd=-2;
            }

        } 

        /* logout command */
        else if(strcmp(command, logout)==0) {
            if(socketfd==-2 && logged_in==false) {
                printf("You have not logged in yet, please log in first\n");
                continue;
            }

            /* send logout Message to the server */
            msg.type = EXIT;
            msg.size = 0;
            strcpy(msg.source, client_ID);
            memset(msg.data, '\0', sizeof(char)*MAX_DATA);
            messageToStrings(msg, msg_buffer);

            if(send(socketfd, msg_buffer, strlen(msg_buffer), 0)==-1){
                printf("Error in sending the LOGOUT Message to the server\n");
                continue;
            }

            // /* receive ack from the server */
            // int num_bytes_recv = recv(socketfd, recv_buffer, strlen(recv_buffer), 0);
            // if(num_bytes_recv==-1) {
            //     printf("Error in receiving the Message from the server\n");
            // }

            close(socketfd);
            socketfd=-2;
            logged_in = false;
        } 

        /* joinsession command */
        else if(strcmp(command, joinsession)==0) {

        } 

        /* leavesession command */
        else if(strcmp(command, leavesession)==0) {
            
        } 

        /* createsession command */
        else if(strcmp(command, createsession)==0) {
            
        } 

        /* list command */
        else if(strcmp(command, list)==0) {
            
        } 

        /* quit command */
        else if(strcmp(command, quit)==0) {
            print("Terminating the program!\n");
            break;
        } 

        /* text - not a command */
        else {

        }

        return 0;

    }

    // /*!!!!!!!!!!!!!!!! Only For Debug Purpose !!!!!!!!!!!!!!!!*/
    // printf("\n");

}