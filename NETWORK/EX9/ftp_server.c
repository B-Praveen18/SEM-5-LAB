#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 2121
#define SIZE 2048

char root_dir[PATH_MAX];

int send_all(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t n = send(fd, buf, len, 0);
        if (n <= 0) return -1;
        buf += n;
        len -= (size_t)n;
    }
    return 0;
}

int send_reply(int fd, const char *msg) {
    return send_all(fd, msg, strlen(msg));
}

int read_line(int fd, char *buf, size_t size) {
    size_t i = 0;
    char ch;
    while (i + 1 < size) {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) return (int)n;
        if (ch == '\n') break;
        if (ch != '\r') buf[i++] = ch;
    }
    buf[i] = '\0';
    return 1;
}

/* Lab version: operations are restricted to a single name inside root_dir. */
int make_path(char *out, size_t size, const char *name) {
    if (!name || !*name || strchr(name, '/') || strchr(name, '\\') ||
        strstr(name, "..") || !strcmp(name, ".")) return -1;
    if (snprintf(out, size, "%s/%s", root_dir, name) >= (int)size) return -1;
    return 0;
}

void handle_command(int fd, char *line, int *user_ok, int *logged_in) {
    char cmd[32] = "", arg1[PATH_MAX] = "", arg2[PATH_MAX] = "";
    char path1[PATH_MAX], path2[PATH_MAX], reply[SIZE];
    sscanf(line, "%31s %4095s %4095s", cmd, arg1, arg2);
    for (int i = 0; cmd[i]; i++) cmd[i] = (char)toupper((unsigned char)cmd[i]);

    if (!strcmp(cmd, "USER")) {
        *user_ok = !strcmp(arg1, "system");
        *logged_in = 0;
        send_reply(fd, *user_ok ? "331 Password required\r\n" : "530 Invalid username\r\n");
    } else if (!strcmp(cmd, "PASS")) {
        *logged_in = *user_ok && !strcmp(arg1, "system123");
        send_reply(fd, *logged_in ? "230 Login successful\r\n" : "530 Login incorrect\r\n");
    } else if (!strcmp(cmd, "QUIT")) {
        send_reply(fd, "221 Goodbye\r\n");
    } else if (!*logged_in) {
        send_reply(fd, "530 Please login first\r\n");
    } else if (!strcmp(cmd, "PWD")) {
        snprintf(reply, sizeof(reply), "257 \"%.2000s\"\r\n", root_dir);
        send_reply(fd, reply);
    } else if (!strcmp(cmd, "STRU")) {
        send_reply(fd, !strcmp(arg1, "F") ? "200 File structure selected\r\n" : "504 Only STRU F is supported\r\n");
    } else if (!strcmp(cmd, "DIR") || !strcmp(cmd, "LIST")) {
        DIR *dir = opendir(root_dir);
        struct dirent *entry;
        size_t used = (size_t)snprintf(reply, sizeof(reply), "200 Directory listing:");
        if (!dir) { send_reply(fd, "550 Cannot open directory\r\n"); return; }
        while ((entry = readdir(dir)) != NULL) {
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
            int wrote = snprintf(reply + used, sizeof(reply) - used, " %s", entry->d_name);
            if (wrote < 0 || (size_t)wrote >= sizeof(reply) - used) break;
            used += (size_t)wrote;
        }
        closedir(dir);
        if (used + 3 < sizeof(reply)) strcat(reply, "\r\n");
        else { reply[sizeof(reply)-3] = '\r'; reply[sizeof(reply)-2] = '\n'; reply[sizeof(reply)-1] = '\0'; }
        send_reply(fd, reply);
    } else if (!strcmp(cmd, "CREATE")) {
        if (make_path(path1, sizeof(path1), arg1) < 0) { send_reply(fd, "550 Invalid filename\r\n"); return; }
        FILE *f = fopen(path1, "wx");
        if (f) { fclose(f); send_reply(fd, "250 File created successfully\r\n"); }
        else send_reply(fd, "550 Cannot create file (it may already exist)\r\n");
    } else if (!strcmp(cmd, "DEL") || !strcmp(cmd, "DELE")) {
        if (make_path(path1, sizeof(path1), arg1) < 0) { send_reply(fd, "550 Invalid filename\r\n"); return; }
        send_reply(fd, remove(path1) == 0 ? "250 File deleted successfully\r\n" : "550 Delete failed or file not found\r\n");
    } else if (!strcmp(cmd, "MKD")) {
        if (make_path(path1, sizeof(path1), arg1) < 0) { send_reply(fd, "550 Invalid directory name\r\n"); return; }
        send_reply(fd, mkdir(path1, 0755) == 0 ? "257 Directory created\r\n" : "550 Cannot create directory\r\n");
    } else if (!strcmp(cmd, "RMD")) {
        if (make_path(path1, sizeof(path1), arg1) < 0) { send_reply(fd, "550 Invalid directory name\r\n"); return; }
        send_reply(fd, rmdir(path1) == 0 ? "250 Directory removed\r\n" : "550 Cannot remove directory\r\n");
    } else if (!strcmp(cmd, "MOVE") || !strcmp(cmd, "RNTO")) {
        if (make_path(path1, sizeof(path1), arg1) < 0 || make_path(path2, sizeof(path2), arg2) < 0) { send_reply(fd, "550 Invalid filename\r\n"); return; }
        send_reply(fd, rename(path1, path2) == 0 ? "250 File moved/renamed successfully\r\n" : "550 Move/rename failed\r\n");
    } else if (!strcmp(cmd, "CP")) {
        if (make_path(path1, sizeof(path1), arg1) < 0 || make_path(path2, sizeof(path2), arg2) < 0) { send_reply(fd, "550 Invalid filename\r\n"); return; }
        FILE *src = fopen(path1, "rb");
        if (!src) { send_reply(fd, "550 Source file not found\r\n"); return; }
        FILE *dst = fopen(path2, "wb");
        if (!dst) { fclose(src); send_reply(fd, "550 Cannot create destination file\r\n"); return; }
        int ch, failed = 0;
        while ((ch = fgetc(src)) != EOF) if (fputc(ch, dst) == EOF) { failed = 1; break; }
        if (ferror(src)) failed = 1;
        fclose(src);
        if (fclose(dst) != 0) failed = 1;
        send_reply(fd, failed ? "550 Copy failed\r\n" : "250 File copied successfully\r\n");
    } else {
        send_reply(fd, "502 Command not implemented\r\n");
    }
}

int main(void) {
    int server_fd, client_fd, opt = 1, user_ok = 0, logged_in = 0;
    struct sockaddr_in addr;
    char line[SIZE];
    struct stat st;

    printf("Enter server folder path (default ./ftp_root): ");
    if (!fgets(root_dir, sizeof(root_dir), stdin)) return 1;
    root_dir[strcspn(root_dir, "\n")] = '\0';
    if (!root_dir[0]) strcpy(root_dir, "./ftp_root");
    if (stat(root_dir, &st) < 0) {
        if (mkdir(root_dir, 0755) < 0) { perror("mkdir"); return 1; }
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "The server path must be a directory.\n"); return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(server_fd, 5) < 0) {
        perror("bind/listen"); close(server_fd); return 1;
    }
    printf("FTP-like TCP server listening on port %d\n", PORT);
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) { perror("accept"); close(server_fd); return 1; }
    send_reply(client_fd, "220 Simple FTP server ready\r\n");

    while (read_line(client_fd, line, sizeof(line)) > 0) {
        printf("Client: %s\n", line);
        handle_command(client_fd, line, &user_ok, &logged_in);
        if (!strncmp(line, "QUIT", 4) || !strncmp(line, "quit", 4)) break;
    }
    close(client_fd);
    close(server_fd);
    printf("Server closed.\n");
    return 0;
}
