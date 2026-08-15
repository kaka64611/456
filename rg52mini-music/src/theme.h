#ifndef THEME_H
#define THEME_H

#include <SDL2/SDL.h>

typedef struct Theme {
    // Colors
    Uint8 bg_r, bg_g, bg_b;
    Uint8 panel_r, panel_g, panel_b;
    Uint8 accent_r, accent_g, accent_b;
    Uint8 text_r, text_g, text_b;
    Uint8 text_dim_r, text_dim_g, text_dim_b;
    Uint8 selected_r, selected_g, selected_b;
    
    // Layout
    int top_bar_height;
    int bottom_bar_height;
    int left_panel_width; // playlist width
    
    char name[64];
} Theme;

// Load theme from directory
Theme *theme_load(const char *dir);

// Create default theme (AiMusic dark blue style)
Theme *theme_default(void);

// Get theme by index (0 to theme_get_count()-1)
Theme *theme_get_by_index(int index);

// Get total number of themes
int theme_get_count(void);

// Free theme
void theme_free(Theme *t);

#endif
