#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "input.h"
#include "playlist.h"
#include "player.h"
#include "ui.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

typedef enum {
    VIEW_LIST,
    VIEW_LOADING,
    VIEW_PLAYING,
    VIEW_ERROR,
    VIEW_SEARCH
} ViewMode;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    ChannelList *channels;       // Current displayed list
    ChannelList *all_channels;   // Original full list
    ChannelList *filtered;       // Search filtered list
    TVPlayer *player;
    Theme *theme;
    ViewMode view;
    int selected;
    int scroll;
    int volume;
    int show_overlay;
    int overlay_timer;
    int running;
    char error_msg[512];
    // Search
    char search_query[64];
    int search_kb_x;
    int search_kb_y;
    int search_active;
} AppState;

static AppState *app = NULL;

static int app_init(const char *tv_dir) {
    app = (AppState *)calloc(1, sizeof(AppState));
    if (!app) return -1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return -1;
    }

    app->window = SDL_CreateWindow("RG52MINI TV",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN);
    if (!app->window) {
        fprintf(stderr, "Window create failed: %s\n", SDL_GetError());
        return -1;
    }

    app->renderer = SDL_CreateRenderer(app->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app->renderer) {
        fprintf(stderr, "Renderer create failed: %s\n", SDL_GetError());
        return -1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF init failed: %s\n", TTF_GetError());
        return -1;
    }

    ui_init(app->renderer, "assets/fonts/NotoSansCJKsc-Regular.otf");
    app->theme = ui_get_default_theme();

    input_init();

    // Load channels
    app->all_channels = playlist_create();
    app->filtered = playlist_create();
    int count = playlist_load_directory(app->all_channels, tv_dir);
    if (count <= 0) {
        // Try alternative paths (case-insensitive fallback)
        count = playlist_load_directory(app->all_channels, "/roms/tv");
    }
    if (count <= 0) {
        count = playlist_load_directory(app->all_channels, "/roms/TV");
    }
    app->channels = app->all_channels;
    printf("Loaded %d channels total\n", app->channels->count);

    // Init player
    app->player = player_create();
    app->volume = 70;
    app->view = VIEW_LIST;
    app->selected = 0;
    app->scroll = 0;
    app->running = 1;
    app->show_overlay = 0;
    app->overlay_timer = 0;

    return 0;
}

static void app_cleanup(void) {
    if (!app) return;
    if (app->player) player_destroy(app->player);
    if (app->channels) playlist_destroy(app->channels);
    ui_cleanup();
    input_cleanup();
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    TTF_Quit();
    SDL_Quit();
    free(app);
    app = NULL;
}

static void search_filter(void) {
    if (!app->filtered) return;
    playlist_clear(app->filtered);

    if (strlen(app->search_query) == 0) {
        // Empty query: show all
        for (int i = 0; i < app->all_channels->count; i++) {
            playlist_add_channel(app->filtered,
                app->all_channels->items[i].name,
                app->all_channels->items[i].url,
                app->all_channels->items[i].logo,
                app->all_channels->items[i].group);
        }
        return;
    }

    // Case-insensitive substring match
    for (int i = 0; i < app->all_channels->count; i++) {
        char name_lower[MAX_NAME_LEN];
        char query_lower[64];
        strncpy(name_lower, app->all_channels->items[i].name, MAX_NAME_LEN-1);
        name_lower[MAX_NAME_LEN-1] = '\0';
        strncpy(query_lower, app->search_query, 63);
        query_lower[63] = '\0';

        for (int j = 0; name_lower[j]; j++) name_lower[j] = tolower(name_lower[j]);
        for (int j = 0; query_lower[j]; j++) query_lower[j] = tolower(query_lower[j]);

        if (strstr(name_lower, query_lower) != NULL) {
            playlist_add_channel(app->filtered,
                app->all_channels->items[i].name,
                app->all_channels->items[i].url,
                app->all_channels->items[i].logo,
                app->all_channels->items[i].group);
        }
    }
}

static void search_enter(void) {
    printf("SEARCH: enter search mode\n");
    app->search_query[0] = '\0';
    app->search_kb_x = 0;
    app->search_kb_y = 0;
    app->search_active = 1;
    app->view = VIEW_SEARCH;
    search_filter();
    printf("SEARCH: filtered %d channels\n", app->filtered->count);
}

static void search_exit(int apply) {
    app->search_active = 0;
    if (apply && app->filtered->count > 0) {
        // Use filtered list
        app->channels = app->filtered;
    } else {
        // Restore full list
        app->channels = app->all_channels;
    }
    app->selected = 0;
    app->scroll = 0;
    app->view = VIEW_LIST;
}

static void search_input_char(char c) {
    int len = strlen(app->search_query);
    if (len < 62) {
        app->search_query[len] = c;
        app->search_query[len+1] = '\0';
        search_filter();
    }
}

static void search_backspace(void) {
    int len = strlen(app->search_query);
    if (len > 0) {
        app->search_query[len-1] = '\0';
        search_filter();
    }
}

static void play_selected(void) {
    if (!app || app->channels->count == 0) return;
    Channel *ch = &app->channels->items[app->selected];
    app->view = VIEW_LOADING;
    app->error_msg[0] = '\0';

    if (player_load(app->player, ch->url)) {
        app->view = VIEW_PLAYING;
        app->show_overlay = 1;
        app->overlay_timer = 180; // 3 seconds
    } else {
        strncpy(app->error_msg, "Failed to load channel", sizeof(app->error_msg)-1);
        app->view = VIEW_ERROR;
    }
}

static void handle_action(InputAction action) {
    // Search mode handling (priority)
    if (app->view == VIEW_SEARCH) {
        static const int kb_row_lens[] = {13, 13, 10, 4};
        switch (action) {
            case ACTION_UP:
                if (app->search_kb_y > 0) {
                    app->search_kb_y--;
                    if (app->search_kb_x >= kb_row_lens[app->search_kb_y])
                        app->search_kb_x = kb_row_lens[app->search_kb_y] - 1;
                }
                return;
            case ACTION_DOWN:
                if (app->search_kb_y < 3) {
                    app->search_kb_y++;
                    if (app->search_kb_x >= kb_row_lens[app->search_kb_y])
                        app->search_kb_x = kb_row_lens[app->search_kb_y] - 1;
                }
                return;
            case ACTION_PAGE_UP: // Left
                if (app->search_kb_x > 0) app->search_kb_x--;
                return;
            case ACTION_PAGE_DOWN: // Right
                if (app->search_kb_x < kb_row_lens[app->search_kb_y] - 1) app->search_kb_x++;
                return;
            case ACTION_SELECT: { // A = input
                if (app->search_kb_y < 2) {
                    // Letters
                    const char *row = (app->search_kb_y == 0) ? "ABCDEFGHIJKLM" : "NOPQRSTUVWXYZ";
                    search_input_char(row[app->search_kb_x]);
                } else if (app->search_kb_y == 2) {
                    // Numbers
                    const char *row = "0123456789";
                    search_input_char(row[app->search_kb_x]);
                } else {
                    // Special keys row 3: 0=backspace, 1=space, 2=clear, 3=confirm
                    if (app->search_kb_x == 0) search_backspace();
                    else if (app->search_kb_x == 1) search_input_char(' ');
                    else if (app->search_kb_x == 2) { app->search_query[0] = '\0'; search_filter(); }
                    else if (app->search_kb_x == 3) { search_exit(1); }
                }
                return;
            }
            case ACTION_BACK: // B = backspace
                search_backspace();
                return;
            case ACTION_MENU: // X = exit search without apply
                search_exit(0);
                return;
            case ACTION_TOGGLE_PANEL: // Start = confirm search
                search_exit(1);
                return;
            default:
                return;
        }
    }

    switch (action) {
        case ACTION_UP:
            if (app->view == VIEW_LIST) {
                if (app->selected > 0) {
                    app->selected--;
                    if (app->selected < app->scroll) app->scroll = app->selected;
                }
            }
            break;
        case ACTION_DOWN:
            if (app->view == VIEW_LIST) {
                if (app->selected < app->channels->count - 1) {
                    app->selected++;
                    int visible = 11;
                    if (app->selected >= app->scroll + visible)
                        app->scroll = app->selected - visible + 1;
                }
            }
            break;
        case ACTION_PAGE_UP:
            if (app->view == VIEW_LIST) {
                int visible = 11;
                app->selected -= visible;
                if (app->selected < 0) app->selected = 0;
                app->scroll -= visible;
                if (app->scroll < 0) app->scroll = 0;
            }
            break;
        case ACTION_PAGE_DOWN:
            if (app->view == VIEW_LIST) {
                int visible = 11;
                app->selected += visible;
                if (app->selected >= app->channels->count)
                    app->selected = app->channels->count - 1;
                app->scroll += visible;
                if (app->scroll + visible > app->channels->count)
                    app->scroll = app->channels->count - visible;
                if (app->scroll < 0) app->scroll = 0;
            }
            break;
        case ACTION_SELECT:
            if (app->view == VIEW_LIST) {
                play_selected();
            }
            break;
        case ACTION_BACK:
            if (app->view == VIEW_PLAYING || app->view == VIEW_ERROR || app->view == VIEW_LOADING) {
                player_stop(app->player);
                app->view = VIEW_LIST;
            }
            break;
        case ACTION_PREV:
            if (app->view == VIEW_PLAYING) {
                if (app->selected > 0) {
                    app->selected--;
                    play_selected();
                }
            } else if (app->view == VIEW_LIST) {
                if (app->selected > 0) {
                    app->selected--;
                    if (app->selected < app->scroll) app->scroll = app->selected;
                }
            }
            break;
        case ACTION_NEXT:
            if (app->view == VIEW_PLAYING) {
                if (app->selected < app->channels->count - 1) {
                    app->selected++;
                    play_selected();
                }
            } else if (app->view == VIEW_LIST) {
                if (app->selected < app->channels->count - 1) {
                    app->selected++;
                    int visible = 11;
                    if (app->selected >= app->scroll + visible)
                        app->scroll = app->selected - visible + 1;
                }
            }
            break;
        case ACTION_VOL_DOWN:
            app->volume -= 10;
            if (app->volume < 0) app->volume = 0;
            player_set_volume(app->player, app->volume);
            app->show_overlay = 1;
            app->overlay_timer = 120;
            break;
        case ACTION_VOL_UP:
            app->volume += 10;
            if (app->volume > 100) app->volume = 100;
            player_set_volume(app->player, app->volume);
            app->show_overlay = 1;
            app->overlay_timer = 120;
            break;
        case ACTION_TOGGLE_PANEL:
            if (app->view == VIEW_PLAYING) {
                app->show_overlay = !app->show_overlay;
                app->overlay_timer = app->show_overlay ? 180 : 0;
            }
            break;
        case ACTION_MENU:
            if (app->view == VIEW_LIST) {
                search_enter();
            }
            break;
        case ACTION_QUIT:
            app->running = 0;
            break;
        default:
            break;
    }
}

static void update(void) {
    player_poll_events(app->player);

    if (app->overlay_timer > 0) {
        app->overlay_timer--;
        if (app->overlay_timer == 0) app->show_overlay = 0;
    }

    // Check if playback ended
    if (app->view == VIEW_PLAYING && !player_is_playing(app->player)) {
        strncpy(app->error_msg, "Playback ended", sizeof(app->error_msg)-1);
        app->view = VIEW_ERROR;
    }
}

static void render(void) {
    SDL_RenderClear(app->renderer);

    switch (app->view) {
        case VIEW_LIST:
            ui_draw_channel_list(app->renderer, app->channels,
                app->selected, app->scroll, app->theme);
            break;
        case VIEW_LOADING:
            ui_draw_loading(app->renderer, "正在加载频道...", app->theme);
            break;
        case VIEW_PLAYING:
            // Video is rendered by mpv directly to screen
            // We just draw overlay
            if (app->show_overlay) {
                Channel *ch = &app->channels->items[app->selected];
                ui_draw_playing_overlay(app->renderer, ch, app->volume, app->theme);
            }
            break;
        case VIEW_ERROR:
            ui_draw_error(app->renderer, app->error_msg, app->theme);
            break;
        case VIEW_SEARCH:
            ui_draw_search(app->renderer, app->search_query,
                app->search_kb_x, app->search_kb_y,
                app->filtered->count, app->theme);
            break;
    }

    SDL_RenderPresent(app->renderer);
}

int main(int argc, char *argv[]) {
    const char *tv_dir = "/roms/tv";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            tv_dir = argv[++i];
        }
    }

    printf("RG52MINI TV Player starting...\n");
    printf("TV directory: %s\n", tv_dir);

    if (app_init(tv_dir) < 0) {
        fprintf(stderr, "App init failed\n");
        return 1;
    }

    printf("App initialized, %d channels\n", app->channels->count);

    while (app->running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            InputAction action = input_process(&event);
            if (action != ACTION_NONE) {
                handle_action(action);
            }
        }

        update();
        render();
        SDL_Delay(16); // ~60fps
    }

    printf("Shutting down...\n");
    app_cleanup();
    return 0;
}
