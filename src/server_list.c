#include "server_list.h"

#include "assert.h"

server_list_entry *server_list_entry_create(char *name, char *url, bool patch_digest)
{
    server_list_entry *entry = (server_list_entry *)malloc(sizeof(server_list_entry));
    ASSERT_NONZERO(entry, "Failed to allocate memory for server_list_entry");

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

int count_server_list_entries(server_list_entry *head)
{
    int count = 0;
    server_list_entry *current = head;
    while (current != NULL)
    {
        count++;

        current = current->next;
    }
    return count;
}