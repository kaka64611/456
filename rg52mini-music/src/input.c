#include "input.h"
#include <string.h>

static SDL_Joystick *joystick = NULL;
static int alt_held = 0; // Track Select (alt) key for combos

void input_init(void) {
    // Joystick handled by gptokeyb, do not open directly
    printf("Input: using gptokeyb keyboard mapping\n");
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
        int alt = (mod & KMOD_ALT) != 0 || alt_held;
        
        // Track alt key state
        if (key == SDLK_LALT || key == SDLK_RALT || key == SDLK_MODE) {
            alt_held = 1;
            return ACTION_NONE;
        }
        
        switch (key) {
            case SDLK_UP:    return ACTION_UP;
            case SDLK_DOWN:  return ACTION_DOWN;
            case SDLK_LEFT:  return ACTION_VOL_DOWN;
            case SDLK_RIGHT: return ACTION_VOL_UP;
            case SDLK_a:     return ACTION_PLAY;       // A = 播放
            case SDLK_b:     return ACTION_PAUSE;      // B = 暂停
            case SDLK_x:     return ACTION_TOGGLE_EQ;  // X = 切换EQ
            case SDLK_y:     return ACTION_PLAY_MODE;  // Y = 播放模式
            case SDLK_l:     return ACTION_PREV;       // L1 = 上一首
            case SDLK_r:     return ACTION_NEXT;       // R1 = 下一首
            case SDLK_o:     return ACTION_THEME_PREV; // L2 = 上一个主题
            case SDLK_p:     return ACTION_THEME_NEXT; // R2 = 下一个主题
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (alt) return ACTION_QUIT;             // Select+Start = 退出
                return ACTION_TOGGLE_PANEL;              // Start = 切换面板
            case SDLK_ESCAPE:
                return ACTION_QUIT;                      // Back = 退出
            case SDLK_SPACE:
                return ACTION_PLAY_PAUSE;
            case SDLK_t:
                return ACTION_TOGGLE_PANEL;
            default:
                return ACTION_NONE;
        }
    }
    
    if (event->type == SDL_KEYUP) {
        SDL_Keycode key = event->key.keysym.sym;
        if (key == SDLK_LALT || key == SDLK_RALT || key == SDLK_MODE) {
            alt_held = 0;
        }
    }
    
    // Joystick events are handled by gptokeyb (converted to keyboard)
    // Direct joystick handling disabled to avoid conflicts
    
    return ACTION_NONE;
}
