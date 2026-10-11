
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#define PORT 8082
#define SIZE 1024

int main()
{
    int sockfd, n;
    pid_t pid;
    struct sockaddr_in server, client;
    socklen_t len = sizeof(client);
    char message[SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("bind");
        close(sockfd);
        return 1;
    }

    printf("UDP Server waiting...\n");

    n = recvfrom(sockfd, message, SIZE - 1, 0,
                 (struct sockaddr *)&client, &len);

    if (n < 0)
    {
        perror("recvfrom");
        close(sockfd);
        return 1;
    }

    printf("Client connected\n");

    pid = fork();

    if (pid == 0)
    {
        while (1)
        {
            n = recvfrom(sockfd, message, SIZE - 1, 0, NULL, NULL);
            if (n < 0)
                continue;

            message[n] = '\0';
            printf("\nClient: %s\n", message);
            fflush(stdout);

            if (strcmp(message, "exit") == 0)
            {
                sendto(sockfd, "exit", 4, 0,
                       (struct sockaddr *)&client, len);
                kill(getppid(), SIGTERM);
                close(sockfd);
                exit(0);
            }
        }
    }
    else if (pid > 0)
    {
        while (1)
        {
            printf("\r\033[KClient: %s\n", message);
            printf("Server: ");
            fflush(stdout);

            if (fgets(message, SIZE, stdin) == NULL)
                strcpy(message, "exit");

            message[strcspn(message, "\n")] = '\0';

            sendto(sockfd, message, strlen(message), 0,
                   (struct sockaddr *)&client, len);

            if (strcmp(message, "exit") == 0)
                break;
        }

        kill(pid, SIGTERM);
    }
    else
    {
        perror("fork");
        close(sockfd);
        return 1;
    }

    close(sockfd);
    return 0;
}
