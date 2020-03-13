/******************************************************************
Name: Lifang Yan
Date: 3-13-20
Course: CS344 - Operating System
Assignment: OTP
Descripting: This assignment is going to use port to transfer info from client to server
then encrypt msg using one time pad, then we use another set of pad to decrypt msg 
and send back to client 
File Description: This is client side, send msg to server 
************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg){ perror(msg); exit(0);} //error function for reporint issues 
int badChar(char* s){ //this function used to check the input text file has invalid ch or not 
    int i = 0;
    int badCh = 0;
    while(s[i] != '\n'){
        if(((s[i] >= 'A') && (s[i] <= 'Z')) || (s[i] == ' ')){//only captalized letters or space were valid
            badCh = 0;
        }
        else{
            return 1;
        }
        i++;
    }
    return 0;
}

int main(int argc, char* argv[]){
    int socketFD, portNumber, charsWritten, charsRead, otpvalue;
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;
    int sizeOfText;//use to store the size of received text and size of key 
    int sizeOfKey;
    char *buffer_text = NULL;
    char *buffer_key = NULL;
    size_t len_text = 0;
    size_t len_key = 0;
    char buffer[1024];

    if(argc < 4){fprintf(stderr, "USAGE: %s hostname port\n", argv[0]); exit(0); } //check usage & args

    //set up the server address struct 
    memset((char*)&serverAddress, '\0', sizeof(serverAddress)); //clear out the address struct 
    portNumber = atoi(argv[3]); //get the port number, convert to an integer from a string 
    serverAddress.sin_family = AF_INET; //create a network-capable socket 
    serverAddress.sin_port = htons(portNumber); //store the port number 
    serverHostInfo = gethostbyname("localhost"); // //convert the machine name into a special form of address
    if(serverHostInfo == NULL){ fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); //copy in the address

    //set up the socket 
    socketFD = socket(AF_INET, SOCK_STREAM, 0);//create the socket 
    if(socketFD < 0){ fprintf(stderr, "CLIENT: ERROR opening socket\n");}

    otpvalue = 1;
    setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &otpvalue, sizeof(int));//make socket resuable 
    //connect to server 
    int d = connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
   // if(connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){//connect socket to address
    if(d < 0){
        fprintf(stderr, "CLIENT: ERROR connecting\n");
        exit(2);
    }
    //check to make sure it is communicating with right socket 
    char* prog = "otp_enc";
    //send the firle name to make sure this will connect to right port 
    charsWritten = send(socketFD, prog, strlen(prog), 0); 
    if(charsWritten < 0) {fprintf(stderr, "CLIENT: ERROR writing to socket"); }
    if(charsWritten < strlen(prog)){ printf("CLIENT: WARNING: Not all data written to socket!\n"); }
    charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0); //read data from socket
    if(charsRead < 0){ fprintf(stderr, "CLIENT: ERROR reading from socket"); }
    //check to make sure if it is right connect, if not exit with 2 
    if(strcmp(buffer, "N") == 0){
        fprintf(stderr, "otp_enc cannot use otp_dec_d\n");
        exit(2);
    }else{
        //first we are going to read message, then we are going to read key 
        //read message and get the size of message 
        FILE* infile_text = fopen(argv[1], "r"); //read only mode 
        //test if file is existing or not 
        if(infile_text == NULL){ fprintf(stderr, "ERROR, could not open file!\n"); exit(1); }
        getline(&buffer_text, &len_text, infile_text);
        fclose(infile_text);
        //check if text file has invalid character 
        if(badChar(buffer_text)){
            fprintf(stderr, "Error: readin text file has invalid characters!\n");
            free(buffer_text);
            return 1;
        }

        //read key and get the size of key, the key has to be at least as big as message  
        FILE* infile_key = fopen(argv[2], "r");
        if(infile_key == NULL){ fprintf(stderr, "ERROR, could not open key file!\n"); exit(1); }
        getline(&buffer_key, &len_key, infile_key);
        fclose(infile_key);
        //get the size of key and text, and check if the size of key is as big as text 
        sizeOfText = strlen(buffer_text);
        sizeOfKey = strlen(buffer_key);
        if(sizeOfText > sizeOfKey){
            //printf("Error, key '%s' is too short\n", argv[2]);
            fprintf(stderr, "Error, key is too short\n");
            exit(1);
        }
        else{

            memset(buffer, '\0', sizeof(buffer));
            sprintf(buffer, "%d", sizeOfText);
            //send text size to server  
            charsWritten = send(socketFD, buffer, sizeof(buffer), 0);
            if(charsWritten < 0) {fprintf(stderr, "CLIENT: ERROR writing to socket"); }
            if(charsWritten < 1){fprintf(stderr, "CLIENT: ERROR: incorrect send size of text and key file"); }
            //recv() would be something like recv(socket, buffer + 50, 100 - 50, 0)
            //send message to server
            buffer_text[strcspn(buffer_text, "\n")] = '\0';
            buffer_key[strcspn(buffer_key, "\n")] = '\0';
            //printf("size after remove new line %d\n", strlen(buffer_text));
            if(sizeOfText <= 1000){
                charsWritten = send(socketFD, buffer_text, sizeOfText-1, 0); //write to the server
                memset(buffer, '\0', sizeof(buffer)); 
                charsWritten = send(socketFD, buffer_key, sizeOfText-1, 0); //write to the server 
            }else{
                int left = sizeOfText;
                int index = 0;
                while(left > 1000){
                    charsWritten = send(socketFD, buffer_text+index, 1000, 0); //write to the server 
                    index += 1000;
                    left = left - 1000;
                }
                charsWritten = send(socketFD, buffer_text+index, left - 1, 0); //write to the server 
                //send key character to server 
                left = sizeOfText;
                index = 0;
                while(left > 1000){
                    charsWritten = send(socketFD, buffer_key+index, 1000, 0);
                    index += 1000;
                    left = left - 1000;
                }
                charsWritten = send(socketFD, buffer_key+index, left-1, 0);
            }
            if(charsWritten < 0) {fprintf(stderr, "CLIENT: ERROR writing text to socket"); }
            //send two integers tell the server the length of characters are going to send to server 
            //define new string to store the data received from server 
            char ciphertext[100000];
            memset(buffer, '\0', sizeof(buffer));     
            if(sizeOfText <= 1000){
                charsRead = recv(socketFD, buffer, sizeOfText-1, 0);
            }
            else{
                int left = sizeOfText;
                int index = 0;
                while(left > 1000){
                    charsRead = recv(socketFD, buffer, 1000, 0);
                    index += 1000;
                    left -= 1000;
                    strcat(ciphertext, buffer);
                    memset(buffer, '\0', sizeof(buffer));
                }
                charsRead = recv(socketFD, buffer, left-1, 0); 
                        
            }
            strcat(ciphertext, buffer);
            strcat(ciphertext, "\n");
            printf("%s", ciphertext);
        }
    }
    close(socketFD); //close the socket
    return 0;
}