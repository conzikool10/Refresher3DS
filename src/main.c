#include <SDL2/SDL.h>
#include <stdbool.h>
#include <lv2/spu.h>
#include <stdio.h>
#include <dirent.h>
#include <stdint.h>
#include <io/pad.h>

#include "endian.h"
#include "sdl2_picofont.h"
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
        char full_name[PATH_MAX] = {0};
        snprintf(full_name, PATH_MAX, "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            // Skip over . and .. and non-9 character directories
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strlen(entry->d_name) == 9)
            {
                // Skip over non-games
                if (memcmp(entry->d_name, "NP", 2) != 0 && entry->d_name[0] != 'B')
                    continue;

                SDL_Log("Found game: %s, full path: %s", entry->d_name, full_name);

                char param_sfo_path[PATH_MAX] = {0};
                // Get the path to the PARAM.SFO file
                snprintf(param_sfo_path, PATH_MAX, "%s/PARAM.SFO", full_name);

                // Try to get the title of the game
                char *title = get_title(param_sfo_path);
                // If it fails, skip over the game
                if (title == NULL)
                {
                    SDL_Log("Unable to get title for %s", param_sfo_path);
                    continue;
                }

                // Add the game to the list
                game_list_entry *next_entry = game_list_entry_create(strdup(title), strdup(entry->d_name), strdup(full_name));

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

typedef struct state_t
{
    int selection;
    bool lastUp;
    bool lastDown;
    uint32_t gameCount;
    game_list_entry *games;
} state_t;

int handleControllerInput(state_t *state, bool *isPadConnected)
{
    padInfo2 padInfo;

    // Get the pad info
    if (ioPadGetInfo2(&padInfo) != 0)
    {
        SDL_Log("Unable to get pad info");
        return -1;
    }

    // If it claims there is a pad connected
    if (padInfo.port_status[0])
    {
        padData data;

        // Try to get the pad data
        if (ioPadGetData(0, &data) == 0)
        {
            // If the user presses down, increment the selection
            if (data.BTN_DOWN && !state->lastDown)
            {
                state->selection++;
                if (state->selection >= state->gameCount)
                    state->selection = 0;
            }

            // If the user presses up, decrement the selection
            if (data.BTN_UP && !state->lastUp)
            {
                state->selection--;
                if (state->selection < 0)
                    state->selection = state->gameCount - 1;
            }

            // Update our lastDown and lastUp variables
            state->lastDown = data.BTN_DOWN;
            state->lastUp = data.BTN_UP;

            // Clear the pad buffer
            ioPadClearBuf(0);

            // Set the pad as definitely connected
            (*isPadConnected) = true;
        }
    }

    return 0;
}

int main()
{
    // Initialize SDL with Video support
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Create a new window
    SDL_Window *window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == NULL)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return 1;
    }

    // Initialize the gamepad
    if (ioPadInit(1) != 0)
    {
        SDL_Log("Unable to initialize pad");
        return 1;
    }

    // Create a new renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    // Initialize our font renderer
    font_ctx *font = FontStartup(renderer);

    // Initialize the state of the app
    state_t state = {0};

    // Iterate over the installed games, and get their info
    if (iterate_games("/dev_hdd0/game", &state.games, &state.gameCount) != 0)
    {
        SDL_Log("Unable to iterate games");
        return 1;
    }

    // Loop until the user closes the app
    SDL_Event ev;
    bool quit = false;
    while (!quit)
    {
        // Poll all the new events
        while (SDL_PollEvent(&ev))
        {
            // If the user closes the app,
            if (ev.type == SDL_QUIT)
            {
                // Mark the app to quit on the next loop
                quit = true;
            }
        }

        // Clear the screen to black
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Set the initial state of the text rendering
        SDL_Rect dstscale = {.x = 10, .y = 10, .h = 4, .w = 3};

        // Set the draw color to white, this doesn't do anything, but it's here for goodness sake
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // Draw the title
        FontPrintToRenderer(font, "RefresherPS3", &dstscale);

        // Move the text down by the height of the text
        dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;

        // Set the scale to 1
        dstscale.w = 1;
        dstscale.h = 1;

        // Draw the game list
        game_list_entry *entry = state.games;
        int i = 0;
        while (entry != NULL)
        {
            char display_name[256] = {0};

            // Make a pretty display name
            snprintf(display_name, 256, "%s%s (%s) [%s]", i == state.selection ? ">>> " : "", entry->title, entry->title_id, entry->path);

            // Draw the display name
            FontPrintToRenderer(font, display_name, &dstscale);
            // Move the text down by the height of the text
            dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;

            // Move to the next item in the list
            entry = entry->next;

            i++;
        }

        // Handle controller input
        bool isPadConnected = false;
        if (handleControllerInput(&state, &isPadConnected) != 0)
        {
            SDL_Log("Unable to handle controller input");
            return 1;
        }

        // If the pad is not connected, display a message
        if (!isPadConnected)
        {
            SDL_Rect dstscale = {.x = 50, .y = 250, .h = 5, .w = 5};
            FontPrintToRenderer(font, "No controller connected", &dstscale);
        }

        SDL_RenderPresent(renderer);
    }

    if (ioPadEnd() != 0)
    {
        SDL_Log("Unable to end pad");
        return 1;
    }

    FontExit(font);
    SDL_Quit();

    return 0;
}
