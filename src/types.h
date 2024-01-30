#pragma once

#include <sys/thread.h>
#include <sys/mutex.h>

#include "game_list.h"
#include "server_list.h"

#define OSK_TEXT_BUFFER_LENGTH 256

typedef enum STATE_SCENE
{
    STATE_SCENE_SELECT_NONE = 0,
    STATE_SCENE_SELECT_GAME,
    STATE_SCENE_SELECT_SERVER,
    STATE_SCENE_MANAGE_SERVERS,
    STATE_SCENE_PATCHING,
    STATE_SCENE_DONE_PATCHING,
    STATE_SCENE_ERROR
} STATE_SCENE;

inline char *get_scene_name(STATE_SCENE scene)
{
    switch (scene)
    {
    case STATE_SCENE_SELECT_GAME:
        return "Select Game";
    case STATE_SCENE_SELECT_SERVER:
        return "Select Server";
    case STATE_SCENE_MANAGE_SERVERS:
        return "Manage Servers";
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

typedef enum PATCHING_STATE
{
    PATCHING_STATE_NOT_STARTED = 0,
    PATCHING_STATE_BACKING_UP,
    PATCHING_STATE_DECRYPTING,
    PATCHING_STATE_SEARCHING,
    PATCHING_STATE_PATCHING,
    PATCHING_STATE_ENCRYPTING,
    PATCHING_STATE_DONE,
    PATCHING_STATE_ERROR,
} PATCHING_STATE;

inline char *get_patching_state_name(PATCHING_STATE state)
{
    switch (state)
    {
    case PATCHING_STATE_NOT_STARTED:
        return "Not Started";
    case PATCHING_STATE_BACKING_UP:
        return "Backing Up";
    case PATCHING_STATE_DECRYPTING:
        return "Decrypting";
    case PATCHING_STATE_SEARCHING:
        return "Searching";
    case PATCHING_STATE_PATCHING:
        return "Patching";
    case PATCHING_STATE_ENCRYPTING:
        return "Encrypting";
    case PATCHING_STATE_DONE:
        return "Done";
    case PATCHING_STATE_ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

typedef struct patching_info_t
{
    bool is_running;
    sys_ppu_thread_t *thread;
    sys_lwmutex_t *mutex;
    PATCHING_STATE state;
    char *last_error;
} patching_info_t;

typedef enum INPUT_STATE
{
    INPUT_STATE_NONE = 0,
    INPUT_STATE_AUTODISCOVER_URL,
    INPUT_STATE_NAME,
    INPUT_STATE_PATCH_URL,
} INPUT_STATE;

typedef struct state_t
{
    int selection;
    bool last_up;
    bool last_down;
    bool last_cross;
    bool last_circle;
    uint32_t game_count;
    game_list_entry *games;
    server_list_entry *servers;
    int server_count;
    STATE_SCENE scene;
    bool cross_pressed;
    bool circle_pressed;
    game_list_entry *selected_game;
    server_list_entry *selected_server;
    patching_info_t patching_info;
    char idps[16];
    // Wrap menu after this many times
    int wrap_count;
    u16 osk_buffer[OSK_TEXT_BUFFER_LENGTH];
    char *last_error;
    INPUT_STATE input_state;
    char *input_name;
} state_t;

#define MUTEX_SCOPE(mutex, ...)                                    \
    ASSERT_ZERO(sysLwMutexLock(mutex, 0), "Unable to lock mutex"); \
    __VA_ARGS__;                                                   \
    ASSERT_ZERO(sysLwMutexUnlock(mutex), "Unable to unlock mutex");
