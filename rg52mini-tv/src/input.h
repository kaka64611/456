#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

typedef enum {
    ACTION_NONE = 0,
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_PAGE_UP,    // D-pad Left = page up
    ACTION_PAGE_DOWN,  // D-pad Right = page down
    ACTION_SELECT,      // A = play/select
    ACTION_BACK,        // B = back
    ACTION_MENU,        // X = menu/options
    ACTION_INFO,        // Y = info
    ACTION_PREV,        // L1 = prev channel
    ACTION_NEXT,        // R1 = next channel
    ACTION_VOL_DOWN,    // L2 = volume down
    ACTION_VOL_UP,      // R2 = volume up
    ACTION_TOGGLE_PANEL,// Start = toggle channel list
    ACTION_QUIT         // Select+Start = quit
} InputAction;

void input_init(void);
void input_cleanup(void);
InputAction input_process(SDL_Event *event);

#endif
