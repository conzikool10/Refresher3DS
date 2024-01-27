#include "server_list.h"

server_list_entry *server_list_entry_create(char *name, char *url, bool patch_digest)
{
    server_list_entry *entry = (server_list_entry *)malloc(sizeof(server_list_entry));
    entry->name = strdup(name);
    entry->url = strdup(url);
    entry->patch_digest = patch_digest;
    entry->next = NULL;
    return entry;
}

void server_list_entry_destroy(server_list_entry *entry)
{
    free(entry->name);
    free(entry->url);
    free(entry);
}