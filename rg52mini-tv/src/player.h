#ifndef PLAYER_H
#define PLAYER_H

typedef struct PlayerState PlayerState;

PlayerState *player_init(void);
void player_destroy(PlayerState *p);
int player_play(PlayerState *p, const char *url);
void player_stop(PlayerState *p);
void player_set_volume(PlayerState *p, int volume);
int player_get_volume(PlayerState *p);
int player_is_playing(PlayerState *p);
const char *player_get_error(PlayerState *p);
void player_poll_events(PlayerState *p);

#endif
