/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Included by app.c: the actual firmware's band renderer, not an external visualizer. */
static uint16_t bw_river_blend(uint16_t a, uint16_t b, unsigned amount)
{
    unsigned inv = 32u - amount;
    return (uint16_t)(((((a >> 11) * inv + (b >> 11) * amount) >> 5) << 11) |
        (((((a >> 5) & 63u) * inv + ((b >> 5) & 63u) * amount) >> 5) << 5) |
        (((a & 31u) * inv + (b & 31u) * amount) >> 5));
}

static void bw_draw_river_scene(uint32_t now, int setup)
{
    const bw_river_state *r = &bw.river;
    unsigned phase = r->phase, line = r->line;
    uint32_t t = r->ms, age = t - r->phase_at;
    float bloom = phase == BW_RIVER_STRIKE || phase == BW_RIVER_REVEAL ? 0.0f : r->bloom;
    if (phase == BW_RIVER_FIGHT)
        bloom *= 0.4f; /* the line, fish movement and tension must remain legible */
    const bw_sheet *sh = &bw_art_river[0];
    unsigned weather = bw_weather(&bw), frame = weather == BW_WEATHER_STORM ? 2u : 1u;
    if (weather == BW_WEATHER_DAY || weather == BW_WEATHER_DUSK)
        frame = 0;
    uint16_t palette[16];
    for (unsigned i = 0; i < 16; i++)
    {
        uint16_t original = sh->px ? sh->pal[i] : C_BG;
        unsigned lum = ((original >> 11) + (original & 31u) + ((original >> 5) & 63u)) / 4u;
        uint16_t tint = r->world == BW_WORLD_UNDERTOW ? RGB(lum * 5, lum * 2, lum * 8) :
                        r->world == BW_WORLD_FIREFLIES ? RGB(lum * 3, lum * 7, lum * 5) :
                        RGB(lum * 5, lum * 3, lum * 8);
        /* Reflections become luminous, while the silhouettes preserve depth and place. */
        palette[i] = bw_river_blend(original, tint, (unsigned)(bloom * 28.0f));
    }
    for (int32_t y = bw_oy; y < bw_oy + 48; y++)
    {
        if (!sh->px)
        {
            R(0, y, 240, 1, C_BG);
            continue;
        }
        float depth = y > 100 ? (float)(y - 100) / 140.0f : 0;
        int32_t warp = (int32_t)(bloom * depth * 13.0f *
            fm_sinf((float)y * 0.10f + (float)t * 0.00035f));
        const uint8_t *px = sh->px + frame * 240u * 120u;
        for (int32_t x = 0; x < 240; x++)
        {
            int32_t sx = clamp(x + warp, 0, 239);
            unsigned n = (unsigned)y * 240u + (unsigned)sx, b = px[n / 2u];
            P(x, y, palette[n & 1u ? b >> 4 : b & 15u]);
        }
    }
    /* Submerged contour ribbons: each layer emerges as the same musical bloom opens its mix and FM depth.
     * Slow refraction and parallax retain the river's horizon. No flashing or per-note colour toggles. */
    unsigned layers = (unsigned)(bloom * 7.0f);
    static int16_t stream_x[7][49], stream_y[7][49];
    for (unsigned j = 0; j < layers; j++)
    {
        float reveal = fm_clampf(bloom * 7.0f - (float)j, 0, 1);
        uint16_t glow = bw_river_blend(RGB(14, 30, 37),
            j & 1u ? RGB(130, 78, 202) : RGB(57, 183, 165), (unsigned)(reveal * 28));
        int32_t prev_x = 0, prev_y = 0;
        for (unsigned q = 0; q <= 48; q++)
        {
            if (bw_oy == 0) /* calculate once per frame, reuse across the five LCD bands */
            {
                float a = (float)q * FM_TWO_PI / 48.0f;
                float drift = (float)t * 0.00012f + (float)j * 0.65f;
                float rad = 18.0f + j * 11.0f + 3.0f * fm_sinf(a * 2.0f + drift);
                stream_x[j][q] = (int16_t)(120 + (int32_t)(fm_cosf(a) * rad * 1.5f));
                stream_y[j][q] = (int16_t)(155 + (int32_t)(fm_sinf(a) * rad * 0.48f +
                    4.0f * fm_sinf(a * 2.0f - drift)));
            }
            int32_t x = stream_x[j][q], y = stream_y[j][q];
            if (q)
            {
                L(prev_x, prev_y + 3, x, y + 3, bw_river_blend(C_BG, glow, 6));
                L(prev_x, prev_y + 2, x, y + 2, bw_river_blend(C_BG, glow, 12));
                L(prev_x, prev_y + 1, x, y + 1, bw_river_blend(C_BG, glow, 20));
                L(prev_x, prev_y, x, y, glow);
            }
            prev_x = x;
            prev_y = y;
        }
    }
    if (bloom > 0.3f)
        for (unsigned j = 0; j < (unsigned)(bloom * 24); j++)
        {
            int32_t x = 20 + (int32_t)(j * 73u % 200u) +
                (int32_t)(fm_sinf((float)t * 0.00021f + j) * bloom * 10.0f);
            int32_t y = 109 + (int32_t)(j * 19u % 100u);
            P(x, y, j & 1 ? RGB(96, 117, 136) : RGB(60, 140, 124));
        }
    int st = bw_fish_state(&bw, line);
    int32_t bx = 143, by = 158 + (int32_t)(fm_sinf((float)t * 0.0014f) * 2.0f);
    if (st != BW_FISH_IDLE && phase < BW_RIVER_STRIKE)
    {
        if (phase == BW_RIVER_APPROACH)
        {
            float approach = fm_clampf((float)age / 12000.0f, 0, 1);
            int32_t radius = 25 - (int32_t)(approach * 18);
            L(bx - radius * 2, by + 5, bx + radius * 2, by + 5, RGB(76, 144, 140));
            L(bx - radius, by + 8, bx + radius, by + 8, RGB(60, 90, 124));
        }
        if (st == BW_FISH_BITE)
            by += 5;
        L(bx - 5, by + 3, bx + 5, by + 3, C_MUTE);
        R(bx, by - 3, 2, 4, C_TXT);
        R(bx, by + 1, 2, 3, C_PINK);
    }
    if (phase == BW_RIVER_STRIKE)
    {
        /* The accumulated refraction resolves into one precise point, then the taut line. */
        unsigned radius = age < 1000 ? 36u - age * 30u / 1000u : 6u;
        for (unsigned j = 0; j < 3; j++)
        {
            int32_t rad = (int32_t)radius + (int32_t)j * 9;
            L(bx - rad, by + (int32_t)j * 4, bx + rad, by + (int32_t)j * 4, j ? C_MUTE : C_TXT);
        }
        L(205, 229, 180, 74, C_TXT);
        L(180, 74, bx, by, C_YEL);
        TS(66, 38, &FONT_M, "FISH ON!", C_TXT);
        return;
    }
    if (phase == BW_RIVER_FIGHT)
    {
        float tension = bw.track[line].manual ? bw.fight_tension[line] : r->pressure * 100;
        int running = bw.track[line].manual ? bw.fight_run[line] > 0 : r->pressure > 0.55f;
        int32_t fx = 120 + (int32_t)(fm_sinf((float)t * (running ? 0.004f : 0.001f)) * (running ? 38 : 12));
        int32_t fy = 156 + (int32_t)(r->pressure * 15);
        L(204, 228, 177, 70 + (int32_t)(tension * 0.2f), C_TXT);
        L(177, 70 + (int32_t)(tension * 0.2f), fx, fy, C_YEL);
        L(fx - 18, fy + 3, fx + 18, fy + 3, C_TXT);
        L(fx - 10, fy + 7, fx + 10, fy + 7, C_MUTE);
        TS(12, 16, &FONT_B, running ? "LET IT RUN" : "REEL GENTLY", C_TXT);
        R(16, 218, 208, 4, C_BAR);
        R(16, 218, (int32_t)(208.0f * fm_clampf(tension / 100.0f, 0, 1)), 4,
            tension > 78 ? C_PINK : C_YEL);
        T(16, 228, &FONT_T, "TENSION                 SELECT / K1", C_TXT);
        return;
    }
    if (phase == BW_RIVER_REVEAL)
    {
        const bw_event *ev = &bw.event;
        R(12, 74, 216, 123, RGB(6, 14, 20));
        if (ev->result == BW_FIGHT_LANDED)
        {
            bw_draw_fish_at(70, 89, ev->species, 10, 0, 2);
            TR(221, 147, &FONT_B, bw_species_names[ev->species], ev->rare ? C_GOLD : C_TXT);
            char length[24] = "", weight[24] = "";
            bw_length_text(length, ev->mm);
            bw_weight_text(weight, ev->g);
            T(22, 169, &FONT_XS, length, C_MUTE);
            TR(218, 169, &FONT_XS, weight, C_MUTE);
            TS(34, 203, &FONT_XS, "THE RIVER REMEMBERS", C_TXT);
        }
        else
        {
            TS(26, 108, &FONT_B, ev->result == BW_FIGHT_SNAPPED ? "THE LINE GAVE WAY" : "BACK INTO THE DEEP", C_TXT);
            T(32, 146, &FONT_XS, "THE WATER SETTLES", C_MUTE);
        }
        return;
    }
    /* Controls appear when invited; idle returns to an unobstructed river. */
    if (!bw.playing || now - bw_ui.last_input < 4500u || setup || st == BW_FISH_BITE)
    {
        TS(10, 12, &FONT_XS, bw_world_names[r->world], C_TXT);
        TR(230, 12, &FONT_XS, bw.playing ? "LISTENING" : "PLAY TO ENTER", C_TXT);
        if (st == BW_FISH_BITE)
            TS(63, 199, &FONT_B, "BITE - REC", C_YEL);
        else if (setup)
        {
            bw_knob_cell kc[4];
            const bw_track *tr = &bw.track[bw_ui.track];
            TS(10, 176, &FONT_XS, "REC: PLACE / LIFT LINE", C_TXT);
            bw_kcell(&kc[0], "MIN S", tr->wait_min, 600, 0);
            bw_kcell(&kc[1], "MAX S", tr->wait_max, 600, 0);
            bw_kcell(&kc[2], "MODE", tr->manual, 1, tr->manual ? "FISH" : "LISTEN");
            bw_kcell(&kc[3], "BAIT", tr->bait, BW_NBAIT - 1, bw_bait_names[tr->bait]);
            bw_knob_bar(kc, C_YEL);
        }
        else
        {
            T(10, 213, &FONT_T, "K1 CURRENT  K2 BED  K3 LIGHT  K4 DISTANCE", C_TXT);
            if ((int32_t)(bw_ui.message_until - now) > 0)
                T(10, 227, &FONT_T, bw_ui.message, C_YEL);
            else
                T(10, 227, &FONT_T, "PRESET WORLD  SEQ LINE  REC PLACE / LIFT", C_MUTE);
        }
    }
}
