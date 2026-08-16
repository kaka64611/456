#include "playlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

ChannelList *playlist_create(void) {
    ChannelList *pl = (ChannelList *)calloc(1, sizeof(ChannelList));
    return pl;
}

void playlist_destroy(ChannelList *pl) {
    if (pl) free(pl);
}

void playlist_clear(ChannelList *pl) {
    if (!pl) return;
    pl->count = 0;
    pl->current_index = 0;
}

int playlist_add_channel(ChannelList *pl, const char *name, const char *url,
                         const char *logo, const char *group) {
    if (!pl || pl->count >= MAX_CHANNELS) return -1;
    Channel *ch = &pl->items[pl->count];
    strncpy(ch->name, name ? name : "Unknown", MAX_NAME_LEN - 1);
    strncpy(ch->url, url ? url : "", MAX_URL_LEN - 1);
    strncpy(ch->logo, logo ? logo : "", MAX_URL_LEN - 1);
    strncpy(ch->group, group ? group : "", MAX_NAME_LEN - 1);
    pl->count++;
    return 0;
}

// Parse #EXTINF line for name, logo, group
static void parse_extinf(const char *line, char *name, int name_len,
                         char *logo, int logo_len, char *group, int group_len) {
    if (name) name[0] = '\0';
    if (logo) logo[0] = '\0';
    if (group) group[0] = '\0';

    // Extract tvg-logo
    const char *p = strstr(line, "tvg-logo=\"");
    if (p) {
        p += 11;
        const char *end = strchr(p, '"');
        if (end && logo) {
            int len = end - p;
            if (len >= logo_len) len = logo_len - 1;
            strncpy(logo, p, len);
            logo[len] = '\0';
        }
    }

    // Extract group-title
    p = strstr(line, "group-title=\"");
    if (p) {
        p += 13;
        const char *end = strchr(p, '"');
        if (end && group) {
            int len = end - p;
            if (len >= group_len) len = group_len - 1;
            strncpy(group, p, len);
            group[len] = '\0';
        }
    }

    // Name is after the last comma
    p = strrchr(line, ',');
    if (p && name) {
        p++;
        while (*p == ' ') p++;
        strncpy(name, p, name_len - 1);
        name[name_len - 1] = '\0';
        // Remove trailing newline
        int len = strlen(name);
        while (len > 0 && (name[len-1] == '\n' || name[len-1] == '\r')) {
            name[--len] = '\0';
        }
    }
}

int playlist_load_m3u(ChannelList *pl, const char *filepath) {
    if (!pl || !filepath) return -1;

    FILE *f = fopen(filepath, "r");
    if (!f) {
        printf("Cannot open M3U: %s\n", filepath);
        return -1;
    }

    char line[2048];
    char extinf_name[MAX_NAME_LEN] = "";
    char extinf_logo[MAX_URL_LEN] = "";
    char extinf_group[MAX_NAME_LEN] = "";
    int has_extinf = 0;
    int count = 0;

    while (fgets(line, sizeof(line), f)) {
        // Remove trailing whitespace
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == ' ')) {
            line[--len] = '\0';
        }

        if (len == 0) continue;

        if (strncmp(line, "#EXTM3U", 7) == 0) {
            continue;
        }

        if (strncmp(line, "#EXTINF", 7) == 0) {
            parse_extinf(line, extinf_name, sizeof(extinf_name),
                        extinf_logo, sizeof(extinf_logo),
                        extinf_group, sizeof(extinf_group));
            has_extinf = 1;
            continue;
        }

        if (line[0] == '#') continue;

        // This is a URL line
        if (has_extinf) {
            playlist_add_channel(pl, extinf_name, line, extinf_logo, extinf_group);
            has_extinf = 0;
            count++;
        } else {
            // Simple format: URL only, use filename as name
            const char *name = strrchr(line, '/');
            if (name) name++; else name = line;
            playlist_add_channel(pl, name, line, "", "");
            count++;
        }
    }

    fclose(f);
    printf("Loaded %d channels from %s\n", count, filepath);
    return count;
}

int playlist_load_directory(ChannelList *pl, const char *dirpath) {
    if (!pl || !dirpath) return -1;

    DIR *dir = opendir(dirpath);
    if (!dir) {
        printf("Cannot open directory: %s\n", dirpath);
        return -1;
    }

    int total = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        int len = strlen(name);
        if (len < 5) continue;

        // Check for .m3u or .m3u8 extension
        const char *ext = name + len - 4;
        if (strcasecmp(ext, ".m3u") == 0 || strcasecmp(ext, "m3u8") == 0) {
            char fullpath[MAX_PATH_LEN];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, name);
            total += playlist_load_m3u(pl, fullpath);
        }
    }

    closedir(dir);
    printf("Total channels loaded: %d\n", total);
    return total;
}
