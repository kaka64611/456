#include "player.h"
#include <mpv/client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PlayerState {
    mpv_handle *mpv;
    int volume;
    int is_playing;
    char error[256];
};

PlayerState *player_init(void) {
    PlayerState *p = (PlayerState *)calloc(1, sizeof(PlayerState));
    if (!p) return NULL;

    p->mpv = mpv_create();
    if (!p->mpv) {
        strcpy(p->error, "Failed to create mpv handle");
        return p;
    }

    // Set mpv options for embedded/headless playback
    mpv_set_option_string(p->mpv, "vo", "drm");
    mpv_set_option_string(p->mpv, "hwdec", "auto");
    mpv_set_option_string(p->mpv, "ao", "alsa");
    mpv_set_option_string(p->mpv, "volume", "70");
    mpv_set_option_string(p->mpv, "terminal", "no");
    mpv_set_option_string(p->mpv, "msg-level", "all=no");
    mpv_set_option_string(p->mpv, "cache", "yes");
    mpv_set_option_string(p->mpv, "cache-secs", "10");
    mpv_set_option_string(p->mpv, "network-timeout", "10");
    mpv_set_option_string(p->mpv, "reconnect-on-timeout", "yes");
    mpv_set_option_string(p->mpv, "reconnect-on-error", "yes");

    if (mpv_initialize(p->mpv) < 0) {
        strcpy(p->error, "Failed to initialize mpv");
        return p;
    }

    p->volume = 70;
    p->is_playing = 0;
    printf("mpv initialized\n");
    return p;
}

void player_destroy(PlayerState *p) {
    if (!p) return;
    if (p->mpv) {
        mpv_terminate_destroy(p->mpv);
    }
    free(p);
}

int player_play(PlayerState *p, const char *url) {
    if (!p || !p->mpv || !url) return -1;

    printf("Playing: %s\n", url);

    const char *cmd[] = {"loadfile", url, NULL};
    int ret = mpv_command(p->mpv, cmd);
    if (ret < 0) {
        snprintf(p->error, sizeof(p->error), "Failed to load: %s", mpv_error_string(ret));
        printf("mpv error: %s\n", p->error);
        p->is_playing = 0;
        return -1;
    }

    p->is_playing = 1;
    return 0;
}

void player_stop(PlayerState *p) {
    if (!p || !p->mpv) return;
    const char *cmd[] = {"stop", NULL};
    mpv_command(p->mpv, cmd);
    p->is_playing = 0;
}

void player_set_volume(PlayerState *p, int volume) {
    if (!p || !p->mpv) return;
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    p->volume = volume;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", volume);
    mpv_set_property_string(p->mpv, "volume", buf);
}

int player_get_volume(PlayerState *p) {
    return p ? p->volume : 0;
}

int player_is_playing(PlayerState *p) {
    return p ? p->is_playing : 0;
}

const char *player_get_error(PlayerState *p) {
    return p ? p->error : NULL;
}

void player_poll_events(PlayerState *p) {
    if (!p || !p->mpv) return;
    while (1) {
        mpv_event *event = mpv_wait_event(p->mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) break;
        if (event->event_id == MPV_EVENT_END_FILE) {
            p->is_playing = 0;
            printf("Playback ended\n");
        }
        if (event->event_id == MPV_EVENT_FILE_LOADED) {
            p->is_playing = 1;
            printf("File loaded\n");
        }
    }
}
