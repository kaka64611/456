/*
 * RG52MINI Music Player - A custom music player for Anbernic RG52MINI
 * Based on SDL2, inspired by AiMusic UI layout
 * Screen: 1280x720, AArch64, EmuELEC 4.7
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "player.h"
#include "playlist.h"
#include "ui.h"
#include "theme.h"
#include "input.h"
#include "lyrics.h"
#include "spectrum.h"
#include "eq.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720
#define FPS 60

// Global state
typedef enum {
    VIEW_MAIN,
    VIEW_BROWSER,
    VIEW_SETTINGS
} ViewType;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font_small;
    TTF_Font *font_medium;
    TTF_Font *font_large;
    
    ViewType current_view;
    int running;
    
    PlayerState *player;
    Playlist *playlist;
    Theme *theme;
    Lyrics *lyrics;
    Spectrum *spectrum;
    EQState *eq;
    
    int selected_index;
    int list_scroll;
    int right_panel_mode; // 0=lyrics, 1=spectrum, 2=cover
    int show_help;
} AppState;

AppState *app = NULL;

// Initialize SDL and app state
int app_init(const char *music_dir) {
    app = (AppState *)calloc(1, sizeof(AppState));
    if (!app) return -1;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return -1;
    }
    
    // Create window (fullscreen for RG52MINI)
    app->window = SDL_CreateWindow("RG52MINI Music",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);
    if (!app->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return -1;
    }
    
    app->renderer = SDL_CreateRenderer(app->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app->renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        return -1;
    }
    
    // Initialize TTF
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF init failed: %s\n", TTF_GetError());
        return -1;
    }
    
    // Load fonts
    app->font_small = TTF_OpenFont("assets/fonts/NotoSansCJKsc-Regular.otf", 20);
    app->font_medium = TTF_OpenFont("assets/fonts/NotoSansCJKsc-Regular.otf", 24);
    app->font_large = TTF_OpenFont("assets/fonts/NotoSansCJKsc-Regular.otf", 32);
    if (!app->font_small || !app->font_medium || !app->font_large) {
        fprintf(stderr, "Font load failed: %s\n", TTF_GetError());
        // Continue without fonts if possible
    }
    
    // Initialize audio player
    app->player = player_init();
    if (!app->player) {
        fprintf(stderr, "Player init failed\n");
        return -1;
    }
    
    // Initialize playlist
    app->playlist = playlist_init();
    if (!app->playlist) {
        fprintf(stderr, "Playlist init failed\n");
        return -1;
    }
    
    // Load music from directory
    if (music_dir) {
        playlist_scan_directory(app->playlist, music_dir);
    }
    
    // Initialize theme
    app->theme = theme_load("assets/themes/default");
    if (!app->theme) {
        // Use default theme
        app->theme = theme_default();
    }
    
    // Initialize lyrics
    app->lyrics = lyrics_init();
    
    // Initialize spectrum
    app->spectrum = spectrum_init(64); // 64 bands
    
    // Initialize EQ
    app->eq = eq_init();
    eq_set_enabled(app->eq, 1);
    
    // Input
    input_init();
    
    app->current_view = VIEW_MAIN;
    app->running = 1;
    app->selected_index = 0;
    app->list_scroll = 0;
    app->right_panel_mode = 1; // Start with spectrum
    app->show_help = 0;
    
    // Auto-play first track if playlist not empty
    if (app->playlist->count > 0) {
        player_play(app->player, app->playlist->items[0].path);
        lyrics_load_for_track(app->lyrics, app->playlist->items[0].path);
    }
    
    return 0;
}

void app_cleanup() {
    if (!app) return;
    
    input_cleanup();
    if (app->eq) eq_free(app->eq);
    if (app->spectrum) spectrum_free(app->spectrum);
    if (app->lyrics) lyrics_free(app->lyrics);
    if (app->theme) theme_free(app->theme);
    if (app->playlist) playlist_free(app->playlist);
    if (app->player) player_free(app->player);
    if (app->font_small) TTF_CloseFont(app->font_small);
    if (app->font_medium) TTF_CloseFont(app->font_medium);
    if (app->font_large) TTF_CloseFont(app->font_large);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    TTF_Quit();
    SDL_Quit();
    free(app);
    app = NULL;
}

// Handle key/joystick input
void handle_input(SDL_Event *event) {
    InputAction action = input_process(event);
    
    switch (action) {
        case ACTION_UP:
            if (app->selected_index > 0) {
                app->selected_index--;
                if (app->selected_index < app->list_scroll)
                    app->list_scroll = app->selected_index;
            }
            break;
        case ACTION_DOWN:
            if (app->selected_index < app->playlist->count - 1) {
                app->selected_index++;
            }
            break;
        case ACTION_SELECT:
        case ACTION_PLAY_PAUSE:
            if (app->playlist->count > 0) {
                if (player_is_playing(app->player) && 
                    strcmp(player_current_track(app->player), 
                           app->playlist->items[app->selected_index].path) == 0) {
                    player_toggle_pause(app->player);
                } else {
                    player_play(app->player, 
                        app->playlist->items[app->selected_index].path);
                    lyrics_load_for_track(app->lyrics,
                        app->playlist->items[app->selected_index].path);
                }
            }
            break;
        case ACTION_NEXT:
            player_next(app->player, app->playlist);
            break;
        case ACTION_PREV:
            player_prev(app->player, app->playlist);
            break;
        case ACTION_VOL_UP:
            player_set_volume(app->player, 
                player_get_volume(app->player) + 5);
            break;
        case ACTION_VOL_DOWN:
            player_set_volume(app->player, 
                player_get_volume(app->player) - 5);
            break;
        case ACTION_SEEK_FORWARD:
            player_seek(app->player, 5);
            break;
        case ACTION_SEEK_BACKWARD:
            player_seek(app->player, -5);
            break;
        case ACTION_TOGGLE_PANEL:
            app->right_panel_mode = (app->right_panel_mode + 1) % 3;
            break;
        case ACTION_TOGGLE_VIEW:
            app->current_view = (app->current_view + 1) % 3;
            break;
        case ACTION_QUIT:
            app->running = 0;
            break;
        default:
            break;
    }
}

// Main render function
void render() {
    SDL_Renderer *r = app->renderer;
    Theme *t = app->theme;
    
    // Clear with background color
    SDL_SetRenderDrawColor(r, t->bg_r, t->bg_g, t->bg_b, 255);
    SDL_RenderClear(r);
    
    // Draw top bar
    ui_draw_top_bar(r, app->player, app->playlist, app->font_medium,
        app->font_large, t);
    
    // Draw main area (left playlist + right spectrum/lyrics)
    ui_draw_main_area(r, app->playlist, app->selected_index,
        app->list_scroll, app->right_panel_mode, app->lyrics,
        app->spectrum, app->player, app->font_small, app->font_medium, t);
    
    // Draw bottom bar
    ui_draw_bottom_bar(r, app->right_panel_mode, app->player,
        app->font_small, t);
    
    // Draw help overlay
    if (app->show_help) {
        ui_draw_help(r, app->font_medium, t);
    }
    
    SDL_RenderPresent(r);
}

// Update state (called once per frame)
void update() {
    // Update player state
    player_update(app->player);
    
    // Update spectrum with audio data
    if (app->spectrum && player_is_playing(app->player)) {
        spectrum_update(app->spectrum, app->player);
    }
    
    // Update lyrics position
    if (app->lyrics && player_is_playing(app->player)) {
        lyrics_update(app->lyrics, player_get_position(app->player));
    }
    
    // Auto-advance to next track
    if (player_track_finished(app->player)) {
        player_next(app->player, app->playlist);
    }
    
    // Update selected index to match current track
    const char *current = player_current_track(app->player);
    if (current) {
        for (int i = 0; i < app->playlist->count; i++) {
            if (strcmp(app->playlist->items[i].path, current) == 0) {
                app->selected_index = i;
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    const char *music_dir = "/roms/BGM";
    
    // Parse command line args
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            music_dir = argv[++i];
        }
    }
    
    printf("RG52MINI Music Player starting...\n");
    printf("Music directory: %s\n", music_dir);
    
    if (app_init(music_dir) < 0) {
        fprintf(stderr, "App init failed\n");
        return 1;
    }
    
    printf("App initialized, playlist has %d tracks\n", app->playlist->count);
    
    // Main loop
    Uint32 last_time = SDL_GetTicks();
    const Uint32 frame_delay = 1000 / FPS;
    
    while (app->running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                app->running = 0;
            }
            handle_input(&event);
        }
        
        update();
        render();
        
        // Frame limiting
        Uint32 current_time = SDL_GetTicks();
        Uint32 elapsed = current_time - last_time;
        if (elapsed < frame_delay) {
            SDL_Delay(frame_delay - elapsed);
        }
        last_time = current_time;
    }
    
    printf("Shutting down...\n");
    app_cleanup();
    return 0;
}
