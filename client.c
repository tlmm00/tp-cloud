#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h> // gettimeofday
#include <dirent.h>

int BUFFER_SIZE = 1024;
char *IP = "127.0.0.1";
int PORT = 5566;
char *DIR_PATH = "./dir";

void sendMsg(int sock, char *buffer, char *msg){
    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, msg);
    printf("Sending: %s\n", buffer);
    send(sock, buffer, strlen(buffer), 0);
}

char* recvMsg(int sock, char *buffer){
    bzero(buffer, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Received: %s\n", buffer);

    return buffer;
}

int main() {
    int sock;
    struct sockaddr_in addr;
    socklen_t addr_size;

    char buffer[BUFFER_SIZE];
    int n;

    // creating socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror("[-] Socket error");
        exit(1);
    }
    printf("[+] TCP server socket created.\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = PORT;
    addr.sin_addr.s_addr = inet_addr(IP);

    // connect to server
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    printf("Connected to the server.\n");

    // send msg READY
    sendMsg(sock, buffer, "READY");

    // TODO: gettimeofday()
    struct timeval start, end;

    // Record the start time
    if (gettimeofday(&start, NULL) != 0) {
        perror("Failed to get start time");
        return 1;
    }

    // receive READY ACK
    recvMsg(sock, buffer);
    sleep(1);
    if(strcmp(buffer, "READY ACK")==0){
        // start sending file names
        DIR *dr = opendir(DIR_PATH);
        struct dirent *en;

        while((en = readdir(dr)) != NULL){
            char *file_name = en->d_name;


            if(strcmp(file_name, ".")!=0 && strcmp(file_name, "..")!=0){

                while(strcmp(buffer, "ACK")!=0){
                    sendMsg(sock, buffer, file_name);
                    sleep(1);
                    recvMsg(sock, buffer);
                }
            }

            // clean buffer before next message
            bzero(buffer, BUFFER_SIZE);
        }
        closedir(dr);
    }

    // send msg BYE
    sendMsg(sock, buffer, "BYE");
    
    // Record the end time
    if (gettimeofday(&end, NULL) != 0) {
        perror("Failed to get end time");
        return 1;
    }

    // Calculate the elapsed time
    long seconds = end.tv_sec - start.tv_sec; // Difference in seconds
    long microseconds = end.tv_usec - start.tv_usec; // Difference in microseconds

    printf("\nTotal time of operation: %ld,%lds\n", seconds, microseconds);

    close(sock);
    printf("Disconnected from the server. \n\n");

    return 0;
}