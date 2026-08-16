#ifndef PLAYER_H
#define PLAYER_H

#include <mpv/client.h>
#include <stdbool.h>

typedef struct {
    void *lib_handle;
    mpv_handle *mpv;
    bool is_playing;
    char current_url[1024];
    int volume;
    // Function pointers for dynamic loading
    mpv_handle* (*mpv_create)(void);
    int (*mpv_initialize)(mpv_handle*);
    int (*mpv_command)(mpv_handle*, const char**);
    int (*mpv_set_option_string)(mpv_handle*, const char*, const char*);
    int (*mpv_set_property_string)(mpv_handle*, const char*, const char*);
    char* (*mpv_get_property_string)(mpv_handle*, const char*);
    void (*mpv_free)(void*);
    mpv_event* (*mpv_wait_event)(mpv_handle*, double);
    void (*mpv_terminate_destroy)(mpv_handle*);
} TVPlayer;

TVPlayer* player_create(void);
void player_destroy(TVPlayer *p);
bool player_load(TVPlayer *p, const char *url);
void player_stop(TVPlayer *p);
void player_set_volume(TVPlayer *p, int volume);
int player_get_volume(TVPlayer *p);
bool player_is_playing(TVPlayer *p);
void player_poll_events(TVPlayer *p);

#endif
