#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h> // gettimeofday
#include <dirent.h>
#include <stdbool.h>

#define MAX_STR_LEN 200

int BUFFER_SIZE = 1024;
char *IP = "127.0.0.1";
char *IP6 = "::1";
int PORT = 5566;
char *DIR_PATH = "./dir";
int TOTAL_DATA_SEND = 0;

char* charStuffing(char* input){

    char* result = (char*)malloc(MAX_STR_LEN);
    if (result == NULL) {
        fprintf(stderr, "[-] Memory allocation failed.\n");
        exit(1);
    }

    // Check if the string is "BYE" or starts with "--"
    if (strcmp(input, "BYE") == 0 || strncmp(input, "--", 2) == 0) {
        snprintf(result, MAX_STR_LEN, "--%s", input);
    } else {
        snprintf(result, MAX_STR_LEN, "%s", input); // Return the original string
    }

    return result;
}

void sendMsg(int sock, char *buffer, char *msg, bool stuf){
    bzero(buffer, BUFFER_SIZE);

    if (stuf)
        strcpy(buffer, charStuffing(msg));
    else
        strcpy(buffer, msg);

    printf("Sending: %s\n", buffer);
    TOTAL_DATA_SEND += strlen(buffer);
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
    if (inet_pton(AF_INET6, IP6, &addr->sin6_addr) <= 0) {
        perror("[-] Invalid IPv6 address");
        close(*sock);
        exit(1);
    }
}

int main() {
    bool is_ipv6 = true;

    int sock;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    socklen_t addr_size;
    
    char buffer[BUFFER_SIZE];

    if(is_ipv6){
        configSockIPv6(&sock, &addr6);
        connect(sock, (struct sockaddr*)&addr6, sizeof(addr6));
    }else {
        configSockIPv4(&sock, &addr);
        connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    }

    // connect to server
    printf("[+] Connected to the server.\n");

    // send msg READY
    sendMsg(sock, buffer, "READY", true);

    // TODO: gettimeofday()
    struct timeval start, end;

    // Record the start time
    if (gettimeofday(&start, NULL) != 0) {
        perror("[-] Failed to get start time");
        return 1;
    }
    // receive READY ACK
    recvMsg(sock, buffer);
    //sleep(1);
    if(strcmp(buffer, "READY ACK")==0){
        // start sending file names
        DIR *dr = opendir(DIR_PATH);
        struct dirent *en;

        while((en = readdir(dr)) != NULL){
            char *file_name = en->d_name;

            // if file name is not "." or ".."
            if(strcmp(file_name, ".")!=0 && strcmp(file_name, "..")!=0){
                

                while(strcmp(buffer, "ACK")!=0){
                    sendMsg(sock, buffer, file_name, true);
                    //sleep(1);
                    recvMsg(sock, buffer);
                }
            }

            // clean buffer before next message
            bzero(buffer, BUFFER_SIZE);
        }
        closedir(dr);
    }

    // send msg BYE
    sendMsg(sock, buffer, "BYE", false);
    
    // Record the end time
    if (gettimeofday(&end, NULL) != 0) {
        perror("[-] Failed to get end time");
        return 1;
    }

    // Calculate the elapsed time
    double seconds = end.tv_sec - start.tv_sec; // Difference in seconds
    double microseconds = end.tv_usec - start.tv_usec; // Difference in microseconds

    double totalTime = seconds + microseconds*1e-6;

    printf("\nTotal time of operation: %.6fs\n", totalTime);

    // 8 bit for each char send
    double thoughput = TOTAL_DATA_SEND * 8 / totalTime;
    printf("Thoughput: %.6f bps\n", thoughput);
    close(sock);
    printf("Disconnected from the server. \n\n");

    return 0;
}