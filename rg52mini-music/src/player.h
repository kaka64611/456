#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL_mixer.h>
#include "playlist.h"
#include "eq.h"

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

// Check if paused
int player_is_paused(PlayerState *p);

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
void player_next(PlayerState *p, Playlist *pl);

// Previous track
void player_prev(PlayerState *p, Playlist *pl);

// Check if track finished
int player_track_finished(PlayerState *p);

// Update player state (call each frame)
void player_update(PlayerState *p);

// Get audio buffer for spectrum
int player_get_audio_buffer(PlayerState *p, short *buffer, int samples);

// Set EQ state for postmix processing
void player_set_eq(PlayerState *p, EQState *eq);

// Set play mode (0=sequence, 1=repeat, 2=shuffle)
void player_set_play_mode(PlayerState *p, int mode);

// Get play mode
int player_get_play_mode(PlayerState *p);

// Get actual audio sample rate
int player_get_sample_rate(PlayerState *p);

#endif
