#ifndef PLAYLIST_H
#define PLAYLIST_H

#define MAX_CHANNELS 512
#define MAX_NAME_LEN 256
#define MAX_URL_LEN 1024
#define MAX_PATH_LEN 1024

typedef struct {
    char name[MAX_NAME_LEN];
    char url[MAX_URL_LEN];
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
void playlist_clear(ChannelList *pl);

#endif
