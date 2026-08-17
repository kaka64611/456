#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

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

bool player_test_url(const char *url, int *speed_kbps, long *size_bytes, int timeout_sec) {
    if (!url || !speed_kbps || !size_bytes) return false;
    *speed_kbps = 0;
    *size_bytes = 0;

    // Check if curl exists
    if (system("which curl > /dev/null 2>&1") != 0) {
        printf("Player test: curl not found, skipping test\n");
        return true;
    }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "curl -s -o /dev/null -w '%%{speed_download} %%{size_download}' "
        "--max-time %d --connect-timeout 5 -r 0-1048576 \"%s\" 2>/dev/null",
        timeout_sec, url);

    printf("Player test: %s\n", cmd);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        printf("Player test: popen failed, skipping\n");
        return true;
    }

    char result[256] = "";
    if (fgets(result, sizeof(result), fp)) {
        double speed = 0;
        long size = 0;
        printf("Player test result: '%s'\n", result);
        if (sscanf(result, "%lf %ld", &speed, &size) == 2 && size > 0) {
            *speed_kbps = (int)(speed / 1024.0);
            *size_bytes = size;
            pclose(fp);
            return true;
        }
    }
    pclose(fp);
    printf("Player test: failed\n");
    return false;
}

bool player_load(TVPlayer *p, const char *url) {
    if (!p || !url) return false;

    strncpy(p->current_url, url, sizeof(p->current_url) - 1);
    p->is_playing = true;

    // Clear previous mpv log
    system("rm -f /roms/ports/rg52mini-tv/mpv.log");

    // Build mpv command - use safe compatible settings, no cache-pause
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "mpv --fs --ao=alsa --volume=%d --cache=yes --cache-secs=5 "
        "--network-timeout=20 --keep-open=always "
        "--force-window=yes --vd-lavc-threads=4 "
        "--input-gamepad=yes "
        "--input-conf=/roms/ports/rg52mini-tv/mpv-input.conf "
        "--msg-level=all=v --terminal=yes \"%s\" "
        ">> /roms/ports/rg52mini-tv/mpv.log 2>&1",
        p->volume, url);

    printf("Player: launching mpv: %s\n", cmd);

    // Execute mpv (blocks until mpv exits)
    time_t start = time(NULL);
    int ret = system(cmd);
    time_t end = time(NULL);
    int elapsed = (int)(end - start);

    int exit_code = WEXITSTATUS(ret);
    printf("Player: mpv exited with code %d, ran for %d seconds\n", exit_code, elapsed);

    p->is_playing = false;

    // Consider it success if mpv ran more than 3 seconds (user watched something)
    // or if exit code is 0 and ran more than 2 seconds
    if (elapsed >= 3) {
        return true;
    }
    return false;
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
