#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "playlist.h"
#include "player.h"

typedef struct {
    Uint8 r, g, b;
} ThemeColor;

typedef struct {
    ThemeColor bg;
    ThemeColor panel;
    ThemeColor text;
    ThemeColor dim;
    ThemeColor accent;
    ThemeColor selected;
} Theme;

void ui_init(SDL_Renderer *r, const char *font_path);
void ui_cleanup(void);
void ui_draw_channel_list(SDL_Renderer *r, ChannelList *pl, int selected, int scroll, Theme *t);
void ui_draw_playing_overlay(SDL_Renderer *r, Channel *ch, int volume, Theme *t);
void ui_draw_loading(SDL_Renderer *r, const char *text, Theme *t);
void ui_draw_error(SDL_Renderer *r, const char *text, Theme *t);
Theme *ui_get_default_theme(void);

#endif
