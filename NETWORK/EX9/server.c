
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define PORT 2121

void reply(int c, const char *msg)
{
    send(c, msg, strlen(msg), 0);
}

void handle(int c)
{
    char b[512], cmd[20], a[100], d[100];
    int n, user = 0, login = 0;

    reply(c, "220 Server ready\n");

    while ((n = recv(c, b, sizeof(b)-1, 0)) > 0)
    {
        b[n] = '\0';
        cmd[0] = a[0] = d[0] = '\0';
        sscanf(b, "%19s %99s %99s", cmd, a, d);

        if (!strcmp(cmd, "USER"))
        {
            user = !strcmp(a, "system");
            login = 0;
            reply(c, user ? "331 Enter password\n" : "530 Invalid user\n");
        }
        else if (!strcmp(cmd, "PASS"))
        {
            login = user && !strcmp(a, "system123");
            reply(c, login ? "230 Login successful\n" : "530 Login failed\n");
        }
        else if (!strcmp(cmd, "QUIT"))
        {
            reply(c, "221 Terminated\n");
            break;
        }
        else if (!login)
            reply(c, "530 Login first\n");
        else if (!strcmp(cmd, "PWD"))
            reply(c, "257 Server folder: ftp_root\n");
        else if (!strcmp(cmd, "DIR"))
        {
            DIR *dir = opendir("ftp_root");
            struct dirent *e;
            char list[400] = "Files: ";

            if (!dir)
                reply(c, "550 Directory error\n");
            else
            {
                while ((e = readdir(dir)) != NULL)
                    if (e->d_name[0] != '.' &&
                        strlen(list) + strlen(e->d_name) + 2 < sizeof(list))
                    {
                        strcat(list, e->d_name);
                        strcat(list, " ");
                    }

                closedir(dir);
                strcat(list, "\n");
                reply(c, list);
            }
        }
        else if (!a[0] || strchr(a, '/') || strchr(d, '/') ||
                 strstr(a, "..") || strstr(d, ".."))
            reply(c, "550 Invalid filename\n");
        else if (!strcmp(cmd, "CREATE"))
        {
            char p[150];
            snprintf(p, sizeof(p), "ftp_root/%s", a);
            FILE *f = fopen(p, "wx");

            if (f) { fclose(f); reply(c, "250 Created\n"); }
            else reply(c, "550 Create failed\n");
        }
        else if (!strcmp(cmd, "DELETE"))
        {
            char p[150];
            snprintf(p, sizeof(p), "ftp_root/%s", a);
            reply(c, remove(p) == 0 ? "250 Deleted\n" : "550 Delete failed\n");
        }
        else if (!strcmp(cmd, "CP") && d[0])
        {
            char p[150], q[150];
            int ch;
            FILE *f, *g;

            snprintf(p, sizeof(p), "ftp_root/%s", a);
            snprintf(q, sizeof(q), "ftp_root/%s", d);
            f = fopen(p, "rb");
            g = fopen(q, "wb");

            if (!f || !g)
            {
                if (f) fclose(f);
                if (g) fclose(g);
                reply(c, "550 Copy failed\n");
            }
            else
            {
                while ((ch = fgetc(f)) != EOF) fputc(ch, g);
                fclose(f);
                fclose(g);
                reply(c, "250 Copied\n");
            }
        }
        else if (!strcmp(cmd, "MV") && d[0])
        {
            char p[150], q[150];
            snprintf(p, sizeof(p), "ftp_root/%s", a);
            snprintf(q, sizeof(q), "ftp_root/%s", d);
            reply(c, rename(p, q) == 0 ? "250 Moved\n" : "550 Move failed\n");
        }
        else
            reply(c, "502 Invalid command\n");
    }

    close(c);
}

int main(void)
{
    int s, c, opt = 1;
    struct sockaddr_in addr = {0};

    mkdir("ftp_root", 0755);
    signal(SIGCHLD, SIG_IGN);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 1; }

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(s, 5) < 0)
    {
        perror("bind/listen");
        close(s);
        return 1;
    }

    printf("Server running on port %d\n", PORT);

    while (1)
    {
        c = accept(s, NULL, NULL);
        if (c < 0) continue;

        if (fork() == 0)
        {
            close(s);
            handle(c);
            exit(0);
        }

        close(c);
    }
}
