#ifndef MPV_CLIENT_H
#define MPV_CLIENT_H

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mpv_handle mpv_handle;
typedef struct mpv_render_context mpv_render_context;

typedef enum mpv_event_id {
    MPV_EVENT_NONE = 0,
    MPV_EVENT_SHUTDOWN = 1,
    MPV_EVENT_LOG_MESSAGE = 2,
    MPV_EVENT_GET_PROPERTY_REPLY = 3,
    MPV_EVENT_SET_PROPERTY_REPLY = 4,
    MPV_EVENT_COMMAND_REPLY = 5,
    MPV_EVENT_START_FILE = 6,
    MPV_EVENT_END_FILE = 7,
    MPV_EVENT_FILE_LOADED = 8,
    MPV_EVENT_IDLE = 11,
    MPV_EVENT_TICK = 14,
    MPV_EVENT_CLIENT_MESSAGE = 16,
    MPV_EVENT_VIDEO_RECONFIG = 17,
    MPV_EVENT_AUDIO_RECONFIG = 18,
    MPV_EVENT_SEEK = 20,
    MPV_EVENT_PLAYBACK_RESTART = 21,
    MPV_EVENT_PROPERTY_CHANGE = 22,
    MPV_EVENT_QUEUE_OVERFLOW = 24
} mpv_event_id;

typedef struct mpv_event_end_file {
    int32_t reason;
    int32_t error;
} mpv_event_end_file;

typedef struct mpv_event {
    uint32_t event_id;
    int error;
    uint64_t reply_userdata;
    void *data;
} mpv_event;

typedef enum mpv_format {
    MPV_FORMAT_NONE = 0,
    MPV_FORMAT_STRING = 1,
    MPV_FORMAT_OSD_STRING = 2,
    MPV_FORMAT_FLAG = 3,
    MPV_FORMAT_INT64 = 4,
    MPV_FORMAT_DOUBLE = 5,
    MPV_FORMAT_NODE = 6,
    MPV_FORMAT_NODE_ARRAY = 7,
    MPV_FORMAT_NODE_MAP = 8,
    MPV_FORMAT_BYTE_ARRAY = 9
} mpv_format;

mpv_handle *mpv_create(void);
int mpv_initialize(mpv_handle *ctx);
int mpv_command(mpv_handle *ctx, const char **args);
int mpv_command_string(mpv_handle *ctx, const char *args);
int mpv_set_option_string(mpv_handle *ctx, const char *name, const char *data);
int mpv_set_property_string(mpv_handle *ctx, const char *name, const char *data);
char *mpv_get_property_string(mpv_handle *ctx, const char *name);
void mpv_free(void *data);
mpv_event *mpv_wait_event(mpv_handle *ctx, double timeout);
void mpv_terminate_destroy(mpv_handle *ctx);
int mpv_request_log_messages(mpv_handle *ctx, const char *min_level);
const char *mpv_error_string(int error);

#ifdef __cplusplus
}
#endif

#endif
