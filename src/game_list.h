#include <stdint.h>

typedef struct game_list_entry
{
    struct game_list_entry *next;
    char *title;
    char *title_id;
    char *path;
} game_list_entry;

inline game_list_entry *game_list_entry_create(char *title, char *title_id, char *path)
{
    game_list_entry *entry = (game_list_entry *)malloc(sizeof(game_list_entry));
    entry->title = title;
    entry->title_id = title_id;
    entry->path = path;
    entry->next = NULL;
    return entry;
}

inline void game_list_entry_destroy(game_list_entry *entry)
{
    free(entry->title);
    free(entry->title_id);
    free(entry->path);
    free(entry);
}