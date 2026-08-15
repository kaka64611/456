/* 
 * Gmu Music Player - RG52MINI EQ enhanced version
 * audio.h with 5-band equalizer API
 */
#define MIN_BUFFER_FILL 32768
#define AUDIO_MAX_SW_VOLUME 16
#ifndef _AUDIO_H
#define _AUDIO_H
#include <sys/types.h>

/* === 5-band Equalizer === */
#define EQ_BANDS 5
void     audio_eq_init(void);
void     audio_eq_set_enabled(int enabled);
int      audio_eq_is_enabled(void);
void     audio_eq_set_band(int band, float gain_db);
float    audio_eq_get_band(int band);
void     audio_eq_process(int16_t *samples, int count, int channels);
/* ======================== */

int      audio_device_open(int samplerate, int channels);
int      audio_fill_buffer(char *data, size_t size);
size_t   audio_get_playtime(void);
void     audio_buffer_init(void);
void     audio_buffer_clear(void);
void     audio_buffer_free(void);
void     audio_device_close(void);
size_t   audio_buffer_get_fill(void);
size_t   audio_buffer_get_size(void);
int      audio_get_status(void);
void     audio_force_pause(int pause);
int      audio_set_pause(int pause_state);
int      audio_get_pause(void);
void     audio_set_volume(unsigned int vol);
unsigned int audio_get_volume(void);
size_t   audio_set_sample_counter(size_t sample);
size_t   audio_increase_sample_counter(size_t sample_offset);
size_t   audio_get_sample_count(void);
void     audio_wait_until_more_data_is_needed(void);
void     audio_set_done(void);
void     audio_set_fade_volume(unsigned int percent);
int      audio_fade_out_step(unsigned int step_size);
void     audio_reset_fade_volume(void);
int      audio_fade_out_in_progress(void);
int16_t *audio_spectrum_get_current_amplitudes(void);
void     audio_spectrum_register_for_access(void);
void     audio_spectrum_unregister(void);
int      audio_spectrum_read_lock(void);
void     audio_spectrum_read_unlock(void);
#endif
