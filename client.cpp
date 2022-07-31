#include <cstdio>
#include <iostream>
#include <memory>
#include <utility>
#include <chrono>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 2000

int main(int argc, const char *argv[]) {
    
    int secs = atoi(argv[2]);
    uint32_t requestId = htonl(atoi(argv[1])); 
    char message[] = "Wake up lovely service";
    uint32_t cookieSize = htonl(strlen(message));

    using namespace std::chrono;
     
    // timestamp for wake up, 5 Seconds in the future
    uint64_t sec = duration_cast<seconds>(system_clock::now().time_since_epoch()).count() + secs;
    std::cout << sec << " seconds since the Epoch\n";

    // convert from 64 bit to 2 32 bit integer
    uint32_t low = htonl((uint32_t)(sec >> 32));
    uint32_t high = htonl((uint32_t)sec);

    // send stuff over tcp socket
    int sock = 0, valread, client_fd;
    struct sockaddr_in serv_addr;
    char buffer[1024];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
  
    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
  
    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // use htonl to send the data 
    send(sock, &requestId, sizeof(uint32_t), 0);
    send(sock, &low, sizeof(uint32_t), 0);
    send(sock, &high, sizeof(uint32_t), 0);
    send(sock, &cookieSize, sizeof(uint32_t), 0);
    send(sock, message, strlen(message), 0);

    printf("Hello message sent\n");

    // read header - request id
    valread = read(sock, buffer, 4);
    printf("read bytes: %d\n", valread);
    printf("Message: %s\n", buffer);

    // read header - cookie size
    valread = read(sock, buffer, 4);
    printf("read bytes: %d\n", valread);
    printf("Message: %s\n", buffer);

    // read message
    valread = read(sock, buffer, 22);
    printf("read bytes: %d\n", valread);
    printf("Message: %s\n", buffer);

    // closing the connected socket
    close(client_fd);
    return 0;
}