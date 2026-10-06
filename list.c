#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

list_t *list_alloc(void) { return calloc(1, sizeof(list_t)); }
void list_free(list_t *list) {
    if (!list) return;
    node_t *node = list->head;
    while (node) { node_t *next = node->next; free(node); node = next; }
    free(list);
}
int list_length(list_t *list) {
    int length = 0;
    for (node_t *n = list ? list->head : NULL; n; n = n->next) ++length;
    return length;
}
void list_add_at_index(list_t *list, int value, int index) {
    if (!list || index < 1) return;
    node_t **link = &list->head;
    for (int i = 1; i < index; ++i) {
        if (!*link) return;
        link = &(*link)->next;
    }
    node_t *node = malloc(sizeof(*node));
    if (!node) { perror("malloc"); exit(EXIT_FAILURE); }
    node->value = value; node->next = *link; *link = node;
}
void list_add_to_front(list_t *list, int value) { list_add_at_index(list, value, 1); }
void list_add_to_back(list_t *list, int value) { list_add_at_index(list, value, list_length(list) + 1); }
int list_remove_at_index(list_t *list, int index) {
    if (!list || index < 1) return -1;
    node_t **link = &list->head;
    for (int i = 1; i < index && *link; ++i) link = &(*link)->next;
    if (!*link) return -1;
    node_t *node = *link;
    int value = node->value;
    *link = node->next; free(node); return value;
}
int list_remove_from_front(list_t *list) { return list_remove_at_index(list, 1); }
int list_remove_from_back(list_t *list) { return list_remove_at_index(list, list_length(list)); }
int list_get_elem_at(list_t *list, int index) {
    if (!list || index < 1) return -1;
    node_t *node = list->head;
    for (int i = 1; i < index && node; ++i) node = node->next;
    return node ? node->value : -1;
}
char *listToString(list_t *list) {
    size_t size = 5;
    for (node_t *n = list ? list->head : NULL; n; n = n->next)
        size += (size_t)snprintf(NULL, 0, "%d->", n->value);
    char *text = malloc(size);
    if (!text) return NULL;
    size_t used = 0;
    for (node_t *n = list ? list->head : NULL; n; n = n->next)
        used += (size_t)snprintf(text + used, size - used, "%d->", n->value);
    strcpy(text + used, "NULL");
    return text;
}
