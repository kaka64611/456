#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL_mixer.h>

typedef struct PlayerState PlayerState;

// Initialize audio player
PlayerState *player_init(void);

// Free player
void player_free(PlayerState *p);

// Play a track
int player_play(PlayerState *p, const char *path);

// Toggle pause
void player_toggle_pause(PlayerState *p);

// Check if playing
int player_is_playing(PlayerState *p);

// Get current track path
const char *player_current_track(PlayerState *p);

// Get position in seconds
double player_get_position(PlayerState *p);

// Get duration in seconds
double player_get_duration(PlayerState *p);

// Seek by offset seconds
void player_seek(PlayerState *p, double offset);

// Set volume (0-100)
void player_set_volume(PlayerState *p, int volume);

// Get volume
int player_get_volume(PlayerState *p);

// Next track
void player_next(PlayerState *p, struct Playlist *pl);

// Previous track
void player_prev(PlayerState *p, struct Playlist *pl);

// Check if track finished
int player_track_finished(PlayerState *p);

// Update player state (call each frame)
void player_update(PlayerState *p);

// Get audio buffer for spectrum
int player_get_audio_buffer(PlayerState *p, short *buffer, int samples);

#endif
