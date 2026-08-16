#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Forward declaration
void filledCircleRGBA(SDL_Renderer *r, int cx, int cy, int radius,
    Uint8 rr, Uint8 gg, Uint8 bb, Uint8 aa);

// Helper: render text
void ui_render_text(SDL_Renderer *r, TTF_Font *font,
    const char *text, int x, int y, SDL_Color color) {
    if (!font || !text || text[0] == '\0') return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect rect = { x, y, surf->w, surf->h };
    SDL_RenderCopy(r, tex, NULL, &rect);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

void ui_render_text_centered(SDL_Renderer *r, TTF_Font *font,
    const char *text, int cx, int y, SDL_Color color) {
    if (!font || !text) return;
    int w, h;
    TTF_SizeUTF8(font, text, &w, &h);
    ui_render_text(r, font, text, cx - w/2, y, color);
}

// Helper: draw rounded rect
void ui_draw_rounded_rect(SDL_Renderer *r, int x, int y,
    int w, int h, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    
    // Fill center
    SDL_Rect center = { x + radius, y, w - 2*radius, h };
    SDL_RenderFillRect(r, &center);
    center.x = x; center.y = y + radius;
    center.w = w; center.h = h - 2*radius;
    SDL_RenderFillRect(r, &center);
    
    // Corners (simplified - just fill rects for now)
    SDL_Rect tl = { x, y, radius, radius };
    SDL_Rect tr = { x + w - radius, y, radius, radius };
    SDL_Rect bl = { x, y + h - radius, radius, radius };
    SDL_Rect br = { x + w - radius, y + h - radius, radius, radius };
    SDL_RenderFillRect(r, &tl);
    SDL_RenderFillRect(r, &tr);
    SDL_RenderFillRect(r, &bl);
    SDL_RenderFillRect(r, &br);
}

// Format time as MM:SS
static void format_time(double seconds, char *buf, int len) {
    int m = (int)seconds / 60;
    int s = (int)seconds % 60;
    snprintf(buf, len, "%02d:%02d", m, s);
}

// Draw top bar
void ui_draw_top_bar(SDL_Renderer *r, PlayerState *player,
    Playlist *pl, TTF_Font *font_med, TTF_Font *font_large, Theme *t) {
    int h = t->top_bar_height;
    SDL_Color text_color = { t->text_r, t->text_g, t->text_b, 255 };
    SDL_Color accent = { t->accent_r, t->accent_g, t->accent_b, 255 };
    SDL_Color dim = { t->text_dim_r, t->text_dim_g, t->text_dim_b, 255 };
    
    // Background
    SDL_Color panel = { t->panel_r, t->panel_g, t->panel_b, 255 };
    ui_draw_rounded_rect(r, 10, 5, 1260, h - 10, 12, panel);
    
    // Left: mode indicators
    ui_render_text(r, font_med, "循环", 30, 20, accent);
    ui_render_text(r, font_med, "均衡", 100, 20, dim);
    
    // Volume bar
    int vol = player_get_volume(player);
    SDL_SetRenderDrawColor(r, 60, 80, 100, 255);
    SDL_Rect vol_bg = { 30, 55, 150, 8 };
    SDL_RenderFillRect(r, &vol_bg);
    SDL_SetRenderDrawColor(r, t->accent_r, t->accent_g, t->accent_b, 255);
    SDL_Rect vol_fg = { 30, 55, 150 * vol / 100, 8 };
    SDL_RenderFillRect(r, &vol_fg);
    
    // Center: playback controls (circles)
    int cx = 640, cy = h/2;
    SDL_SetRenderDrawColor(r, 80, 100, 130, 255);
    // Prev button
    filledCircleRGBA(r, cx - 80, cy, 22, 80, 100, 130, 255);
    // Play/Pause button (larger)
    filledCircleRGBA(r, cx, cy, 30, t->accent_r, t->accent_g, t->accent_b, 255);
    // Next button
    filledCircleRGBA(r, cx + 80, cy, 22, 80, 100, 130, 255);
    
    // Play/pause icon
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    if (player_is_playing(player)) {
        // Pause icon (two bars)
        SDL_Rect p1 = { cx - 6, cy - 10, 5, 20 };
        SDL_Rect p2 = { cx + 1, cy - 10, 5, 20 };
        SDL_RenderFillRect(r, &p1);
        SDL_RenderFillRect(r, &p2);
    } else {
        // Play icon (triangle)
        SDL_RenderDrawLine(r, cx - 5, cy - 10, cx - 5, cy + 10);
        SDL_RenderDrawLine(r, cx - 5, cy - 10, cx + 10, cy);
        SDL_RenderDrawLine(r, cx + 10, cy, cx - 5, cy + 10);
    }
    
    // Prev/Next triangles
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderDrawLine(r, cx - 75, cy - 6, cx - 75, cy + 6);
    SDL_RenderDrawLine(r, cx - 75, cy - 6, cx - 85, cy);
    SDL_RenderDrawLine(r, cx - 85, cy, cx - 75, cy + 6);
    SDL_RenderDrawLine(r, cx + 75, cy - 6, cx + 75, cy + 6);
    SDL_RenderDrawLine(r, cx + 75, cy - 6, cx + 85, cy);
    SDL_RenderDrawLine(r, cx + 85, cy, cx + 75, cy + 6);
    
    // Right: song info
    const char *current = player_current_track(player);
    if (current) {
        char title[256];
        playlist_get_display_name(current, title, sizeof(title));
        ui_render_text(r, font_large, title, 820, 15, text_color);
        ui_render_text(r, font_med, "未知艺术家", 820, 50, dim);
    } else {
        ui_render_text(r, font_large, "未播放", 820, 25, dim);
    }
    
    // Time + progress bar + duration (full width at bottom of top bar)
    char timebuf[32], durbuf[32];
    double pos = player_get_position(player);
    double dur = player_get_duration(player);
    format_time(pos, timebuf, sizeof(timebuf));
    if (dur > 0) {
        format_time(dur, durbuf, sizeof(durbuf));
    } else {
        strcpy(durbuf, "--:--");
    }
    // Time labels
    ui_render_text(r, font_small, timebuf, 820, 68, dim);
    ui_render_text(r, font_small, durbuf, 1190, 68, dim);
    // Progress bar (full width under song info)
    SDL_SetRenderDrawColor(r, t->panel_r, t->panel_g, t->panel_b, 200);
    SDL_Rect pb_bg = { 820, 78, 440, 5 };
    SDL_RenderFillRect(r, &pb_bg);
    double ratio = (dur > 0.1) ? (pos / dur) : 0;
    if (ratio > 1.0) ratio = 1.0;
    if (ratio < 0) ratio = 0;
    SDL_SetRenderDrawColor(r, t->accent_r, t->accent_g, t->accent_b, 255);
    SDL_Rect pb_fg = { 820, 78, (int)(440 * ratio), 5 };
    SDL_RenderFillRect(r, &pb_fg);
}

// Draw main area (LEFT: spectrum/lyrics, RIGHT: playlist) - AiMusic style
void ui_draw_main_area(SDL_Renderer *r, Playlist *pl,
    int selected, int scroll, int panel_mode, Lyrics *lyrics,
    Spectrum *spec, PlayerState *player,
    TTF_Font *font_small, TTF_Font *font_med, TTF_Font *font_large, Theme *t) {
    int top = t->top_bar_height;
    int bottom = 720 - t->bottom_bar_height;
    int main_h = bottom - top;
    int right_w = t->left_panel_width; // right panel = playlist width
    int left_x = 10;
    int left_w = 1280 - right_w - 20;
    int right_x = 1280 - right_w + 10;
    
    SDL_Color text_color = { t->text_r, t->text_g, t->text_b, 255 };
    SDL_Color accent = { t->accent_r, t->accent_g, t->accent_b, 255 };
    SDL_Color dim = { t->text_dim_r, t->text_dim_g, t->text_dim_b, 255 };
    SDL_Color sel_color = { t->selected_r, t->selected_g, t->selected_b, 255 };
    SDL_Color panel = { t->panel_r, t->panel_g, t->panel_b, 255 };
    
    // === LEFT panel: Spectrum / Lyrics / Cover (large area) ===
    ui_draw_rounded_rect(r, left_x, top + 5, left_w, main_h - 10, 12, panel);
    
    if (panel_mode == 0) {
        // Lyrics mode
        ui_render_text(r, font_med, "歌词", left_x + 20, top + 15, accent);
        
        // Current lyric (large, centered)
        const char *cur_lyric = lyrics_get_current(lyrics);
        if (cur_lyric) {
            ui_render_text_centered(r, font_large, cur_lyric,
                left_x + left_w/2, top + main_h/2 - 20, text_color);
        } else {
            ui_render_text_centered(r, font_med, "暂无歌词",
                left_x + left_w/2, top + main_h/2, dim);
            ui_render_text_centered(r, font_small,
                "将歌词文件(.lrc)与音乐放在同一目录",
                left_x + left_w/2, top + main_h/2 + 35, dim);
        }
        
        // Surrounding lyrics (smaller, dimmer)
        int cur_idx = lyrics_get_current_index(lyrics);
        for (int i = -3; i <= 3; i++) {
            if (i == 0) continue;
            int li = cur_idx + i;
            if (li >= 0 && li < lyrics->count) {
                int y_off = top + main_h/2 + i * 35;
                SDL_Color c = (abs(i) == 1) ? dim : (SDL_Color){100,120,140,255};
                ui_render_text_centered(r, font_small,
                    lyrics_get_line(lyrics, li), left_x + left_w/2, y_off, c);
            }
        }
    } else if (panel_mode == 1) {
        // Spectrum mode - AiMusic style: title + time + progress + big spectrum
        ui_render_text(r, font_med, "频谱", left_x + 20, top + 15, accent);
        
        // Song title (centered)
        const char *current = player_current_track(player);
        if (current) {
            char title[256];
            playlist_get_display_name(current, title, sizeof(title));
            ui_render_text_centered(r, font_med, title,
                left_x + left_w/2, top + 50, text_color);
        }
        
        // Time display
        char tbuf[32], dbuf[32];
        format_time(player_get_position(player), tbuf, sizeof(tbuf));
        format_time(player_get_duration(player), dbuf, sizeof(dbuf));
        ui_render_text(r, font_small, tbuf, left_x + 30, top + 85, dim);
        ui_render_text(r, font_small, dbuf, left_x + left_w - 80, top + 85, dim);
        
        // Progress bar
        double pos = player_get_position(player);
        double dur = player_get_duration(player);
        if (dur > 0) {
            SDL_SetRenderDrawColor(r, 60, 80, 100, 255);
            SDL_Rect pb = { left_x + 30, top + 110, left_w - 60, 6 };
            SDL_RenderFillRect(r, &pb);
            SDL_SetRenderDrawColor(r, t->accent_r, t->accent_g, t->accent_b, 255);
            SDL_Rect pf = { left_x + 30, top + 110, (int)((left_w - 60) * pos / dur), 6 };
            SDL_RenderFillRect(r, &pf);
        }
        
        // Big spectrum bars (AiMusic style)
        if (spec) {
            spectrum_draw(spec, r, left_x + 30, top + 140,
                left_w - 60, main_h - 200, accent);
        }
    } else {
        // Cover mode
        ui_render_text(r, font_med, "封面", left_x + 20, top + 15, accent);
        ui_render_text_centered(r, font_med, "暂无封面",
            left_x + left_w/2, top + main_h/2, dim);
    }
    
    // === RIGHT panel: Playlist (AiMusic style) ===
    ui_draw_rounded_rect(r, right_x, top + 5, right_w - 20, main_h - 10, 12, panel);
    
    // Playlist header
    char header[64];
    snprintf(header, sizeof(header), "播放列表  %d首", pl->count);
    ui_render_text(r, font_med, header, right_x + 20, top + 15, accent);
    
    // Playlist items
    int item_h = 40;
    int start_y = top + 55;
    int visible = (main_h - 70) / item_h;
    
    for (int i = 0; i < visible && (i + scroll) < pl->count; i++) {
        int idx = i + scroll;
        int y = start_y + i * item_h;
        
        // Highlight selected
        if (idx == selected) {
            SDL_SetRenderDrawColor(r, sel_color.r, sel_color.g, sel_color.b, 180);
            SDL_Rect sel = { right_x + 10, y - 2, right_w - 40, item_h - 4 };
            SDL_RenderFillRect(r, &sel);
        }
        
        // Track name
        ui_render_text(r, font_small, pl->items[idx].title, right_x + 25, y + 8,
            idx == selected ? accent : text_color);
        
        
        // Playing indicator
        const char *cur = player_current_track(player);
        if (cur && strcmp(cur, pl->items[idx].path) == 0) {
            ui_render_text(r, font_small, ">", right_x + 10, y + 8, accent);
        }
    }
}

// Draw bottom bar
void ui_draw_bottom_bar(SDL_Renderer *r, int panel_mode,
    PlayerState *player, TTF_Font *font_small, Theme *t) {
    int y = 720 - t->bottom_bar_height;
    SDL_Color text_color = { t->text_r, t->text_g, t->text_b, 255 };
    SDL_Color accent = { t->accent_r, t->accent_g, t->accent_b, 255 };
    SDL_Color dim = { t->text_dim_r, t->text_dim_g, t->text_dim_b, 255 };
    SDL_Color panel = { t->panel_r, t->panel_g, t->panel_b, 255 };
    
    ui_draw_rounded_rect(r, 10, y + 5, 1260, t->bottom_bar_height - 10, 10, panel);
    
    // Mode buttons
    const char *modes[] = { "歌词", "频谱", "封面" };
    for (int i = 0; i < 3; i++) {
        int bx = 30 + i * 100;
        SDL_Color c = (i == panel_mode) ? accent : dim;
        ui_render_text(r, font_small, modes[i], bx, y + 15, c);
    }
    
    
    // Battery (simulated)
    ui_render_text(r, font_small, "电量 97%", 1150, y + 15, accent);
}

// Draw help overlay
void ui_draw_help(SDL_Renderer *r, TTF_Font *font, Theme *t) {
    (void)r; (void)font; (void)t;
    // Help overlay disabled per user request
}

// Need this for filled circles
void filledCircleRGBA(SDL_Renderer *r, int cx, int cy, int radius,
    Uint8 rr, Uint8 gg, Uint8 bb, Uint8 aa) {
    SDL_SetRenderDrawColor(r, rr, gg, bb, aa);
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderDrawPoint(r, cx + x, cy + y);
            }
        }
    }
}
