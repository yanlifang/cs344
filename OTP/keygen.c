/**********************************************************************
Name: Lifang Yan
Date: 3-13-20
Course: CS344 - Operating System
Assignment: OTP
Description: This file is used to generate a random key for encrypt message 
within this key, 27 characters are included, which are capalized A to Z
and space character. Use rand() to get ascii code corresponding dec from 64 to 90
which 64 corresponds to space
*************************************************************************/
#include <stdio.h>
#include <time.h>

int main(int argc, char* argv[]){
    //use srand() to generate seed 
    time_t t;
    srand((unsigned int) time(&t));
    //the length of key will be 2nd command arguments, then use atoi to convert char to integer 
    int max = atoi(argv[1]);
    //ch use to store key characters 
    char ch[max+1];
   
    int i;
    for(i = 0; i < max; i++){
        ch[i] = rand() % 27 + 64;
        //if we get @ ch, replace it with space 
        if(ch[i] == '@'){
            ch[i] = ' ';
        }
    }
    //add new line character in the end of key 
    ch[max] = '\0';
    //if command line does not specify the output file, print to screen, otherwise redi to key file 
    if(argc == 2){
        printf("%s\n", ch);
    }
    return 0;
}
