#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#define BUF_SIZE 1000

int main(int argc, char *argv[]) {
    int Sock;
    char buf[BUF_SIZE];
    int nbytes;
    struct sockaddr_in add, from;
    socklen_t from_len = sizeof(from);

    int LocalPort = 0;
    char *LocalIP = "149.156.203.6";
    int RemPort = 13333;
    char *RemIP = "127.0.0.1";

    printf("You can run this program as follows: %s [server_IP [server_PORT]]\n", argv[0]);

    // Create socket
    if ((Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error. Socket not created.");
        return 1;
    }
    printf("Socket created. Descriptor: %d\n", Sock);

    // Binding socket to the LocalIP and port assigned by system (LocalPort=0)
    memset(&add, 0, sizeof(struct sockaddr_in));
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = inet_addr(LocalIP);
    add.sin_port = htons((u_short)LocalPort);

    if (bind(Sock, (struct sockaddr*)&add, sizeof(add)) < 0) {
        perror("Error. Socket not bound.");
        close(Sock);
        return 1;
    }
    printf("Socket is bound\n");

    // Get remote server address and port from arguments
    if (argc > 1) RemIP = argv[1];
    if (argc > 2) RemPort = atoi(argv[2]);

    printf("Sock will communicate with %s:%d\n", RemIP, RemPort);

    // Prepare request message
    memset(buf, ' ', BUF_SIZE); // Fill buffer with spaces
    strncpy(buf, "get interface list", strlen("get interface list")); // Copy the request string

    memset(&add, 0, sizeof(struct sockaddr_in));
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = inet_addr(RemIP);
    add.sin_port = htons((u_short)RemPort);

    // Send request to server
    if (sendto(Sock, buf, BUF_SIZE, 0, (struct sockaddr*)&add, sizeof(add)) < 0) {
        perror("Error sending request.");
        close(Sock);
        return 1;
    }

    // Receive response from server
    nbytes = recvfrom(Sock, buf, BUF_SIZE, 0, (struct sockaddr*)&from, &from_len);
    if (nbytes < 0) {
        perror("Error receiving response.");
        close(Sock);
        return 1;
    }

    buf[nbytes] = '\0';
    printf("Received response:\n%s\n", buf);

    close(Sock);
    return 0;
}
