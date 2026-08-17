#ifndef PLAYLIST_H
#define PLAYLIST_H

#define MAX_CHANNELS 512
#define MAX_NAME_LEN 256
#define MAX_URL_LEN 1024
#define MAX_PATH_LEN 1024
#define MAX_URLS_PER_CHANNEL 16

typedef struct {
    char name[MAX_NAME_LEN];
    char urls[MAX_URLS_PER_CHANNEL][MAX_URL_LEN];
    int url_count;
    int preferred_url;  // Index of preferred/saved source (-1 = none)
    char logo[MAX_URL_LEN];
    char group[MAX_NAME_LEN];
} Channel;

typedef struct {
    Channel items[MAX_CHANNELS];
    int count;
    int current_index;
} ChannelList;

ChannelList *playlist_create(void);
void playlist_destroy(ChannelList *pl);
int playlist_load_m3u(ChannelList *pl, const char *filepath);
int playlist_load_directory(ChannelList *pl, const char *dirpath);
int playlist_add_channel(ChannelList *pl, const char *name, const char *url, const char *logo, const char *group);
int playlist_add_url_to_channel(ChannelList *pl, const char *name, const char *url);
int playlist_find_channel(ChannelList *pl, const char *name);
void playlist_clear(ChannelList *pl);
void playlist_save_preferences(ChannelList *pl, const char *filepath);
void playlist_load_preferences(ChannelList *pl, const char *filepath);

#endif
