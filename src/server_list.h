#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#ifndef SERVER_LIST_H
#define SERVER_LIST_H

typedef struct server_list_entry
{
    struct server_list_entry *next;
    char *name;
    char *url;
    bool patch_digest;
} server_list_entry;

server_list_entry *server_list_entry_create(char *name, char *url, bool patch_digest);

void server_list_entry_destroy(server_list_entry *entry);

int count_server_list_entries(server_list_entry *head);

#endif