#include <SDL2/SDL.h>
#include <dirent.h>
#include <stdio.h>

#include "paramsfo.h"
#include "game_list.h"
#include "assert.h"

#define CONTENT_ID_LENGTH 0x30

char *find_license(char *path, char *content_id)
{
    DIR *directory = NULL;
    directory = opendir(path);
    ASSERT_NONZERO(directory, "Failed to open game directory");

    struct dirent *entry = NULL;
    while ((entry = readdir(directory)) != NULL)
    {
        char full_path[MAXPATHLEN + 2] = {0};
        snprintf(full_path, MAXPATHLEN + 2, "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_REG)
        {
            // Skip over . and ..
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                SDL_Log("%s / %s, cmp: %d", entry->d_name, content_id, memcmp(entry->d_name, content_id, CONTENT_ID_LENGTH));

                // strlen(content_id) with limit of CONTENT_ID_LENGTH
                int length = 0;
                for (int i = 0; i < CONTENT_ID_LENGTH; i++)
                {
                    if (content_id[i] == '\0')
                        break;

                    length++;
                }

                if (memcmp(entry->d_name, content_id, length) == 0)
                {
                    SDL_Log("Found license: %s, full path: %s", entry->d_name, full_path);

                    closedir(directory);

                    // Return the path to the license folder
                    return strdup(path);
                }
            }
        }
    }

    closedir(directory);

    return NULL;
}

char *find_license_from_all_users(char *content_id)
{
    char *path = "/dev_hdd0/home";

    DIR *directory = NULL;
    directory = opendir(path);
    ASSERT_NONZERO(directory, "Failed to open game directory");

    struct dirent *entry = NULL;
    while ((entry = readdir(directory)) != NULL)
    {
        char full_path[MAXPATHLEN + sizeof "/dev_hdd0/home" + 2] = {0};
        snprintf(full_path, MAXPATHLEN + sizeof "/dev_hdd0/home" + 2, "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            // Skip over . and .. and non-9 character directories
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                char license_dir_path[MAXPATHLEN] = {0};
                snprintf(license_dir_path, MAXPATHLEN, "%s/exdata", full_path);

                char *license = find_license(license_dir_path, content_id);
                if (license != NULL)
                {
                    closedir(directory);

                    return license;
                }
            }
        }
    }

    SDL_Log("Failed to find license for %s", content_id);

    closedir(directory);

    return NULL;
}