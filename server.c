#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_STR_LEN 200

int BUFFER_SIZE = 1024;
char *IP = "127.0.0.1";
int PORT = 5566;
FILE *OUT_FILE;

char* charDestuffing(char* input){
    const char* prefix = "--";
    size_t prefix_len = strlen(prefix);

    // Check if the string starts with `--`
    if (strncmp(input, prefix, prefix_len) == 0) {
        // Allocate memory for the result string
        char* result = (char*)malloc(strlen(input) - prefix_len + 1);
        if (result == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            exit(1);
        }
        // Copy the string without the leading `--`
        strcpy(result, input + prefix_len);
        return result;
    } else {
        // Allocate memory and return a copy of the original string
        char* result = (char*)malloc(strlen(input) + 1);
        if (result == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            exit(1);
        }
        strcpy(result, input);
        return result;
    }
}

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
    int server_sock, clinet_sock;
    int bytes_received;

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    char buffer[BUFFER_SIZE];
    int n;



    // creating server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0){
        perror("[-] Socket error");
        exit(1);
    }
    printf("[+] TCP server socket created.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = PORT;
    server_addr.sin_addr.s_addr = inet_addr(IP);

    // bind the address and port number
    n = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (n<0){
        perror("[-] Bind error");
        exit(1);
    }
    printf("[+] Bind to the port number: %d\n", PORT);

    // listen to the client
    listen(server_sock, 5);
    printf("Listening...\n");

    while(1){
        addr_size = sizeof(client_addr);
        clinet_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        printf("[+] Client connected.\n");

        // receive msg from client
        recvMsg(clinet_sock, buffer);

        // if msg is READY
        if (strcmp(buffer,"READY")==0){

            // TODO: correct filename construction 
            OUT_FILE = fopen("127.0.0.1:DIR", "w");
            if(OUT_FILE == NULL){
                perror("[-] Failed to open output file");
                return 1;
            }
            
            // server send READY ACK
            sendMsg(clinet_sock, buffer, "READY ACK");
            sleep(1);

            recvMsg(clinet_sock, buffer);

            
            // while not receive BYE
            while(strcmp(buffer, "BYE")!=0){
                // write received msg to file
                fprintf(OUT_FILE, "%s\n", charDestuffing(buffer));

                // send ACK
                sendMsg(clinet_sock, buffer, "ACK");
                
                // receive client msg
                recvMsg(clinet_sock, buffer);
            }

            fclose(OUT_FILE);
        }

        // if msg is BYE
        if(strcmp(buffer, "BYE")==0){
            // close connection
            close(clinet_sock);
            printf("[+] Client disconnected. \n\n");
        }   
    }

    return 0;
}