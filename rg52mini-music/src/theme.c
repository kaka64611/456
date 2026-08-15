#include "theme.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Theme *theme_default(void) {
    Theme *t = (Theme *)calloc(1, sizeof(Theme));
    if (!t) return NULL;
    
    // AiMusic dark blue style
    t->bg_r = 10;  t->bg_g = 22;  t->bg_b = 40;
    t->panel_r = 20; t->panel_g = 45; t->panel_b = 75;
    t->accent_r = 0; t->accent_g = 212; t->accent_b = 255;
    t->text_r = 255; t->text_g = 255; t->text_b = 255;
    t->text_dim_r = 160; t->text_dim_g = 180; t->text_dim_b = 200;
    t->selected_r = 0; t->selected_g = 100; t->selected_b = 150;
    
    t->top_bar_height = 90;
    t->bottom_bar_height = 50;
    t->left_panel_width = 420;
    
    strncpy(t->name, "AiMusic Dark Blue", sizeof(t->name) - 1);
    
    return t;
}

Theme *theme_load(const char *dir) {
    // For now, just return default theme
    // TODO: parse theme config file
    (void)dir;
    return theme_default();
}

void theme_free(Theme *t) {
    if (t) free(t);
}
