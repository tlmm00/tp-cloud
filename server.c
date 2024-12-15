#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_STR_LEN 200

int BUFFER_SIZE = 1024;
char *IP = "127.0.0.1";
char *IP6 = "::1";
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
            fprintf(stderr, "[-] Memory allocation failed.\n");
            exit(1);
        }
        // Copy the string without the leading `--`
        strcpy(result, input + prefix_len);
        return result;
    } else {
        // Allocate memory and return a copy of the original string
        char* result = (char*)malloc(strlen(input) + 1);
        if (result == NULL) {
            fprintf(stderr, "[-] Memory allocation failed.\n");
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

void configSockIPv4(int *sock, struct sockaddr_in *addr) {

    // creating socket
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        perror("[-] Socket error");
        exit(1);
    }
    printf("[+] IPv4 TCP server socket created.\n");

    memset(addr, '\0', sizeof(&addr));
    addr->sin_family = AF_INET;
    addr->sin_port = PORT;
    addr->sin_addr.s_addr = inet_addr(IP);

    // Bind the socket to the address and port
    if (bind(*sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0) {
        perror("[-] Bind error");
        close(*sock);
        exit(1);
    }
    printf("[+] Bind to the port number: %d\n", PORT);
}

void configSockIPv6(int *sock, struct sockaddr_in6 *addr) {

    // Creating socket
    *sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (*sock < 0) {
        perror("[-] Socket error");
        exit(1);
    }
    printf("[+] IPv6 TCP server socket created.\n");

    // Initialize the address structure
    memset(addr, 0, sizeof(struct sockaddr_in6));
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(PORT); // Convert port to network byte order

    // Set IPv6 address
    addr->sin6_addr = in6addr_any; // Bind to any IPv6 address

    // Bind the socket to the address and port
    if (bind(*sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in6)) < 0) {
        perror("[-] Bind error");
        close(*sock);
        exit(1);
    }
    printf("[+] Bind to the port number: %d\n", PORT);
}

int main() {
    bool is_ipv6 = true;
    
    int server_sock, clinet_sock;
    int bytes_received;

    struct sockaddr_in server_addr, client_addr;
    struct sockaddr_in6 server_addr6, client_addr6;

    socklen_t addr_size;

    char buffer[BUFFER_SIZE];

    if(is_ipv6){
        configSockIPv6(&server_sock,&server_addr6);
    }else {
        configSockIPv4(&server_sock,&server_addr);
    }
    
    // listen to the client
    listen(server_sock, 5);
    printf("Listening...\n");

    while(1){
        if(is_ipv6){
            addr_size = sizeof(client_addr6);
            clinet_sock = accept(server_sock, (struct sockaddr*)&client_addr6, &addr_size);        
        }else {
            addr_size = sizeof(client_addr);
            clinet_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        }

        printf("[+] Client connected.\n");

        // receive msg from client
        recvMsg(clinet_sock, buffer);

        // if msg is READY
        if (strcmp(buffer,"READY")==0){

            // TODO: correct filename construction 
            OUT_FILE = fopen("127.0.0.1:dir", "w");
            if(OUT_FILE == NULL){
                perror("[-] Failed to open output file");
                return 1;
            }
            
            // server send READY ACK
            sendMsg(clinet_sock, buffer, "READY ACK");
            //sleep(1);

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