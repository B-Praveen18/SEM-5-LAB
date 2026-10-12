#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 15000
#define SIZE 1024

int main() {
    int sockfd, n;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);
    char message[SIZE], reply[SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    printf("UDP Half-Duplex Server waiting on port %d...\n", PORT);
    printf("Type 'over' to pass turn, 'exit' to quit.\n\n");

    /* Wait for first message from client */
    client_len = sizeof(client);
    n = recvfrom(sockfd, message, SIZE - 1, 0,
                 (struct sockaddr *)&client, &client_len);
    if (n < 0) {
        perror("recvfrom");
        close(sockfd);
        return 1;
    }
    message[n] = '\0';

    while (1) {
        /* ========== SERVER RECEIVING PHASE ========== */
        while (1) {
            printf("Client: %s\n", message);

            if (strcmp(message, "exit") == 0) {
                printf("Client exited. Closing server.\n");
                close(sockfd);
                return 0;
            }

            if (strcmp(message, "over") == 0) {
                printf("[Server's turn]\n");
                break;
            }

            /* Get next message from client */
            client_len = sizeof(client);
            n = recvfrom(sockfd, message, SIZE - 1, 0,
                         (struct sockaddr *)&client, &client_len);
            if (n < 0) {
                perror("recvfrom");
                close(sockfd);
                return 1;
            }
            message[n] = '\0';
        }

        /* ========== SERVER SENDING PHASE ========== */
        while (1) {
            printf("Server: ");
            fgets(reply, SIZE, stdin);
            reply[strcspn(reply, "\n")] = '\0';

            sendto(sockfd, reply, strlen(reply), 0,
                   (struct sockaddr *)&client, client_len);

            if (strcmp(reply, "exit") == 0) {
                printf("Server exiting.\n");
                close(sockfd);
                return 0;
            }

            if (strcmp(reply, "over") == 0) {
                printf("[Client's turn]\n");
                /* Fetch first message of client's next turn */
                client_len = sizeof(client);
                n = recvfrom(sockfd, message, SIZE - 1, 0,
                             (struct sockaddr *)&client, &client_len);
                if (n < 0) {
                    perror("recvfrom");
                    close(sockfd);
                    return 1;
                }
                message[n] = '\0';
                break;  /* Back to receiving phase */
            }
        }
    }

    close(sockfd);
    return 0;
}
