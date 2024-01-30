#pragma once

#include <SDL2/SDL.h>

#define ASSERT_ZERO(condition, message)                                                          \
    if ((condition) != 0)                                                                        \
    {                                                                                            \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s - %s:%d\n", message, __FILE__, __LINE__); \
        exit(1);                                                                                 \
    }

#define ASSERT_NONZERO(condition, message)                                                                         \
    if ((condition) == 0)                                                                                          \
    {                                                                                                              \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s - %d - %s:%d\n", message, (condition), __FILE__, __LINE__); \
        exit(1);                                                                                                   \
    }

#define ASSERTSDL_ZERO(condition, message)                                                       \
    if ((condition) != 0)                                                                        \
    {                                                                                            \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s - %s:%d\n", message, __FILE__, __LINE__); \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL Error: %s\n", SDL_GetError());           \
        exit(1);                                                                                 \
    }

#define ASSERTSDL_NONZERO(condition, message)                                                    \
    if ((condition) == 0)                                                                        \
    {                                                                                            \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s - %s:%d\n", message, __FILE__, __LINE__); \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL Error: %s\n", SDL_GetError());           \
        exit(1);                                                                                 \
    }
