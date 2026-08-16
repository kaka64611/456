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

// Process 16-bit integer audio buffer (for postmix callback)
void eq_process_short(EQState *eq, short *buffer, int samples, int channels);

// EQ preset names
#define EQ_PRESET_COUNT 6
const char *eq_get_preset_name(int index);

// Apply EQ preset by index (0=Flat,1=Pop,2=Dance,3=Jazz,4=Rock,5=Classical)
void eq_apply_preset(EQState *eq, int preset);

// Get current preset index
int eq_get_current_preset(EQState *eq);

#endif
