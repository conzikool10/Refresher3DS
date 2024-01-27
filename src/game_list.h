#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef GAME_LIST_H
#define GAME_LIST_H

typedef struct game_list_entry
{
    struct game_list_entry *next;
    char *title;
    char *title_id;
    char *path;
} game_list_entry;

game_list_entry *game_list_entry_create(char *title, char *title_id, char *path);

void game_list_entry_destroy(game_list_entry *entry);

#endif