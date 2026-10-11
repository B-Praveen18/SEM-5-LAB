
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 2121

int main(void)
{
    int s, ch, n;
    char b[512], name[100], dest[100];
    struct sockaddr_in addr = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 1; }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        close(s);
        return 1;
    }

    n = recv(s, b, sizeof(b)-1, 0);
    if (n <= 0) { close(s); return 1; }
    b[n] = '\0';
    printf("%s", b);

    /* LOGIN */
    while (1)
    {
        printf("Username: ");
        scanf("%99s", name);

        snprintf(b, sizeof(b), "USER %s\n", name);
        send(s, b, strlen(b), 0);

        n = recv(s, b, sizeof(b)-1, 0);
        if (n <= 0) { close(s); return 1; }
        b[n] = '\0';
        printf("%s", b);

        if (strncmp(b, "331", 3) != 0)
        {
            printf("Try username again.\n");
            continue;
        }

        printf("Password: ");
        scanf("%99s", name);

        snprintf(b, sizeof(b), "PASS %s\n", name);
        send(s, b, strlen(b), 0);

        n = recv(s, b, sizeof(b)-1, 0);
        if (n <= 0) { close(s); return 1; }
        b[n] = '\0';
        printf("%s", b);

        if (strncmp(b, "230", 3) == 0)
            break;

        printf("Login failed. Start again.\n");
    }

    /* MENU */
    while (1)
    {
        printf("\n1.CREATE  2.DELETE  3.PWD  4.DIR");
        printf("  5.CP  6.MV  7.EXIT\nChoice: ");

        if (scanf("%d", &ch) != 1) break;

        if (ch == 1 || ch == 2 || ch == 5 || ch == 6)
        {
            printf("Filename: ");
            scanf("%99s", name);

            if (ch == 1)
                snprintf(b, sizeof(b), "CREATE %s\n", name);
            else if (ch == 2)
                snprintf(b, sizeof(b), "DELETE %s\n", name);
            else
            {
                printf("Destination: ");
                scanf("%99s", dest);

                snprintf(b, sizeof(b), "%s %s %s\n",
                         ch == 5 ? "CP" : "MV", name, dest);
            }
        }
        else if (ch == 3)
            strcpy(b, "PWD\n");
        else if (ch == 4)
            strcpy(b, "DIR\n");
        else if (ch == 7)
            strcpy(b, "QUIT\n");
        else
            continue;

        send(s, b, strlen(b), 0);

        n = recv(s, b, sizeof(b)-1, 0);
        if (n <= 0) break;
        b[n] = '\0';
        printf("%s", b);

        if (ch == 7) break;
    }

    close(s);
    return 0;
}
