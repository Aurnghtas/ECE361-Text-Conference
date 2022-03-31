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
#include <pthread.h>
#include <signal.h>

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

/********************
 * Global Variables *
 ********************/
bool in_session = false; // whether the client joins a conference session or not
char curr_session[100]; // current session the client is in
volatile sig_atomic_t flag = 0;
int socketfd = -2;
bool logged_in = false; // whether the client logs in or not
char curr_ID[100]; // current client who is using the program
pthread_t receive_thread;

/*****************************************
 * Detects if the user input Control + C *
 *****************************************/
void controlCFunction(int sig) {
    printf("Program crashes! Terminating the program!\n");

    /* send logout Message to the server first */
    Message crash_msg;
    char crash_msg_buffer[MAX_MSG_TO_STRING]; // buffer for converting Message to strings

    crash_msg.type = EXIT;
    strcpy(crash_msg.source, curr_ID);
    strcpy(crash_msg.data, " \0");
    crash_msg.size = strlen(crash_msg.data);
    messageToStrings(crash_msg, crash_msg_buffer);

    if(send(socketfd, crash_msg_buffer, strlen(crash_msg_buffer), 0)==-1){
        printf("Error in sending the LOGOUT Message to the server\n");
    }

    if(logged_in) {
        int rv = pthread_cancel(receive_thread);
        if(rv==0) {
            printf("Successfully shutdown the receive thread\n");
        } else {
            printf("NOT successfully shutdown the receive thread\n");
        }
    }

    close(socketfd);
    exit(0);
}

/*******************************************************
 *      this is the function run by a new thread       *
 * all it does is to wait for messages from the server *
 *******************************************************/
void *receive_thread_start(void *fd) {
    int *socketfd = (int *)fd;

    /* we have 8 types of message - LO_ACK, LO_NAK, JN_ACK, JN_NAK, NS_ACK, NS_NAK, MESSAGE, QU_ACK *
     * but LO_ACK and LO_NAK are used by the main thread to verify the socketfd -> 6 types left */

    Message recv_msg_2;
    char recv_buffer_2[MAX_MSG_TO_STRING];

    while(true) {
        int num_bytes_recv_2 = recv(*socketfd, recv_buffer_2, MAX_MSG_TO_STRING, 0);
        if(num_bytes_recv_2==-1) {
            printf("Error in receiving acks from the server in receive thread\n");
            break;
        }

        recv_buffer_2[num_bytes_recv_2] = '\0'; // add the string terminator to the buffer

        recv_msg_2 = stringsToMessage(recv_buffer_2);

        if(recv_msg_2.type==JN_ACK) {
            printf("Successfully join session %s\n", recv_msg_2.data);
            in_session = true;
            strcpy(curr_session, recv_msg_2.data); 
        }

        else if(recv_msg_2.type==JN_NAK) {
            printf("Failed to join session %s, please retry\n", recv_msg_2.data);
        }

        else if(recv_msg_2.type==NS_ACK) {
            printf("Successfully create session %s\n", recv_msg_2.data);
            in_session = true;
            strcpy(curr_session, recv_msg_2.data);
        }

        else if(recv_msg_2.type==NS_NAK) {
            printf("Failed to create session %s\n", recv_msg_2.data);
        }

        else if(recv_msg_2.type==MESSAGE) {
            printf("%s: %s\n", recv_msg_2.source, recv_msg_2.data);
        }

        else if(recv_msg_2.type==QU_ACK) {
            printf("Following are the users and sessions online\n");
            printf("%s\n", recv_msg_2.data);
        }

        else {
            printf("Can NOT identify the message type!\n");
        }
        
    }

    return 0;
}

/*************************************************************************** 
 * some parts of the code are cited from https://beej.us/guide/bgnet/html/ *
 ***************************************************************************/
int main(int argc, char *argv[]) {
    if(argc != 1) {
        printf("Usage: client\n");
        exit(1);
    }

    char user_input[MAX_INPUT_LEN];
    char users_message[MAX_INPUT_LEN];
    char *command;

    /* handle control c situation */
    signal(SIGINT, controlCFunction);

    while(true) {
        Message msg;
        char msg_buffer[MAX_MSG_TO_STRING]; // buffer for converting Message to strings
        Message recv_msg;
        char recv_buffer[MAX_MSG_TO_STRING]; // buffer for receiving strings from the server
        memset(user_input, '\0', sizeof(char)*MAX_INPUT_LEN);
        fgets(user_input, MAX_INPUT_LEN, stdin); // gets the user input
        strcpy(users_message, user_input);
        command = strtok(user_input, " ");

        /*****************
         * login command *
         *****************/
        if(strcmp(command, login)==0) {
            if(socketfd!=-2) {
                printf("An client ID is already in use, please quit the previous client ID first\n");
                continue;
            }

            char *client_ID, *pwd, *server_IP, *server_port;

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

            strcpy(curr_ID, client_ID); // save client_ID in curr_ID for future reference

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
                printf("Failed to create a socketfd in login\n");
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
            int num_bytes_recv = recv(socketfd, recv_buffer, MAX_MSG_TO_STRING, 0);
            if(num_bytes_recv==-1) {
                printf("Error in receiving ack for LOGIN Message from the server\n");
            }

            recv_buffer[num_bytes_recv] = '\0'; // add the string terminator to the buffer

            recv_msg = stringsToMessage(recv_buffer);
            
            if(recv_msg.type==LO_ACK) {
                printf("Successfully log in with %s\n", client_ID);
                logged_in = true;
                pthread_create(&receive_thread, NULL, receive_thread_start, (void *)&socketfd);
            } else if(recv_msg.type=LO_NAK) {
                printf("Failed to log in with %s, reason: %s\n", client_ID, recv_msg.data);
                close(socketfd);
                socketfd=-2;
            }

        } 

        /******************
         * logout command *
         ******************/
        else if(strcmp(command, logout)==0) {
            if(socketfd==-2 && logged_in==false) {
                printf("You have not logged in yet, please log in first\n");
                continue;
            }

            /* send logout Message to the server */
            msg.type = EXIT;
            strcpy(msg.source, curr_ID);
            strcpy(msg.data, " \0");
            msg.size = strlen(msg.data);
            messageToStrings(msg, msg_buffer);

            if(send(socketfd, msg_buffer, strlen(msg_buffer), 0)==-1){
                printf("Error in sending the LOGOUT Message to the server\n");
                continue;
            }

            if(logged_in) {
                int rv = pthread_cancel(receive_thread);
                if(rv==0) {
                    printf("Successfully shutdown the receive thread\n");
                } else {
                    printf("NOT successfully shutdown the receive thread\n");
                }
            }
            
            close(socketfd);
            socketfd=-2;
            logged_in = false;
        } 

        /***********************
         * joinsession command *
         ***********************/
        else if(strcmp(command, joinsession)==0) {
            if(logged_in==false) {
                printf("Please log in first before joining a conference session\n");
                continue;
            } 

            if(in_session==true) {
                printf("Please leave your current conference session to join a new one\n");
                continue;
            }

            if(logged_in==true && in_session==false) {
                char *session_ID;

                if((session_ID = strtok(NULL, "\n"))==NULL){
                    printf("Usage for join session: /joinsession <session ID>\n");
                    continue;
                }

                msg.type = JOIN;
                strcpy(msg.source, curr_ID);
                strcpy(msg.data, session_ID);
                msg.size = strlen(msg.data);
                messageToStrings(msg, msg_buffer);

                if(send(socketfd, msg_buffer, strlen(msg_buffer), 0)==-1){
                    printf("Error in sending the joinsession Message to the server\n");
                    continue;
                }
                
            }
        } 

        /************************
         * leavesession command *
         ************************/
        else if(strcmp(command, leavesession)==0) {
            if(logged_in==false) {
                printf("Please log in first before leaving a conference session\n");
                continue;
            } 

            if(in_session==false) {
                printf("Please join a conference session before you can leave\n");
                continue;
            }

            if(logged_in==true && in_session==true) {
                msg.type = LEAVE_SESS;
                strcpy(msg.source, curr_ID);
                strcpy(msg.data, " \0");
                msg.size = strlen(msg.data);       
                messageToStrings(msg, msg_buffer);

                if(send(socketfd, msg_buffer, strlen(msg_buffer), 0)==-1){
                    printf("Error in sending the leavesession Message to the server\n");
                    continue;
                }
                
                in_session=false;
                
            }
        } 

        /*************************
         * createsession command *
         *************************/
        else if(strcmp(command, createsession)==0) {
            if(logged_in==false) {
                printf("Please log in first before creating a conference session\n");
                continue;
            } 

            if(in_session==true) {
                printf("Please leave your current conference session to create and join a new one\n");
                continue;
            }

            if(logged_in==true && in_session==false) {
                char *session_ID;

                if((session_ID = strtok(NULL, "\n"))==NULL){
                    printf("Usage for create session: /createsession <session ID>\n");
                    continue;
                }

                msg.type = NEW_SESS;
                strcpy(msg.source, curr_ID);
                strcpy(msg.data, session_ID);
                msg.size = strlen(msg.data);
                messageToStrings(msg, msg_buffer);

                if(send(socketfd, msg_buffer, strlen(msg_buffer), 0)==-1){
                    printf("Error in sending the createsession Message to the server\n");
                    continue;
                }

            }

        } 

        /****************
         * list command *
         ****************/
        else if(strcmp(command, list)==0) {
            if(logged_in == false) {
                printf("Please log in first before using /list command\n");
                continue;
            }


            msg.type = QUERY;
            strcpy(msg.source, " \0");
            strcpy(msg.data, " \0");
            msg.size = strlen(msg.data);
            messageToStrings(msg, msg_buffer);

            if(send(socketfd, msg_buffer, strlen(msg_buffer), 0)==-1){
                printf("Error in sending the list Message to the server\n");
                continue;
            }

        } 

        /******************************************************************
         * quit command - exit the conference, then terminate the program *
         ******************************************************************/
        else if(strcmp(command, quit)==0) {
            printf("Terminating the program!\n");

            if(socketfd==-2 && logged_in==false) {
                break;
            }

            /* send logout Message to the server first */
            msg.type = EXIT;
            strcpy(msg.source, curr_ID);
            strcpy(msg.data, " \0");
            msg.size = strlen(msg.data);
            messageToStrings(msg, msg_buffer);

            if(send(socketfd, msg_buffer, strlen(msg_buffer), 0)==-1){
                printf("Error in sending the LOGOUT Message to the server\n");
            }

            if(logged_in) {
                int rv = pthread_cancel(receive_thread);
                if(rv==0) {
                    printf("Successfully shutdown the receive thread\n");
                } else {
                    printf("NOT successfully shutdown the receive thread\n");
                }
            }

            close(socketfd);
            break;
        } 

        /************************
         * text - not a command *
         ************************/
        else {
            if(logged_in==false) {
                printf("Please log in first before sending messages\n");
                continue;
            } 

            if(in_session==false) {
                printf("Please join a session first before sending messages\n");
                continue;
            }

            msg.type = MESSAGE;
            strcpy(msg.source, curr_ID);
            strcpy(msg.data, users_message);
            msg.size = strlen(msg.data);
            messageToStrings(msg, msg_buffer);

            if(send(socketfd, msg_buffer, strlen(msg_buffer), 0)==-1){
                printf("Error in sending Message to the server\n");
            }
        }

    }

    return 0;

}