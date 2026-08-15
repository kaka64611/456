#ifndef EQ_H
#define EQ_H

#define EQ_BANDS 5

typedef struct EQState {
    float gains[EQ_BANDS]; // dB
    int enabled;
    float frequencies[EQ_BANDS];
} EQState;

// Initialize EQ (5-band)
EQState *eq_init(void);

// Free EQ
void eq_free(EQState *eq);

// Set band gain in dB
void eq_set_band(EQState *eq, int band, float gain_db);

// Get band gain
float eq_get_band(EQState *eq, int band);

// Enable/disable EQ
void eq_set_enabled(EQState *eq, int enabled);

// Process audio buffer through EQ
void eq_process(EQState *eq, float *buffer, int samples, int channels, int sample_rate);

#endif
