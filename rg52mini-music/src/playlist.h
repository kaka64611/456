#ifndef PLAYLIST_H
#define PLAYLIST_H

#define MAX_PATH_LEN 512
#define MAX_TRACKS 1000

typedef struct {
    char path[MAX_PATH_LEN];
    char title[256];
    char artist[256];
    int duration;
} Track;

typedef struct Playlist {
    Track items[MAX_TRACKS];
    int count;
    int current_index;
} Playlist;

// Initialize playlist
Playlist *playlist_init(void);

// Free playlist
void playlist_free(Playlist *pl);

// Scan directory for music files
int playlist_scan_directory(Playlist *pl, const char *dir);

// Add a track
int playlist_add(Playlist *pl, const char *path);

// Clear playlist
void playlist_clear(Playlist *pl);

// Get track name from path
void playlist_get_display_name(const char *path, char *out, int max_len);

#endif
