/******************************************************************
Name: Lifang Yan
Date: 3-13-20
Course: CS344 - Operating System
Assignment: OTP
Descripting: This assignment is going to use port to transfer info from client to server
then encrypt msg using one time pad, then we use another set of pad to decrypt msg 
and send back to client 
File Description: This file acts as a server, receive msg from client, and actually encrypt msg here
************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char* msg) { perror(msg); exit(1);} //error function used for reporint issues
void encryptStep(char *buffer, char *key, char* encrypt, int length);//use one time pad to encrypt msg 

int main(int argc, char *argv[]){
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead, status;
    socklen_t sizeOfClientInfo;
    struct sockaddr_in serverAddress, clientAddress;
    int size_of_text;
    pid_t pid;
    char buf[1024];

    //check if it is rihgt number of command 
    if(argc < 2) { fprintf(stderr, "USAGE: %s port\n", argv[0]); exit(1); }

    //set up the address struct for this process (the server)
    memset((char*)&serverAddress, '\0', sizeof(serverAddress)); //clear out the address struct
    portNumber = atoi(argv[1]); //get the port number, convert to an integer from a string 
    serverAddress.sin_family = AF_INET; //create a network-capable socket 
    serverAddress.sin_port = htons(portNumber); //store the port number 
    serverAddress.sin_addr.s_addr = INADDR_ANY; //any address is allowed for connection to this process

    //set up the socket 
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if(listenSocketFD < 0){ fprintf(stderr, "USAGE: %s port\n", argv[0]); exit(1);};

    //enable the socket to begin listening 
    if(bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
        fprintf(stderr, "ERROR on binding");
    }

    //listen call tells the socket to listen to the incoming connection
    //listen function places all incoming connection into a backlog until accept call accepts the connection 
    listen(listenSocketFD, 5); //flip the socket on - it can now receive up to 5 connections 

    while(1){ //up to 5 sockets 
        //accept a connection, blocking if one is not available until one connects  
        //accept will write connection client's address info into the address structure and the size of that structure
        //and return a new socket file descriptor for accepted conneciton, so original socket file descriptor can continue to accept new connections
        sizeOfClientInfo = sizeof(clientAddress); //get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr* )&clientAddress, &sizeOfClientInfo);//accept
        //within the child process, first check to make suire it is communicating with otp_enc
        //after verifying, then recieve plaintext and key via the communication socket 
        //check to make sure the key at least as big as the plaintext 
        if(establishedConnectionFD < 0) { fprintf(stderr, "ERROR on accept");}
        //get the message from client and display it 
        //use fork to control multiple sockets 
        
        pid = fork();//use fork to control each connection 
        switch(pid){
            case -1:{
                fprintf(stderr, "Hull breach! Fork failed!\n");
            }
            case 0://if fork create successfully 
            {
                memset(buf, '\0', 1024);//clear out buf 
                charsRead = 0;
                charsRead = recv(establishedConnectionFD, buf, 1023, 0);//check if it is correct client 
                if(charsRead < 0){fprintf(stderr, "ERROR writing to socket"); }
                if(strcmp(buf, "otp_dec") == 0){
                    charsRead = send(establishedConnectionFD, "N", 1, 0);
                }
                else{
                    charsRead = send(establishedConnectionFD, "Y", 1, 0);
                    if(charsRead < 0){fprintf(stderr, "ERROR writing to socket"); }
                    //get size of input file and key 

                    memset(buf, '\0', sizeof(buf));
                    charsRead = recv(establishedConnectionFD, buf, sizeof(buf), 0);
                    if(charsRead < 0){fprintf(stderr, "ERROR receiving size from client"); }
                    size_of_text = atoi(buf);
                           
                    char text_msg[100000];
                    char key[100000];
                    char encrypt_msg[100000];
                    //get the message from the client and display it
                    memset(buf, '\0', sizeof(buf));
                    if(size_of_text <= 1000){//if size < 1000, transfer once 
                        charsRead = recv(establishedConnectionFD, buf, size_of_text-1, 0);
                        strcat(text_msg, buf); 
                        memset(buf, '\0', sizeof(buf));
                        charsRead = recv(establishedConnectionFD, buf, size_of_text-1, 0);
                        strcat(key, buf);
                    }else{//if size over 1000, recv 1000 once 
                        int left = size_of_text;
                        int index = 0;
                        while(left > 1000){
                            charsRead = recv(establishedConnectionFD, buf, 1000, 0);
                            index += 1000;
                            left -= 1000;
                            strcat(text_msg, buf);
                            memset(buf, '\0', sizeof(buf));
                        }
                        charsRead = recv(establishedConnectionFD, buf, left-1, 0); 
                        strcat(text_msg, buf); //use strcat add new received char to text_msg

                        left = size_of_text; //key size we use the same as text
                        index = 0;
                        while(left > 1000){
                            charsRead = recv(establishedConnectionFD, buf, 1000, 0);
                            index += 1000;
                            left -= 1000;
                            strcat(key, buf);
                            memset(buf, '\0', sizeof(buf));
                        }
                        charsRead = recv(establishedConnectionFD, buf, left-1, 0); 
                        strcat(key, buf); //same as received text 
                    }
                    memset(buf, '\0', sizeof(buf));
            
                    //encrypt message using key received from socket 
                    encryptStep(text_msg, key, encrypt_msg, size_of_text);
                    
                    //send a success message back to the client 
                    if(size_of_text <= 1000){
                        charsRead = send(establishedConnectionFD, encrypt_msg, size_of_text-1, 0); //write to the server 
                    }else{
                        int left = size_of_text;
                        int index = 0;
                        while(left > 1000){
                            charsRead = send(establishedConnectionFD, encrypt_msg+index, 1000, 0); //write to the server 
                            index += 1000;
                            left = left - 1000;
                        }
                        charsRead = send(establishedConnectionFD, encrypt_msg+index, left - 1, 0); //write to the server 
                    }
                    //send success bac
                    if(charsRead < 0){ fprintf(stderr, "ERROR writing to socket"); }
                    exit(0);
                }
            }
            default:{
                pid_t actualpid = waitpid(pid, &status, WNOHANG);
            } 
        }
        close(establishedConnectionFD);//close the existing socket which is connected to the client  
    }
    close(listenSocketFD);//close the listening pocket

    return 0;
}

/**************************************************************
encryptStep(char buffer, char key, char * msg)
this function used to encrypt the message using one time pad key
within this key, only include 26 captalized letters from A to Z, and one space
I define space as 64, so all 27 characters are continued 
when I see a white space, I will define that value as 64, same as when I see 64, define that character as space
***********************************************************************/
void encryptStep(char *buffer, char *key, char *msg, int length){
    char encrypt[length];
    int i = 0;
    //while(buffer[i] != '\0'){
    while(i < (length-1)){
        int b, k;
        if(buffer[i] == ' '){ buffer[i] = 64; } //space, define value as 64
        if(key[i] == ' '){ key[i] = 64; } //see space, define value as 64
        b = buffer[i];
        k = key[i];
        int m = (b - 64 + k - 64) % 27 + 64; //calculate the value of corresponding character
        char c = m; //use char c convert integer to letters 
        if(m == 64){
            encrypt[i] = ' ';
            i++;
        }else{
            encrypt[i] = c;
            i++;
        }
    }
    encrypt[i] = '\0'; //last character is new line
    strcpy(msg, encrypt); //copy the string to pointer 
}