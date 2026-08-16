#include "metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// Convert synchsafe integer (4 bytes, 7 bits each) to normal int
static unsigned int synchsafe_to_int(unsigned char *buf) {
    return ((unsigned int)buf[0] << 21) |
           ((unsigned int)buf[1] << 14) |
           ((unsigned int)buf[2] << 7) |
           ((unsigned int)buf[3]);
}

// MP3 bitrate table (MPEG1 Layer3)
static const int mp3_bitrate_v1_l3[16] = {
    0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
};
// MP3 bitrate table (MPEG2 Layer3)
static const int mp3_bitrate_v2_l3[16] = {
    0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0
};
// MP3 sample rate table
static const int mp3_samplerate[4] = { 44100, 48000, 32000, 0 };

// Parse MP3 file and return duration in seconds
static double parse_mp3_duration(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size < 128) { fclose(f); return 0; }
    
    // Skip ID3v2 tag if present
    unsigned char header[10];
    long data_start = 0;
    if (fread(header, 1, 10, f) == 10 && memcmp(header, "ID3", 3) == 0) {
        unsigned int tag_size = synchsafe_to_int(header + 6);
        data_start = 10 + tag_size;
        // Also skip ID3v2 footer if present
        if (header[5] & 0x10) data_start += 10;
    }
    
    // Find first MP3 frame (scan for sync word 0xFFE)
    fseek(f, data_start, SEEK_SET);
    unsigned char frame[4];
    int found = 0;
    long frame_pos = data_start;
    
    // Scan up to 64KB for first frame
    for (int i = 0; i < 65536 && frame_pos + 4 < file_size; i++) {
        if (fread(frame, 1, 4, f) != 4) break;
        // Check sync word: 11 bits all 1 (0xFFE0 or higher)
        if (frame[0] == 0xFF && (frame[1] & 0xE0) == 0xE0) {
            found = 1;
            break;
        }
        frame_pos++;
        fseek(f, frame_pos, SEEK_SET);
    }
    
    if (!found) { fclose(f); return 0; }
    
    // Parse frame header
    int mpeg_version = (frame[1] >> 3) & 0x03; // 0=2.5, 1=reserved, 2=2, 3=1
    int layer = (frame[1] >> 1) & 0x03; // 1=III, 2=II, 3=I
    int bitrate_idx = (frame[2] >> 4) & 0x0F;
    int samplerate_idx = (frame[2] >> 2) & 0x03;
    int padding = (frame[2] >> 1) & 0x01;
    
    if (layer != 1) { fclose(f); return 0; } // Only Layer3 for now
    
    int bitrate, samplerate;
    if (mpeg_version == 3) { // MPEG1
        bitrate = mp3_bitrate_v1_l3[bitrate_idx] * 1000;
        samplerate = mp3_samplerate[samplerate_idx];
    } else { // MPEG2 or 2.5
        bitrate = mp3_bitrate_v2_l3[bitrate_idx] * 1000;
        samplerate = mp3_samplerate[samplerate_idx] / 2;
    }
    
    if (bitrate <= 0 || samplerate <= 0) { fclose(f); return 0; }
    
    // Calculate frame length: 144 * bitrate / samplerate + padding
    int frame_length = (int)(144.0 * bitrate / samplerate) + padding;
    if (frame_length <= 0) { fclose(f); return 0; }
    
    // Estimate total frames
    long audio_size = file_size - frame_pos;
    // Also skip ID3v1 tag at end (128 bytes)
    unsigned char tail[3];
    fseek(f, -128, SEEK_END);
    if (fread(tail, 1, 3, f) == 3 && memcmp(tail, "TAG", 3) == 0) {
        audio_size -= 128;
    }
    
    long total_frames = audio_size / frame_length;
    // MPEG1: 1152 samples per frame, MPEG2: 576
    int samples_per_frame = (mpeg_version == 3) ? 1152 : 576;
    double duration = (double)total_frames * samples_per_frame / samplerate;
    
    fclose(f);
    return duration > 0 ? duration : 0;
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
        
        // Frame size (4 bytes, big-endian for ID3v2.3, synchsafe for v2.4)
        unsigned int frame_size = ((unsigned int)tag[pos+4] << 24) |
                                  ((unsigned int)tag[pos+5] << 16) |
                                  ((unsigned int)tag[pos+6] << 8) |
                                  ((unsigned int)tag[pos+7]);
        if (frame_size > tag_size - pos - 10) {
            // Try synchsafe
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
            if (data_pos < frame_size) data_pos++;
            
            // Skip picture type (1 byte)
            if (data_pos >= frame_size) break;
            data_pos++;
            
            // Skip description (null-terminated, could be UTF-16)
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
    
    // Check extension
    const char *ext = strrchr(music_path, '.');
    if (!ext) return 0;
    ext++;
    
    if (strcasecmp(ext, "mp3") == 0) {
        return parse_mp3_duration(music_path);
    }
    
    // For other formats, return 0 for now
    return 0;
}
