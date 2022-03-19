#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 32
#define MAX_DATA 1024
#define MAX_MSG_TO_STRING 1064

//#define max_package_size 1000

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
    QU_ACK
};

//***************************************************************************
//need helper functions to turn packet(struct used in last labs) into message(required struct of this lab)



//the structure for messages in the conference lab
typedef struct message{
    unsigned int type;  //refers to messageType
    unsigned int size;  //size of the data
    unsigned char source[MAX_NAME];  //ID of the client sending the message
    unsigned char data[MAX_DATA];
<<<<<<< HEAD
}Message;

// helper function converts Message to strings with size MAX_MSG_TO_STRING
void messageToStrings(Message message, char* buffer) {
    int num_char = sprintf(buffer, "%d:%d:%s:%s:", message.type, message.size, message.source, message.data);
    if(num_char<0) {
        printf("Error in messageToStrings\n");
=======
}message;

typedef struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[max_package_size];
}Packet;

FILE* FD = NULL;
//This is the helper function that parse the recieved message into packet,
//it will create a new file if the message is the first one and close file after the last message
int packet_from_message(char* message, int prev_index){
    //first use strtok function to break up the message header
    //referred to this tutorial https://www.cplusplus.com/reference/cstring/strtok/
    const char breaker[2] = ":";
    char* total_frag = strtok(message, breaker);
    char* frag_no = strtok(NULL, breaker); //use null to continue scanning
    char* size = strtok(NULL, breaker);
    char* filename = strtok(NULL, breaker);

    //now turn strings to int values
    unsigned int Total_frag = atoi(total_frag);
    unsigned int Frag_no = atoi(frag_no);
    unsigned int dSize = atoi(size);

    /*if(Frag_no != (prev_index) + 1){
        return -1;
    }*/

    if(Frag_no == 1){
        //this is the first message revieved, need to create a Packet struct and file stream
        Packet recieved;
        recieved.filename = filename;
        recieved.frag_no = Frag_no;
        recieved.size = dSize;
        recieved.total_frag = Total_frag;

        //create file stream, open a file
        FD = fopen(filename, "wb"); //recieved file needs to have same name as sent one
        if(FD == NULL){
            printf("Cannot create file\n"); 
            return -1; // -1 for error
        }
    }

    //now, write data into the created file use fwrite to write to stream
    int header_size = strlen(total_frag) + strlen(frag_no) + strlen(size) + strlen(filename);
    //fwrite, notice need to skip the header
    fwrite(message + header_size + sizeof(char) * 4, sizeof(char), dSize, FD);

    //now check if the last message is done transmitting
    if(Frag_no == Total_frag){
        //fclose(fd);
        return 1; // 1 for end of this file transmission
>>>>>>> f1af571315cb0030b2919816ee2cdf634087ee4e
    }
}

// helper function converts strings to Message
Message stringsToMessage(char* buffer) {
    Message message;
    const char breaker[2] = ":";
    message.type = atoi(strtok(buffer, breaker));
    message.size = atoi(strtok(NULL, breaker)); //use null to continue scanning
    strcpy(message.source, strtok(NULL, breaker));
    strcpy(message.data, strtok(NULL, breaker));
    return message;
}



// typedef struct packet{
//     unsigned int total_frag;
//     unsigned int frag_no;
//     unsigned int size;
//     char* filename;
//     char filedata[max_package_size];
// }Packet;

// //This is the helper function that parse the recieved message into packet,
// //it will create a new file if the message is the first one and close file after the last message
// int packet_from_message(char* message, int prev_index){
//     //first use strtok function to break up the message header
//     //referred to this tutorial https://www.cplusplus.com/reference/cstring/strtok/
//     FILE* FD = NULL;

//     const char breaker[2] = ":";
//     char* total_frag = strtok(message, breaker);
//     char* frag_no = strtok(NULL, breaker); //use null to continue scanning
//     char* size = strtok(NULL, breaker);
//     char* filename = strtok(NULL, breaker);

//     //now turn strings to int values
//     unsigned int Total_frag = atoi(total_frag);
//     unsigned int Frag_no = atoi(frag_no);
//     unsigned int dSize = atoi(size);

//     /*if(Frag_no != (prev_index) + 1){
//         return -1;
//     }*/

//     if(Frag_no == 1){
//         //this is the first message revieved, need to create a Packet struct and file stream
//         Packet recieved;
//         recieved.filename = filename;
//         recieved.frag_no = Frag_no;
//         recieved.size = dSize;
//         recieved.total_frag = Total_frag;

//         //create file stream, open a file
//         FD = fopen(filename, "wb"); //recieved file needs to have same name as sent one
//         if(FD == NULL){
//             printf("Cannot create file\n"); 
//             return -1; // -1 for error
//         }
//     }

//     //now, write data into the created file use fwrite to write to stream
//     int header_size = strlen(total_frag) + strlen(frag_no) + strlen(size) + strlen(filename);
//     //fwrite, notice need to skip the header
//     fwrite(message + header_size + sizeof(char) * 4, sizeof(char), dSize, FD);

//     //now check if the last message is done transmitting
//     if(Frag_no == Total_frag){
//         //fclose(fd);
//         return 1; // 1 for end of this file transmission
//     }

//     return 0; // 0 for success and continue this file transmission
// }

// void message_from_packet(Packet packet, char* message) {
//     int index = sprintf(message, "%d:%d:%d:%s:", packet.total_frag, 
//             packet.frag_no, packet.size, packet.filename);  // fill in everything except data region
//     for(int i=0; i<packet.size; i++) {
//         message[index+i] = packet.filedata[i];  // fill in the data region
//     }
// }

// void constructPacketsArray(Packet* array, int total_packets, char* data, char* fileName, int remaining_file) {
//     for(int index=0; index<total_packets; index++) {
//         array[index].total_frag = total_packets;
//         array[index].frag_no = index+1;

//         /* pay attention to the last packet's size */
//         if(index==total_packets-1 && remaining_file!=0) {
//             array[index].size = remaining_file;
//         } else {
//             array[index].size = max_package_size;
//         }

//         array[index].filename = fileName;

//         if(index==total_packets-1 && remaining_file!=0) {
//             for(int i=0; i<remaining_file; i++) {
//                 array[index].filedata[i] = data[i+index*max_package_size];
//             }
//         } else {
//             for(int i=0; i<max_package_size; i++) {
//                 array[index].filedata[i] = data[i+index*max_package_size];
//             }
//         }
//     }
// }
