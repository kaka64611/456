#include "input.h"
#include <string.h>

static SDL_Joystick *joystick = NULL;
static int alt_held = 0; // Track Select (alt) key for combos

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

// Map RG52MINI buttons to actions
// Handle both joystick (direct) and keyboard (gptokeyb) events
static int select_held = 0; // Track Select button for combo

InputAction input_process(SDL_Event *event) {
    if (!event) return ACTION_NONE;

    // === Direct joystick button events (most reliable) ===
    if (event->type == SDL_JOYBUTTONDOWN) {
        Uint8 btn = event->jbutton.button;
        printf("JOY button down: %d\n", btn);
        // Try standard mapping first, with fallbacks for X/Y
        switch (btn) {
            case 0: return ACTION_PLAY;        // A = Play
            case 1: return ACTION_PAUSE;       // B = Pause
            case 2: printf("JOY X -> EQ\n"); return ACTION_TOGGLE_EQ;   // X = EQ preset
            case 3: printf("JOY Y -> MODE\n"); return ACTION_PLAY_MODE;   // Y = Play mode
            case 4: return ACTION_UP;          // D-pad Up = scroll up
            case 5: return ACTION_DOWN;        // D-pad Down = scroll down
            case 6: return ACTION_THEME_PREV;  // L2 = Theme prev
            case 7: return ACTION_THEME_NEXT;  // R2 = Theme next
            case 8:                            // Select alt
                select_held = 1;
                return ACTION_NONE;
            case 9:                            // Start
                if (select_held) return ACTION_QUIT;
                return ACTION_TOGGLE_PANEL;
            // RG52MINI actual button mapping (from debug log)
            case 14: return ACTION_PREV;       // L1 = Previous
            case 15: return ACTION_NEXT;       // R1 = Next
            case 16: return ACTION_VOL_DOWN;   // D-pad Left = volume down
            case 17: return ACTION_VOL_UP;     // D-pad Right = volume up
            default:
                printf("JOY unhandled button: %d\n", btn);
                return ACTION_NONE;
        }
    }

    if (event->type == SDL_JOYBUTTONUP) {
        Uint8 btn = event->jbutton.button;
        // Select could be button 8 on some devices
        if (btn == 8) select_held = 0;
        return ACTION_NONE;
    }

    // Joystick hat (D-pad)
    if (event->type == SDL_JOYHATMOTION) {
        Uint8 hat = event->jhat.value;
        printf("JOY hat value: %d (up=%d down=%d left=%d right=%d)\n",
            hat, !!(hat&SDL_HAT_UP), !!(hat&SDL_HAT_DOWN),
            !!(hat&SDL_HAT_LEFT), !!(hat&SDL_HAT_RIGHT));
        if (hat & SDL_HAT_UP) return ACTION_UP;
        if (hat & SDL_HAT_DOWN) return ACTION_DOWN;
        if (hat & SDL_HAT_LEFT) return ACTION_VOL_DOWN;
        if (hat & SDL_HAT_RIGHT) return ACTION_VOL_UP;
    }

    // === Keyboard events (gptokeyb fallback) ===
    if (event->type == SDL_KEYDOWN) {
        SDL_Keycode key = event->key.keysym.sym;
        int mod = event->key.keysym.mod;
        int alt = (mod & KMOD_ALT) != 0;

        switch (key) {
            case SDLK_UP:    return ACTION_UP;
            case SDLK_DOWN:  return ACTION_DOWN;
            case SDLK_LEFT:  return ACTION_VOL_DOWN;
            case SDLK_RIGHT: return ACTION_VOL_UP;
            case SDLK_a:     return ACTION_PLAY;
            case SDLK_b:     return ACTION_PAUSE;
            case SDLK_x:     return ACTION_TOGGLE_EQ;
            case SDLK_y:     return ACTION_PLAY_MODE;
            // l/r/o/p removed - shoulder buttons handled by joystick events only
            // This prevents gptokeyb dpad mapping from triggering theme switch
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (alt) return ACTION_QUIT;
                return ACTION_TOGGLE_PANEL;
            case SDLK_ESCAPE:
                return ACTION_QUIT;
            default:
                return ACTION_NONE;
        }
    }

    return ACTION_NONE;
}
