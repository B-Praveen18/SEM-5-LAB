#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 2121
#define SIZE 2048

int control_fd;

int send_all(int fd, const char *buf, size_t len)
{
    while (len > 0)
    {
        ssize_t n = send(fd, buf, len, 0);
        if (n <= 0)
            return -1;
        buf += n;
        len -= (size_t)n;
    }
    return 0;
}

int send_line(const char *line)
{
    char buf[SIZE];
    if (snprintf(buf, sizeof(buf), "%s\r\n", line) >= (int)sizeof(buf))
        return -1;
    return send_all(control_fd, buf, strlen(buf));
}

int read_line(int fd, char *buf, size_t size)
{
    size_t i = 0;
    char ch;
    while (i + 1 < size)
    {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0)
            return (int)n;
        if (ch == '\n')
            break;
        if (ch != '\r')
            buf[i++] = ch;
    }
    buf[i] = '\0';
    return 1;
}

int get_reply(char *reply, size_t size)
{
    int n = read_line(control_fd, reply, size);
    if (n > 0)
        printf("%s\n", reply);
    return n;
}

int main(void)
{
    struct sockaddr_in server;
    char reply[SIZE], username[128], password[128], command[SIZE], arg1[512], arg2[512];
    int choice;

    control_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (control_fd < 0)
    {
        perror("socket");
        return 1;
    }
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(control_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect");
        close(control_fd);
        return 1;
    }
    if (get_reply(reply, sizeof(reply)) <= 0)
    {
        close(control_fd);
        return 1;
    }

    printf("Username: ");
    if (!fgets(username, sizeof(username), stdin))
        return 1;
    username[strcspn(username, "\n")] = '\0';
    snprintf(command, sizeof(command), "USER %s", username);
    send_line(command);
    if (get_reply(reply, sizeof(reply)) <= 0 || strncmp(reply, "331", 3))
    {
        close(control_fd);
        return 1;
    }

    printf("Password: ");
    if (!fgets(password, sizeof(password), stdin))
        return 1;
    password[strcspn(password, "\n")] = '\0';
    snprintf(command, sizeof(command), "PASS %s", password);
    send_line(command);
    if (get_reply(reply, sizeof(reply)) <= 0 || strncmp(reply, "230", 3))
    {
        printf("Authentication failed.\n");
        close(control_fd);
        return 1;
    }

    while (1)
    {
        printf("\n--- FTP MENU ---\n");
        printf("1. PWD       2. DIR       3. CREATE\n");
        printf("4. DEL       5. MKD       6. RMD\n");
        printf("7. MOVE      8. CP        9. STRU F\n");
        printf("10. QUIT\nChoice: ");
        if (scanf("%d", &choice) != 1)
            break;
        getchar();
        arg1[0] = arg2[0] = '\0';

        if (choice == 1)
            strcpy(command, "PWD");
        else if (choice == 2)
            strcpy(command, "DIR");
        else if (choice == 3 || choice == 4 || choice == 5 || choice == 6)
        {
            printf("Filename/directory name: ");
            if (!fgets(arg1, sizeof(arg1), stdin))
                break;
            arg1[strcspn(arg1, "\n")] = '\0';
            const char *op = choice == 3 ? "CREATE" : choice == 4 ? "DEL"
                                                  : choice == 5   ? "MKD"
                                                                  : "RMD";
            snprintf(command, sizeof(command), "%s %s", op, arg1);
        }
        else if (choice == 7 || choice == 8)
        {
            printf("Source name: ");
            if (!fgets(arg1, sizeof(arg1), stdin))
                break;
            arg1[strcspn(arg1, "\n")] = '\0';
            printf("Destination name: ");
            if (!fgets(arg2, sizeof(arg2), stdin))
                break;
            arg2[strcspn(arg2, "\n")] = '\0';
            snprintf(command, sizeof(command), "%s %s %s", choice == 7 ? "MOVE" : "CP", arg1, arg2);
        }
        else if (choice == 9)
            strcpy(command, "STRU F");
        else if (choice == 10)
            strcpy(command, "QUIT");
        else
        {
            printf("Invalid choice.\n");
            continue;
        }

        if (send_line(command) < 0 || get_reply(reply, sizeof(reply)) <= 0)
        {
            printf("Connection lost.\n");
            break;
        }
        if (choice == 10)
            break;
    }
    close(control_fd);
    return 0;
}
