#ifndef METADATA_H
#define METADATA_H

// Extract cover art from MP3 ID3v2 tag to a temp file
// Returns 1 on success, 0 on failure
int metadata_extract_mp3_cover(const char *mp3_path, const char *output_path);

// Get music duration in seconds using SDL_mixer
// Returns duration in seconds, or 0 on failure
double metadata_get_duration(const char *music_path);

#endif
