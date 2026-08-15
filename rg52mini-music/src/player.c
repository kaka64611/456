#include "player.h"
#include "playlist.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>

#define AUDIO_RING_SIZE 8192

struct PlayerState {
    Mix_Music *music;
    char current_path[MAX_PATH_LEN];
    int is_playing;
    int is_paused;
    int volume;
    double position;
    double duration;
    int track_finished;
    Uint32 start_tick;
    Uint32 pause_tick;
    // Real audio ring buffer for spectrum
    short audio_ring[AUDIO_RING_SIZE];
    volatile int audio_write_pos;
    volatile int audio_count;
};

// Global pointer for postmix callback (SDL_mixer callback doesn't pass userdata reliably)
static PlayerState *g_player = NULL;

// Postmix callback: captures real mixed audio for spectrum
static void postmix_callback(void *udata, Uint8 *stream, int len) {
    (void)udata;
    if (!g_player || !stream || len <= 0) return;
    
    short *samples = (short *)stream;
    int frame_count = len / 4; // 16-bit stereo = 4 bytes per frame
    
    for (int i = 0; i < frame_count; i++) {
        // Mix left and right channels to mono
        short left = samples[i * 2];
        short right = samples[i * 2 + 1];
        short mono = (short)(((int)left + (int)right) / 2);
        
        g_player->audio_ring[g_player->audio_write_pos] = mono;
        g_player->audio_write_pos = (g_player->audio_write_pos + 1) % AUDIO_RING_SIZE;
        if (g_player->audio_count < AUDIO_RING_SIZE) {
            g_player->audio_count++;
        }
    }
}

static void music_finished_hook() {
    // This will be handled in update() via Mix_PlayingMusic check
}

PlayerState *player_init(void) {
    PlayerState *p = (PlayerState *)calloc(1, sizeof(PlayerState));
    if (!p) return NULL;
    
    g_player = p;
    
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096) < 0) {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        if (Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 4096) < 0) {
            fprintf(stderr, "Mix_OpenAudio retry failed: %s\n", Mix_GetError());
        }
    }
    
    // Register postmix callback to capture real audio
    Mix_SetPostMix(postmix_callback, NULL);
    Mix_HookMusicFinished(music_finished_hook);
    
    p->volume = 70;
    Mix_VolumeMusic(p->volume);
    
    return p;
}

void player_free(PlayerState *p) {
    if (!p) return;
    Mix_SetPostMix(NULL, NULL);
    if (p->music) Mix_FreeMusic(p->music);
    Mix_CloseAudio();
    if (g_player == p) g_player = NULL;
    free(p);
}

int player_play(PlayerState *p, const char *path) {
    if (!p || !path) return -1;
    
    if (p->music) {
        Mix_HaltMusic();
        Mix_FreeMusic(p->music);
        p->music = NULL;
    }
    
    p->music = Mix_LoadMUS(path);
    if (!p->music) {
        fprintf(stderr, "Failed to load %s: %s\n", path, Mix_GetError());
        return -1;
    }
    
    strncpy(p->current_path, path, MAX_PATH_LEN - 1);
    p->duration = 0;
    // Reset audio ring buffer
    p->audio_write_pos = 0;
    p->audio_count = 0;
    
    if (Mix_PlayMusic(p->music, 0) < 0) {
        fprintf(stderr, "Mix_PlayMusic failed: %s\n", Mix_GetError());
        return -1;
    }
    
    p->is_playing = 1;
    p->is_paused = 0;
    p->track_finished = 0;
    p->position = 0;
    p->start_tick = SDL_GetTicks();
    
    return 0;
}

void player_toggle_pause(PlayerState *p) {
    if (!p || !p->music) return;
    if (p->is_paused) {
        Mix_ResumeMusic();
        p->is_paused = 0;
        p->start_tick = SDL_GetTicks() - (Uint32)(p->position * 1000);
    } else {
        Mix_PauseMusic();
        p->is_paused = 1;
    }
}

int player_is_playing(PlayerState *p) {
    if (!p) return 0;
    return p->is_playing && !p->is_paused;
}

int player_is_paused(PlayerState *p) {
    if (!p) return 0;
    return p->is_paused;
}

const char *player_current_track(PlayerState *p) {
    if (!p) return NULL;
    return p->current_path[0] ? p->current_path : NULL;
}

double player_get_position(PlayerState *p) {
    if (!p) return 0;
    if (p->is_playing && !p->is_paused) {
        p->position = (SDL_GetTicks() - p->start_tick) / 1000.0;
        if (p->position > p->duration && p->duration > 0)
            p->position = p->duration;
    }
    return p->position;
}

double player_get_duration(PlayerState *p) {
    if (!p) return 0;
    return p->duration;
}

void player_seek(PlayerState *p, double offset) {
    if (!p || !p->music) return;
    double new_pos = p->position + offset;
    if (new_pos < 0) new_pos = 0;
    if (new_pos > p->duration && p->duration > 0) new_pos = p->duration;
    
    if (Mix_SetMusicPosition(new_pos) == 0) {
        p->position = new_pos;
        p->start_tick = SDL_GetTicks() - (Uint32)(new_pos * 1000);
    }
}

void player_set_volume(PlayerState *p, int volume) {
    if (!p) return;
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    p->volume = volume;
    Mix_VolumeMusic(volume);
}

int player_get_volume(PlayerState *p) {
    return p ? p->volume : 0;
}

void player_next(PlayerState *p, Playlist *pl) {
    if (!p || !pl || pl->count == 0) return;
    int next = pl->current_index + 1;
    if (next >= pl->count) next = 0;
    pl->current_index = next;
    player_play(p, pl->items[next].path);
}

void player_prev(PlayerState *p, Playlist *pl) {
    if (!p || !pl || pl->count == 0) return;
    int prev = pl->current_index - 1;
    if (prev < 0) prev = pl->count - 1;
    pl->current_index = prev;
    player_play(p, pl->items[prev].path);
}

int player_track_finished(PlayerState *p) {
    if (!p || !p->music) return 0;
    if (!Mix_PlayingMusic() && p->is_playing && !p->is_paused) {
        p->track_finished = 1;
        p->is_playing = 0;
        return 1;
    }
    return 0;
}

void player_update(PlayerState *p) {
    if (!p) return;
    if (p->is_playing && !p->is_paused) {
        double pos = player_get_position(p);
        if (pos > p->duration) {
            p->duration = pos;
        }
    }
}

// Get real audio buffer from ring buffer for spectrum analysis
int player_get_audio_buffer(PlayerState *p, short *buffer, int samples) {
    if (!p || !buffer || samples <= 0) return 0;
    
    if (p->audio_count < samples) {
        // Not enough data yet, fill with silence
        memset(buffer, 0, samples * sizeof(short));
        return p->audio_count;
    }
    
    // Read the most recent 'samples' from ring buffer
    int start = (p->audio_write_pos - samples + AUDIO_RING_SIZE) % AUDIO_RING_SIZE;
    for (int i = 0; i < samples; i++) {
        buffer[i] = p->audio_ring[(start + i) % AUDIO_RING_SIZE];
    }
    return samples;
}
