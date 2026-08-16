#include "metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// Convert synchsafe integer (4 bytes, 7 bits each) to normal int
static unsigned int synchsafe_to_int(unsigned char *buf) {
    return ((unsigned int)buf[0] << 21) |
           ((unsigned int)buf[1] << 14) |
           ((unsigned int)buf[2] << 7) |
           ((unsigned int)buf[3]);
}

// Convert normal 32-bit big-endian to int
static unsigned int be32_to_int(unsigned char *buf) {
    return ((unsigned int)buf[0] << 24) |
           ((unsigned int)buf[1] << 16) |
           ((unsigned int)buf[2] << 8) |
           ((unsigned int)buf[3]);
}

int metadata_extract_mp3_cover(const char *mp3_path, const char *output_path) {
    if (!mp3_path || !output_path) return 0;
    
    FILE *f = fopen(mp3_path, "rb");
    if (!f) return 0;
    
    // Read ID3v2 header (10 bytes)
    unsigned char header[10];
    if (fread(header, 1, 10, f) != 10) { fclose(f); return 0; }
    
    // Check for ID3v2
    if (memcmp(header, "ID3", 3) != 0) { fclose(f); return 0; }
    
    // Get tag size (synchsafe)
    unsigned int tag_size = synchsafe_to_int(header + 6);
    if (tag_size == 0 || tag_size > 10 * 1024 * 1024) { fclose(f); return 0; }
    
    // Read entire tag
    unsigned char *tag = (unsigned char *)malloc(tag_size);
    if (!tag) { fclose(f); return 0; }
    if (fread(tag, 1, tag_size, f) != tag_size) { free(tag); fclose(f); return 0; }
    fclose(f);
    
    // Parse frames
    unsigned int pos = 0;
    int found = 0;
    
    while (pos + 10 <= tag_size) {
        // Frame ID (4 bytes)
        char frame_id[5];
        memcpy(frame_id, tag + pos, 4);
        frame_id[4] = '\0';
        
        // Frame size (4 bytes, big-endian for ID3v2.4, synchsafe for v2.4)
        // Try both: first as normal BE, if too large try synchsafe
        unsigned int frame_size = be32_to_int(tag + pos + 4);
        if (frame_size > tag_size - pos - 10) {
            frame_size = synchsafe_to_int(tag + pos + 4);
        }
        
        if (frame_size == 0 || pos + 10 + frame_size > tag_size) break;
        
        // Check for APIC (attached picture)
        if (memcmp(frame_id, "APIC", 4) == 0) {
            unsigned char *frame_data = tag + pos + 10;
            unsigned int data_pos = 0;
            
            // Skip text encoding (1 byte)
            if (data_pos >= frame_size) break;
            data_pos++;
            
            // Read MIME type (null-terminated)
            char mime[64];
            int mime_len = 0;
            while (data_pos < frame_size && frame_data[data_pos] != 0 && mime_len < 63) {
                mime[mime_len++] = frame_data[data_pos++];
            }
            mime[mime_len] = '\0';
            if (data_pos < frame_size) data_pos++; // skip null
            
            // Skip picture type (1 byte)
            if (data_pos >= frame_size) break;
            data_pos++;
            
            // Skip description (null-terminated, could be UTF-16)
            // For simplicity, skip until we find enough consecutive zeros
            int desc_skipped = 0;
            while (data_pos < frame_size && desc_skipped < 3) {
                if (frame_data[data_pos] == 0) desc_skipped++;
                else desc_skipped = 0;
                data_pos++;
            }
            
            // Remaining data is the picture
            unsigned int pic_size = frame_size - data_pos;
            if (pic_size > 100) {
                FILE *out = fopen(output_path, "wb");
                if (out) {
                    fwrite(frame_data + data_pos, 1, pic_size, out);
                    fclose(out);
                    found = 1;
                    printf("Extracted cover: %s (%d bytes, mime: %s)\n", output_path, pic_size, mime);
                }
            }
            break;
        }
        
        pos += 10 + frame_size;
    }
    
    free(tag);
    return found;
}

double metadata_get_duration(const char *music_path) {
    if (!music_path) return 0;
    
    Mix_Music *music = Mix_LoadMUS(music_path);
    if (!music) {
        fprintf(stderr, "Failed to load for duration: %s: %s\n", music_path, Mix_GetError());
        return 0;
    }
    
    double duration = 0;
    
    // Play briefly, seek to end, get position
    if (Mix_PlayMusic(music, 0) == 0) {
        Mix_SetMusicPosition(99999.0);
        SDL_Delay(100); // Wait for seek to complete
        duration = Mix_GetMusicPosition(music);
        Mix_HaltMusic();
    }
    
    Mix_FreeMusic(music);
    return duration > 0 ? duration : 0;
}
