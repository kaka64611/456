#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

typedef struct {
    bool is_playing;
    char current_url[1024];
    int volume;
    bool has_system_mpv;
} TVPlayer;

TVPlayer* player_create(void);
void player_destroy(TVPlayer *p);
bool player_load(TVPlayer *p, const char *url);
bool player_test_url(const char *url, int *speed_kbps, long *size_bytes, int timeout_sec);
void player_stop(TVPlayer *p);
void player_set_volume(TVPlayer *p, int volume);
int player_get_volume(TVPlayer *p);
bool player_is_playing(TVPlayer *p);
void player_poll_events(TVPlayer *p);

#endif
