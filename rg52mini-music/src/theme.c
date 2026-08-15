#include "theme.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_THEMES 5

static Theme themes[MAX_THEMES];
static int themes_loaded = 0;

static void load_themes(void) {
    // 主题0: AiMusic 深蓝
    themes[0].bg_r = 10;  themes[0].bg_g = 22;  themes[0].bg_b = 40;
    themes[0].panel_r = 20; themes[0].panel_g = 45; themes[0].panel_b = 75;
    themes[0].accent_r = 0; themes[0].accent_g = 212; themes[0].accent_b = 255;
    themes[0].text_r = 255; themes[0].text_g = 255; themes[0].text_b = 255;
    themes[0].text_dim_r = 160; themes[0].text_dim_g = 180; themes[0].text_dim_b = 200;
    themes[0].selected_r = 0; themes[0].selected_g = 100; themes[0].selected_b = 150;
    themes[0].top_bar_height = 90;
    themes[0].bottom_bar_height = 50;
    themes[0].left_panel_width = 420;
    strncpy(themes[0].name, "AiMusic Deep Blue", sizeof(themes[0].name) - 1);

    // 主题1: 暗夜紫
    themes[1].bg_r = 18;  themes[1].bg_g = 12;  themes[1].bg_b = 35;
    themes[1].panel_r = 35; themes[1].panel_g = 25; themes[1].panel_b = 60;
    themes[1].accent_r = 180; themes[1].accent_g = 100; themes[1].accent_b = 255;
    themes[1].text_r = 240; themes[1].text_g = 230; themes[1].text_b = 255;
    themes[1].text_dim_r = 150; themes[1].text_dim_g = 130; themes[1].text_dim_b = 180;
    themes[1].selected_r = 80; themes[1].selected_g = 40; themes[1].selected_b = 120;
    themes[1].top_bar_height = 90;
    themes[1].bottom_bar_height = 50;
    themes[1].left_panel_width = 420;
    strncpy(themes[1].name, "Night Purple", sizeof(themes[1].name) - 1);

    // 主题2: 森林绿
    themes[2].bg_r = 10;  themes[2].bg_g = 30;  themes[2].bg_b = 20;
    themes[2].panel_r = 20; themes[2].panel_g = 55; themes[2].panel_b = 35;
    themes[2].accent_r = 0; themes[2].accent_g = 230; themes[2].accent_b = 120;
    themes[2].text_r = 230; themes[2].text_g = 255; themes[2].text_b = 235;
    themes[2].text_dim_r = 140; themes[2].text_dim_g = 180; themes[2].text_dim_b = 150;
    themes[2].selected_r = 0; themes[2].selected_g = 120; themes[2].selected_b = 60;
    themes[2].top_bar_height = 90;
    themes[2].bottom_bar_height = 50;
    themes[2].left_panel_width = 420;
    strncpy(themes[2].name, "Forest Green", sizeof(themes[2].name) - 1);

    // 主题3: 日落橙
    themes[3].bg_r = 35;  themes[3].bg_g = 18;  themes[3].bg_b = 10;
    themes[3].panel_r = 60; themes[3].panel_g = 35; themes[3].panel_b = 20;
    themes[3].accent_r = 255; themes[3].accent_g = 140; themes[3].accent_b = 50;
    themes[3].text_r = 255; themes[3].text_g = 235; themes[3].text_b = 210;
    themes[3].text_dim_r = 190; themes[3].text_dim_g = 150; themes[3].text_dim_b = 110;
    themes[3].selected_r = 140; themes[3].selected_g = 70; themes[3].selected_b = 20;
    themes[3].top_bar_height = 90;
    themes[3].bottom_bar_height = 50;
    themes[3].left_panel_width = 420;
    strncpy(themes[3].name, "Sunset Orange", sizeof(themes[3].name) - 1);

    // 主题4: 简约白
    themes[4].bg_r = 240;  themes[4].bg_g = 240;  themes[4].bg_b = 245;
    themes[4].panel_r = 255; themes[4].panel_g = 255; themes[4].panel_b = 255;
    themes[4].accent_r = 60; themes[4].accent_g = 120; themes[4].accent_b = 220;
    themes[4].text_r = 30; themes[4].text_g = 30; themes[4].text_b = 40;
    themes[4].text_dim_r = 120; themes[4].text_dim_g = 120; themes[4].text_dim_b = 130;
    themes[4].selected_r = 220; themes[4].selected_g = 230; themes[4].selected_b = 250;
    themes[4].top_bar_height = 90;
    themes[4].bottom_bar_height = 50;
    themes[4].left_panel_width = 420;
    strncpy(themes[4].name, "Simple White", sizeof(themes[4].name) - 1);

    themes_loaded = 1;
}

Theme *theme_default(void) {
    if (!themes_loaded) load_themes();
    Theme *t = (Theme *)calloc(1, sizeof(Theme));
    if (t) memcpy(t, &themes[0], sizeof(Theme));
    return t;
}

Theme *theme_get_by_index(int index) {
    if (!themes_loaded) load_themes();
    if (index < 0) index = MAX_THEMES - 1;
    if (index >= MAX_THEMES) index = 0;
    Theme *t = (Theme *)calloc(1, sizeof(Theme));
    if (t) memcpy(t, &themes[index], sizeof(Theme));
    return t;
}

int theme_get_count(void) {
    if (!themes_loaded) load_themes();
    return MAX_THEMES;
}

Theme *theme_load(const char *dir) {
    (void)dir;
    return theme_default();
}

void theme_free(Theme *t) {
    if (t) free(t);
}
