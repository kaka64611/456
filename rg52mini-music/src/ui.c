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

// Draw panel with shadow, rounded corners and subtle inner highlight
static void ui_draw_gradient_panel(SDL_Renderer *r, int x, int y, int w, int h, int radius, Theme *t) {
    (void)radius;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    
    // Outer shadow (4 layers)
    for (int s = 0; s < 4; s++) {
        int alpha = 50 - s * 12;
        SDL_SetRenderDrawColor(r, 0, 0, 0, alpha);
        SDL_Rect sh = { x - s, y - s + 2, w + s*2, h + s*2 };
        SDL_RenderFillRect(r, &sh);
    }
    
    // Main panel background - solid with slight variation
    SDL_SetRenderDrawColor(r, t->panel_r, t->panel_g, t->panel_b, 235);
    SDL_Rect bg = { x, y, w, h };
    SDL_RenderFillRect(r, &bg);
    
    // Subtle top inner highlight
    SDL_SetRenderDrawColor(r, t->panel_r + 25, t->panel_g + 25, t->panel_b + 30, 60);
    SDL_Rect top_hl = { x + 1, y + 1, w - 2, 2 };
    SDL_RenderFillRect(r, &top_hl);
    
    // Subtle bottom inner shadow
    SDL_SetRenderDrawColor(r, 0, 0, 0, 40);
    SDL_Rect bottom_sh = { x + 1, y + h - 3, w - 2, 2 };
    SDL_RenderFillRect(r, &bottom_sh);
    
    // Border accent line (top)
    SDL_SetRenderDrawColor(r, t->accent_r, t->accent_g, t->accent_b, 100);
    SDL_Rect accent_top = { x, y, w, 1 };
    SDL_RenderFillRect(r, &accent_top);
    
    // Border (other sides, subtle)
    SDL_SetRenderDrawColor(r, t->panel_r - 20, t->panel_g - 20, t->panel_b - 20, 180);
    SDL_Rect border_l = { x, y + 1, 1, h - 1 };
    SDL_Rect border_r = { x + w - 1, y + 1, 1, h - 1 };
    SDL_Rect border_b = { x, y + h - 1, w, 1 };
    SDL_RenderFillRect(r, &border_l);
    SDL_RenderFillRect(r, &border_r);
    SDL_RenderFillRect(r, &border_b);
}

// Draw gradient bar (for spectrum and progress)
static void ui_draw_gradient_bar(SDL_Renderer *r, int x, int y, int w, int h, Theme *t) {
    if (h <= 0) return;
    for (int i = 0; i < h; i++) {
        int ratio = i * 255 / h;
        Uint8 cr = t->accent_r - (t->accent_r * ratio / 510);
        Uint8 cg = t->accent_g - (t->accent_g * ratio / 510);
        Uint8 cb = t->accent_b - (t->accent_b * ratio / 510);
        SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
        SDL_Rect line = { x, y + h - 1 - i, w, 1 };
        SDL_RenderFillRect(r, &line);
    }
}

static void format_time(double seconds, char *buf, int len) {
    int m = (int)seconds / 60;
    int s = (int)seconds % 60;
    snprintf(buf, len, "%02d:%02d", m, s);
}

// Draw top bar
void ui_draw_top_bar(SDL_Renderer *r, PlayerState *player,
    Playlist *pl, TTF_Font *font_med, TTF_Font *font_large, Theme *t, int eq_preset, int volume_show) {
    int h = t->top_bar_height;
    SDL_Color text_color = { t->text_r, t->text_g, t->text_b, 255 };
    SDL_Color accent = { t->accent_r, t->accent_g, t->accent_b, 255 };
    SDL_Color dim = { t->text_dim_r, t->text_dim_g, t->text_dim_b, 255 };
    
    // Background
    SDL_Color panel = { t->panel_r, t->panel_g, t->panel_b, 255 };
    ui_draw_rounded_rect(r, 10, 5, 1260, h - 10, 12, panel);
    
    // Left: play mode and EQ status
    const char *mode_names[] = {"顺序", "循环", "随机"};
    const char *eq_names[] = {"默认", "流行", "舞曲", "爵士", "摇滚", "古典"};
    int play_mode = player_get_play_mode(player);
    if (play_mode < 0 || play_mode > 2) play_mode = 0;
    if (eq_preset < 0 || eq_preset > 5) eq_preset = 0;
    
    char mode_buf[32], eq_buf[32];
    snprintf(mode_buf, sizeof(mode_buf), "模式:%s", mode_names[play_mode]);
    snprintf(eq_buf, sizeof(eq_buf), "EQ:%s", eq_names[eq_preset]);
    
    ui_render_text(r, font_med, mode_buf, 30, 18, accent);
    ui_render_text(r, font_med, eq_buf, 30, 45, dim);
    
    // Volume bar moved to overlay (drawn last)
    
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
    
    // Time display only (no progress bar per user request)
    char timebuf[32];
    double pos = player_get_position(player);
    format_time(pos, timebuf, sizeof(timebuf));
    ui_render_text(r, font_med, timebuf, 1150, 55, dim);
}

// Draw main area (LEFT: spectrum/lyrics, RIGHT: playlist) - AiMusic style
void ui_draw_main_area(SDL_Renderer *r, Playlist *pl,
    int selected, int scroll, int panel_mode, Lyrics *lyrics,
    Spectrum *spec, PlayerState *player, SDL_Texture *cover,
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
    ui_draw_gradient_panel(r, left_x, top + 5, left_w, main_h - 10, 12, t);
    
    if (panel_mode == 0) {
        // Lyrics mode - Karaoke style, current line highlighted in center
        ui_render_text(r, font_med, "歌词", left_x + 20, top + 15, accent);

        int cur_idx = lyrics_get_current_index(lyrics);
        int center_y = top + main_h / 2;
        int line_h = 48;

        if (lyrics->count == 0) {
            ui_render_text_centered(r, font_med, "暂无歌词",
                left_x + left_w/2, center_y, dim);
            ui_render_text_centered(r, font_small,
                "将歌词文件(.lrc)与音乐放在同一目录",
                left_x + left_w/2, center_y + 40, dim);
        } else {
            for (int i = -3; i <= 3; i++) {
                int li = cur_idx + i;
                if (li < 0 || li >= lyrics->count) continue;
                int y_off = center_y + i * line_h - 14;
                if (i == 0) {
                    ui_render_text_centered(r, font_large,
                        lyrics_get_line(lyrics, li),
                        left_x + left_w/2, y_off, accent);
                } else if (abs(i) == 1) {
                    ui_render_text_centered(r, font_med,
                        lyrics_get_line(lyrics, li),
                        left_x + left_w/2, y_off, dim);
                } else {
                    SDL_Color far = {100, 110, 120, 255};
                    ui_render_text_centered(r, font_small,
                        lyrics_get_line(lyrics, li),
                        left_x + left_w/2, y_off, far);
                }
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
        if (cover) {
            // Get texture size
            int tw, th;
            SDL_QueryTexture(cover, NULL, NULL, &tw, &th);
            // Calculate display size (fit in panel, maintain aspect ratio)
            int panel_w = left_w - 80;
            int panel_h = main_h - 100;
            float scale = (float)panel_w / tw;
            if (scale * th > panel_h) scale = (float)panel_h / th;
            int dw = (int)(tw * scale);
            int dh = (int)(th * scale);
            int dx = left_x + (left_w - dw) / 2;
            int dy = top + 50 + (panel_h - dh) / 2;
            // Draw shadow
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(r, 0, 0, 0, 100);
            SDL_Rect shadow = { dx + 4, dy + 4, dw, dh };
            SDL_RenderFillRect(r, &shadow);
            // Draw cover
            SDL_Rect cover_rect = { dx, dy, dw, dh };
            SDL_RenderCopy(r, cover, NULL, &cover_rect);
            // Border
            SDL_SetRenderDrawColor(r, t->accent_r, t->accent_g, t->accent_b, 150);
            SDL_Rect border = { dx - 1, dy - 1, dw + 2, dh + 2 };
            SDL_RenderDrawRect(r, &border);
        } else {
            ui_render_text_centered(r, font_med, "暂无封面",
                left_x + left_w/2, top + main_h/2, dim);
            ui_render_text_centered(r, font_small,
                "将封面图片(jpg/png)与音乐放在同一目录",
                left_x + left_w/2, top + main_h/2 + 40, dim);
        }
    }
    
    // === RIGHT panel: Playlist (AiMusic style) ===
    ui_draw_gradient_panel(r, right_x, top + 5, right_w - 20, main_h - 10, 12, t);
    
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
        
        // Highlight selected (rounded glass effect)
        if (idx == selected) {
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            // Gradient highlight
            for (int py = 0; py < item_h - 6; py++) {
                int alpha = 160 - (py * 60 / (item_h - 6));
                SDL_SetRenderDrawColor(r, t->accent_r, t->accent_g, t->accent_b, alpha);
                SDL_Rect line = { right_x + 12, y - 1 + py, right_w - 44, 1 };
                SDL_RenderFillRect(r, &line);
            }
            // Left accent bar
            SDL_SetRenderDrawColor(r, t->accent_r, t->accent_g, t->accent_b, 255);
            SDL_Rect accent_bar = { right_x + 12, y - 1, 3, item_h - 4 };
            SDL_RenderFillRect(r, &accent_bar);
        } else {
            // Separator line
            SDL_SetRenderDrawColor(r, t->text_dim_r, t->text_dim_g, t->text_dim_b, 40);
            SDL_Rect sep = { right_x + 20, y + item_h - 2, right_w - 50, 1 };
            SDL_RenderFillRect(r, &sep);
        }
        
        // Track name
        ui_render_text(r, font_small, pl->items[idx].title, right_x + 25, y + 8,
            idx == selected ? accent : text_color);
        
        // Track duration (right-aligned)
        if (pl->items[idx].duration > 0) {
            char dur_str[16];
            int mins = pl->items[idx].duration / 60;
            int secs = pl->items[idx].duration % 60;
            snprintf(dur_str, sizeof(dur_str), "%d:%02d", mins, secs);
            // Measure text width for right alignment
            int tw, th;
            TTF_SizeText(font_small, dur_str, &tw, &th);
            ui_render_text(r, font_small, dur_str,
                right_x + right_w - 30 - tw, y + 8,
                idx == selected ? accent : dim);
        }
        
        
        // Playing indicator
        const char *cur = player_current_track(player);
        if (cur && strcmp(cur, pl->items[idx].path) == 0) {
            filledCircleRGBA(r, right_x + 18, y + 18, 4, t->accent_r, t->accent_g, t->accent_b, 255);
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
    
    // Mode buttons (tight spacing)
    const char *modes[] = { "歌词", "频谱", "封面" };
    int mode_x = 25;
    for (int i = 0; i < 3; i++) {
        SDL_Color c = (i == panel_mode) ? accent : dim;
        ui_render_text(r, font_small, modes[i], mode_x, y + 15, c);
        mode_x += 52; // tight spacing
    }
    
    // Separator after mode buttons
    ui_render_text(r, font_small, "|", mode_x, y + 15, dim);
    mode_x += 15;
    
    // Key hints
    typedef struct { const char *key; const char *action; } KeyHint;
    KeyHint hints[] = {
        {"Sel+Start", "退出"},
        {"A", "播放"},
        {"B", "暂停"},
        {"X", "EQ"},
        {"Y", "模式"},
        {"L1", "上首"},
        {"R1", "下首"},
    };
    int hint_count = sizeof(hints) / sizeof(hints[0]);
    
    for (int i = 0; i < hint_count; i++) {
        // Key name in accent
        ui_render_text(r, font_small, hints[i].key, mode_x, y + 15, accent);
        int key_w = 0, dummy;
        TTF_SizeText(font_small, hints[i].key, &key_w, &dummy);
        // Action in dim
        ui_render_text(r, font_small, hints[i].action, mode_x + key_w + 4, y + 15, dim);
        int act_w = 0;
        TTF_SizeText(font_small, hints[i].action, &act_w, &dummy);
        mode_x += key_w + act_w + 4;
        // Separator between hints (except last)
        if (i < hint_count - 1) {
            ui_render_text(r, font_small, "|", mode_x, y + 15, dim);
            mode_x += 12;
        }
    }
    
    // Battery (right aligned)
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


// Floating volume overlay (drawn last, on top of everything)
void ui_draw_volume_overlay(SDL_Renderer *r, PlayerState *player, Theme *t, TTF_Font *font_med) {
    int vol = player_get_volume(player);
    int vw = 500, vh = 28;
    int vx = (1280 - vw) / 2;
    int vy = 720 - 100;
    
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    // Background glass panel
    SDL_SetRenderDrawColor(r, 0, 0, 0, 200);
    SDL_Rect vol_box = { vx - 20, vy - 15, vw + 40, vh + 45 };
    SDL_RenderFillRect(r, &vol_box);
    // Border glow
    SDL_SetRenderDrawColor(r, t->accent_r, t->accent_g, t->accent_b, 100);
    SDL_Rect vol_border = { vx - 20, vy - 15, vw + 40, 2 };
    SDL_RenderFillRect(r, &vol_border);
    // Bar background
    SDL_SetRenderDrawColor(r, 40, 45, 55, 255);
    SDL_Rect vol_bg = { vx, vy, vw, vh };
    SDL_RenderFillRect(r, &vol_bg);
    // Bar foreground (gradient)
    ui_draw_gradient_bar(r, vx, vy, vw * vol / 100, vh, t);
    // Volume text
    char vol_buf[32];
    snprintf(vol_buf, sizeof(vol_buf), "音量 %d%%", vol);
    ui_render_text_centered(r, font_med, vol_buf, 640, vy + vh + 5, (SDL_Color){255,255,255,255});
}
