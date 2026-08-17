#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "input.h"
#include "playlist.h"
#include "player.h"
#include "ui.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// Forward declarations
static void render(void);

typedef enum {
    VIEW_LIST,
    VIEW_LOADING,
    VIEW_PLAYING,
    VIEW_ERROR,
    VIEW_SEARCH,
    VIEW_SOURCE_SELECT
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
    char loading_msg[512];
    // Search
    char search_query[64];
    int search_kb_x;
    int search_kb_y;
    int search_active;
    int source_select_index;
    char font_path[256];
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
    strncpy(app->font_path, "assets/fonts/NotoSansCJKsc-Regular.otf", sizeof(app->font_path)-1);
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

    // Load source preferences
    playlist_load_preferences(app->all_channels, "/roms/ports/rg52mini-tv/source_prefs.txt");
    printf("Source preferences loaded\n");

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
            if (app->filtered->count < MAX_CHANNELS) {
                app->filtered->items[app->filtered->count] = app->all_channels->items[i];
                app->filtered->count++;
            }
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
            if (app->filtered->count < MAX_CHANNELS) {
                app->filtered->items[app->filtered->count] = app->all_channels->items[i];
                app->filtered->count++;
            }
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

// Suspend SDL so mpv can use SDL video output
static void app_suspend_sdl(void) {
    if (!app) return;
    ui_cleanup();
    TTF_Quit();
    if (app->renderer) { SDL_DestroyRenderer(app->renderer); app->renderer = NULL; }
    if (app->window) { SDL_DestroyWindow(app->window); app->window = NULL; }
    SDL_Quit();
    // Wait for video device to be fully released
    usleep(200000);  // 0.2 seconds
}

// Resume SDL after mpv exits
static int app_resume_sdl(void) {
    if (!app) return -1;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "SDL re-init failed: %s\n", SDL_GetError());
        return -1;
    }
    app->window = SDL_CreateWindow("RG52MINI TV",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN);
    if (!app->window) {
        fprintf(stderr, "Window re-create failed: %s\n", SDL_GetError());
        return -1;
    }
    app->renderer = SDL_CreateRenderer(app->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app->renderer) {
        fprintf(stderr, "Renderer re-create failed: %s\n", SDL_GetError());
        return -1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF re-init failed: %s\n", TTF_GetError());
        return -1;
    }
    ui_init(app->renderer, app->font_path);
    app->theme = ui_get_default_theme();
    input_init();
    return 0;
}

static void play_selected(void) {
    if (!app || app->channels->count == 0) return;

    int start_index = app->selected;
    int max_channels = 3;  // Try up to 3 channels
    bool played_ok = false;
    int last_failed_index = -1;
    int successful_url_index = -1;
    int successful_channel_index = -1;

    for (int ch_attempt = 0; ch_attempt < max_channels; ch_attempt++) {
        int idx = (start_index + ch_attempt) % app->channels->count;
        Channel *ch = &app->channels->items[idx];
        app->selected = idx;
        app->scroll = idx;

        // Try each URL for this channel
        for (int url_idx = 0; url_idx < ch->url_count; url_idx++) {
            // Determine which URL to try: preferred first, then others
            int try_url;
            if (url_idx == 0 && ch->preferred_url >= 0) {
                try_url = ch->preferred_url;
            } else if (ch->preferred_url >= 0) {
                // Skip preferred URL on subsequent iterations
                try_url = (url_idx >= ch->preferred_url) ? url_idx : url_idx - 1;
                if (try_url == ch->preferred_url) try_url++;
                if (try_url >= ch->url_count) break;
            } else {
                try_url = url_idx;
            }

            // Show testing screen
            app->view = VIEW_LOADING;
            app->error_msg[0] = '\0';
            snprintf(app->loading_msg, sizeof(app->loading_msg),
                     "%s\n\n源 %d/%d\n正在连接...",
                     ch->name, try_url + 1, ch->url_count);
            render();
            SDL_RenderPresent(app->renderer);

            // Test URL connectivity and measure speed
            int speed_kbps = 0;
            long size_bytes = 0;
            bool test_ok = player_test_url(ch->urls[try_url], &speed_kbps, &size_bytes, 8);

            // Show buffer result
            if (test_ok) {
                snprintf(app->loading_msg, sizeof(app->loading_msg),
                         "%s\n\n源 %d/%d\n连接成功!\n网速: %d KB/s\n已缓冲: %.1f KB\n\n正在启动播放器...",
                         ch->name, try_url + 1, ch->url_count,
                         speed_kbps, size_bytes / 1024.0);
            } else {
                snprintf(app->loading_msg, sizeof(app->loading_msg),
                         "%s\n\n源 %d/%d\n连接失败，正在尝试下一个源...",
                         ch->name, try_url + 1, ch->url_count);
            }
            render();
            SDL_RenderPresent(app->renderer);
            SDL_Delay(1500);

            if (!test_ok) {
                last_failed_index = idx;
                continue;
            }

            // Suspend SDL so mpv can use video output
            app_suspend_sdl();

            // Try to play
            bool ok = player_load(app->player, ch->urls[try_url]);

            // Resume SDL after mpv exits
            app_resume_sdl();

            if (ok) {
                played_ok = true;
                successful_url_index = try_url;
                successful_channel_index = idx;
                break;
            }
            last_failed_index = idx;
        }

        if (played_ok) break;
    }

    app->view = VIEW_LIST;

    if (played_ok && successful_channel_index >= 0) {
        // Save preferred URL for this channel
        Channel *ch = &app->channels->items[successful_channel_index];
        ch->preferred_url = successful_url_index;
        playlist_save_preferences(app->all_channels, "/roms/ports/rg52mini-tv/source_prefs.txt");
        printf("Saved preferred source %d for channel %s\n", successful_url_index, ch->name);
    } else if (last_failed_index >= 0) {
        Channel *ch = &app->channels->items[last_failed_index];
        app->view = VIEW_ERROR;
        snprintf(app->error_msg, sizeof(app->error_msg),
                 "播放失败\n\n频道: %s\n共 %d 个源均无法播放\n\n按任意键返回列表\n按Y键手动选择源",
                 ch->name, ch->url_count);
        render();
        SDL_RenderPresent(app->renderer);
    }
}

static void handle_action(InputAction action) {
    // Source select mode handling
    if (app->view == VIEW_SOURCE_SELECT) {
        Channel *ch = &app->channels->items[app->selected];
        switch (action) {
            case ACTION_UP:
                if (app->source_select_index > 0) app->source_select_index--;
                break;
            case ACTION_DOWN:
                if (app->source_select_index < ch->url_count - 1) app->source_select_index++;
                break;
            case ACTION_SELECT:
            case ACTION_MENU: {
                // Play selected source
                int url_idx = app->source_select_index;
                app->view = VIEW_LOADING;
                snprintf(app->loading_msg, sizeof(app->loading_msg),
                         "%s\n\n手动选择源 %d/%d\n正在连接...", ch->name, url_idx + 1, ch->url_count);
                render();
                SDL_RenderPresent(app->renderer);

                // Test URL
                int speed_kbps = 0;
                long size_bytes = 0;
                bool test_ok = player_test_url(ch->urls[url_idx], &speed_kbps, &size_bytes, 8);

                if (test_ok) {
                    snprintf(app->loading_msg, sizeof(app->loading_msg),
                             "%s\n\n手动选择源 %d/%d\n连接成功!\n网速: %d KB/s\n已缓冲: %.1f KB\n\n正在启动播放器...",
                             ch->name, url_idx + 1, ch->url_count,
                             speed_kbps, size_bytes / 1024.0);
                    render();
                    SDL_RenderPresent(app->renderer);
                    SDL_Delay(1500);
                    app_suspend_sdl();
                    bool ok = player_load(app->player, ch->urls[url_idx]);
                    app_resume_sdl();
                } else {
                    snprintf(app->loading_msg, sizeof(app->loading_msg),
                             "%s\n\n源 %d/%d\n连接失败!",
                             ch->name, url_idx + 1, ch->url_count);
                    render();
                    SDL_RenderPresent(app->renderer);
                    SDL_Delay(1500);
                }
                if (test_ok) {
                    ch->preferred_url = url_idx;
                    playlist_save_preferences(app->all_channels, "/roms/ports/rg52mini-tv/source_prefs.txt");
                }
                app->view = VIEW_LIST;
                break;
            }
            case ACTION_BACK:
                app->view = VIEW_LIST;
                break;
            default:
                break;
        }
        return;
    }

    // Error screen: any key returns to list
    if (app->view == VIEW_ERROR) {
        if (action == ACTION_INFO) {
            // Y key: go to source select
            app->view = VIEW_SOURCE_SELECT;
            app->source_select_index = 0;
        } else {
            app->view = VIEW_LIST;
        }
        return;
    }

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
                app->scroll -= visible;
                if (app->scroll < 0) {
                    // Cycle to last page
                    int total_pages = (app->channels->count + visible - 1) / visible;
                    app->scroll = (total_pages - 1) * visible;
                    if (app->scroll >= app->channels->count)
                        app->scroll = app->channels->count - visible;
                    if (app->scroll < 0) app->scroll = 0;
                }
                app->selected = app->scroll;
                if (app->selected >= app->channels->count)
                    app->selected = app->channels->count - 1;
                printf("PAGE_UP: scroll=%d selected=%d\n", app->scroll, app->selected);
            }
            break;
        case ACTION_PAGE_DOWN:
            if (app->view == VIEW_LIST) {
                int visible = 11;
                app->scroll += visible;
                if (app->scroll >= app->channels->count) {
                    // Cycle to first page
                    app->scroll = 0;
                }
                app->selected = app->scroll;
                if (app->selected >= app->channels->count)
                    app->selected = app->channels->count - 1;
                printf("PAGE_DOWN: scroll=%d selected=%d\n", app->scroll, app->selected);
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
        case ACTION_INFO:
            if (app->view == VIEW_LIST) {
                Channel *ch = &app->channels->items[app->selected];
                if (ch->url_count > 1) {
                    app->view = VIEW_SOURCE_SELECT;
                    app->source_select_index = (ch->preferred_url >= 0) ? ch->preferred_url : 0;
                }
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

    // player_load blocks, no need to check playback ended
}

static void render(void) {
    SDL_RenderClear(app->renderer);

    switch (app->view) {
        case VIEW_LIST:
            ui_draw_channel_list(app->renderer, app->channels,
                app->selected, app->scroll, app->theme);
            break;
        case VIEW_LOADING:
            ui_draw_loading(app->renderer, app->loading_msg[0] ? app->loading_msg : "请稍候...", app->theme);
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
        case VIEW_SOURCE_SELECT:
            ui_draw_source_select(app->renderer,
                &app->channels->items[app->selected],
                app->source_select_index, app->theme);
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
