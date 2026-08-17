#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

TVPlayer* player_create(void) {
    TVPlayer *p = calloc(1, sizeof(TVPlayer));
    if (!p) return NULL;

    // Check if system mpv exists
    p->has_system_mpv = (access("/usr/bin/mpv", X_OK) == 0) ||
                        (access("/bin/mpv", X_OK) == 0) ||
                        (system("which mpv > /dev/null 2>&1") == 0);

    printf("Player: system mpv %s\n", p->has_system_mpv ? "found" : "NOT found");

    p->volume = 70;
    p->is_playing = false;
    p->current_url[0] = '\0';

    return p;
}

void player_destroy(TVPlayer *p) {
    if (!p) return;
    free(p);
}

bool player_load(TVPlayer *p, const char *url) {
    if (!p || !url) return false;

    strncpy(p->current_url, url, sizeof(p->current_url) - 1);
    p->is_playing = true;

    // Clear previous mpv log
    system("rm -f /roms/ports/rg52mini-tv/mpv.log");

    // Build mpv command - let mpv auto-select video output (sdl2 vo not available at runtime)
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "mpv --fs --ao=alsa --volume=%d --cache=yes --cache-secs=30 "
        "--network-timeout=60 "
        "--input-gamepad=yes "
        "--input-conf=/roms/ports/rg52mini-tv/mpv-input.conf "
        "--msg-level=all=v --terminal=yes \"%s\" "
        ">> /roms/ports/rg52mini-tv/mpv.log 2>&1",
        p->volume, url);

    printf("Player: launching mpv: %s\n", cmd);

    // Execute mpv (blocks until mpv exits)
    int ret = system(cmd);

    printf("Player: mpv exited with code %d\n", WEXITSTATUS(ret));

    p->is_playing = false;
    return (ret == 0);
}

void player_stop(TVPlayer *p) {
    if (!p) return;
    // With system() call, mpv runs in foreground and stops on user quit
    p->is_playing = false;
}

void player_set_volume(TVPlayer *p, int volume) {
    if (!p) return;
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    p->volume = volume;
}

int player_get_volume(TVPlayer *p) {
    if (!p) return 70;
    return p->volume;
}

bool player_is_playing(TVPlayer *p) {
    if (!p) return false;
    return p->is_playing;
}

void player_poll_events(TVPlayer *p) {
    // No event polling needed with system() approach
    (void)p;
}
