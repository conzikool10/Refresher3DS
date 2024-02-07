#include <SDL2/SDL.h>
#include <stdbool.h>
#include <lv2/spu.h>
#include <stdio.h>
#include <stdint.h>
#include <io/pad.h>
#include <ppu-lv2.h>
#include <unistd.h>
#include <sysutil/sysutil.h>

#include "endian.h"
#include "sdl2_picofont.h"
#include "idps.h"
#include "games.h"
#include "assert.h"
#include "license.h"
#include "digest.h"
#include "autodiscover.h"
#include "types.h"
#include "patching.h"
#include "save_manager.h"
#include "unicode.h"
#include "osk.h"

int handleControllerInput(state_t *state, bool *is_pad_connected)
{
    padInfo2 pad_info;

    // Get the pad info
    ASSERT_ZERO(ioPadGetInfo2(&pad_info), "Unable to get pad info");

    // If it claims there is a pad connected
    if (pad_info.port_status[0])
    {
        padData data = {0};

        // Try to get the pad data
        if (ioPadGetData(0, &data) == 0)
        {
            // If the user presses down, increment the selection
            if (data.BTN_DOWN && !state->last_down)
            {
                state->selection++;
                if (state->selection >= state->wrap_count)
                    state->selection = 0;
            }

            // If the user presses up, decrement the selection
            if (data.BTN_UP && !state->last_up)
            {
                state->selection--;
                if (state->selection < 0)
                    state->selection = state->wrap_count - 1;
            }

            // If the user presses cross, set the crossPressed flag
            if (data.BTN_CROSS && !state->last_cross)
                state->cross_pressed = true;

            // If the user presses circle, set the circlePressed flag
            if (data.BTN_CIRCLE && !state->last_circle)
                state->circle_pressed = true;

            // Update our lastDown and lastUp variables
            state->last_down = data.BTN_DOWN;
            state->last_up = data.BTN_UP;
            state->last_cross = data.BTN_CROSS;
            state->last_circle = data.BTN_CIRCLE;

            // Clear the pad buffer
            ioPadClearBuf(0);

            // Set the pad as definitely connected
            (*is_pad_connected) = true;
        }
    }

    return 0;
}

void switch_scene(state_t *state, STATE_SCENE scene)
{
    if (state->scene == scene)
        return;

    // If we are coming from the patching scene, wait for the thread to exit
    if (state->scene == STATE_SCENE_PATCHING)
    {
        while (state->patching_info.is_running)
        {
            ASSERT_ZERO(sysThreadYield(), "Unable to yield thread");
        }
    }
    else if (state->scene == STATE_SCENE_ERROR)
    {
        state->last_error = NULL;
    }

    switch (scene)
    {
    case STATE_SCENE_SELECT_GAME:
        state->wrap_count = state->game_count;
        break;
    case STATE_SCENE_SELECT_SERVER:
        // Plus one for the "manage servers" option
        state->wrap_count = state->server_count + 1;
        break;
    case STATE_SCENE_MANAGE_SERVERS:

        break;
    case STATE_SCENE_PATCHING:
        state->patching_info.is_running = true;
        state->patching_info.state = PATCHING_STATE_NOT_STARTED;
        state->patching_info.last_error = NULL;

        // Create the thread
        sysThreadCreate(state->patching_info.thread, patch_game, state, 1000, 0x10000, THREAD_JOINABLE, "PATCHING");

        break;
    case STATE_SCENE_DONE_PATCHING:
        break;
    case STATE_SCENE_SELECT_NONE:
        break;
    case STATE_SCENE_ERROR:
        break;
    }

    state->scene = scene;
    state->selection = 0;
}

bool running = true;

static void program_exit_callback()
{
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
}

// a bit hacky but idc
#define PATCHING_STATE_CASE(check_state)                                                 \
    if (state.patching_info.state == check_state)                                        \
    {                                                                                    \
        snprintf(display_name, 256, ">>> %s <<<", get_patching_state_name(check_state)); \
    }                                                                                    \
    else                                                                                 \
    {                                                                                    \
        snprintf(display_name, 256, "%s", get_patching_state_name(check_state));         \
    }                                                                                    \
    font_print_to_renderer(font, display_name, &font_state);                             \
    font_state.y += FONT_CHAR_HEIGHT * font_state.h;

int main()
{
    autodiscover_t autodiscover;
    ASSERT_ZERO(autodiscover_init(&autodiscover), "Unable to initialize autodiscover");

    atexit(program_exit_callback);

    // Initialize SDL with Video support
    ASSERTSDL_ZERO(SDL_Init(SDL_INIT_VIDEO), "Unable to initialize SDL");

    // Create a new window
    SDL_Window *window = SDL_CreateWindow(
        "Hello World!",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1280,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    ASSERTSDL_NONZERO(window, "Unable to create window");

    // Initialize the pad
    ASSERT_ZERO(ioPadInit(1), "Unable to initialize pad");

    // Create a new renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_SOFTWARE);

    // Initialize our font renderer
    font_ctx *font = font_startup(renderer);

    // Initialize the state of the app
    state_t state = {0};

    char psid[16];

    // Get the IDPS and PSID
    ASSERT_ZERO(get_idps_psid(state.idps, psid), "Unable to get IDPS");

    SDL_Log("IDPS: %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x", state.idps[0], state.idps[1], state.idps[2], state.idps[3], state.idps[4], state.idps[5], state.idps[6], state.idps[7], state.idps[8], state.idps[9], state.idps[10], state.idps[11], state.idps[12], state.idps[13], state.idps[14], state.idps[15]);
    SDL_Log("PSID: %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x", psid[0], psid[1], psid[2], psid[3], psid[4], psid[5], psid[6], psid[7], psid[8], psid[9], psid[10], psid[11], psid[12], psid[13], psid[14], psid[15]);

    // Iterate over the installed games, and get their info
    ASSERT_ZERO(iterate_games("/dev_hdd0/game", &state.games, &state.game_count), "Unable to iterate games");

    // Create the default server entry of production refresh
    state.servers = server_list_entry_create("Refresh", "http://refresh.jvyden.xyz:2095/lbp", true);
    // Load the user's saved entries
    state.servers->next = load_saved_servers();
    // Count the number of servers
    state.server_count = count_server_list_entries(state.servers);

    // Set the initial state to game selection
    switch_scene(&state, STATE_SCENE_SELECT_GAME);

    sys_lwmutex_attr_t mutex_attr = {
        .name = "PATCHING",
        .attr_protocol = SYS_LWMUTEX_PROTOCOL_FIFO,
        .attr_recursive = SYS_LWMUTEX_ATTR_NOT_RECURSIVE,
    };

    // Allocate memory for the mutex
    state.patching_info.mutex = (sys_lwmutex_t *)malloc(sizeof(sys_lwmutex_t));
    ASSERT_NONZERO(state.patching_info.mutex, "Unable to allocate memory for mutex");

    // Create the mutex
    ASSERT_ZERO(sysLwMutexCreate(state.patching_info.mutex, &mutex_attr), "Unable to create mutex");

    // Allocate memory for the thread
    state.patching_info.thread = (sys_ppu_thread_t *)malloc(sizeof(sys_ppu_thread_t));
    ASSERT_NONZERO(state.patching_info.thread, "Unable to allocate memory for thread");

    // Initialize the OSK
    osk_setup(&state);

    // Loop until the user closes the app
    SDL_Event ev;
    while (running)
    {
        sysUtilCheckCallback();

        // Poll all the new events
        while (SDL_PollEvent(&ev))
        {
            // If the user closes the app,
            if (ev.type == SDL_QUIT)
            {
                // Mark the app to quit on the next loop
                running = false;
            }
        }

        // Clear the screen to black
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Set the initial state of the text rendering
        SDL_Rect font_state = {.x = 10, .y = 10, .h = 4, .w = 3};

        // Set the draw color to white, this doesn't do anything, but it's here for goodness sake
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        char title[256] = {0};
        snprintf(title, 256, "RefresherPS3 - %s", get_scene_name(state.scene));

        // Draw the title
        font_print_to_renderer(font, title, &font_state);

        // Move the text down by the height of the text
        font_state.y += FONT_CHAR_HEIGHT * font_state.h;

        // Set the scale to 1
        font_state.w = 1;
        font_state.h = 1;

        // Handle controller input
        bool is_pad_connected = false;
        if (handleControllerInput(&state, &is_pad_connected) != 0)
        {
            SDL_Log("Unable to handle controller input");
            return 1;
        }

        bool can_interact = is_pad_connected;

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
                if (state.selection == i && state.cross_pressed)
                {
                    state.selected_game = entry;
                    switch_scene(&state, STATE_SCENE_SELECT_SERVER);
                }

                char display_name[256] = {0};

                // Make a pretty display name
                snprintf(display_name, 256, "%s%s (%s) [%s]", i == state.selection ? ">>> " : "", entry->title, entry->title_id, entry->path);

                // Draw the display name
                font_print_to_renderer(font, display_name, &font_state);
                // Move the text down by the height of the text
                font_state.y += FONT_CHAR_HEIGHT * font_state.h;

                // Move to the next item in the list
                entry = entry->next;

                i++;
            }

            break;
        }
        case STATE_SCENE_SELECT_SERVER:
        {
            // This should never happen
            if (state.selected_game == NULL)
            {
                SDL_Log("Selected game is NULL");
                return 1;
            }

            // If the user presses circle, switch back to the game selection scene
            if (state.circle_pressed)
            {
                switch_scene(&state, STATE_SCENE_SELECT_GAME);
            }

            if (state.cross_pressed && state.selection == state.server_count)
            {
                switch_scene(&state, STATE_SCENE_MANAGE_SERVERS);
                break;
            }

            // Draw the server list
            server_list_entry *entry = state.servers;
            int i = 0;
            while (entry != NULL)
            {
                // If the user presses cross on the selected game, switch to the patching progress scene
                if (state.selection == i && state.cross_pressed)
                {
                    state.selected_server = entry;
                    switch_scene(&state, STATE_SCENE_PATCHING);
                }

                char display_name[256] = {0};

                // Make a pretty display name
                snprintf(display_name, 256, "%s%s (%s)", i == state.selection ? ">>> " : "", entry->name, entry->url);

                // Draw the display name
                font_print_to_renderer(font, display_name, &font_state);
                // Move the text down by the height of the text
                font_state.y += FONT_CHAR_HEIGHT * font_state.h;

                // Move to the next item in the list
                entry = entry->next;

                i++;
            }

            font_state.y += FONT_CHAR_HEIGHT * font_state.h;
            font_print_to_renderer(
                font,
                state.selection == state.server_count ? ">>> Manage Servers" : "Manage Servers",
                &font_state);
            font_state.y += FONT_CHAR_HEIGHT * font_state.h;

            break;
        }
        case STATE_SCENE_MANAGE_SERVERS:
        {
            state.wrap_count = state.server_count + 1;

            if (state.input_state == INPUT_STATE_AUTODISCOVER_URL)
            {
                if (!is_osk_running())
                {
                    char *autodiscover_url = get_utf8_output();

                    if (autodiscover_url == NULL)
                    {
                        state.input_state = INPUT_STATE_NONE;

                        break;
                    }

                    char *server_brand;
                    char *patch_url;
                    bool patch_digest;
                    if (autodiscover_execute(&autodiscover, autodiscover_url, &server_brand, &patch_url, &patch_digest) != 0)
                    {
                        SDL_Log("Unable to execute autodiscover");
                        state.last_error = "Unable to execute autodiscover";
                        switch_scene(&state, STATE_SCENE_ERROR);
                        break;
                    }

                    server_list_entry *new_entry = server_list_entry_create(server_brand, patch_url, patch_digest);

                    server_list_entry *entry = state.servers;
                    while (true)
                    {
                        if (entry->next == NULL)
                        {
                            entry->next = new_entry;
                            break;
                        }

                        entry = entry->next;
                    }

                    state.server_count = count_server_list_entries(state.servers);

                    free(server_brand);
                    free(patch_url);

                    state.input_state = INPUT_STATE_NONE;

                    // Save the new list
                    if (save_servers(state.servers->next) != 0)
                    {
                        // If it fails, switch to the error scene
                        SDL_Log("Unable to save servers");
                        state.last_error = "Unable to save servers";
                        switch_scene(&state, STATE_SCENE_ERROR);

                        break;
                    }

                    break;
                }

                font_print_to_renderer(
                    font,
                    "Waiting for Autodiscover URL...",
                    &font_state);
                font_state.y += FONT_CHAR_HEIGHT * font_state.h;
                break;
            }

            // If we are inputting the name
            if (state.input_state == INPUT_STATE_NAME)
            {
                // If the OSK has closed
                if (!is_osk_running())
                {
                    // Get the text from the OSK
                    char *name = get_utf8_output();

                    // If its null, then the user cancelled
                    if (name == NULL)
                    {
                        state.input_state = INPUT_STATE_NONE;

                        break;
                    }

                    // Store the name for later
                    state.input_name = strdup(name);

                    // Set the input state to mark we are now listening for the patch URL
                    state.input_state = INPUT_STATE_PATCH_URL;

                    osk_open(u"Enter patch URL", u"");

                    break;
                }

                font_print_to_renderer(
                    font,
                    "Waiting for name...",
                    &font_state);
                font_state.y += FONT_CHAR_HEIGHT * font_state.h;
                break;
            }

            // If we are inputting the URL
            if (state.input_state == INPUT_STATE_PATCH_URL)
            {
                // If the OSK has closed
                if (!is_osk_running())
                {
                    // Get the text from the OSK
                    char *patch_url = get_utf8_output();

                    // If its null, then the user cancelled
                    if (patch_url == NULL)
                    {
                        free(state.input_name);

                        state.input_state = INPUT_STATE_NONE;

                        break;
                    }

                    // Create a new entry
                    server_list_entry *new_entry = server_list_entry_create(state.input_name, patch_url, false);

                    // Add it to the end of the list
                    server_list_entry *entry = state.servers;
                    while (true)
                    {
                        if (entry->next == NULL)
                        {
                            entry->next = new_entry;
                            break;
                        }

                        entry = entry->next;
                    }

                    // Update the server count
                    state.server_count = count_server_list_entries(state.servers);

                    // Reset the input state
                    state.input_state = INPUT_STATE_NONE;

                    // Save the new list
                    if (save_servers(state.servers->next) != 0)
                    {
                        // If it fails, switch to the error scene
                        SDL_Log("Unable to save servers");
                        state.last_error = "Unable to save servers";
                        switch_scene(&state, STATE_SCENE_ERROR);

                        break;
                    }

                    // Free the input name that we stored in the earlier step
                    free(state.input_name);

                    break;
                }

                font_print_to_renderer(
                    font,
                    "Waiting for patch URL...",
                    &font_state);
                font_state.y += FONT_CHAR_HEIGHT * font_state.h;
                break;
            }

            // If the user presses circle, switch back to the server selection scene
            if (state.circle_pressed)
            {
                switch_scene(&state, STATE_SCENE_SELECT_SERVER);
                break;
            }

            font_print_to_renderer(font, "Select a server to delete it.", &font_state);
            font_state.y += FONT_CHAR_HEIGHT * font_state.h * 2;

            state.wrap_count = state.server_count + 2;

            // Force state.selection to not be on the default server.
            if (state.selection == 0)
                state.selection = 1;

            // Stores a flag of whether or not a server was deleted this frame.
            bool deleted = false;

            // Draw the server list
            server_list_entry *last_entry = NULL;
            server_list_entry *entry = state.servers;
            int i = 0;
            while (entry != NULL)
            {
                // If the user presses cross on the selected game, delete that game
                if (state.selection > 0 && state.selection == i && state.cross_pressed)
                {
                    // Remove the entry from the list
                    if (last_entry == NULL)
                    {
                        state.servers = entry->next;
                    }
                    else
                    {
                        last_entry->next = entry->next;
                    }

                    state.server_count = count_server_list_entries(state.servers);

                    // Free the memory
                    server_list_entry_destroy(entry);

                    deleted = true;

                    // Save the new list
                    if (save_servers(state.servers->next) != 0)
                    {
                        // If it fails, switch to the error scene
                        SDL_Log("Unable to save servers");
                        state.last_error = "Unable to save servers";
                        switch_scene(&state, STATE_SCENE_ERROR);

                        break;
                    }

                    break;
                }

                char display_name[256] = {0};

                // Make a pretty display name
                snprintf(display_name, 256, "%s%s (%s)", i == state.selection ? ">>> " : "", entry->name, entry->url);

                // Draw the display name
                font_print_to_renderer(font, display_name, &font_state);
                // Move the text down by the height of the text
                font_state.y += FONT_CHAR_HEIGHT * font_state.h;

                last_entry = entry;

                // Move to the next item in the list
                entry = entry->next;

                i++;
            }

            font_state.y += FONT_CHAR_HEIGHT * font_state.h;
            font_print_to_renderer(
                font,
                state.selection == state.server_count ? ">>> Create new server using Autodiscover" : "Create new server using Autodiscover",
                &font_state);
            font_state.y += FONT_CHAR_HEIGHT * font_state.h;

            if (!deleted && state.selection == state.server_count && state.cross_pressed)
            {
                osk_open(u"Enter URL for Autodiscover", u"http://refresh.jvyden.xyz:2095/");
                state.input_state = INPUT_STATE_AUTODISCOVER_URL;
            }

            font_print_to_renderer(
                font,
                state.selection == state.server_count + 1 ? ">>> Create new server manually" : "Create new server manually",
                &font_state);
            font_state.y += FONT_CHAR_HEIGHT * font_state.h;

            if (!deleted && state.selection == state.server_count + 1 && state.cross_pressed)
            {
                osk_open(u"Enter name for server", u"");
                state.input_state = INPUT_STATE_NAME;
            }

            break;
        }
        case STATE_SCENE_PATCHING:
        {
            MUTEX_SCOPE(state.patching_info.mutex,
                        {
                            char display_name[256] = {0};

                            PATCHING_STATE_CASE(PATCHING_STATE_NOT_STARTED);
                            PATCHING_STATE_CASE(PATCHING_STATE_BACKING_UP);
                            PATCHING_STATE_CASE(PATCHING_STATE_DECRYPTING);
                            PATCHING_STATE_CASE(PATCHING_STATE_SEARCHING);
                            PATCHING_STATE_CASE(PATCHING_STATE_PATCHING);
                            PATCHING_STATE_CASE(PATCHING_STATE_ENCRYPTING);
                            PATCHING_STATE_CASE(PATCHING_STATE_DONE);

                            if (state.patching_info.state == PATCHING_STATE_ERROR)
                            {
                                switch_scene(&state, STATE_SCENE_ERROR);
                            }

                            if (state.patching_info.state == PATCHING_STATE_DONE && !state.patching_info.is_running)
                            {
                                switch_scene(&state, STATE_SCENE_DONE_PATCHING);
                            }
                        });

            can_interact = false;

            break;
        }
        case STATE_SCENE_DONE_PATCHING:
        {
            // If the user pressed circle, switch back to the game selection scene
            if (state.circle_pressed)
            {
                switch_scene(&state, STATE_SCENE_SELECT_GAME);
                break;
            }

            char display[1024] = {0};
            snprintf(display, 1024, "Patched %s to %s! Just open your game and it should work!", state.selected_game->title, state.selected_server->name);

            // Draw the display name
            font_print_to_renderer(font, display, &font_state);
            // Move the text down by the height of the text
            font_state.y += FONT_CHAR_HEIGHT * font_state.h;
            break;
        }
        case STATE_SCENE_ERROR:
        {
            // If the user pressed circle, switch back to the game selection scene
            if (state.circle_pressed)
            {
                switch_scene(&state, STATE_SCENE_SELECT_GAME);
                break;
            }

            // If there was a patching error
            if (state.patching_info.last_error != NULL)
            {
                font_print_to_renderer(font, "Error when patching!!!", &font_state);
                font_state.y += FONT_CHAR_HEIGHT * font_state.h;

                font_print_to_renderer(font, state.patching_info.last_error, &font_state);
                font_state.y += FONT_CHAR_HEIGHT * font_state.h;
            }

            // If there was a general error
            if (state.last_error != NULL)
            {
                font_print_to_renderer(font, state.last_error, &font_state);
                font_state.y += FONT_CHAR_HEIGHT * font_state.h;
            }

            break;
        }
        default:
        {
            SDL_Log("Unknown scene: %d", state.scene);
            return 1;
        }
        }

        if (can_interact)
        {
            // Move the text down by the height of the text
            font_state.y += FONT_CHAR_HEIGHT * font_state.h;
            // Draw the display name
            font_print_to_renderer(font, "Press X to select, press O to go back.", &font_state);
            // Move the text down by the height of the text
            font_state.y += FONT_CHAR_HEIGHT * font_state.h;
        }

        // If the pad is not connected, display a message
        if (!is_pad_connected)
        {
            SDL_Rect font_state = {.x = 50, .y = 250, .h = 5, .w = 5};
            font_print_to_renderer(font, "No controller connected", &font_state);
        }

        // Reset the crossPressed and circlePressed flags, since they are single press
        state.cross_pressed = false;
        state.circle_pressed = false;

        SDL_RenderPresent(renderer);
    }

    if (ioPadEnd() != 0)
    {
        SDL_Log("Unable to end pad");
        return 1;
    }

    ASSERT_ZERO(sysLwMutexDestroy(state.patching_info.mutex), "Unable to destroy mutex");

    font_exit(font);
    SDL_Quit();

    return 0;
}
