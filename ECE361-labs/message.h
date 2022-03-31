#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 32
#define MAX_DATA 1024
#define MAX_MSG_TO_STRING 1064

//message types
enum messageType{
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_ACK,
    JN_NAK,
    LEAVE_SESS,
    NEW_SESS,
    NS_ACK,
    MESSAGE,
    QUERY,
    QU_ACK,

    /* following are self-defined messageType */
    NS_NAK, // for failed to crate a new session
    P_MESSAGE, // for private messaging
    P_ACK, //for successful private message
    P_NAK, //for unsuccessful private message
    KICK, // for kicking out a client
    K_ACK,
    K_NAK,
    GIVEADMIN // for giving administration to another
    G_ACK,
    G_NAK
};

//the structure for messages in the conference lab
typedef struct message{
    unsigned int type;  //refers to messageType
    unsigned int size;  //size of the data
    unsigned char source[MAX_NAME];  //ID of the client sending the message
    unsigned char data[MAX_DATA];
}Message;

// helper function converts Message to strings with size MAX_MSG_TO_STRING
void messageToStrings(Message message, char* buffer) {
    int num_char = sprintf(buffer, "%d:%d:%s:%s", message.type, message.size, message.source, message.data);
    if(num_char<0) {
        printf("Error in messageToStrings\n");
    }
}

// helper function converts strings to Message
Message stringsToMessage(char* buffer) {
    Message message;
    const char breaker[2] = ":";
    message.type = atoi(strtok(buffer, breaker));
    message.size = atoi(strtok(NULL, breaker)); //use null to continue scanning
    strcpy(message.source, strtok(NULL, breaker));
    strcpy(message.data, strtok(NULL, "\n"));
    return message;
}