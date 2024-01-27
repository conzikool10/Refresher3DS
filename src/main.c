#include <SDL2/SDL.h>
#include <stdbool.h>
#include <lv2/spu.h>
#include <stdio.h>
#include <stdint.h>
#include <io/pad.h>

#include "endian.h"
#include "sdl2_picofont.h"
#include "game_list.h"
#include "idps.h"
#include "games.h"

typedef enum STATE_SCENE
{
    STATE_SCENE_SELECT_GAME,
    STATE_SCENE_SELECT_SERVER,
    STATE_SCENE_PATCHING,
    STATE_SCENE_DONE_PATCHING,
    STATE_SCENE_ERROR
} STATE_SCENE;

char *get_scene_name(STATE_SCENE scene)
{
    switch (scene)
    {
    case STATE_SCENE_SELECT_GAME:
        return "Select Game";
    case STATE_SCENE_SELECT_SERVER:
        return "Select Server";
    case STATE_SCENE_PATCHING:
        return "Patching";
    case STATE_SCENE_DONE_PATCHING:
        return "Done Patching";
    case STATE_SCENE_ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

typedef struct state_t
{
    int selection;
    bool lastUp;
    bool lastDown;
    bool lastCross;
    bool lastCircle;
    uint32_t gameCount;
    game_list_entry *games;
    STATE_SCENE scene;
    bool crossPressed;
    bool circlePressed;
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

            // If the user presses cross, set the crossPressed flag
            if (data.BTN_CROSS && !state->lastCross)
                state->crossPressed = true;

            // If the user presses circle, set the circlePressed flag
            if (data.BTN_CIRCLE && !state->lastCircle)
                state->circlePressed = true;

            // Update our lastDown and lastUp variables
            state->lastDown = data.BTN_DOWN;
            state->lastUp = data.BTN_UP;
            state->lastCross = data.BTN_CROSS;
            state->lastCircle = data.BTN_CIRCLE;

            // Clear the pad buffer
            ioPadClearBuf(0);

            // Set the pad as definitely connected
            (*isPadConnected) = true;
        }
    }

    return 0;
}

void switch_scene(state_t *state, STATE_SCENE scene)
{
    state->scene = scene;
    state->selection = 0;
}

int main()
{
    char idps[16];
    char psid[16];

    if (get_idps_psid(idps, psid) != 0)
    {
        SDL_Log("Unable to get IDPS");
        return 1;
    }

    SDL_Log("IDPS: %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x", idps[0], idps[1], idps[2], idps[3], idps[4], idps[5], idps[6], idps[7], idps[8], idps[9], idps[10], idps[11], idps[12], idps[13], idps[14], idps[15]);
    SDL_Log("PSID: %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x", psid[0], psid[1], psid[2], psid[3], psid[4], psid[5], psid[6], psid[7], psid[8], psid[9], psid[10], psid[11], psid[12], psid[13], psid[14], psid[15]);

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

    // Set the initial state to game selection
    state.scene = STATE_SCENE_SELECT_GAME;

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

        char title[256] = {0};
        snprintf(title, 256, "RefresherPS3 - %s", get_scene_name(state.scene));

        // Draw the title
        FontPrintToRenderer(font, title, &dstscale);

        // Move the text down by the height of the text
        dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;

        // Set the scale to 1
        dstscale.w = 1;
        dstscale.h = 1;

        // Handle controller input
        bool isPadConnected = false;
        if (handleControllerInput(&state, &isPadConnected) != 0)
        {
            SDL_Log("Unable to handle controller input");
            return 1;
        }

        switch (state.scene)
        {
        case STATE_SCENE_SELECT_GAME:
        {
            // Draw the game list
            game_list_entry *entry = state.games;
            int i = 0;
            while (entry != NULL)
            {
                // If the user presses cross on the selected game, switch to the server selection scene
                if (state.selection == i && state.crossPressed)
                {
                    switch_scene(&state, STATE_SCENE_SELECT_SERVER);
                }

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

            break;
        }
        case STATE_SCENE_SELECT_SERVER:
        {
            // If the user presses circle, switch back to the game selection scene
            if (state.circlePressed)
            {
                switch_scene(&state, STATE_SCENE_SELECT_GAME);
            }

            // Draw the display name
            FontPrintToRenderer(font, "TODO", &dstscale);
            // Move the text down by the height of the text
            dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;
            break;
        }
        case STATE_SCENE_PATCHING:
        {
            // Draw the display name
            FontPrintToRenderer(font, "TODO", &dstscale);
            // Move the text down by the height of the text
            dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;
            break;
        }
        case STATE_SCENE_DONE_PATCHING:
        {
            // If the user pressed circle, switch back to the game selection scene
            if (state.circlePressed)
            {
                switch_scene(&state, STATE_SCENE_SELECT_GAME);
            }

            // Draw the display name
            FontPrintToRenderer(font, "TODO", &dstscale);
            // Move the text down by the height of the text
            dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;
            break;
        }
        case STATE_SCENE_ERROR:
        {
            // If the user pressed circle, switch back to the game selection scene
            if (state.circlePressed)
            {
                switch_scene(&state, STATE_SCENE_SELECT_GAME);
            }

            // Draw the display name
            FontPrintToRenderer(font, "TODO", &dstscale);
            // Move the text down by the height of the text
            dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;
            break;
        }
        default:
        {
            SDL_Log("Unknown scene: %d", state.scene);
            return 1;
        }
        }

        // If the pad is not connected, display a message
        if (!isPadConnected)
        {
            SDL_Rect dstscale = {.x = 50, .y = 250, .h = 5, .w = 5};
            FontPrintToRenderer(font, "No controller connected", &dstscale);
        }

        // Reset the crossPressed and circlePressed flags, since they are single press
        state.crossPressed = false;
        state.circlePressed = false;

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
