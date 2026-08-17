#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#define LIBMPV_PATH "libmpv.so.2"

TVPlayer* player_create(void) {
    TVPlayer *p = calloc(1, sizeof(TVPlayer));
    if (!p) return NULL;

    // Dynamically load libmpv
    p->lib_handle = dlopen(LIBMPV_PATH, RTLD_NOW | RTLD_GLOBAL);
    if (!p->lib_handle) {
        fprintf(stderr, "Failed to load %s: %s\n", LIBMPV_PATH, dlerror());
        free(p);
        return NULL;
    }

    // Load function pointers
    p->mpv_create = dlsym(p->lib_handle, "mpv_create");
    p->mpv_initialize = dlsym(p->lib_handle, "mpv_initialize");
    p->mpv_command = dlsym(p->lib_handle, "mpv_command");
    p->mpv_set_option_string = dlsym(p->lib_handle, "mpv_set_option_string");
    p->mpv_set_property_string = dlsym(p->lib_handle, "mpv_set_property_string");
    p->mpv_get_property_string = dlsym(p->lib_handle, "mpv_get_property_string");
    p->mpv_free = dlsym(p->lib_handle, "mpv_free");
    p->mpv_wait_event = dlsym(p->lib_handle, "mpv_wait_event");
    p->mpv_terminate_destroy = dlsym(p->lib_handle, "mpv_terminate_destroy");

    if (!p->mpv_create || !p->mpv_initialize || !p->mpv_command ||
        !p->mpv_set_option_string || !p->mpv_set_property_string ||
        !p->mpv_get_property_string || !p->mpv_free || !p->mpv_wait_event ||
        !p->mpv_terminate_destroy) {
        fprintf(stderr, "Failed to load mpv functions: %s\n", dlerror());
        dlclose(p->lib_handle);
        free(p);
        return NULL;
    }

    // Create mpv instance
    p->mpv = p->mpv_create();
    if (!p->mpv) {
        fprintf(stderr, "Failed to create mpv instance\n");
        dlclose(p->lib_handle);
        free(p);
        return NULL;
    }

    // Configure mpv - try KMS direct rendering for EmuELEC
    p->mpv_set_option_string(p->mpv, "vo", "kms");
    p->mpv_set_option_string(p->mpv, "hwdec", "no");
    p->mpv_set_option_string(p->mpv, "ao", "alsa");
    p->mpv_set_option_string(p->mpv, "cache", "yes");
    p->mpv_set_option_string(p->mpv, "cache-secs", "30");
    p->mpv_set_option_string(p->mpv, "network-timeout", "60");
    p->mpv_set_option_string(p->mpv, "terminal", "no");
    p->mpv_set_option_string(p->mpv, "msg-level", "all=v");
    p->mpv_set_option_string(p->mpv, "video-sync", "audio");
    p->mpv_set_option_string(p->mpv, "idle", "yes");
    p->mpv_set_option_string(p->mpv, "prefer-ipv4", "yes");
    p->mpv_set_option_string(p->mpv, "hls-bitrate", "max");

    if (p->mpv_initialize(p->mpv) < 0) {
        fprintf(stderr, "Failed to initialize mpv\n");
        p->mpv_terminate_destroy(p->mpv);
        dlclose(p->lib_handle);
        free(p);
        return NULL;
    }

    p->volume = 70;
    p->is_playing = false;
    p->current_url[0] = '\0';

    return p;
}

void player_destroy(TVPlayer *p) {
    if (!p) return;
    if (p->mpv) {
        p->mpv_terminate_destroy(p->mpv);
    }
    if (p->lib_handle) {
        dlclose(p->lib_handle);
    }
    free(p);
}

bool player_load(TVPlayer *p, const char *url) {
    if (!p || !p->mpv || !url) return false;

    const char *args[] = {"loadfile", url, NULL};
    int ret = p->mpv_command(p->mpv, args);
    if (ret < 0) {
        fprintf(stderr, "Failed to load %s: error %d\n", url, ret);
        return false;
    }

    strncpy(p->current_url, url, sizeof(p->current_url) - 1);
    p->is_playing = true;

    // Set volume
    char vol_str[16];
    snprintf(vol_str, sizeof(vol_str), "%d", p->volume);
    p->mpv_set_property_string(p->mpv, "volume", vol_str);

    return true;
}

void player_stop(TVPlayer *p) {
    if (!p || !p->mpv) return;
    const char *args[] = {"stop", NULL};
    p->mpv_command(p->mpv, args);
    p->is_playing = false;
}

void player_set_volume(TVPlayer *p, int volume) {
    if (!p || !p->mpv) return;
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    p->volume = volume;
    char vol_str[16];
    snprintf(vol_str, sizeof(vol_str), "%d", volume);
    p->mpv_set_property_string(p->mpv, "volume", vol_str);
}

int player_get_volume(TVPlayer *p) {
    if (!p) return 0;
    return p->volume;
}

bool player_is_playing(TVPlayer *p) {
    if (!p) return false;
    return p->is_playing;
}

void player_poll_events(TVPlayer *p) {
    if (!p || !p->mpv) return;

    while (1) {
        mpv_event *event = p->mpv_wait_event(p->mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) break;

        switch (event->event_id) {
            case MPV_EVENT_SHUTDOWN:
                printf("MPV: SHUTDOWN\n");
                p->is_playing = false;
                break;
            case MPV_EVENT_END_FILE:
                printf("MPV: END_FILE\n");
                p->is_playing = false;
                break;
            case MPV_EVENT_IDLE:
                printf("MPV: IDLE\n");
                p->is_playing = false;
                break;
            case MPV_EVENT_FILE_LOADED:
                printf("MPV: FILE_LOADED\n");
                p->is_playing = true;
                break;
            default:
                printf("MPV: event %d\n", event->event_id);
                break;
        }
    }
}
