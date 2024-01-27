#include <SDL2/SDL.h>
#include <dirent.h>
#include <stdio.h>

#include "paramsfo.h"
#include "game_list.h"

#define PATH_MAX 256

int iterate_games(const char *path, game_list_entry **list, uint32_t *count)
{
    DIR *directory = NULL;
    if ((directory = opendir(path)) == NULL)
    {
        fprintf(stderr, "Can't open %s\n", path);
        return -1;
    }

    (*count) = 0;

    // Initialize the list to NULL
    (*list) = NULL;

    game_list_entry *last_entry = NULL;

    struct dirent *entry = NULL;
    while ((entry = readdir(directory)) != NULL)
    {
        char full_path[PATH_MAX] = {0};
        snprintf(full_path, PATH_MAX, "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            // Skip over . and .. and non-9 character directories
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strlen(entry->d_name) == 9)
            {
                // Skip over non-games
                if (memcmp(entry->d_name, "NP", 2) != 0 && entry->d_name[0] != 'B')
                    continue;

                SDL_Log("Found game: %s, full path: %s", entry->d_name, full_path);

                char param_sfo_path[PATH_MAX] = {0};
                // Get the path to the PARAM.SFO file
                snprintf(param_sfo_path, PATH_MAX, "%s/PARAM.SFO", full_path);

                // Try to get the title of the game
                char *title = get_title(param_sfo_path);
                // If it fails, skip over the game
                if (title == NULL)
                {
                    SDL_Log("Unable to get title for %s", param_sfo_path);
                    continue;
                }

                // Add the game to the list
                game_list_entry *next_entry = game_list_entry_create(strdup(title), strdup(entry->d_name), strdup(full_path));

                // If the list isn't empty, add the entry to the end of the list
                if (*list != NULL)
                {
                    last_entry->next = next_entry;
                    last_entry = next_entry;
                }
                // Otherwise, set the start of the list to the entry
                else
                {
                    (*list) = next_entry;
                    last_entry = next_entry;
                }

                (*count)++;
            }
        }
    }

    closedir(directory);
    return 0;
}