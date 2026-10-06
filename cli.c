#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#define PORT 9001
#define MAX_COMMAND_LINE_LEN 1024

char *getCommandLine(char *line) {
    while (fgets(line, MAX_COMMAND_LINE_LEN, stdin)) {
        size_t length = strlen(line);
        if (length && line[length - 1] == '\n') line[--length] = '\0';
        else if (!feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            fprintf(stderr, "Command too long.\n");
            continue;
        }
        if (strspn(line, " \t\r") != length) return line;
    }
    if (ferror(stdin)) perror("fgets");
    return NULL;
}
int main(void) {
    signal(SIGPIPE, SIG_IGN);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return EXIT_FAILURE; }
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connect"); close(fd); return EXIT_FAILURE;
    }
    char line[MAX_COMMAND_LINE_LEN], response[MAX_COMMAND_LINE_LEN + 1];
    int status = EXIT_SUCCESS;
    for (;;) {
        printf("Enter Command (or menu): "); fflush(stdout);
        if (!getCommandLine(line)) break;
        char command[MAX_COMMAND_LINE_LEN];
        sscanf(line, "%1023s", command);
        if (!strcmp(command, "menu")) {
            puts("COMMANDS:\n---------\n1. print\n2. get_length\n3. add_back <value>\n4. add_front <value>\n5. add_position <index> <value>\n6. remove_back\n7. remove_front\n8. remove_position <index>\n9. get <index>\n10. exit");
            continue;
        }
        size_t length = strlen(line), sent = 0;
        while (sent < length) {
            ssize_t n = send(fd, line + sent, length - sent, 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) { perror("send"); status = EXIT_FAILURE; goto done; }
            sent += (size_t)n;
        }
        if (!strcmp(line, "exit")) break;
        /* Stream output until the NUL marker, with bounded memory use. */
        printf("\nSERVER RESPONSE: ");
        for (;;) {
            ssize_t n = recv(fd, response, sizeof(response), 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) {
                if (n < 0) perror("recv");
                else fprintf(stderr, "Server disconnected.\n");
                status = EXIT_FAILURE; goto done;
            }
            char *end = memchr(response, '\0', (size_t)n);
            size_t count = end ? (size_t)(end - response) : (size_t)n;
            fwrite(response, 1, count, stdout);
            if (end) break;
        }
        putchar('\n');
    }
done:
    close(fd);
    return status;
}
