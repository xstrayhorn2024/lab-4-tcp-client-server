#define _POSIX_C_SOURCE 200809L
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include "list.h"

#define PORT 9001
#define BUFSIZE 1024
static volatile sig_atomic_t stopping = 0;
static void stop_server(int sig) { (void)sig; stopping = 1; }

/* Reject missing, nonnumeric, and out-of-range arguments. */
static int integer(const char *s, int *value) {
    char *end; long n;
    if (!s) return 0;
    errno = 0; n = strtol(s, &end, 10);
    if (errno || *end || end == s || n < INT_MIN || n > INT_MAX) return 0;
    *value = (int)n; return 1;
}
static int respond(int fd, const char *reply) {
    size_t sent = 0, length = strlen(reply) + 1;
    /* A terminating NUL marks the end of a reply, even across TCP reads. */
    while (sent < length && !stopping) {
        ssize_t n = send(fd, reply + sent, length - sent, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}
int main(void) {
    int listener = -1, client = -1, status = EXIT_FAILURE, reuse = 1;
    list_t *list = list_alloc();
    struct sigaction action = {0};
    action.sa_handler = stop_server;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    signal(SIGPIPE, SIG_IGN);
    if (!list) { perror("list_alloc"); goto cleanup; }
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { perror("socket"); goto cleanup; }
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind"); goto cleanup;
    }
    if (listen(listener, 1) < 0) { perror("listen"); goto cleanup; }
    if (!stopping) client = accept(listener, NULL, NULL);
    if (client < 0) {
        if (!stopping) perror("accept");
        else status = EXIT_SUCCESS;
        goto cleanup;
    }
    status = EXIT_SUCCESS;
    /* Process one request at a time; the client waits for its reply.
       Only normal control flow frees memory, never the signal handler. */
    while (!stopping) {
        char buf[BUFSIZE], reply[BUFSIZE] = {0};
        ssize_t n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            if (n < 0) { perror("recv"); status = EXIT_FAILURE; }
            break;
        }
        buf[n] = '\0';
        char *command = strtok(buf, " \t\r\n");
        char *a = strtok(NULL, " \t\r\n");
        char *b = strtok(NULL, " \t\r\n");
        char *extra = strtok(NULL, " \t\r\n");
        int value = 0, index = 0;
        strcpy(reply, "ERROR: invalid command or arguments");
        if (!command) { /* Return an error for an empty request. */ }
        else if (!strcmp(command, "exit") && !a) break;
        else if (!strcmp(command, "get_length") && !a)
            snprintf(reply, sizeof(reply), "Length = %d", list_length(list));
        else if (!strcmp(command, "print") && !a) {
            char *text = listToString(list);
            if (!text) strcpy(reply, "ERROR: allocation failed");
            else {
                /* listToString allocates exactly enough space, so large lists
                   can be sent without truncation or a fixed output limit. */
                int result = respond(client, text);
                free(text);
                if (result < 0) break;
                continue;
            }
        } else if ((!strcmp(command, "add_front") || !strcmp(command, "add_back")) && !b && integer(a, &value)) {
            if (!strcmp(command, "add_front")) list_add_to_front(list, value);
            else list_add_to_back(list, value);
            snprintf(reply, sizeof(reply), "ACK%d", value);
        } else if (!strcmp(command, "add_position") && !extra && integer(a, &index) && integer(b, &value)) {
            if (index < 1 || index > list_length(list) + 1) strcpy(reply, "ERROR: invalid index");
            else {
                list_add_at_index(list, value, index);
                snprintf(reply, sizeof(reply), "ACK%d", value);
            }
        } else if (!strcmp(command, "remove_front") && !a)
            snprintf(reply, sizeof(reply), "ACK%d", list_remove_from_front(list));
        else if (!strcmp(command, "remove_back") && !a)
            snprintf(reply, sizeof(reply), "ACK%d", list_remove_from_back(list));
        else if (!strcmp(command, "remove_position") && !b && integer(a, &index))
            snprintf(reply, sizeof(reply), "ACK%d", list_remove_at_index(list, index));
        else if (!strcmp(command, "get") && !b && integer(a, &index))
            snprintf(reply, sizeof(reply), "VALUE = %d", list_get_elem_at(list, index));
        else if (!strcmp(command, "menu") && !a) strcpy(reply, "ACK");
        if (respond(client, reply) < 0) break;
    }
/* Every exit path shares this cleanup, including signals and disconnects. */
cleanup:
    if (client >= 0) close(client);
    if (listener >= 0) close(listener);
    list_free(list);
    return status;
}
