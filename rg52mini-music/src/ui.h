#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "player.h"
#include "playlist.h"
#include "theme.h"
#include "lyrics.h"
#include "spectrum.h"

// Draw top bar (controls + song info + cover)
void ui_draw_top_bar(SDL_Renderer *r, PlayerState *player,
    Playlist *pl, TTF_Font *font_med, TTF_Font *font_large, Theme *t, int eq_preset, int volume_show);

// Draw main area (left playlist + right spectrum/lyrics)
void ui_draw_main_area(SDL_Renderer *r, Playlist *pl,
    int selected, int scroll, int panel_mode, Lyrics *lyrics,
    Spectrum *spec, PlayerState *player,
    TTF_Font *font_small, TTF_Font *font_med, TTF_Font *font_large, Theme *t);

// Draw bottom bar (mode switch + status)
void ui_draw_bottom_bar(SDL_Renderer *r, int panel_mode,
    PlayerState *player, TTF_Font *font_small, Theme *t);

// Draw help overlay
void ui_draw_help(SDL_Renderer *r, TTF_Font *font, Theme *t);

// Helper: render text
void ui_render_text(SDL_Renderer *r, TTF_Font *font,
    const char *text, int x, int y, SDL_Color color);

// Helper: render text centered
void ui_render_text_centered(SDL_Renderer *r, TTF_Font *font,
    const char *text, int cx, int y, SDL_Color color);

// Helper: draw rounded rect
void ui_draw_rounded_rect(SDL_Renderer *r, int x, int y,
    int w, int h, int radius, SDL_Color color);

void ui_draw_volume_overlay(SDL_Renderer *r, PlayerState *player, Theme *t, TTF_Font *font_med);

#endif
