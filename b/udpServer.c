// UDP Server
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <ifaddrs.h>

#define BUF_SIZE 1000
#define RESPONSE_BUF_SIZE 1000
#define INTERFACE_LIST_REQUEST "get interface list"

int create_socket(int port, char *ip) {
    int Sock;
    struct sockaddr_in add;

    // Create socket
    if ((Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error. Socket not created.");
        return -1;
    }
    printf("Socket created. Descriptor: %d\n", Sock);

    // Bind socket to the network interface and port
    memset(&add, 0, sizeof(struct sockaddr_in));
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = inet_addr(ip);
    add.sin_port = htons((u_short)port);

    if (bind(Sock, (struct sockaddr*)&add, sizeof(add)) < 0) {
        perror("Error. Socket not bound.");
        close(Sock);
        return -1;
    }
    printf("Socket is bound to %s:%d\n", ip, port);

    return Sock;
}

void handle_request(int Sock) {
    char buf[BUF_SIZE];
    int nbytes;
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);

    // Receive request from client
    nbytes = recvfrom(Sock, buf, BUF_SIZE, 0, (struct sockaddr*)&from, &from_len);
    if (nbytes > 0) {
        buf[nbytes] = '\0';
        printf("Received request from %s:%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        printf("Data: %s\n", buf);

        if (strncmp(buf, INTERFACE_LIST_REQUEST, strlen(INTERFACE_LIST_REQUEST)) == 0) {
            char response[RESPONSE_BUF_SIZE] = "";
            int response_len = 0;

            struct ifaddrs *ifaddr, *ifa;
            if (getifaddrs(&ifaddr) == -1) {
                perror("Error getting network interfaces");
                return;
            }

            for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL) continue;

                char addr[INET_ADDRSTRLEN];
                if (ifa->ifa_addr->sa_family == AF_INET) {
                    inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, addr, sizeof(addr));
                    int len = snprintf(response + response_len, RESPONSE_BUF_SIZE - response_len, "%s: %s\n", ifa->ifa_name, addr);
                    if (len < 0 || response_len + len >= RESPONSE_BUF_SIZE - 1) {
                        break; // Ensure we don't overflow the buffer
                    }
                    response_len += len;
                }
            }

            freeifaddrs(ifaddr);

            // Pad response to RESPONSE_BUF_SIZE characters
            while (response_len < RESPONSE_BUF_SIZE - 1) {
                response[response_len++] = ' ';
            }
            response[RESPONSE_BUF_SIZE - 1] = '\0';

            if (sendto(Sock, response, RESPONSE_BUF_SIZE, 0, (struct sockaddr*)&from, from_len) < 0) {
                perror("Error sending response");
            }
        } else {
            printf("Unknown request: %s\n", buf);
        }
    } else if (nbytes < 0) {
        perror("Error receiving data");
    } else {
        printf("No data received.\n");
    }
}

int main(int argc, char *argv[]) {
    int Sock;
    int Server_Port = 12900;
    char *Server_IP = "0.0.0.0";

    printf("You can run this program as follows: %s [Server_Port [Server_IP]]\n", argv[0]);

    if (argc > 1) Server_Port = atoi(argv[1]);
    if (argc > 2) Server_IP = argv[2];

    Sock = create_socket(Server_Port, Server_IP);
    if (Sock < 0) {
        return 1;
    }

    // Main loop to handle incoming requests
    while (1) {
        handle_request(Sock);
    }

    close(Sock);
    return 0;
}

