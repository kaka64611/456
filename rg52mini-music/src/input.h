#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

typedef enum {
    ACTION_NONE,
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_SELECT,
    ACTION_BACK,
    ACTION_PLAY_PAUSE,
    ACTION_NEXT,
    ACTION_PREV,
    ACTION_VOL_UP,
    ACTION_VOL_DOWN,
    ACTION_SEEK_FORWARD,
    ACTION_SEEK_BACKWARD,
    ACTION_TOGGLE_PANEL,
    ACTION_TOGGLE_VIEW,
    ACTION_TOGGLE_EQ,
    ACTION_HELP,
    ACTION_QUIT
} InputAction;

// Initialize input (joystick etc)
void input_init(void);

// Cleanup input
void input_cleanup(void);

// Process SDL event and return action
InputAction input_process(SDL_Event *event);

#endif
