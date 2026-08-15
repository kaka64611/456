#include "spectrum.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Spectrum *spectrum_init(int num_bands) {
    Spectrum *s = (Spectrum *)calloc(1, sizeof(Spectrum));
    if (!s) return NULL;
    
    s->num_bands = num_bands;
    s->bands = (float *)calloc(num_bands, sizeof(float));
    s->peaks = (float *)calloc(num_bands, sizeof(float));
    s->bar_width = 8;
    s->gap = 3;
    
    return s;
}

void spectrum_free(Spectrum *s) {
    if (!s) return;
    if (s->bands) free(s->bands);
    if (s->peaks) free(s->peaks);
    free(s);
}

// Simple DFT for spectrum analysis
static void compute_spectrum(short *input, int samples, float *output, int bands) {
    // Use a simplified frequency band calculation
    // Group FFT bins into logarithmic bands
    for (int b = 0; b < bands; b++) {
        float freq_low = 20.0f * powf(2.0f, (float)b * 10.0f / bands);
        float freq_high = 20.0f * powf(2.0f, (float)(b + 1) * 10.0f / bands);
        if (freq_high > 20000.0f) freq_high = 20000.0f;
        
        // Simplified: use Goertzel algorithm for center frequency
        float center = (freq_low + freq_high) / 2.0f;
        float coeff = 2.0f * cosf(2.0f * M_PI * center / 44100.0f);
        float s1 = 0, s2 = 0;
        
        int window = samples > 1024 ? 1024 : samples;
        for (int i = 0; i < window; i++) {
            float sample = input[i] / 32768.0f;
            float s0 = sample + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        
        float power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
        output[b] = sqrtf(power) * 5.0f;
        
        // Normalize
        if (output[b] > 1.0f) output[b] = 1.0f;
    }
}

void spectrum_update(Spectrum *s, PlayerState *player) {
    if (!s || !player) return;
    
    // Get audio buffer
    short buffer[2048];
    int samples = player_get_audio_buffer(player, buffer, 2048);
    if (samples <= 0) {
        // Fade out
        for (int i = 0; i < s->num_bands; i++) {
            s->bands[i] *= 0.9f;
            s->peaks[i] *= 0.95f;
        }
        return;
    }
    
    // Compute spectrum
    compute_spectrum(buffer, samples, s->bands, s->num_bands);
    
    // Smooth and update peaks
    for (int i = 0; i < s->num_bands; i++) {
        // Smooth
        static float prev[128] = {0};
        s->bands[i] = prev[i] * 0.6f + s->bands[i] * 0.4f;
        prev[i] = s->bands[i];
        
        // Update peak
        if (s->bands[i] > s->peaks[i]) {
            s->peaks[i] = s->bands[i];
        } else {
            s->peaks[i] *= 0.98f;
        }
    }
}

void spectrum_draw(Spectrum *s, SDL_Renderer *r,
    int x, int y, int w, int h, SDL_Color color) {
    if (!s || s->num_bands == 0) return;
    
    int total_width = s->num_bands * (s->bar_width + s->gap) - s->gap;
    int start_x = x + (w - total_width) / 2;
    int base_y = y + h - 10;
    
    for (int i = 0; i < s->num_bands; i++) {
        int bx = start_x + i * (s->bar_width + s->gap);
        int bar_h = (int)(s->bands[i] * (h - 20));
        if (bar_h < 2) bar_h = 2;
        if (bar_h > h - 20) bar_h = h - 20;
        
        // Gradient color (bottom bright, top dim)
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 255);
        SDL_Rect bar = { bx, base_y - bar_h, s->bar_width, bar_h };
        SDL_RenderFillRect(r, &bar);
        
        // Peak indicator
        int peak_h = (int)(s->peaks[i] * (h - 20));
        if (peak_h > bar_h) {
            SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
            SDL_Rect peak = { bx, base_y - peak_h, s->bar_width, 2 };
            SDL_RenderFillRect(r, &peak);
        }
    }
    
    // Baseline
    SDL_SetRenderDrawColor(r, 60, 80, 100, 255);
    SDL_RenderDrawLine(r, x, base_y, x + w, base_y);
}
