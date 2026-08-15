#include "input.h"
#include <string.h>

static SDL_Joystick *joystick = NULL;

void input_init(void) {
    if (SDL_NumJoysticks() > 0) {
        joystick = SDL_JoystickOpen(0);
        if (joystick) {
            printf("Opened joystick: %s\n", SDL_JoystickName(joystick));
        }
    }
}

void input_cleanup(void) {
    if (joystick) {
        SDL_JoystickClose(joystick);
        joystick = NULL;
    }
}

// Map RG52MINI buttons to actions
// gptokeyb translates gamepad buttons to keyboard keys
// So we primarily handle keyboard events
InputAction input_process(SDL_Event *event) {
    if (!event) return ACTION_NONE;
    
    if (event->type == SDL_KEYDOWN) {
        SDL_Keycode key = event->key.keysym.sym;
        int mod = event->key.keysym.mod;
        int alt = (mod & KMOD_ALT) != 0;
        
        switch (key) {
            case SDLK_UP:    return ACTION_UP;
            case SDLK_DOWN:  return ACTION_DOWN;
            case SDLK_LEFT:  return ACTION_LEFT;
            case SDLK_RIGHT: return ACTION_RIGHT;
            case SDLK_a:     return alt ? ACTION_NONE : ACTION_SELECT;
            case SDLK_b:     return alt ? ACTION_TOGGLE_VIEW : ACTION_BACK;
            case SDLK_x:     return alt ? ACTION_TOGGLE_EQ : ACTION_NEXT;
            case SDLK_y:     return alt ? ACTION_HELP : ACTION_PREV;
            case SDLK_l:     return alt ? ACTION_TOGGLE_PANEL : ACTION_VOL_DOWN;
            case SDLK_r:     return ACTION_VOL_UP;
            case SDLK_o:     return ACTION_SEEK_BACKWARD;
            case SDLK_p:     return ACTION_SEEK_FORWARD;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                return alt ? ACTION_QUIT : ACTION_PLAY_PAUSE;
            case SDLK_ESCAPE:
                return ACTION_QUIT;
            case SDLK_SPACE:
                return ACTION_PLAY_PAUSE;
            case SDLK_t:
                return ACTION_TOGGLE_PANEL;
            default:
                return ACTION_NONE;
        }
    }
    
    // Also handle joystick events directly
    if (event->type == SDL_JOYBUTTONDOWN) {
        Uint8 btn = event->jbutton.button;
        switch (btn) {
            case 0: return ACTION_SELECT;     // A
            case 1: return ACTION_BACK;       // B
            case 2: return ACTION_NEXT;       // X
            case 3: return ACTION_PREV;       // Y
            case 4: return ACTION_VOL_DOWN;   // L1
            case 5: return ACTION_VOL_UP;     // R1
            case 6: return ACTION_SEEK_BACKWARD; // L2
            case 7: return ACTION_SEEK_FORWARD;  // R2
            case 8: return ACTION_PLAY_PAUSE; // Select
            case 9: return ACTION_TOGGLE_VIEW; // Start
            default: return ACTION_NONE;
        }
    }
    
    if (event->type == SDL_JOYHATMOTION) {
        Uint8 hat = event->jhat.value;
        if (hat & SDL_HAT_UP) return ACTION_UP;
        if (hat & SDL_HAT_DOWN) return ACTION_DOWN;
        if (hat & SDL_HAT_LEFT) return ACTION_LEFT;
        if (hat & SDL_HAT_RIGHT) return ACTION_RIGHT;
    }
    
    return ACTION_NONE;
}
