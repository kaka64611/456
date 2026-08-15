#include "playlist.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

static int is_music_file(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    ext++;
    return (strcasecmp(ext, "mp3") == 0 ||
            strcasecmp(ext, "flac") == 0 ||
            strcasecmp(ext, "ogg") == 0 ||
            strcasecmp(ext, "oga") == 0 ||
            strcasecmp(ext, "wav") == 0 ||
            strcasecmp(ext, "m4a") == 0 ||
            strcasecmp(ext, "aac") == 0 ||
            strcasecmp(ext, "opus") == 0 ||
            strcasecmp(ext, "mod") == 0 ||
            strcasecmp(ext, "xm") == 0 ||
            strcasecmp(ext, "s3m") == 0 ||
            strcasecmp(ext, "it") == 0);
}

Playlist *playlist_init(void) {
    Playlist *pl = (Playlist *)calloc(1, sizeof(Playlist));
    return pl;
}

void playlist_free(Playlist *pl) {
    if (pl) free(pl);
}

int playlist_add(Playlist *pl, const char *path) {
    if (!pl || pl->count >= MAX_TRACKS) return -1;
    
    Track *t = &pl->items[pl->count];
    strncpy(t->path, path, MAX_PATH_LEN - 1);
    playlist_get_display_name(path, t->title, sizeof(t->title));
    t->artist[0] = '\0';
    t->duration = 0;
    pl->count++;
    return 0;
}

void playlist_clear(Playlist *pl) {
    if (pl) {
        pl->count = 0;
        pl->current_index = 0;
    }
}

static int compare_tracks(const void *a, const void *b) {
    const Track *ta = (const Track *)a;
    const Track *tb = (const Track *)b;
    return strcmp(ta->title, tb->title);
}

int playlist_scan_directory(Playlist *pl, const char *dir) {
    if (!pl || !dir) return -1;
    
    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return -1;
    }
    
    struct dirent *entry;
    char fullpath[MAX_PATH_LEN];
    int added = 0;
    
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);
        
        struct stat st;
        if (stat(fullpath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Recurse into subdirectories (max depth 2)
                added += playlist_scan_directory(pl, fullpath);
            } else if (is_music_file(entry->d_name)) {
                if (playlist_add(pl, fullpath) == 0) {
                    added++;
                }
            }
        }
    }
    closedir(d);
    
    // Sort by title
    if (pl->count > 1) {
        qsort(pl->items, pl->count, sizeof(Track), compare_tracks);
    }
    
    return added;
}

void playlist_get_display_name(const char *path, char *out, int max_len) {
    if (!path || !out) return;
    
    // Get filename without path
    const char *name = strrchr(path, '/');
    if (name) name++;
    else name = path;
    
    // Copy without extension
    strncpy(out, name, max_len - 1);
    out[max_len - 1] = '\0';
    
    char *dot = strrchr(out, '.');
    if (dot) *dot = '\0';
}
