#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <SDL2/SDL.h>
#include "player.h"

typedef struct Spectrum {
    float *bands;
    float *peaks;
    int num_bands;
    int bar_width;
    int gap;
} Spectrum;

// Initialize spectrum analyzer
Spectrum *spectrum_init(int num_bands);

// Free spectrum
void spectrum_free(Spectrum *s);

// Update spectrum from audio player
void spectrum_update(Spectrum *s, PlayerState *player);

// Draw spectrum bars
void spectrum_draw(Spectrum *s, SDL_Renderer *r,
    int x, int y, int w, int h, SDL_Color color);

#endif
