/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "bw_engine.c"
static bw_engine bw;
typedef struct
{
    uint32_t magic, version, seed;
    uint16_t tempo;
    bw_track track[4];
} bw_saved;
typedef char bw_save_must_fit[(sizeof(bw_saved) <= PLAT_STORE_MAX) ? 1 : -1];
static struct
{
    uint32_t buttons, keys, last_draw, message_until;
    uint8_t track, page, op, edit_page, route_source, route_dest;
    int8_t octave;
    uint8_t dirty, draw;
    char message[32];
    int16_t preview[4];
} bw_ui;
enum
{
    BW_RIVER,
    BW_FISHING,
    BW_EDIT,
    BW_ENV,
    BW_GLOBAL,
    BW_STATS
};
static const char *const bw_pages[] = {"RIVER",    "FISHING", "FM EDIT",
                                       "ENVELOPE", "GLOBAL",  "CATCH LOG"};
static const char *const bw_names[] = {"BLUEGILL", "LARGEMOUTH BASS", "BROWN TROUT", "CATFISH"};
static uint16_t bw_color(unsigned k)
{
    static const uint16_t c[] = {RGB(97, 173, 118), RGB(226, 150, 94), RGB(87, 164, 203),
                                 RGB(174, 150, 193)};
    return c[k % 4];
}
static void bw_text_number(char *p, int v)
{
    char d[12];
    unsigned n = 0;
    uint32_t x = v < 0 ? 0u - (uint32_t)v : (uint32_t)v;
    if (v < 0)
        *p++ = '-';
    do
    {
        d[n++] = (char)('0' + x % 10);
        x /= 10;
    } while (x);
    while (n)
        *p++ = d[--n];
    *p = 0;
}
static void bw_text_copy(char *out, const char *s, unsigned n)
{
    unsigned i = 0;
    while (s && s[i] && i + 1 < n)
    {
        out[i] = s[i];
        i++;
    }
    out[i] = 0;
}
void app_say(const char *a, const char *b)
{
    bw_text_copy(bw_ui.message, a, sizeof bw_ui.message);
    if (b && *b)
    {
        unsigned n = 0;
        while (bw_ui.message[n])
            n++;
        if (n + 2 < sizeof bw_ui.message)
        {
            bw_ui.message[n++] = ' ';
            bw_text_copy(bw_ui.message + n, b, sizeof bw_ui.message - n);
        }
    }
    bw_ui.draw = 1;
    bw_ui.message_until = plat_ms() + 1500u;
    if (ann_on())
        ann_say(bw_ui.message, 0, ANN_NOW);
}
static int bw_saved_valid(const bw_saved *s)
{
    if (s->magic != 0x42573031u || s->version != 1 || s->tempo < 30 || s->tempo > 240 || !s->seed)
        return 0;
    for (unsigned k = 0; k < 4; k++)
        if (!bw_track_valid(&s->track[k]))
            return 0;
    return 1;
}
static void bw_load(void)
{
    bw_saved s;
    if (plat_store_load(OBJ_SESSION, &s, sizeof s) == sizeof s && bw_saved_valid(&s))
    {
        for (unsigned k = 0; k < 4; k++)
            bw.track[k] = s.track[k];
        bw.tempo = s.tempo;
        bw.rng = s.seed;
    }
}
void app_init(void)
{
    bw_init(&bw);
    bw_load();
    /* Simulated reboots call app_init again in the same process. Reset all UI state. */
    unsigned char *p = (unsigned char *)&bw_ui;
    for (unsigned i = 0; i < sizeof bw_ui; i++)
        p[i] = 0;
    for (unsigned k = 0; k < 4; k++)
        bw_ui.preview[k] = -1;
    bw_ui.draw = 1;
}
static void bw_save(void)
{
    bw_saved s;
    memset(&s, 0, sizeof s);
    uint32_t lock = plat_lock();
    s.magic = 0x42573031u;
    s.version = 1;
    s.tempo = bw.tempo;
    s.seed = bw.rng;
    for (unsigned k = 0; k < 4; k++)
        s.track[k] = bw.track[k];
    plat_unlock(lock);
    if (plat_store_save(OBJ_SESSION, &s, sizeof s) == 0)
    {
        lock = plat_lock();
        bw_ui.dirty = memcmp(bw.track, s.track, sizeof s.track) != 0 || bw.tempo != s.tempo ||
                      bw.rng != s.seed;
        plat_unlock(lock);
        app_say("SAVED", "");
    }
    else
        app_say("SAVE FAILED", "");
}
void app_panic(void)
{
    bw_transport(&bw, 0);
    for (unsigned k = 0; k < 4; k++)
        bw.voice[k].active = 0;
}
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t pkt;
    int port;
    while (plat_midi_in(&pkt, &port))
    {
        unsigned status = (pkt >> 8) & 255, note = (pkt >> 16) & 127, velocity = (pkt >> 24) & 127;
        unsigned k = status & 15;
        if (k < 4 && !bw.playing)
        {
            if ((status & 240) == 144 && velocity)
            {
                bw_note_on(&bw, k, note, velocity);
                bw_ui.preview[k] = (int16_t)note;
            }
            else if (((status & 240) == 128 || ((status & 240) == 144 && !velocity)) &&
                     bw_ui.preview[k] == (int)note)
            {
                bw_note_off(&bw, k);
                bw_ui.preview[k] = -1;
            }
        }
    }
    uint32_t before = 0, after = 0;
    for (unsigned k = 0; k < 4; k++)
        before += bw.track[k].catches;
    bw_render(&bw, lr, n, master_q12);
    for (unsigned k = 0; k < 4; k++)
        after += bw.track[k].catches;
    if (before != after)
        bw_ui.dirty = 1;
}
static void bw_knob(unsigned knob, int d)
{
    bw_track *t = &bw.track[bw_ui.track];
    bw_operator *o = &t->op[bw_ui.op];
    if (bw_ui.page == BW_FISHING)
    {
        if (knob == 0)
        {
            t->wait_min = (uint16_t)clamp(t->wait_min + d, 0, 600);
            if (t->wait_max < t->wait_min)
                t->wait_max = t->wait_min;
        }
        if (knob == 1)
        {
            t->wait_max = (uint16_t)clamp(t->wait_max + d, t->wait_min, 600);
        }
        if (knob == 2)
            t->manual = (uint8_t)clamp(t->manual + d, 0, 1);
        if (knob == 3)
            t->variation = (uint8_t)clamp(t->variation + d, 1, 16);
        if (bw.hooked[bw_ui.track])
            bw_cast(&bw, bw_ui.track);
    }
    else if (bw_ui.page == BW_EDIT)
    {
        if (bw_ui.edit_page == 0)
        {
            if (knob == 0)
                o->ratio = (uint16_t)clamp(o->ratio + d * 5, 1, 1600);
            if (knob == 1)
                o->level = (uint8_t)clamp(o->level + d, 0, 100);
            if (knob == 2)
                o->feedback = (uint8_t)clamp(o->feedback + d, 0, 100);
            if (knob == 3)
                bw_algorithm(t, (unsigned)clamp(t->algorithm + d, 0, 7));
        }
        else if (bw_ui.edit_page == 1)
        {
            if (knob == 0)
                o->fixed = (uint8_t)clamp(o->fixed + d, 0, 1);
            if (knob == 1)
                o->hz = (uint16_t)clamp(o->hz + d * 10, 1, 16000);
            if (knob == 2)
                o->velocity = (uint8_t)clamp(o->velocity + d, 0, 100);
            if (knob == 3)
                t->carrier[bw_ui.op] = (uint8_t)clamp(t->carrier[bw_ui.op] + d, 0, 100);
        }
        else
        {
            if (knob == 0)
                bw_ui.route_source = (uint8_t)clamp(bw_ui.route_source + d, 1, 5);
            if (bw_ui.route_dest >= bw_ui.route_source)
                bw_ui.route_dest = bw_ui.route_source - 1;
            if (knob == 1)
                bw_ui.route_dest = (uint8_t)clamp(bw_ui.route_dest + d, 0, bw_ui.route_source - 1);
            if (knob == 2)
            {
                uint8_t *r = &t->route[bw_ui.route_source][bw_ui.route_dest];
                *r = (uint8_t)clamp(*r + d, 0, 100);
            }
            if (knob == 3)
                t->carrier[bw_ui.route_source] =
                    (uint8_t)clamp(t->carrier[bw_ui.route_source] + d, 0, 100);
        }
    }
    else if (bw_ui.page == BW_ENV)
    {
        if (knob == 0)
            o->attack = (uint16_t)clamp(o->attack + d * 5, 0, 10000);
        if (knob == 1)
            o->decay = (uint16_t)clamp(o->decay + d * 5, 0, 10000);
        if (knob == 2)
            o->sustain = (uint8_t)clamp(o->sustain + d, 0, 100);
        if (knob == 3)
            o->release = (uint16_t)clamp(o->release + d * 5, 0, 10000);
    }
    else if (bw_ui.page == BW_GLOBAL)
    {
        if (knob == 0)
            bw.tempo = (uint16_t)clamp(bw.tempo + d, 30, 240);
        if (knob == 1)
            t->root = (uint8_t)clamp(t->root + d, 0, 96);
        if (knob == 2)
            t->volume = (uint8_t)clamp(t->volume + d, 0, 100);
    }
    else if (bw_ui.page == BW_RIVER)
    {
        if (knob == 0)
            t->volume = (uint8_t)clamp(t->volume + d, 0, 100);
        if (knob == 1)
            t->root = (uint8_t)clamp(t->root + d, 0, 96);
    }
    bw_ui.dirty = 1;
}
static void bw_input(void)
{
    uint32_t buttons = plat_buttons(), keys = plat_keys(), pressed = buttons & ~bw_ui.buttons;
    uint32_t lock = plat_lock();
    int e;
    int turn[NE];
    if (pressed || keys != bw_ui.keys)
        bw_ui.draw = 1;
    for (unsigned k = 0; k < NE; k++)
    {
        turn[k] = plat_enc((int)k);
        if (turn[k])
            bw_ui.draw = 1;
    }
    if ((e = turn[EN_ALGO]))
        bw_ui.track = (uint8_t)clamp(bw_ui.track + e, 0, 3);
    if ((e = turn[EN_SELECT]))
    {
        if (bw_ui.page == BW_EDIT || bw_ui.page == BW_ENV)
            bw_ui.op = (uint8_t)clamp(bw_ui.op + e, 0, 5);
        else
            bw_ui.page = (uint8_t)clamp(bw_ui.page + e, 0, BW_STATS);
    }
    if ((e = turn[EN_PRESET]) && bw_ui.page == BW_EDIT)
        bw_ui.edit_page = (uint8_t)clamp(bw_ui.edit_page + e, 0, 2);
    for (unsigned k = 0; k < 4; k++)
        if ((e = turn[EN_K1 + k]))
            bw_knob(k, e);
    if (pressed & (1u << B_HOME))
        bw_ui.page = BW_RIVER;
    if (pressed & (1u << B_SEQ))
        bw_ui.page = BW_FISHING;
    if (pressed & (1u << B_EDIT))
        bw_ui.page = BW_EDIT;
    if (pressed & (1u << B_ENV))
        bw_ui.page = BW_ENV;
    if (pressed & (1u << B_GLO))
        bw_ui.page = BW_GLOBAL;
    if (pressed & (1u << B_SEL))
        bw_ui.page = BW_STATS;
    if (pressed & ((1u << B_FX) | (1u << B_LFO) | (1u << B_ARP)))
        app_say("NOT ASSIGNED", "");
    if (pressed & (1u << B_PLAY))
    {
        bw_transport(&bw, !bw.playing);
        for (unsigned k = 0; k < 4; k++)
            bw_ui.preview[k] = -1;
    }
    if (pressed & (1u << B_REC))
    {
        if (bw.bite[bw_ui.track])
        {
            if (bw_land(&bw, bw_ui.track))
                bw_ui.dirty = 1;
        }
        else if (bw.hooked[bw_ui.track])
            bw_retrieve(&bw, bw_ui.track);
        else
            bw_cast(&bw, bw_ui.track);
    }
    if (pressed & (1u << B_OCTUP))
        bw_ui.octave = (int8_t)clamp(bw_ui.octave + 1, -2, 2);
    if (pressed & (1u << B_OCTDN))
        bw_ui.octave = (int8_t)clamp(bw_ui.octave - 1, -2, 2);
    if (!bw.playing)
        for (unsigned k = 0; k < 27; k++)
        {
            int note = clamp(53 + (int)k + bw_ui.octave * 12, 0, 127);
            if ((keys & (1u << k)) && !(bw_ui.keys & (1u << k)))
            {
                bw_note_on(&bw, bw_ui.track, (unsigned)note, 100);
                bw_ui.preview[bw_ui.track] = (int16_t)note;
            }
            if (!(keys & (1u << k)) && (bw_ui.keys & (1u << k)))
                for (unsigned t = 0; t < 4; t++)
                    if (bw_ui.preview[t] == note)
                    {
                        bw_note_off(&bw, t);
                        bw_ui.preview[t] = -1;
                    }
        }
    bw_ui.buttons = buttons;
    bw_ui.keys = keys;
    plat_unlock(lock);
    if (pressed & (1u << B_SAVE))
        bw_save();
}
static void bw_draw_fish(int x, int y, unsigned k, int selected)
{
    /* Original coarse pixel silhouette; species artwork awaits the user's cast. */
    uint16_t c = bw_color(k);
    if (!selected)
        c = RGB(50, 80, 85);
    cv_rect(x + 8, y + 4, 24, 12, c);
    cv_rect(x + 12, y, 12, 20, c);
    cv_rect(x, y + 2, 4, 16, c);
    cv_rect(x + 4, y + 5, 4, 10, c);
    cv_rect(x + 30, y + 7, 8, 6, c);
    cv_rect(x + 28, y + 5, 3, 3, C_BLACK);
    cv_rect(x + 16, y - 3, 8, 3, c);
}
static void bw_label(int x, int y, const char *label, int value)
{
    char number[12];
    bw_text_number(number, value);
    cv_text(x, y, &FONT_XS, label, C_GRAY);
    cv_text(x, y + 15, &FONT_B, number, C_WHITE);
}
static void bw_draw(void)
{
    bw_track t;
    uint32_t catches[4], best[4], last[4], missed;
    uint8_t hooks[4], bites[4], step, playing;
    unsigned tempo, remaining, op = bw_ui.op;
    uint32_t lock = plat_lock();
    t = bw.track[bw_ui.track];
    step = bw.step;
    playing = bw.playing;
    tempo = bw.tempo;
    for (unsigned k = 0; k < 4; k++)
    {
        hooks[k] = bw.hooked[k];
        bites[k] = bw.bite[k];
        catches[k] = bw.track[k].catches;
        best[k] = bw.track[k].best_mm;
        last[k] = bw.track[k].last_mm;
    }
    int32_t delta = (int32_t)(bw.deadline[bw_ui.track] - bw.clock);
    remaining = delta > 0 ? (unsigned)delta / BW_RATE : 0;
    missed = bw.missed[bw_ui.track];
    plat_unlock(lock);
    const bw_operator *o = &t.op[op];
    char number[12];
    for (unsigned band = 0; band < 5; band++)
    {
        cv_begin(240, 48, RGB(5, 16, 23));
        if (band == 0)
        {
            cv_text(7, 2, &FONT_B, APP_NAME, C_WHITE);
            cv_text(7, 22, &FONT_XS,
                    bw_ui.message[0] && (int32_t)(bw_ui.message_until - plat_ms()) > 0
                        ? bw_ui.message
                        : bw_pages[bw_ui.page],
                    bw_color(bw_ui.track));
            cv_text(159, 2, &FONT_XS, playing ? "PLAYING" : "STOPPED", C_GRAY);
            bw_text_number(number, (int)bw_ui.track + 1);
            cv_text(184, 22, &FONT_B, number, C_WHITE);
            if (bw_ui.dirty)
                cv_text(217, 2, &FONT_XS, "*", C_HI);
        }
        else if (bw_ui.page == BW_RIVER)
        {
            unsigned k = band - 1;
            uint16_t c = bw_color(k);
            for (int x = 0; x < 240; x += 12)
                cv_rect(x, 39, 8, 2, RGB(18, 47, 58));
            bw_draw_fish(13, 12, k, k == bw_ui.track);
            cv_text(62, 1, &FONT_XS, bw_names[k], k == bw_ui.track ? C_WHITE : C_GRAY);
            cv_text(62, 20, &FONT_XS,
                    bites[k]   ? "BITE! REC TO LAND"
                    : hooks[k] ? "HOOK PLACED"
                               : "REC TO CAST",
                    c);
            if (hooks[k])
            {
                cv_rect(52, 0, 1, 23, C_GRAY);
                cv_rect(48, 22, 5, 2, C_WHITE);
            }
        }
        else if (bw_ui.page == BW_FISHING)
        {
            if (band == 1)
            {
                bw_draw_fish(15, 15, bw_ui.track, 1);
                cv_text(64, 8, &FONT_B,
                        bites[bw_ui.track]   ? "BITE!"
                        : hooks[bw_ui.track] ? "WAITING"
                                             : "NO HOOK",
                        C_WHITE);
                bw_label(162, 0, "SECONDS", (int)remaining);
            }
            if (band == 2)
            {
                bw_label(8, 0, "K1 MIN SEC", t.wait_min);
                bw_label(128, 0, "K2 MAX SEC", t.wait_max);
            }
            if (band == 3)
            {
                cv_text(8, 0, &FONT_XS, "K3 MODE", C_GRAY);
                cv_text(8, 17, &FONT_B, t.manual ? "TIMED" : "AUTO", C_WHITE);
                bw_label(128, 0, "K4 VARIATION", t.variation);
            }
            if (band == 4)
            {
                cv_text(8, 0, &FONT_XS, "REC CAST / RETRIEVE / LAND", C_WHITE);
                cv_text(8, 21, &FONT_XS, "TIMED BITE: 2 SECOND WINDOW", C_GRAY);
            }
        }
        else if (bw_ui.page == BW_STATS)
        {
            unsigned k = band - 1;
            cv_text(7, 0, &FONT_XS, bw_names[k], bw_color(k));
            bw_label(7, 16, "COUNT", (int)catches[k]);
            bw_label(85, 16, "BEST MM", (int)best[k]);
            bw_label(164, 16, "LAST MM", (int)last[k]);
        }
        else if (bw_ui.page == BW_GLOBAL)
        {
            if (band == 1)
            {
                bw_label(8, 0, "K1 TEMPO", (int)tempo);
                bw_label(128, 0, "K2 ROOT MIDI", t.root);
            }
            if (band == 2)
            {
                bw_label(8, 0, "K3 VOLUME", t.volume);
                bw_label(128, 0, "MISSED BITES", (int)missed);
            }
            if (band == 3)
                cv_text(8, 8, &FONT_XS, "ALGO: TRACK  /  SEL: STATS", C_WHITE);
            if (band == 4)
                cv_text(8, 8, &FONT_XS, "SAVE: SESSION + CATCH LOG", C_GRAY);
        }
        else
        {
            if (band == 1)
            {
                bw_label(8, 0, "SELECT OPERATOR", (int)op + 1);
                bw_label(150, 0, "PAGE", bw_ui.edit_page + 1);
            }
            if (bw_ui.page == BW_ENV)
            {
                if (band == 2)
                {
                    bw_label(8, 0, "K1 ATTACK MS", o->attack);
                    bw_label(128, 0, "K2 DECAY MS", o->decay);
                }
                if (band == 3)
                {
                    bw_label(8, 0, "K3 SUSTAIN %", o->sustain);
                    bw_label(128, 0, "K4 RELEASE MS", o->release);
                }
            }
            else if (bw_ui.edit_page == 0)
            {
                if (band == 2)
                {
                    bw_label(8, 0, "K1 RATIO /100", o->ratio);
                    bw_label(128, 0, "K2 LEVEL %", o->level);
                }
                if (band == 3)
                {
                    bw_label(8, 0, "K3 FEEDBACK", o->feedback);
                    bw_label(128, 0, "K4 ALGORITHM", t.algorithm + 1);
                }
            }
            else if (bw_ui.edit_page == 1)
            {
                if (band == 2)
                {
                    bw_label(8, 0, "K1 FIXED MODE", o->fixed);
                    bw_label(128, 0, "K2 FIXED HZ", o->hz);
                }
                if (band == 3)
                {
                    bw_label(8, 0, "K3 VELOCITY %", o->velocity);
                    bw_label(128, 0, "K4 OUTPUT %", t.carrier[op]);
                }
            }
            else
            {
                if (band == 2)
                {
                    bw_label(8, 0, "K1 SOURCE OP", bw_ui.route_source + 1);
                    bw_label(128, 0, "K2 DEST OP", bw_ui.route_dest + 1);
                }
                if (band == 3)
                {
                    bw_label(8, 0, "K3 MODULATION", t.route[bw_ui.route_source][bw_ui.route_dest]);
                    bw_label(128, 0, "K4 SOURCE OUT", t.carrier[bw_ui.route_source]);
                }
            }
            if (band == 4)
            {
                cv_text(8, 0, &FONT_XS, "PRESETS: EDIT PAGE 1-3", C_GRAY);
                cv_text(8, 20, &FONT_XS, "STOP + KEYS: AUDITION PATCH", C_WHITE);
            }
        }
        lcd_blit(0, band * 48, 240, 48, cv_px);
    }
    uint32_t leds = (playing ? 1u << B_PLAY : 0) | (bw_ui.dirty ? 1u << B_SAVE : 0) |
                    (bites[bw_ui.track] ? 1u << B_REC : 0);
    plat_leds(leds, playing ? 1u << step : 0);
    bw_ui.last_draw = plat_ms();
    bw_ui.draw = 0;
}
void app_ui_init(void)
{
    bw_ui.route_source = 1;
    bw_ui.draw = 1;
    bw_draw();
}
void app_input_only(void)
{
    bw_input();
}
void app_frame(void)
{
    bw_input();
    if (bw_ui.draw || plat_ms() - bw_ui.last_draw >= 100)
        bw_draw();
    if (ann_on() && ann_where())
        ann_say(bw_pages[bw_ui.page], 0, ANN_NOW);
    ann_tick(plat_ms());
}
#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    unsigned k = bw_ui.track;
    const struct
    {
        const char *name;
        int value;
    } checks[] = {{"track", (int)k},
                  {"page", bw_ui.page},
                  {"playing", bw.playing},
                  {"hooked", bw.hooked[k]},
                  {"bite", bw.bite[k]},
                  {"catches", (int)bw.track[k].catches},
                  {"best_mm", (int)bw.track[k].best_mm},
                  {"last_mm", (int)bw.track[k].last_mm},
                  {"manual", bw.track[k].manual},
                  {"missed", (int)bw.missed[k]},
                  {"wait_min", bw.track[k].wait_min},
                  {"wait_max", bw.track[k].wait_max},
                  {"level", bw.track[k].op[bw_ui.op].level},
                  {"dirty", bw_ui.dirty},
                  {"valid", bw_track_valid(&bw.track[k])},
                  {"operator", bw_ui.op}};
    for (unsigned i = 0; i < sizeof checks / sizeof checks[0]; i++)
        if (strcmp(what, checks[i].name) == 0)
        {
            *got = checks[i].value;
            return 1;
        }
    return 0;
}

#endif /* PLAT_HOST: simulator assertions are not part of the device image. */
