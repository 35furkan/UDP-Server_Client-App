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

int main(int argc, char *argv[]) {
    int Sock;
    int Server_Port = 12900;
    char *Server_IP = "0.0.0.0";
    int nbytes;
    char buf[BUF_SIZE];
    struct sockaddr_in add, from;
    socklen_t from_len = sizeof(from);

    printf("You can run this program as follows: %s [Server_Port [Server_IP]]\n", argv[0]);

    // Create socket
    if ((Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error. Socket not created.");
        return 1;
    }
    printf("Socket created. Descriptor: %d\n", Sock);

    // Get network interface and port on which the server will listen
    if (argc > 1) Server_Port = atoi(argv[1]);
    if (argc > 2) Server_IP = argv[2];

    // Bind socket to the network interface and port
    memset(&add, 0, sizeof(struct sockaddr_in));
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = inet_addr(Server_IP);
    add.sin_port = htons((u_short)Server_Port);

    if (bind(Sock, (struct sockaddr*)&add, sizeof(add)) < 0) {
        perror("Error. Socket not bound.");
        close(Sock);
        return 1;
    }
    printf("Socket is bound\n");

    // Main loop to handle incoming requests
    while (1) {
        memset(buf, 0, sizeof(buf));
        memset(&from, 0, sizeof(struct sockaddr_in));

        // Receive request from client
        nbytes = recvfrom(Sock, buf, BUF_SIZE, 0, (struct sockaddr*)&from, &from_len);
        if (nbytes > 0) {
            buf[nbytes] = '\0';
            printf("Received request from %s:%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
            printf("Data: %s\n", buf);

            if (strncmp(buf, "get interface list", 18) == 0) {
                // Prepare and send the list of interfaces
                struct ifaddrs *ifaddr, *ifa;
                char response[BUF_SIZE] = "";
                int response_len = 0;

                if (getifaddrs(&ifaddr) == -1) {
                    perror("Error getting network interfaces");
                    close(Sock);
                    return 1;
                }

                for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                    if (ifa->ifa_addr == NULL) continue;

                    char addr[INET_ADDRSTRLEN];
                    if (ifa->ifa_addr->sa_family == AF_INET) {
                        inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, addr, sizeof(addr));
                        int len = snprintf(response + response_len, sizeof(response) - response_len, "%s: %s\n", ifa->ifa_name, addr);
                        response_len += len;
                        if (response_len >= BUF_SIZE - 1) {
                            break; // Ensure we don't overflow the buffer
                        }
                    }
                }

                freeifaddrs(ifaddr);

                // Pad response to 1000 characters
                while (response_len < BUF_SIZE - 1) {
                    response[response_len++] = ' ';
                }
                response[BUF_SIZE - 1] = '\0';

                if (sendto(Sock, response, BUF_SIZE, 0, (struct sockaddr*)&from, from_len) < 0) {
                    perror("Error sending response");
                }
            } else {
                printf("Unknown request: %s\n", buf);
            }
        } else if (nbytes < 0) {
            perror("Error receiving data");
        }
    }

    close(Sock);
    return 0;
}
