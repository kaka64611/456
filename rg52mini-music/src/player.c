#include "player.h"
#include "playlist.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>

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
};

static void music_finished_hook() {
    // This will be handled in update() via Mix_PlayingMusic check
}

PlayerState *player_init(void) {
    PlayerState *p = (PlayerState *)calloc(1, sizeof(PlayerState));
    if (!p) return NULL;
    
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        // Try alternate format
        if (Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 4096) < 0) {
            fprintf(stderr, "Mix_OpenAudio retry failed: %s\n", Mix_GetError());
        }
    }
    
    Mix_HookMusicFinished(music_finished_hook);
    p->volume = 70;
    Mix_VolumeMusic(p->volume);
    
    return p;
}

void player_free(PlayerState *p) {
    if (!p) return;
    if (p->music) Mix_FreeMusic(p->music);
    Mix_CloseAudio();
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
    // Mix_MusicDuration requires SDL2_mixer >= 2.6.0, not available on EE4.7
    // Duration will be estimated from position / track length if needed
    p->duration = 0;
    
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
    // Track max position as estimated duration (Mix_MusicDuration not available)
    if (p->is_playing && !p->is_paused) {
        double pos = player_get_position(p);
        if (pos > p->duration) {
            p->duration = pos;
        }
    }
}

int player_get_audio_buffer(PlayerState *p, short *buffer, int samples) {
    // SDL_mixer doesn't easily expose raw audio buffer
    // Generate simulated multi-frequency data that looks like music spectrum
    static float phase1 = 0, phase2 = 0, phase3 = 0, phase4 = 0;
    static float energy = 0.5f;
    for (int i = 0; i < samples; i++) {
        phase1 += 0.05f;   // 低频
        phase2 += 0.15f;   // 中低频
        phase3 += 0.4f;    // 中频
        phase4 += 0.9f;    // 高频
        // 模拟音乐的动态能量变化
        energy += ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        if (energy < 0.2f) energy = 0.2f;
        if (energy > 1.0f) energy = 1.0f;
        
        float sample = sin(phase1) * 0.4f + sin(phase2) * 0.3f 
                     + sin(phase3) * 0.2f + sin(phase4) * 0.1f;
        buffer[i] = (short)(sample * 8000 * energy * (p->is_playing ? 1 : 0));
    }
    return samples;
}
