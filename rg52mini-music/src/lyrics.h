#ifndef LYRICS_H
#define LYRICS_H

#include "playlist.h"

#define MAX_LYRIC_LINES 200
#define MAX_LYRIC_TEXT  512

typedef struct {
    double time;  // seconds
    char text[MAX_LYRIC_TEXT];
} LyricLine;

typedef struct Lyrics {
    LyricLine lines[MAX_LYRIC_LINES];
    int count;
    int current_index;
    char track_path[MAX_PATH_LEN];
} Lyrics;

// Initialize lyrics
Lyrics *lyrics_init(void);

// Free lyrics
void lyrics_free(Lyrics *l);

// Load lyrics for a track (looks for .lrc/.txt)
int lyrics_load_for_track(Lyrics *l, const char *track_path);

// Update current lyric based on playback position
void lyrics_update(Lyrics *l, double position);

// Get current lyric text
const char *lyrics_get_current(Lyrics *l);

// Get lyric at index
const char *lyrics_get_line(Lyrics *l, int index);

// Get current line index
int lyrics_get_current_index(Lyrics *l);

#endif
