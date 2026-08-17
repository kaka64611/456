#include "input.h"
#include <stdio.h>
#include <string.h>

static SDL_Joystick *joystick = NULL;
static int select_held = 0;

void input_init(void) {
    if (SDL_NumJoysticks() > 0) {
        joystick = SDL_JoystickOpen(0);
        if (joystick) {
            printf("Opened joystick: %s\n", SDL_JoystickName(joystick));
        }
    } else {
        printf("No joystick found, using keyboard\n");
    }
}

void input_cleanup(void) {
    if (joystick) {
        SDL_JoystickClose(joystick);
        joystick = NULL;
    }
}

InputAction input_process(SDL_Event *event) {
    if (!event) return ACTION_NONE;

    // Direct joystick button events
    if (event->type == SDL_JOYBUTTONDOWN) {
        Uint8 btn = event->jbutton.button;
        printf("JOY button down: %d\n", btn);
        switch (btn) {
            case 1: return ACTION_SELECT;     // A (BUTTON 1) = Select/Play
            case 0: return ACTION_BACK;       // B (BUTTON 0) = Back
            case 2: return ACTION_MENU;       // X = Menu
            case 3: return ACTION_INFO;       // Y = Info
            case 4: return ACTION_PREV;       // L1 = Prev channel
            case 5: return ACTION_NEXT;       // R1 = Next channel
            case 6: return ACTION_VOL_DOWN;   // L2 = Vol down
            case 7: return ACTION_VOL_UP;     // R2 = Vol up
            case 8: select_held = 1; return ACTION_NONE; // Select
            case 9:
                if (select_held) return ACTION_QUIT;
                return ACTION_TOGGLE_PANEL;  // Start
            case 14: return ACTION_UP;        // D-pad Up
            case 15: return ACTION_DOWN;      // D-pad Down
            case 16: return ACTION_PAGE_UP;   // D-pad Left = Page up
            case 17: return ACTION_PAGE_DOWN; // D-pad Right = Page down
            default:
                printf("JOY unhandled button: %d\n", btn);
                return ACTION_NONE;
        }
    }

    if (event->type == SDL_JOYBUTTONUP) {
        if (event->jbutton.button == 8) select_held = 0;
        return ACTION_NONE;
    }

    // Keyboard fallback
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_UP: return ACTION_UP;
            case SDLK_DOWN: return ACTION_DOWN;
            case SDLK_LEFT: return ACTION_LEFT;
            case SDLK_RIGHT: return ACTION_RIGHT;
            case SDLK_RETURN: return ACTION_SELECT;
            case SDLK_ESCAPE: return ACTION_BACK;
            case SDLK_x: return ACTION_MENU;
            case SDLK_y: return ACTION_INFO;
            case SDLK_q: return ACTION_QUIT;
            default: return ACTION_NONE;
        }
    }

    return ACTION_NONE;
}
