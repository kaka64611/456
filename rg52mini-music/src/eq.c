#include "eq.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// BiQuad filter state
typedef struct {
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
} BiQuad;

static BiQuad filters[EQ_BANDS];
static int filters_initialized = 0;
static int current_preset = 0;
static float g_sample_rate = 44100.0f;

static void calc_biquad_peaking(BiQuad *f, float freq, float gain_db,
    float q, float sample_rate) {
    float a = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * M_PI * freq / sample_rate;
    float alpha = sinf(w0) / (2.0f * q);
    
    float b0 = 1.0f + alpha * a;
    float b1 = -2.0f * cosf(w0);
    float b2 = 1.0f - alpha * a;
    float a0 = 1.0f + alpha / a;
    float a1 = -2.0f * cosf(w0);
    float a2 = 1.0f - alpha / a;
    
    f->b0 = b0 / a0;
    f->b1 = b1 / a0;
    f->b2 = b2 / a0;
    f->a1 = a1 / a0;
    f->a2 = a2 / a0;
    f->x1 = f->x2 = f->y1 = f->y2 = 0;
}

static void init_filters(EQState *eq) {
    for (int i = 0; i < EQ_BANDS; i++) {
        calc_biquad_peaking(&filters[i], eq->frequencies[i],
            eq->gains[i], 1.0f, g_sample_rate);
    }
    filters_initialized = 1;
}

EQState *eq_init(void) {
    EQState *eq = (EQState *)calloc(1, sizeof(EQState));
    if (!eq) return NULL;
    
    // 5-band frequencies
    eq->frequencies[0] = 60.0f;
    eq->frequencies[1] = 230.0f;
    eq->frequencies[2] = 910.0f;
    eq->frequencies[3] = 3600.0f;
    eq->frequencies[4] = 14000.0f;
    
    // Flat response by default
    for (int i = 0; i < EQ_BANDS; i++) {
        eq->gains[i] = 0.0f;
    }
    
    eq->enabled = 1;
    init_filters(eq);
    
    return eq;
}

void eq_free(EQState *eq) {
    if (eq) free(eq);
}

void eq_set_band(EQState *eq, int band, float gain_db) {
    if (!eq || band < 0 || band >= EQ_BANDS) return;
    if (gain_db < -12.0f) gain_db = -12.0f;
    if (gain_db > 12.0f) gain_db = 12.0f;
    eq->gains[band] = gain_db;
    calc_biquad_peaking(&filters[band], eq->frequencies[band],
        gain_db, 1.0f, g_sample_rate);
}

float eq_get_band(EQState *eq, int band) {
    if (!eq || band < 0 || band >= EQ_BANDS) return 0.0f;
    return eq->gains[band];
}

void eq_set_enabled(EQState *eq, int enabled) {
    if (eq) eq->enabled = enabled;
}

void eq_process_short(EQState *eq, short *buffer, int samples, int channels) {
    if (!eq || !eq->enabled || !filters_initialized || !buffer) return;
    for (int i = 0; i < samples; i++) {
        for (int ch = 0; ch < channels; ch++) {
            float x = (float)buffer[i * channels + ch] / 32768.0f;
            for (int b = 0; b < EQ_BANDS; b++) {
                BiQuad *f = &filters[b];
                float y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
                        - f->a1 * f->y1 - f->a2 * f->y2;
                f->x2 = f->x1; f->x1 = x;
                f->y2 = f->y1; f->y1 = y;
                x = y;
            }
            // Clamp and convert back
            if (x > 1.0f) x = 1.0f;
            if (x < -1.0f) x = -1.0f;
            buffer[i * channels + ch] = (short)(x * 32767.0f);
        }
    }
}

void eq_process(EQState *eq, float *buffer, int samples,
    int channels, int sample_rate) {
    if (!eq || !eq->enabled || !filters_initialized) return;
    
    for (int ch = 0; ch < channels; ch++) {
        for (int i = 0; i < samples; i++) {
            float x = buffer[i * channels + ch];
            for (int b = 0; b < EQ_BANDS; b++) {
                BiQuad *f = &filters[b];
                float y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
                        - f->a1 * f->y1 - f->a2 * f->y2;
                f->x2 = f->x1;
                f->x1 = x;
                f->y2 = f->y1;
                f->y1 = y;
                x = y;
            }
            buffer[i * channels + ch] = x;
        }
    }
}

static const char *preset_names[EQ_PRESET_COUNT] = {
    "Flat", "Pop", "Dance", "Jazz", "Rock", "Classical"
};

static const float preset_gains[EQ_PRESET_COUNT][EQ_BANDS] = {
    { 0,  0,  0,  0,  0},  // Flat
    {-2,  3,  6,  6, -2},  // Pop (enhanced)
    { 9,  6,  0,  3,  6},  // Dance (enhanced bass + treble)
    { 4,  3,  0,  3, -2},  // Jazz
    { 8,  5, -2,  3,  6},  // Rock (enhanced bass + treble)
    { 6,  4,  0,  3,  6},  // Classical
};

const char *eq_get_preset_name(int index) {
    if (index < 0 || index >= EQ_PRESET_COUNT) return "Flat";
    return preset_names[index];
}

void eq_apply_preset(EQState *eq, int preset) {
    if (!eq || preset < 0 || preset >= EQ_PRESET_COUNT) return;
    current_preset = preset;
    for (int i = 0; i < EQ_BANDS; i++) {
        eq->gains[i] = preset_gains[preset][i];
        calc_biquad_peaking(&filters[i], eq->frequencies[i],
            eq->gains[i], 1.0f, g_sample_rate);
    }
}

int eq_get_current_preset(EQState *eq) {
    (void)eq;
    return current_preset;
}

void eq_set_sample_rate(EQState *eq, float sample_rate) {
    if (sample_rate < 8000.0f || sample_rate > 192000.0f) return;
    g_sample_rate = sample_rate;
    if (eq) {
        init_filters(eq);
        // Re-apply current preset with new sample rate
        eq_apply_preset(eq, current_preset);
    }
}
