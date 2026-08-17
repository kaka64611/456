#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TTF_Font *font_small = NULL;
static TTF_Font *font_medium = NULL;
static TTF_Font *font_large = NULL;

void ui_init(SDL_Renderer *r, const char *font_path) {
    (void)r;
    font_small = TTF_OpenFont(font_path, 20);
    font_medium = TTF_OpenFont(font_path, 28);
    font_large = TTF_OpenFont(font_path, 36);
    if (!font_small || !font_medium || !font_large) {
        printf("Failed to load font: %s\n", TTF_GetError());
    }
}

void ui_cleanup(void) {
    if (font_small) TTF_CloseFont(font_small);
    if (font_medium) TTF_CloseFont(font_medium);
    if (font_large) TTF_CloseFont(font_large);
}

static void render_text(SDL_Renderer *r, TTF_Font *font, const char *text,
                        int x, int y, SDL_Color color) {
    if (!font || !text || text[0] == '\0') return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect rect = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, NULL, &rect);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

static void render_text_centered(SDL_Renderer *r, TTF_Font *font, const char *text,
                                  int cx, int y, SDL_Color color) {
    if (!font || !text || text[0] == '\0') return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect rect = {cx - surf->w/2, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, NULL, &rect);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

Theme *ui_get_default_theme(void) {
    static Theme t = {
        .bg = {15, 20, 35},
        .panel = {25, 35, 55},
        .text = {230, 235, 245},
        .dim = {140, 150, 170},
        .accent = {80, 160, 255},
        .selected = {50, 80, 140}
    };
    return &t;
}

void ui_draw_channel_list(SDL_Renderer *r, ChannelList *pl, int selected, int scroll, Theme *t) {
    // Background
    SDL_SetRenderDrawColor(r, t->bg.r, t->bg.g, t->bg.b, 255);
    SDL_RenderClear(r);

    // Header
    SDL_Color accent = {t->accent.r, t->accent.g, t->accent.b, 255};
    SDL_Color text = {t->text.r, t->text.g, t->text.b, 255};
    SDL_Color dim = {t->dim.r, t->dim.g, t->dim.b, 255};

    char header[64];
    snprintf(header, sizeof(header), "电视频道  (%d个)", pl->count);
    render_text(r, font_large, header, 40, 30, accent);

    // Channel list
    int item_h = 50;
    int start_y = 100;
    int visible = 11;

    for (int i = 0; i < visible && (i + scroll) < pl->count; i++) {
        int idx = i + scroll;
        int y = start_y + i * item_h;

        if (idx == selected) {
            // Selected item background
            SDL_SetRenderDrawColor(r, t->selected.r, t->selected.g, t->selected.b, 255);
            SDL_Rect sel = {30, y - 4, 1220, item_h - 6};
            SDL_RenderFillRect(r, &sel);
            // Accent bar
            SDL_SetRenderDrawColor(r, t->accent.r, t->accent.g, t->accent.b, 255);
            SDL_Rect bar = {30, y - 4, 4, item_h - 6};
            SDL_RenderFillRect(r, &bar);
        }

        // Channel number
        char num[16];
        snprintf(num, sizeof(num), "%02d", idx + 1);
        render_text(r, font_medium, num, 55, y + 8, dim);

        // Channel name
        render_text(r, font_medium, pl->items[idx].name, 110, y + 8,
                    idx == selected ? accent : text);

        // Group name
        if (pl->items[idx].group[0]) {
            render_text(r, font_small, pl->items[idx].group, 900, y + 14, dim);
        }
    }

    // Footer hint
    render_text(r, font_small, "A:播放 B:返回 上下:选择 左右:翻页 L1/R1:换台 L2/R2:音量 Start:列表 Sel+Start:退出",
                40, 680, dim);
}

void ui_draw_playing_overlay(SDL_Renderer *r, Channel *ch, int volume, Theme *t) {
    (void)r;
    // Semi-transparent overlay at bottom
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 150);
    SDL_Rect bar = {0, 660, 1280, 60};
    SDL_RenderFillRect(r, &bar);

    SDL_Color text = {255, 255, 255, 255};
    SDL_Color accent = {t->accent.r, t->accent.g, t->accent.b, 255};

    if (ch) {
        render_text(r, font_medium, ch->name, 30, 672, text);
        if (ch->group[0]) {
            render_text(r, font_small, ch->group, 30, 700, accent);
        }
    }

    // Volume
    char volbuf[32];
    snprintf(volbuf, sizeof(volbuf), "音量 %d%%", volume);
    render_text(r, font_small, volbuf, 1150, 680, text);
}

void ui_draw_loading(SDL_Renderer *r, const char *text, Theme *t) {
    SDL_SetRenderDrawColor(r, t->bg.r, t->bg.g, t->bg.b, 255);
    SDL_RenderClear(r);

    SDL_Color accent = {t->accent.r, t->accent.g, t->accent.b, 255};
    render_text_centered(r, font_large, text ? text : "加载中...", 640, 340, accent);
}

void ui_draw_error(SDL_Renderer *r, const char *text, Theme *t) {
    SDL_SetRenderDrawColor(r, 30, 15, 15, 255);
    SDL_RenderClear(r);

    SDL_Color red = {255, 100, 100, 255};
    SDL_Color text_color = {230, 230, 230, 255};
    render_text_centered(r, font_large, "播放错误", 640, 280, red);
    if (text) {
        render_text_centered(r, font_medium, text, 640, 340, text_color);
    }
    render_text_centered(r, font_small, "按B键返回频道列表", 640, 420, text_color);
}
