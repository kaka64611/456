#include "lyrics.h"
#include "playlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Lyrics *lyrics_init(void) {
    Lyrics *l = (Lyrics *)calloc(1, sizeof(Lyrics));
    return l;
}

void lyrics_free(Lyrics *l) {
    if (l) free(l);
}

// Parse time string like [00:12.34] to seconds
static double parse_time(const char *str) {
    int min = 0, sec = 0, ms = 0;
    if (sscanf(str, "[%d:%d.%d]", &min, &sec, &ms) >= 2) {
        return min * 60.0 + sec + ms / 100.0;
    }
    if (sscanf(str, "[%d:%d]", &min, &sec) >= 2) {
        return min * 60.0 + sec;
    }
    return -1;
}

// Check if line looks like real lyrics (not JSON/URL/data)
static int is_valid_lyric_line(const char *line) {
    if (!line || line[0] == '\0') return 0;
    int len = strlen(line);
    // Too long = probably not lyrics
    if (len > 150) return 0;
    // Contains JSON braces or URL patterns
    if (strchr(line, '{') || strchr(line, '}')) return 0;
    if (strstr(line, "http") || strstr(line, "://")) return 0;
    // Too many special characters
    int special = 0;
    for (int i = 0; i < len; i++) {
        if (line[i] == '"' || line[i] == '\\' || line[i] == ':') special++;
    }
    if (special > len / 4) return 0;
    return 1;
}

int lyrics_load_for_track(Lyrics *l, const char *track_path) {
    if (!l || !track_path) return -1;
    
    // Reset
    l->count = 0;
    l->current_index = 0;
    strncpy(l->track_path, track_path, MAX_PATH_LEN - 1);
    
    // Try .lrc file
    char lrc_path[MAX_PATH_LEN];
    strncpy(lrc_path, track_path, MAX_PATH_LEN - 1);
    char *dot = strrchr(lrc_path, '.');
    if (dot) {
        strcpy(dot, ".lrc");
    } else {
        strcat(lrc_path, ".lrc");
    }
    
    FILE *f = fopen(lrc_path, "r");
    if (!f) {
        // Try .txt
        if (dot) strcpy(dot, ".txt");
        else strcat(lrc_path, ".txt");
        f = fopen(lrc_path, "r");
    }
    
    if (!f) {
        return -1; // No lyrics file
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), f) && l->count < MAX_LYRIC_LINES) {
        // Remove trailing newline
        line[strcspn(line, "\r\n")] = '\0';
        
        // Skip empty lines
        if (line[0] == '\0') continue;
        
        // Skip invalid lines (JSON, URL, too long)
        if (!is_valid_lyric_line(line)) continue;
        
        // Try to parse LRC format [mm:ss.xx]text
        if (line[0] == '[') {
            double time = parse_time(line);
            if (time >= 0) {
                // Find text after last ]
                char *text = strrchr(line, ']');
                if (text) text++;
                else text = "";
                
                // Skip metadata tags
                if (strncmp(text, "ti:", 3) == 0 ||
                    strncmp(text, "ar:", 3) == 0 ||
                    strncmp(text, "al:", 3) == 0 ||
                    strncmp(text, "by:", 3) == 0 ||
                    strncmp(text, "offset:", 7) == 0) {
                    continue;
                }
                
                // Skip empty text
                if (text[0] == '\0') continue;
                
                strncpy(l->lines[l->count].text, text, MAX_LYRIC_TEXT - 1);
                l->lines[l->count].time = time;
                l->count++;
            }
        } else if (line[0] != '\0') {
            // Plain text line - assign sequential time (5 sec intervals)
            l->lines[l->count].time = l->count * 5.0;
            strncpy(l->lines[l->count].text, line, MAX_LYRIC_TEXT - 1);
            l->count++;
        }
    }
    fclose(f);
    
    // Sort by time
    for (int i = 0; i < l->count - 1; i++) {
        for (int j = i + 1; j < l->count; j++) {
            if (l->lines[j].time < l->lines[i].time) {
                LyricLine tmp = l->lines[i];
                l->lines[i] = l->lines[j];
                l->lines[j] = tmp;
            }
        }
    }
    
    return l->count;
}

void lyrics_update(Lyrics *l, double position) {
    if (!l || l->count == 0) return;
    
    // Find current lyric line
    int idx = 0;
    for (int i = 0; i < l->count; i++) {
        if (l->lines[i].time <= position) {
            idx = i;
        } else {
            break;
        }
    }
    l->current_index = idx;
}

const char *lyrics_get_current(Lyrics *l) {
    if (!l || l->count == 0) return NULL;
    if (l->current_index >= 0 && l->current_index < l->count) {
        return l->lines[l->current_index].text;
    }
    return NULL;
}

const char *lyrics_get_line(Lyrics *l, int index) {
    if (!l || index < 0 || index >= l->count) return "";
    return l->lines[index].text;
}

int lyrics_get_current_index(Lyrics *l) {
    return l ? l->current_index : 0;
}
