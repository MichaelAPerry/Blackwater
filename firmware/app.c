/* SPDX-License-Identifier: GPL-3.0-or-later */
/* BLACKWATER's application: controls, screens, saves, the "fish on" clip. The engine is bw_engine.c. */
#include "bw_engine.h"
#include "fastmath.h"
#include "bw_clip.c"
#include "bw_art.h"
static bw_engine bw;
static bw_clip_state bw_fish_on;

/* ------------------------------------------------------------------ saved objects */
#define BW_MAGIC 0x42573032u /* "BW02" */
#define BW_VERSION 2u
typedef struct
{
    uint32_t magic, version, seed;
    bw_song song;
    bw_track track[4];
} bw_saved;
typedef struct
{
    uint32_t magic, version;
    bw_patch user[4][4];
} bw_saved_patches;
typedef struct
{
    uint32_t magic, version;
    bw_log log;
} bw_saved_log;
typedef struct
{
    uint32_t magic, version;
    uint8_t units, clip_on, clip_level, autosave, slot, midi_out, pad[2];
} bw_prefs;
typedef char bw_save_fits[(sizeof(bw_saved) <= PLAT_STORE_MAX) ? 1 : -1];
typedef char bw_patches_fit[(sizeof(bw_saved_patches) <= PLAT_STORE_MAX) ? 1 : -1];
static const uint32_t bw_slot_obj[4] = {OBJ_SESSION, OBJ_SLOT2, OBJ_SLOT3, OBJ_SLOT4};
static bw_saved_patches bw_user;
static bw_prefs bw_pref;

/* ------------------------------------------------------------------ pages */
enum
{
    BW_RIVER,
    BW_FISHING,
    BW_THEME,
    BW_LOG,
    BW_SCALE,
    BW_EDIT,
    BW_ENV,
    BW_LFO,
    BW_FX,
    BW_ARP,
    BW_GLOBAL,
    BW_HELP,
    BW_NPAGES
};
static const char *const bw_pages[BW_NPAGES] = {"RIVER", "FISHING", "THEME", "CATCH LOG", "SCALE", "FM EDIT",
                                                "ENVELOPE", "LFO", "EFFECTS", "ARP", "GLOBAL", "HELP"};
static const uint8_t bw_subpages[BW_NPAGES] = {1, 1, 2, 2, 1, 4, 1, 1, 4, 1, 3, 2};
static struct
{
    uint32_t buttons, keys, last_draw, message_until, popup_until, last_input, event_seq, fight_ms;
    uint32_t fish_on_seen, fish_on_until, reel_turns;
    uint8_t track, page, sub[BW_NPAGES], op, route_source, route_dest, patch_pick, slot_pick;
    uint8_t dirty, draw, held;
    int8_t preview[4];
    char message[32];
    bw_event popup;
} bw_ui;
static uint16_t bw_color(unsigned k)
{
    static const uint16_t c[] = {RGB(110, 200, 140), RGB(240, 160, 90), RGB(100, 180, 235), RGB(190, 160, 220)};
    return c[k % 4];
}
/* BLACKWATER's own palette (docs/ART_PROMPTS.md): river water, reeds, lantern light, the bobber, bone */
#define C_BG RGB(6, 14, 12)       /* deep water */
#define C_PANEL RGB(14, 30, 27)   /* shallows: the selected row, panels */
#define C_BAR RGB(30, 50, 44)     /* slack line */
#define C_YEL RGB(232, 170, 70)   /* lantern amber: global values, the time */
#define C_PINK RGB(222, 78, 58)   /* bobber red: alerts, recording */
#define C_GOLD RGB(240, 204, 96)  /* trophy: rare fish */
#define C_TXT RGB(222, 214, 194)  /* bone white */
#define C_MUTE RGB(116, 136, 114) /* reed green: labels */

/* ------------------------------------------------------------------ text */
static void bw_cat(char *out, unsigned n, const char *s)
{
    unsigned i = 0;
    while (out[i] && i + 1 < n)
        i++;
    while (*s && i + 1 < n)
        out[i++] = *s++;
    out[i] = 0;
}
static void bw_num(char *p, int v)
{
    char t[12];
    unsigned n = 0, neg = v < 0;
    unsigned u = neg ? (unsigned)(-v) : (unsigned)v;
    do
    {
        t[n++] = (char)('0' + u % 10);
        u /= 10;
    } while (u && n < sizeof t);
    if (neg)
        *p++ = '-';
    while (n)
        *p++ = t[--n];
    *p = 0;
}
static void bw_dec1(char *p, uint32_t tenths)
{
    bw_num(p, (int)(tenths / 10));
    char d[3] = {'.', (char)('0' + tenths % 10), 0};
    bw_cat(p, 12, d);
}
static void bw_length_text(char *out, uint32_t mm)
{
    out[0] = 0;
    if (bw_pref.units)
    {
        bw_dec1(out, (mm * 100u + 127u) / 254u); /* tenths of an inch */
        bw_cat(out, 16, " IN");
    }
    else
    {
        bw_dec1(out, mm); /* millimetres are tenths of a centimetre */
        bw_cat(out, 16, " CM");
    }
}
static void bw_weight_text(char *out, uint32_t g)
{
    out[0] = 0;
    if (bw_pref.units)
    {
        bw_dec1(out, (g * 10u + 226u) / 454u);
        bw_cat(out, 16, " LB");
    }
    else if (g < 1000)
    {
        bw_num(out, (int)g);
        bw_cat(out, 16, " G");
    }
    else
    {
        bw_dec1(out, (g + 50u) / 100u);
        bw_cat(out, 16, " KG");
    }
}
static void bw_text_copy(char *out, const char *s, unsigned n)
{
    unsigned i = 0;
    while (s[i] && i + 1 < n)
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
        bw_cat(bw_ui.message, sizeof bw_ui.message, " ");
        bw_cat(bw_ui.message, sizeof bw_ui.message, b);
    }
    bw_ui.draw = 1;
    bw_ui.message_until = plat_ms() + 1800u;
    if (ann_on())
        ann_say(bw_ui.message, 0, ANN_NOW);
}

/* ------------------------------------------------------------------ hooks the engine calls */
void bw_midi_send(uint32_t pkt)
{
    plat_midi_send(pkt, TO_USB | TO_BLE);
}
uint32_t bw_midi_room(void)
{
    return plat_midi_out_room(TO_USB);
}
static uint32_t bw_clip_plays;
static volatile uint32_t bw_fish_on_seq; /* a rare fish took the line: the FISH ON! moment */
static volatile uint8_t bw_fish_on_track;
void bw_on_rare(unsigned k)
{
    bw_fish_on_track = (uint8_t)(k & 3u);
    bw_fish_on_seq++;
    if (bw_pref.clip_on && bw_clip_present() && !bw_fish_on.active)
        bw_clip_plays++;
    if (bw_pref.clip_on)
        bw_clip_trigger(&bw_fish_on);
}

/* ------------------------------------------------------------------ storage */
static void bw_prefs_default(void)
{
    memset(&bw_pref, 0, sizeof bw_pref);
    bw_pref.magic = BW_MAGIC;
    bw_pref.version = BW_VERSION;
    bw_pref.clip_on = 1;
    bw_pref.clip_level = 80;
    bw_pref.autosave = 1;
    bw_pref.midi_out = 1;
}
static int bw_saved_valid(const bw_saved *s)
{
    if (s->magic != BW_MAGIC || s->version != BW_VERSION || !s->seed || !bw_song_valid(&s->song))
        return 0;
    for (unsigned k = 0; k < 4; k++)
        if (!bw_track_valid(&s->track[k]))
            return 0;
    return 1;
}
static int bw_load_slot(unsigned slot)
{
    static bw_saved s;
    if (plat_store_load(bw_slot_obj[slot % 4], &s, sizeof s) == (int)sizeof s && bw_saved_valid(&s))
    {
        uint32_t lock = plat_lock();
        bw_all_notes_off(&bw);
        bw.song = s.song;
        for (unsigned k = 0; k < 4; k++)
        {
            bw.track[k] = s.track[k];
            bw.step[k] = 0;
            bw.tcount[k] = 0;
        }
        bw.rng = s.seed;
        plat_unlock(lock);
        return 1;
    }
    return 0;
}
static void bw_load_all(void)
{
    static bw_saved_log l;
    if (plat_store_load(OBJ_PREFS, &bw_pref, sizeof bw_pref) != (int)sizeof bw_pref || bw_pref.magic != BW_MAGIC ||
        bw_pref.version != BW_VERSION || bw_pref.slot > 3 || bw_pref.units > 1 || bw_pref.clip_on > 1 ||
        bw_pref.clip_level > 100 || bw_pref.autosave > 1 || bw_pref.midi_out > 1)
        bw_prefs_default();
    if (plat_store_load(OBJ_PATCHES, &bw_user, sizeof bw_user) != (int)sizeof bw_user || bw_user.magic != BW_MAGIC)
    {
        memset(&bw_user, 0, sizeof bw_user);
        bw_user.magic = BW_MAGIC;
        bw_user.version = BW_VERSION;
        for (unsigned k = 0; k < 4; k++)
            for (unsigned i = 0; i < 4; i++)
                bw_patch_factory(&bw_user.user[k][i], k, i);
    }
    if (plat_store_load(OBJ_LOG, &l, sizeof l) == (int)sizeof l && l.magic == BW_MAGIC && l.version == BW_VERSION)
        bw.log = l.log;
    bw_load_slot(bw_pref.slot);
    bw.midi_out = bw_pref.midi_out;
}
static int bw_save_all(unsigned slot)
{
    static bw_saved s;
    static bw_saved_log l;
    memset(&s, 0, sizeof s);
    uint32_t lock = plat_lock();
    s.magic = BW_MAGIC;
    s.version = BW_VERSION;
    s.seed = bw.rng ? bw.rng : 1u;
    s.song = bw.song;
    for (unsigned k = 0; k < 4; k++)
        s.track[k] = bw.track[k];
    l.magic = BW_MAGIC;
    l.version = BW_VERSION;
    l.log = bw.log;
    plat_unlock(lock);
    bw_pref.slot = (uint8_t)(slot % 4);
    int r = plat_store_save(bw_slot_obj[slot % 4], &s, sizeof s);
    r |= plat_store_save(OBJ_LOG, &l, sizeof l);
    r |= plat_store_save(OBJ_PREFS, &bw_pref, sizeof bw_pref);
    if (r == 0)
        bw_ui.dirty = 0;
    return r;
}
static void bw_save(void)
{
    char t[16] = "SLOT ";
    bw_num(t + 5, bw_pref.slot + 1);
    if (bw_save_all(bw_pref.slot) == 0)
        app_say("SAVED", t);
    else
        app_say("SAVE FAILED", "");
}
int app_backup_playing(void)
{
    return bw.playing;
}
void app_backup_flush(void)
{
    if (bw_ui.dirty)
        bw_save_all(bw_pref.slot);
}
void app_backup_hold(int on)
{
    bw_ui.held = (uint8_t)(on != 0);
}
void app_backup_restored(void)
{
    uint32_t lock = plat_lock();
    bw_init(&bw);
    plat_unlock(lock);
    bw_load_all();
    bw_ui.dirty = 0;
    bw_ui.draw = 1;
}

/* ------------------------------------------------------------------ init and render */
void app_init(void)
{
    bw_init(&bw);
    bw_load_all();
    /* simulated reboots call app_init again in the same process: all UI state starts over */
    memset(&bw_ui, 0, sizeof bw_ui);
    for (unsigned k = 0; k < 4; k++)
        bw_ui.preview[k] = -1;
    memset(&bw_fish_on, 0, sizeof bw_fish_on);
    bw_clip_plays = 0;
    bw_fish_on_seq = 0;
    bw_ui.route_source = 1;
    bw_ui.draw = 1;
    bw_ui.event_seq = bw.event.seq;
}
void app_panic(void)
{
    bw_transport(&bw, 0);
    for (unsigned i = 0; i < BW_VOICES; i++)
        bw.voice[i].active = 0;
    bw_fish_on.active = 0;
}
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t pkt;
    int port;
    while (plat_midi_in(&pkt, &port))
    {
        unsigned status = (pkt >> 8) & 255, note = (pkt >> 16) & 127, velocity = (pkt >> 24) & 127;
        unsigned k = status & 15;
        if (status >= 0xF8)
        {
            bw_midi_clock(&bw, status);
            continue;
        }
        if (k < 4 && (status & 240) == 144 && velocity)
            bw_live_note(&bw, k, note, 1, velocity);
        else if (k < 4 && ((status & 240) == 128 || (status & 240) == 144))
            bw_live_note(&bw, k, note, 0, 0);
    }
    uint32_t seq = bw.event.seq;
    bw_render(&bw, lr, n, master_q12);
    if (bw.event.seq != seq)
        bw_ui.dirty = 1;
    bw_clip_mix(&bw_fish_on, lr, n, master_q12 * bw_pref.clip_level / 100u);
}

/* ------------------------------------------------------------------ controls */
static int bw_step_val(int v, int d, int lo, int hi)
{
    return clamp(v + d, lo, hi);
}
static void bw_patch_store(unsigned k, unsigned slot)
{
    bw_user.user[k][slot % 4] = bw.track[k].patch;
    if (plat_store_save(OBJ_PATCHES, &bw_user, sizeof bw_user) == 0)
        app_say("PATCH STORED", "");
    else
        app_say("STORE FAILED", "");
}
static void bw_knob(unsigned knob, int d)
{
    unsigned k = bw_ui.track, sub = bw_ui.sub[bw_ui.page];
    bw_track *t = &bw.track[k];
    bw_song *s = &bw.song;
    bw_patch *p = &t->patch;
    bw_operator *o = &p->op[bw_ui.op];
    switch (bw_ui.page)
    {
    case BW_RIVER:
        if (knob == 0)
            t->volume = (uint8_t)bw_step_val(t->volume, d, 0, 100);
        if (knob == 1)
            t->pan = (uint8_t)bw_step_val(t->pan, d, 0, 100);
        if (knob == 2)
            t->mute = (uint8_t)bw_step_val(t->mute, d, 0, 1);
        if (knob == 3)
            t->solo = (uint8_t)bw_step_val(t->solo, d, 0, 1);
        break;
    case BW_FISHING:
        if (knob == 0)
        {
            t->wait_min = (uint16_t)bw_step_val(t->wait_min, d, 0, 600);
            if (t->wait_max < t->wait_min)
                t->wait_max = t->wait_min;
        }
        if (knob == 1)
            t->wait_max = (uint16_t)bw_step_val(t->wait_max, d, t->wait_min, 600);
        if (knob == 2)
            t->manual = (uint8_t)bw_step_val(t->manual, d, 0, 1);
        if (knob == 3)
            t->bait = (uint8_t)bw_step_val(t->bait, d, 0, BW_NBAIT - 1);
        if (knob < 2 && bw_fish_state(&bw, k) == BW_FISH_WAIT)
            bw_cast(&bw, k); /* new waits start now */
        break;
    case BW_THEME:
        if (sub == 0)
        {
            if (knob == 0)
            {
                unsigned len = t->cur.len, nl = (unsigned)bw_step_val((int)len, d, 4, BW_STEPS);
                for (unsigned i = len; i < nl; i++) /* new steps repeat the start */
                    t->cur.s[i] = t->cur.s[i - len];
                t->cur.len = (uint8_t)nl;
            }
            if (knob == 1)
                t->density = (uint8_t)bw_step_val(t->density, d, 0, 100);
            if (knob == 2)
                t->octave = (int8_t)bw_step_val(t->octave, d, -3, 3);
        }
        else if (d > 0)
        {
            if (knob == 0)
            {
                if (t->has_prev)
                {
                    bw_release(&bw, k);
                    app_say("RELEASED", "THEME RESTORED");
                }
                else
                    app_say("NOTHING TO RELEASE", "");
            }
            if (knob == 1)
            {
                bw_restore_home(&bw, k);
                app_say("HOME THEME", "");
            }
            if (knob == 2)
            {
                bw_make_home(&bw, k);
                app_say("NEW HOME THEME", "");
            }
            if (knob == 3)
            {
                bw.rec_arm[k] = 1;
                bw.rec_left[k] = t->cur.len;
                app_say("REC ARMED", "PLAY WHITE KEYS");
            }
        }
        else if (knob == 3)
            bw.rec_arm[k] = 0;
        break;
    case BW_SCALE:
        if (knob == 0)
            s->key = (uint8_t)((s->key + 12 + (d > 0 ? 1 : -1)) % 12);
        if (knob == 1)
            s->scale = (uint8_t)bw_step_val(s->scale, d, 0, BW_SCALES - 1);
        if (knob == 2)
            s->progression = (uint8_t)bw_step_val(s->progression, d, 0, BW_PROGRESSIONS - 1);
        if (knob == 3)
            s->swing = (uint8_t)bw_step_val(s->swing, d, 0, 60);
        break;
    case BW_EDIT:
        if (sub == 0)
        {
            if (knob == 0)
                o->ratio = (uint16_t)bw_step_val(o->ratio, d * 5, 5, 1600);
            if (knob == 1)
                o->level = (uint8_t)bw_step_val(o->level, d, 0, 100);
            if (knob == 2)
                o->feedback = (uint8_t)bw_step_val(o->feedback, d, 0, 100);
            if (knob == 3)
                bw_algorithm(p, (unsigned)bw_step_val(p->algorithm, d, 0, 7));
        }
        else if (sub == 1)
        {
            if (knob == 0)
                o->fixed = (uint8_t)bw_step_val(o->fixed, d, 0, 1);
            if (knob == 1)
                o->hz = (uint16_t)bw_step_val(o->hz, d * 10, 1, 16000);
            if (knob == 2)
                o->velocity = (uint8_t)bw_step_val(o->velocity, d, 0, 100);
            if (knob == 3)
                p->carrier[bw_ui.op] = (uint8_t)bw_step_val(p->carrier[bw_ui.op], d, 0, 100);
        }
        else if (sub == 2)
        {
            if (knob == 0)
                bw_ui.route_source = (uint8_t)bw_step_val(bw_ui.route_source, d, 1, 5);
            if (bw_ui.route_dest >= bw_ui.route_source)
                bw_ui.route_dest = (uint8_t)(bw_ui.route_source - 1);
            if (knob == 1)
                bw_ui.route_dest = (uint8_t)bw_step_val(bw_ui.route_dest, d, 0, bw_ui.route_source - 1);
            if (knob == 2)
            {
                uint8_t *r = &p->route[bw_ui.route_source][bw_ui.route_dest];
                *r = (uint8_t)bw_step_val(*r, d, 0, 100);
            }
            if (knob == 3)
                p->carrier[bw_ui.route_source] = (uint8_t)bw_step_val(p->carrier[bw_ui.route_source], d, 0, 100);
        }
        else
        {
            if (knob == 0)
                bw_ui.patch_pick = (uint8_t)bw_step_val(bw_ui.patch_pick, d, 0, 7);
            if (knob == 1 && d > 0)
            {
                if (bw_ui.patch_pick < 4)
                    bw_patch_factory(p, k, bw_ui.patch_pick);
                else
                    *p = bw_user.user[k][bw_ui.patch_pick - 4];
                t->patch_slot = bw_ui.patch_pick;
                app_say("PATCH LOADED", "");
            }
            if (knob == 2 && d > 0)
            {
                if (bw_ui.patch_pick >= 4)
                {
                    bw_patch_store(k, bw_ui.patch_pick - 4u);
                }
                else
                    app_say("PICK A USER SLOT", "U1-U4");
            }
        }
        break;
    case BW_ENV:
        if (knob == 0)
            o->attack = (uint16_t)bw_step_val(o->attack, d * 5, 0, 10000);
        if (knob == 1)
            o->decay = (uint16_t)bw_step_val(o->decay, d * 5, 0, 10000);
        if (knob == 2)
            o->sustain = (uint8_t)bw_step_val(o->sustain, d, 0, 100);
        if (knob == 3)
            o->release = (uint16_t)bw_step_val(o->release, d * 5, 0, 10000);
        break;
    case BW_LFO:
        if (knob == 0)
            t->lfo_rate = (uint8_t)bw_step_val(t->lfo_rate, d, 0, 100);
        if (knob == 1)
            t->lfo_depth = (uint8_t)bw_step_val(t->lfo_depth, d, 0, 100);
        if (knob == 2)
            t->lfo_target = (uint8_t)bw_step_val(t->lfo_target, d, 0, BW_NLFO_TARGET - 1);
        if (knob == 3)
            t->lfo_shape = (uint8_t)bw_step_val(t->lfo_shape, d, 0, BW_NLFO_SHAPE - 1);
        break;
    case BW_FX:
    {
        uint8_t *v[4][4] = {{&t->send_rev, &t->send_dly, &t->send_cho, &t->drive},
                            {&s->rev_size, &s->rev_tone, &s->rev_level, &s->dly_time},
                            {&s->dly_feedback, &s->dly_level, &s->cho_rate, &s->cho_depth},
                            {&s->tone, &s->width, 0, 0}};
        if (v[sub][knob])
            *v[sub][knob] = (uint8_t)bw_step_val(*v[sub][knob], d, 0, 100);
        break;
    }
    case BW_ARP:
        if (knob == 0)
            s->arp_mode = (uint8_t)bw_step_val(s->arp_mode, d, 0, BW_NARP_MODE - 1);
        if (knob == 1)
            s->arp_rate = (uint8_t)bw_step_val(s->arp_rate, d, 0, 4);
        if (knob == 2)
            s->arp_octaves = (uint8_t)bw_step_val(s->arp_octaves, d, 1, 3);
        if (knob == 3)
            s->arp_gate = (uint8_t)bw_step_val(s->arp_gate, d * 5, 5, 100);
        break;
    case BW_GLOBAL:
        if (sub == 0)
        {
            if (knob == 0)
                s->tempo = (uint16_t)bw_step_val(s->tempo, d, 30, 240);
            if (knob == 1)
                s->sync = (uint8_t)bw_step_val(s->sync, d, 0, 1);
            if (knob == 2)
                s->weather = (uint8_t)bw_step_val(s->weather, d, 0, BW_NWEATHER - 1);
            if (knob == 3)
                s->pb_event = (uint8_t)bw_step_val(s->pb_event, d, 0, 1);
        }
        else if (sub == 1)
        {
            if (knob == 0)
                bw_pref.units = (uint8_t)bw_step_val(bw_pref.units, d, 0, 1);
            if (knob == 1)
                bw_pref.clip_on = (uint8_t)bw_step_val(bw_pref.clip_on, d, 0, 1);
            if (knob == 2)
                bw_pref.clip_level = (uint8_t)bw_step_val(bw_pref.clip_level, d * 5, 0, 100);
            if (knob == 3)
                bw_pref.autosave = (uint8_t)bw_step_val(bw_pref.autosave, d, 0, 1);
        }
        else
        {
            if (knob == 0)
                bw_ui.slot_pick = (uint8_t)bw_step_val(bw_ui.slot_pick, d, 0, 3);
            if (knob == 1 && d > 0)
            {
                if (bw_load_slot(bw_ui.slot_pick))
                {
                    bw_pref.slot = bw_ui.slot_pick;
                    app_say("LOADED", "");
                }
                else
                    app_say("EMPTY SLOT", "");
            }
            if (knob == 2 && d > 0)
            {
                bw_pref.slot = bw_ui.slot_pick;
                bw_save();
            }
            if (knob == 3)
            {
                bw_pref.midi_out = (uint8_t)bw_step_val(bw_pref.midi_out, d, 0, 1);
                bw.midi_out = bw_pref.midi_out;
            }
        }
        break;
    default:
        break;
    }
    bw_ui.dirty = 1;
}
/* knobs whose actions touch storage run outside the lock */
static int bw_knob_stores(unsigned knob, int d)
{
    unsigned sub = bw_ui.sub[bw_ui.page];
    return d > 0 && ((bw_ui.page == BW_EDIT && sub == 3 && knob == 2) ||
                     (bw_ui.page == BW_GLOBAL && sub == 2 && (knob == 1 || knob == 2)));
}
static void bw_page(unsigned page)
{
    /* the same button again: its next sub-page (HOME: river <-> catch log; SEQ: fishing <-> theme) */
    if (page == BW_RIVER && bw_ui.page == BW_RIVER)
        page = BW_LOG;
    else if (page == BW_FISHING && bw_ui.page == BW_FISHING)
        page = BW_THEME;
    else if (page == bw_ui.page && bw_subpages[page] > 1)
        bw_ui.sub[page] = (uint8_t)((bw_ui.sub[page] + 1) % bw_subpages[page]);
    bw_ui.page = (uint8_t)page;
}
static void bw_input(void)
{
    static const uint8_t white[16] = PLAT_WHITE_KEYS, black[11] = PLAT_BLACK_KEYS;
    uint32_t now = plat_ms();
    uint32_t buttons = plat_buttons(), keys = plat_keys(), pressed = buttons & ~bw_ui.buttons;
    int turn[NE], e;
    if (bw_ui.held)
        return;
    for (unsigned k = 0; k < NE; k++)
        turn[k] = plat_enc((int)k);
    if (pressed || keys != bw_ui.keys)
        bw_ui.draw = 1;
    for (unsigned k = 0; k < NE; k++)
        if (turn[k])
            bw_ui.draw = 1;
    if (pressed || keys != bw_ui.keys || turn[0] || turn[1] || turn[2] || turn[3] || turn[4] || turn[5] || turn[6])
        bw_ui.last_input = now;
    unsigned k = bw_ui.track;
    int fighting = bw_fish_state(&bw, k) == BW_FISH_FIGHT;
    uint32_t lock = plat_lock();
    if ((e = turn[EN_ALGO]) && !fighting)
    {
        /* the held macros and live notes stay with the track they started on */
        bw_ui.track = (uint8_t)clamp(bw_ui.track + e, 0, 3);
        bw.live_track = bw_ui.track;
    }
    if (fighting)
    {
        /* the fight: SELECT or K1 reels in */
        unsigned dt = now - bw_ui.fight_ms;
        bw_ui.fight_ms = now;
        int r = bw_fight(&bw, k, turn[EN_SELECT] + turn[EN_K1], dt);
        if (turn[EN_SELECT] + turn[EN_K1] > 0)
            bw_ui.reel_turns += (uint32_t)(turn[EN_SELECT] + turn[EN_K1]);
        if (r != BW_FIGHT_ON)
            bw_ui.draw = 1;
        turn[EN_SELECT] = turn[EN_K1] = 0;
    }
    if ((e = turn[EN_SELECT]))
    {
        /* on EDIT and ENVELOPE it picks the operator; past the sixth (or before the first) it goes on paging */
        int op = (int)bw_ui.op + e;
        if ((bw_ui.page == BW_EDIT || bw_ui.page == BW_ENV) && op >= 0 && op <= 5)
            bw_ui.op = (uint8_t)op;
        else
            bw_ui.page = (uint8_t)clamp(bw_ui.page + (e > 0 ? 1 : -1), 0, BW_NPAGES - 1);
    }
    if ((e = turn[EN_PRESET]))
    {
        if (bw_ui.page == BW_FISHING)
        {
            bw.track[k].depth = (uint8_t)clamp(bw.track[k].depth + e, 0, BW_NDEPTH - 1);
            bw_ui.dirty = 1;
        }
        else if (bw_subpages[bw_ui.page] > 1)
            bw_ui.sub[bw_ui.page] = (uint8_t)clamp(bw_ui.sub[bw_ui.page] + e, 0, bw_subpages[bw_ui.page] - 1);
    }
    int store_knob = -1, store_d = 0;
    for (unsigned i = 0; i < 4; i++)
        if ((e = turn[EN_K1 + i]))
        {
            if (bw_knob_stores(i, e))
                store_knob = (int)i, store_d = e;
            else
                bw_knob(i, e);
        }
    static const uint8_t page_of[NB] = {BW_FX, BW_SCALE, BW_ENV, BW_LFO, BW_EDIT, BW_GLOBAL, BW_RIVER, 0xFF, BW_ARP,
                                        BW_FISHING, 0xFF, 0xFF, 0xFF, 0xFF};
    for (unsigned b = 0; b < NB; b++)
        if ((pressed >> b & 1) && page_of[b] != 0xFF && !fighting)
        {
            if (b == B_ARP && bw_ui.page == BW_ARP)
            {
                bw.arp = (uint8_t)((bw.arp + 1) % 3);
                if (!bw.arp)
                {
                    for (unsigned i = 0; i < 16; i++)
                        bw.arp_held[i] = -1;
                    if (bw.live_note[k] >= 0)
                        bw_live_note(&bw, k, (unsigned)bw.live_note[k], 0, 0);
                    bw.live_note[k] = -1;
                }
                static const char *const arp_state[3] = {"ARP OFF", "ARP ON", "ARP LATCH"};
                app_say(arp_state[bw.arp], "");
            }
            else
                bw_page(page_of[b]);
        }
    if (pressed & (1u << B_PLAY))
        bw_transport(&bw, !bw.playing);
    if (pressed & (1u << B_REC))
    {
        int st = bw_fish_state(&bw, k);
        if (st == BW_FISH_BITE)
        {
            if (bw_hookset(&bw, k))
            {
                bw_ui.page = BW_FISHING;
                bw_ui.fight_ms = now;
                app_say(bw.fish_rare[k] ? "FISH ON!" : "HOOK SET", "REEL WITH SELECT");
            }
        }
        else if (st == BW_FISH_WAIT)
            bw_retrieve(&bw, k);
        else if (st == BW_FISH_IDLE)
            bw_cast(&bw, k);
    }
    if (pressed & (1u << B_OCTUP))
        bw.live_oct = (int8_t)clamp(bw.live_oct + 1, -2, 2);
    if (pressed & (1u << B_OCTDN))
        bw.live_oct = (int8_t)clamp(bw.live_oct - 1, -2, 2);
    /* the keys: white ones play the fish in the scale (or the arpeggiator), black ones bend its FM */
    uint32_t changed = keys ^ bw_ui.keys;
    for (unsigned i = 0; i < 16; i++)
        if (changed >> white[i] & 1)
            bw_live_key(&bw, i, (int)(keys >> white[i] & 1), 100);
    for (unsigned i = 0; i < 11; i++)
        if (changed >> black[i] & 1)
            bw_macro(&bw, k, i, (int)(keys >> black[i] & 1));
    bw_ui.buttons = buttons;
    bw_ui.keys = keys;
    plat_unlock(lock);
    if (store_knob >= 0)
        bw_knob((unsigned)store_knob, store_d);
    if (pressed & (1u << B_SAVE))
        bw_save();
    /* autosave: stopped, changed, and nothing touched for ten seconds (a flash write pauses the sound) */
    if (bw_pref.autosave && bw_ui.dirty && !bw.playing && now - bw_ui.last_input > 10000u)
    {
        bw_save_all(bw_pref.slot);
        bw_ui.last_input = now;
    }
}

/* ------------------------------------------------------------------ drawing, in screen coordinates; each band
 * draws what falls in it (the canvas is one 48-row band) */
static int32_t bw_oy;
static int32_t bw_ymax = 240; /* nothing below this row (the FISH ON! wave's wipe) */
static int bw_in(int32_t y, int32_t h)
{
    return y + h > bw_oy && y < bw_oy + 48 && y < bw_ymax;
}
static void R(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    if (y + h > bw_ymax)
        h = bw_ymax - y;
    if (h > 0 && bw_in(y, h))
        cv_rect(x, y - bw_oy, w, h, c);
}
static int32_t T(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    if (bw_in(y, (int32_t)f->h) && y + (int32_t)f->h <= bw_ymax)
        return cv_text(x, y - bw_oy, f, s, c);
    return x + text_w(f, s);
}
static int32_t TS(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    T(x + 1, y + 1, f, s, C_BLACK); /* a shadow: readable over the river */
    return T(x, y, f, s, c);
}
static void TR(int32_t xr, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    T(xr - text_w(f, s), y, f, s, c);
}
static void L(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    if (bw_in(y0 < y1 ? y0 : y1, (y0 < y1 ? y1 - y0 : y0 - y1) + 1))
        cv_line(x0, y0 - bw_oy, x1, y1 - bw_oy, c);
}
static void P(int32_t x, int32_t y, uint16_t c)
{
    if (bw_in(y, 1))
        cv_pset(x, y - bw_oy, c);
}
static void bw_sheet_draw2(const bw_sheet *sh, unsigned frame, int32_t x, int32_t y, int32_t scale, uint16_t tint)
{
    if (!sh->px || !bw_in(y, sh->h * scale))
        return;
    frame %= (unsigned)(sh->cols * sh->rows);
    const uint8_t *px = sh->px + frame * ((uint32_t)sh->w * sh->h / 2u);
    for (unsigned j = 0; j < sh->h; j++)
    {
        int32_t yy = y + (int32_t)j * scale;
        if (!bw_in(yy, scale))
            continue;
        for (unsigned i = 0; i < sh->w; i++)
        {
            unsigned n = j * sh->w + i, b = px[n / 2], c = n & 1 ? b >> 4 : b & 15u;
            if (c == 0 && sh->key)
                continue;
            uint16_t col = tint ? tint : sh->pal[c];
            if (scale == 1)
                P(x + (int32_t)i, yy, col);
            else
                R(x + (int32_t)i * scale, yy, scale, scale, col);
        }
    }
}
static void bw_sheet_draw(const bw_sheet *sh, unsigned frame, int32_t x, int32_t y, int flip, uint16_t tint)
{
    (void)flip;
    bw_sheet_draw2(sh, frame, x, y, 1, tint);
}
/* a prop (docs/ART_PROMPTS.md 8): 0 hook, 1 bobber, 2 tilted, 3 under, 4-7 splash, 8-10 ripples, 11 worm,
 * 12 crankbait, 13 spinner, 14 ruler, 15 trophy, 16 lantern, 17 bubble */
static int bw_prop(unsigned cell, int32_t x, int32_t y)
{
    if (!bw_art_props[0].px)
        return 0;
    bw_sheet_draw(&bw_art_props[0], cell, x, y, 0, 0);
    return 1;
}
/* the fish, until its art arrives: a pixel shape per kind, coloured per species */
/* frames (prompts 2-5): 0-3 swimming, 4-5 idle, 6 taking the bait, 7-9 fighting, 10 landed, 11 silhouette;
 * dim: the silhouette. A rare fish without its own sheet borrows its cousin's, with a gold glint */
static void bw_draw_fish_at(int32_t x, int32_t y, unsigned sp, unsigned frame, int dim, int32_t scale)
{
    const bw_sheet *sh = &bw_art_fish[sp % BW_SPECIES];
    int borrowed = 0;
    if (!sh->px && sp >= 4 && bw_art_fish[sp - 4].px)
        sh = &bw_art_fish[sp - 4], borrowed = 1;
    if (sh->px)
    {
        bw_sheet_draw2(sh, dim ? 11u : frame, x, y, scale, 0);
        if (borrowed && !dim)
            R(x + 20 * scale, y + 2 * scale, 2 * scale, 2 * scale, C_GOLD);
        return;
    }
    (void)scale;
    static const uint16_t body[8] = {RGB(90, 150, 120), RGB(110, 140, 70),  RGB(190, 150, 80),  RGB(110, 120, 135),
                                     RGB(150, 160, 150), RGB(210, 190, 60), RGB(80, 120, 90),   RGB(150, 120, 80)};
    static const uint16_t mark[8] = {RGB(240, 150, 50), RGB(40, 60, 30),   RGB(200, 40, 40),   RGB(220, 220, 230),
                                     RGB(40, 50, 40),   RGB(220, 70, 40),  RGB(230, 120, 60),  RGB(70, 50, 30)};
    uint16_t c = dim ? RGB(35, 55, 70) : body[sp % 8], m = dim ? c : mark[sp % 8];
    unsigned kind = sp % 4, sw = frame & 1;
    int32_t h = kind == BW_BLUEGILL ? 18 : kind == BW_CATFISH ? 10 : 12, cy = y + 12;
    R(x + 10, cy - h / 2, 28, h, c);                          /* body */
    R(x + 6, cy - h / 2 + 2, 4, h - 4, c);
    R(x + 38, cy - h / 2 + 2, 6, h - 4, c);                   /* head */
    R(x, cy - 7 + (int32_t)sw, 6, 14 - 2 * (int32_t)sw, c);   /* tail */
    R(x + 16, cy - h / 2 - 3, 12, 3, c);                      /* dorsal */
    R(x + 40, cy - 2, 2, 2, C_BLACK);                         /* eye */
    if (kind == BW_BLUEGILL)
        R(x + 34, cy - 2, 3, 4, m);                           /* ear flap / crappie speckle */
    else if (kind == BW_BASS)
        R(x + 10, cy, 30, 2, m);                              /* lateral stripe / peacock bars */
    else if (kind == BW_TROUT)
    {
        R(x + 14, cy - 3, 2, 2, m);
        R(x + 22, cy + 1, 2, 2, m);
        R(x + 30, cy - 2, 2, 2, m);
    }
    else
    {
        R(x + 44, cy + 1, 4, 1, m);                           /* barbels */
        R(x + 44, cy + 3, 3, 1, m);
    }
    if (sp >= 4 && !dim)
        R(x + 20, cy - h / 2 - 5, 2, 2, C_GOLD);              /* the rare one's glint */
}
static void bw_draw_fish(int32_t x, int32_t y, unsigned sp, unsigned frame, int dim)
{
    bw_draw_fish_at(x, y, sp, frame, dim, 1);
}
static void bw_bar(int32_t x, int32_t y, int32_t w, int value, int max, uint16_t c)
{
    int32_t f = max > 0 ? (int32_t)((int64_t)w * clamp(value, 0, max) / max) : 0;
    int32_t bx = x + f - 2 < x ? x : x + f - 2;
    R(x, y + 2, w, 1, C_BAR);   /* the line */
    R(x, y + 1, f, 3, c);       /* reeled in so far */
    R(bx, y - 2, 4, 4, C_PINK); /* the bobber: red over white */
    R(bx, y + 2, 4, 3, C_TXT);
}
/* a parameter: label, value, bar (MIKU-FM's mixer rows) */
static void bw_param(int32_t x, int32_t y, const char *label, const char *value, int v, int max, uint16_t c)
{
    /* label above, value below it on the right, the line under both: a column 108 wide */
    T(x, y, &FONT_XS, label, C_MUTE);
    TR(x + 108, y + 13, &FONT_B, value, C_TXT);
    if (max > 0)
        bw_bar(x, y + 32, 108, v, max, c);
}
static void bw_param_n(int32_t x, int32_t y, const char *label, int v, int max, uint16_t c)
{
    char t[12];
    bw_num(t, v);
    bw_param(x, y, label, t, v, max, c);
}
static int bw_kit(const bw_sheet *sh, unsigned cell, int32_t x, int32_t y, int32_t scale)
{
    if (!sh->px)
        return 0;
    bw_sheet_draw2(sh, cell, x, y, scale, 0);
    return 1;
}
static unsigned bw_weather_icon(void) /* the icons: day, dusk, night, storm */
{
    static const uint8_t cell[BW_NWEATHER] = {0, 1, 2, 3, 0};
    return cell[bw_weather(&bw)];
}
static void bw_header(const char *title, uint16_t c, const char *right)
{
    char t[40];
    R(0, 0, 240, 48, C_BG);
    int32_t tx = 6;
    if (bw_ui.page == BW_RIVER && bw_kit(&bw_art_logo[0], 0, 2, 3, 1))
        tx = -1; /* the wordmark is the title */
    else if (bw_kit(&bw_art_icons[0], bw_ui.page, 4, 0, 2))
        tx = 42;
    if (tx >= 0)
        T(tx, 1, &FONT_M, title, c);
    TR(234, 4, &FONT_XS, bw_species_names[bw_ui.track], bw_color(bw_ui.track));
    t[0] = 0;
    bw_cat(t, sizeof t, "BPM ");
    bw_num(t + 4, bw.song.tempo);
    bw_cat(t, sizeof t, "  ");
    bw_cat(t, sizeof t, bw_key_names[bw.song.key]);
    bw_cat(t, sizeof t, " ");
    bw_cat(t, sizeof t, bw_scale_names[bw.song.scale]);
    int32_t lx = tx > 6 ? tx : 6;
    if (tx < 0) /* RIVER: the wordmark fills the left; the state on the right only */
        ;
    else if (bw_ui.message[0] && (int32_t)(bw_ui.message_until - plat_ms()) > 0)
        T(lx, 29, &FONT_XS, bw_ui.message, C_PINK);
    else
        T(lx, 29, &FONT_XS, t, C_YEL);
    TR(234, 29, &FONT_XS, right, bw.playing ? C_YEL : C_MUTE);
    if (!(bw_ui.message[0] && (int32_t)(bw_ui.message_until - plat_ms()) > 0)) /* (a message needs the room) */
        bw_kit(&bw_art_weather[0], bw_weather_icon(), 234 - text_w(&FONT_XS, right) - 20, 27, 1);
    if (bw_ui.dirty)
        T(226, 16, &FONT_XS, "*", C_PINK);
    for (int32_t x = 0; x < 240; x += 8) /* the waterline */
        R(x + (int32_t)((plat_ms() / 400u) % 8u), 46, 5, 1, C_BAR);
}
static const char *bw_state_text(unsigned k)
{
    switch (bw_fish_state(&bw, k))
    {
    case BW_FISH_WAIT:
        return "LINE IN THE WATER";
    case BW_FISH_BITE:
        return "BITE! REC TO SET";
    case BW_FISH_FIGHT:
        return "FISH ON - REEL!";
    default:
        return "REC TO CAST";
    }
}
static void bw_draw_bobber(int32_t x, int32_t y, unsigned k, uint32_t ms)
{
    k &= 3u;
    int st = bw_fish_state(&bw, k);
    int32_t bob = (ms / 300u + k) & 1;
    int32_t left = (int32_t)(bw.deadline[k] - bw.clock);
    int nibble = st == BW_FISH_WAIT && bw.playing && left >= 0 && left < (int32_t)BW_RATE;
    if (nibble)
        bob = (ms / 90u) & 1 ? 2 : -1;
    if (bw_art_props[0].px)
    {
        int32_t px = x - 11, py = y - 14;
        if (st == BW_FISH_BITE || st == BW_FISH_FIGHT)
        {
            bw_prop(8u + (ms / 150u) % 3u, px, py + 4); /* rings */
            bw_prop(3, px, py + 2);                     /* pulled under */
        }
        else
            bw_prop(nibble ? 2u : 1u, px, py + bob);
        return;
    }
    if (st == BW_FISH_BITE || st == BW_FISH_FIGHT)
    {
        int32_t r = (int32_t)(ms / 60u % 8u);
        R(x - r, y + 6, 2 * r + 2, 1, C_TXT); /* rings */
        R(x - 1, y + 4, 4, 2, RGB(220, 60, 50)); /* pulled under */
        return;
    }
    R(x, y + bob, 3, 3, RGB(230, 70, 60));
    R(x, y + 3 + bob, 3, 3, C_WHITE);
    R(x + 1, y - 6 + bob, 1, 6, C_MUTE);
}
static void bw_draw_theme(int32_t y0, int32_t h, unsigned k)
{
    const bw_theme *th = &bw.track[k].cur;
    int lo = 99, hi = -99;
    for (unsigned i = 0; i < th->len; i++)
        if (th->s[i].gate == BW_GATE_NOTE)
        {
            lo = th->s[i].deg < lo ? th->s[i].deg : lo;
            hi = th->s[i].deg > hi ? th->s[i].deg : hi;
        }
    if (lo > hi)
        lo = hi = 0;
    int span = hi - lo + 1 < 8 ? 8 : hi - lo + 1;
    int32_t cw = 228 / (int32_t)th->len, rowh = (h - 6) / span;
    if (rowh < 2)
        rowh = 2;
    R(6, y0, 228, h, C_PANEL);
    for (unsigned i = 0; i < th->len; i += 4)
        R(6 + (int32_t)i * cw, y0, 1, h, C_BAR);
    int32_t lastx = -1, lasty = 0;
    for (unsigned i = 0; i < th->len; i++)
    {
        const bw_step *s = &th->s[i];
        if (s->gate == BW_GATE_NOTE)
        {
            lasty = y0 + h - 4 - (s->deg - lo + 1) * rowh;
            lastx = 6 + (int32_t)i * cw;
            R(lastx, lasty, cw - 1, rowh > 4 ? 4 : rowh, bw_color(k));
        }
        else if (s->gate == BW_GATE_TIE && lastx >= 0)
            R(6 + (int32_t)i * cw - 1, lasty, cw, rowh > 4 ? 4 : rowh, bw_color(k));
    }
    if (bw.playing)
    {
        unsigned st = (bw.step[k] + th->len - 1) % th->len;
        R(6 + (int32_t)st * cw, y0, cw, 2, C_YEL);
    }
}
static void bw_draw_popup(void)
{
    const bw_event *ev = &bw_ui.popup;
    char t[24];
    if (bw_art_frames[0].px) /* the wooden frame: nine pieces, the edges repeated */
    {
        const bw_sheet *fr = &bw_art_frames[0];
        for (int32_t y = 104; y < 192; y += 24)
            for (int32_t x = 8; x < 232; x += 24)
            {
                int32_t xx = x > 208 ? 208 : x, yy = y > 168 ? 168 : y;
                unsigned col = xx == 8 ? 0 : xx == 208 ? 2 : 1, row = yy == 104 ? 0 : yy == 168 ? 2 : 1;
                bw_sheet_draw(fr, row * 3 + col, xx, yy, 0, 0);
            }
        R(15, 111, 210, 74, C_BG);
        if (ev->rare)
            R(15, 111, 210, 1, C_GOLD);
    }
    else
    {
        R(8, 104, 224, 88, C_PANEL);
        R(8, 104, 224, 2, ev->rare ? C_GOLD : bw_color(ev->track));
    }
    if (ev->result == BW_FIGHT_LANDED)
    {
        const bw_sheet *pt = &bw_art_portraits[ev->species % BW_SPECIES];
        if (pt->px)
            bw_sheet_draw(pt, ev->species % 4u, 14, 112, 0, 0);
        else
            bw_draw_fish_at(10, 128, ev->species, 10, 0, 1);
        if (ev->rare && bw_art_banner[0].px)
            bw_sheet_draw(&bw_art_banner[0], 0, 45, 52, 0, 0);
        if (ev->best)
            bw_prop(15, 204, 108);
        T(80, 110, &FONT_XS, ev->rare ? "RARE CATCH!" : "CAUGHT", ev->rare ? C_GOLD : C_MUTE);
        T(80, 126, &FONT_B, bw_species_names[ev->species], C_TXT);
        t[0] = 0;
        bw_length_text(t, ev->mm);
        T(80, 146, &FONT_B, t, C_YEL);
        t[0] = 0;
        bw_weight_text(t, ev->g);
        T(160, 146, &FONT_B, t, C_YEL);
        T(80, 168, &FONT_XS, ev->key_change ? "NEW BEST: KEY CHANGE" : ev->best ? "NEW PERSONAL BEST" : "THEME CHANGED",
          ev->best ? C_PINK : C_MUTE);
    }
    else
    {
        T(20, 120, &FONT_B, ev->result == BW_FIGHT_SNAPPED ? "LINE SNAPPED" : "THREW THE HOOK", C_PINK);
        T(20, 146, &FONT_XS, ev->rare ? "IT WAS SOMETHING BIG..." : "IT GOT AWAY", C_MUTE);
    }
}
/* FISH ON!: a strike, 1.6 s. The bobber is yanked under; a flash and a shake; the portrait slams in from the left
 * over spinning speed lines; FISH crashes in from the right and ON! drops in; spray bursts up from the water; a
 * wave sweeps up and washes it all away (bw_draw draws the page beneath it). t: ms since the strike */
#define BW_STRIKE_MS 1600u
static float bw_ease_back(float u) /* overshoots, then settles: 0 -> 1 */
{
    u = fm_clampf(u, 0.0f, 1.0f) - 1.0f;
    return 1.0f + u * u * (2.7f * u + 1.7f);
}
static void bw_text_punch(int32_t x, int32_t y, const char *s, uint16_t c)
{
    static const int8_t o[8][2] = {{-2, 0}, {2, 0}, {0, -2}, {0, 2}, {-2, -2}, {2, 2}, {-2, 2}, {2, -2}};
    for (unsigned i = 0; i < 8; i++) /* a heavy dark outline: the words read over anything */
        T(x + o[i][0], y + o[i][1], &FONT_L, s, C_BLACK);
    T(x + 3, y + 4, &FONT_L, s, RGB(120, 70, 20));
    T(x, y, &FONT_L, s, c);
}
static void bw_draw_fish_on(uint32_t t)
{
    /* the shake: hard at the strike, dying away by 0.6 s */
    int32_t sx = 0, sy = 0;
    if (t > 180 && t < 700)
    {
        int32_t amp = (int32_t)(7.0f * (1.0f - (float)(t - 180) / 520.0f)) + 1;
        sx = (t / 30u) & 1 ? amp : -amp;
        sy = (t / 45u) & 1 ? amp / 2 : -amp / 2;
    }
    /* 0-180 ms: the bobber goes under, rings spreading */
    if (t < 180)
    {
        R(0, 48, 240, 192, C_BG);
        int32_t r = (int32_t)t / 6;
        for (int32_t a = 0; a < 3; a++)
        {
            int32_t rr = r - a * 9;
            if (rr > 0)
            {
                L(120 - rr * 2, 150, 120 + rr * 2, 150, a ? C_BAR : C_TXT);
                L(120 - rr, 150 - rr / 3, 120 + rr, 150 - rr / 3, C_BAR);
            }
        }
        if (!bw_prop(3, 108, 132 + (int32_t)t / 20))
            R(116, 140 + (int32_t)t / 20, 8, 6, C_PINK);
        return;
    }
    /* the flash */
    if (t < 260)
    {
        R(0, 48, 240, 192, t < 220 ? RGB(240, 236, 220) : C_GOLD);
        return;
    }
    R(0, 48, 240, 192, RGB(10, 22, 20));
    /* speed lines, turning */
    float spin = (float)t * 0.0025f;
    for (int i = 0; i < 18; i++)
    {
        float an = spin + (float)i * (FM_TWO_PI / 18.0f), co = fm_cosf(an), si = fm_sinf(an);
        L(64 + sx + (int32_t)(co * 40.0f), 116 + sy + (int32_t)(si * 40.0f), 64 + sx + (int32_t)(co * 260.0f),
          116 + sy + (int32_t)(si * 260.0f), i & 1 ? RGB(22, 44, 38) : RGB(40, 66, 52));
    }
    /* the burst on the water, behind everything: star, explosion, spray, rings */
    if (t < 1300)
        bw_kit(&bw_art_burst[0], t < 330 ? 0u : t < 560 ? 1u : t < 860 ? 2u : 3u, 56, 132 + sy, 2);
    /* the portrait: in from the left, overshooting */
    int32_t px = -120 + (int32_t)(132.0f * bw_ease_back((float)(t - 260) / 260.0f));
    if (bw_art_portrait[0].px)
    {
        R(px + sx - 3, 57 + sy, 118, 118, C_GOLD);
        bw_sheet_draw2(&bw_art_portrait[0], 0, px + sx, 60 + sy, 1, 0);
    }
    else /* no portrait in this build: the fish itself, twice the size, thrashing */
        bw_draw_fish_at(px + sx, 92 + sy, bw.fish[bw_fish_on_track], 7u + t / 120u % 3u, 0, 2);
    /* the words: FISH from the right, ON! dropping in */
    int32_t fx = 240 - (int32_t)(110.0f * bw_ease_back((float)((int32_t)t - 360) / 220.0f));
    if (t > 360)
        bw_text_punch(fx + sx, 74 + sy, "FISH", C_GOLD);
    int32_t oy = -40 + (int32_t)(150.0f * bw_ease_back((float)((int32_t)t - 520) / 220.0f));
    if (t > 520)
        bw_text_punch(150 + sx, oy + sy, "ON!", C_GOLD);
    /* spray: thrown up from the water, falling back */
    float st = (float)(t - 260) / 1000.0f;
    for (unsigned i = 0; i < 40; i++)
    {
        uint32_t h = i * 2654435761u;
        float vx = (float)((int)(h >> 8 & 255) - 128) * 0.9f, vy = 160.0f + (float)(h >> 16 & 127);
        int32_t x = 120 + (int32_t)(vx * st), y = 240 - (int32_t)(vy * st - 180.0f * st * st);
        if (y < 238 && y > 48)
            R(x + sx, y + sy, i % 3 ? 2 : 3, i % 3 ? 2 : 3, i & 1 ? C_TXT : RGB(150, 200, 210));
    }
    /* the water line */
    if (!bw_kit(&bw_art_water[0], t / 120u % 4u, 0, 224, 1))
        for (int32_t x = 0; x < 240; x += 4)
            R(x, 226 + (int32_t)(3.0f * fm_sinf((float)x * 0.09f + (float)t * 0.012f)), 4, 14, RGB(16, 40, 44));
}
/* the wave that washes FISH ON! away: its top edge at row y (the page shows beneath), foam on the crest */
static void bw_draw_wave(int32_t y, uint32_t t)
{
    for (int32_t x = 0; x < 240; x += 3)
    {
        int32_t yy = y + (int32_t)(5.0f * fm_sinf((float)x * 0.07f + (float)t * 0.02f));
        R(x, yy, 3, 3, C_TXT);
        R(x, yy + 3, 3, 4, RGB(60, 120, 130));
    }
}
/* the patch's routing: modulators left, carriers right, a line for each route, the chosen operator outlined */
static void bw_draw_algorithm(const bw_patch *p, int32_t y0, int32_t h, unsigned sel, uint16_t c)
{
    int depth[6], col_n[6] = {0, 0, 0, 0, 0, 0}, slot[6], xs[6], ys[6];
    for (int i = 0; i < 6; i++) /* a modulator's depth: one past the deepest (lower-numbered) operator it feeds */
    {
        depth[i] = 0;
        for (int j = 0; j < i; j++)
            if (p->route[i][j] && depth[j] + 1 > depth[i])
                depth[i] = depth[j] + 1;
    }
    for (int i = 0; i < 6; i++)
        slot[i] = col_n[depth[i]]++;
    R(6, y0, 228, h, C_PANEL);
    for (int i = 0; i < 6; i++)
    {
        depth[i] = depth[i] > 5 ? 5 : depth[i];
        int32_t bh = (h - 4) / (col_n[depth[i]] > 0 ? col_n[depth[i]] : 1);
        xs[i] = 198 - depth[i] * 38;
        ys[i] = y0 + 2 + slot[i] * bh + (bh - 7) / 2;
    }
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < i; j++)
            if (p->route[i][j] && p->op[i].level)
                L(xs[i] + 22, ys[i] + 3, xs[j], ys[j] + 3, (uint16_t)(p->route[i][j] > 50 ? C_TXT : C_MUTE));
    for (int i = 0; i < 6; i++)
    {
        int32_t w = 2 + (int32_t)p->op[i].level * 20 / 100; /* brightness by level: a bar inside the box */
        R(xs[i], ys[i], 22, 7, i == (int)sel ? C_YEL : C_BAR);
        R(xs[i] + 1, ys[i] + 1, 20, 5, C_BG);
        R(xs[i] + 1, ys[i] + 1, w > 20 ? 20 : w, 5, p->carrier[i] ? c : C_MUTE);
        if (p->op[i].feedback)
            R(xs[i] + 9, ys[i] - 2, 4, 1, C_PINK); /* feedback */
        if (p->carrier[i])
            L(xs[i] + 22, ys[i] + 3, 232, ys[i] + 3, C_BAR); /* to the output */
    }
}
static void bw_draw_page(uint32_t ms)
{
    char t[24], u[24];
    unsigned k = bw_ui.track, sub = bw_ui.sub[bw_ui.page];
    const bw_track *tr = &bw.track[k];
    const bw_song *s = &bw.song;
    const bw_operator *o = &tr->patch.op[bw_ui.op];
    uint16_t c = bw_color(k);
    switch (bw_ui.page)
    {
    case BW_RIVER:
    {
        /* the river at the weather: dusk (and day), night, storm */
        static const uint8_t panel[BW_NWEATHER] = {0, 0, 1, 2, 0};
        const bw_sheet *bg = &bw_art_river[0];
        int art = bg->px != 0;
        if (art)
            bw_sheet_draw(bg, panel[bw_weather(&bw)], 0, 0, 0, 0);
        for (unsigned i = 0; i < 4; i++)
        {
            int32_t y = 48 + (int32_t)i * 48;
            int st = bw_fish_state(&bw, i);
            if (i == k)
            {
                if (!art)
                    R(0, y, 240, 48, C_PANEL);
                R(0, y + 4, 3, 40, bw_color(i)); /* the chosen fish */
            }
            if (!art)
                for (int32_t x = 0; x < 240; x += 12)
                    R(x + (int32_t)((ms / 200u + i * 3) % 12u), y + 45, 6, 1, RGB(20, 50, 70));
            unsigned sp = st == BW_FISH_FIGHT ? bw.fish[i] : i;
            /* no line: the fish idles; a line in the water: it swims; a bite, a fight */
            unsigned frame = st == BW_FISH_FIGHT  ? 7u + ms / 150u % 3u
                             : st == BW_FISH_BITE ? 6u
                             : st == BW_FISH_WAIT ? ms / 220u % 4u
                                                  : 4u + ms / 700u % 2u;
            bw_draw_fish(8, y + 12, sp, frame, 0);
            TS(62, y + 3, &FONT_XS, bw_species_names[i], i == k ? C_TXT : C_MUTE);
            TS(62, y + 22, &FONT_XS, bw.track[i].mute ? "MUTED" : bw_state_text(i), bw_color(i));
            if (st != BW_FISH_IDLE)
                bw_draw_bobber(222, y + 24, i, ms);
            bw_bar(168, y + 10, 36, bw.track[i].volume, 100, bw_color(i));
            int32_t m = (int32_t)(fm_minf(1.0f, bw.meter[i] * 2.0f) * 36.0f); /* what the fish is playing now */
            R(168, y + 22, 36, 3, C_BG);
            R(168, y + 22, m, 3, m > 32 ? C_PINK : bw_color(i));
        }
        break;
    }
    case BW_FISHING:
    {
        int st = bw_fish_state(&bw, k);
        if (st == BW_FISH_FIGHT)
        {
            float tn = bw.fight_tension[k], d = bw.fight_dist[k];
            bw_draw_fish(20, 60 + (int32_t)(ms / 80u % 3u), bw.fish[k], 7u + ms / 120u % 3u, 0);
            /* the rod bends with the tension; the reel turns as you wind */
            bw_kit(&bw_art_fight[0], 4u + (unsigned)fm_minf(3.0f, tn / 26.0f), 186, 86, 1);
            T(80, 54, &FONT_B, bw.fish_rare[k] ? "SOMETHING BIG!" : "FISH ON!", bw.fish_rare[k] ? C_GOLD : C_YEL);
            T(80, 74, &FONT_XS, bw.fight_run[k] > 0.0f ? "IT'S RUNNING - EASE OFF" : "REEL: SELECT OR K1",
              bw.fight_run[k] > 0.0f ? C_PINK : C_MUTE);
            T(8, 104, &FONT_XS, "LINE TENSION", C_MUTE);
            R(8, 124, 224, 10, C_BAR);
            R(8, 124, (int32_t)(224.0f * fm_clampf(tn / 100.0f, 0, 1)), 10,
              tn > 80 ? RGB(240, 70, 60) : tn > 55 ? C_YEL : RGB(110, 200, 140));
            T(8, 146, &FONT_XS, "DISTANCE", C_MUTE);
            R(8, 166, 224, 10, C_BAR);
            R(8, 166, (int32_t)(224.0f * fm_clampf(d / fm_maxf(bw.fight_max[k], 1.0f), 0, 1)), 10, c);
            int reel = bw_kit(&bw_art_fight[0], bw_ui.reel_turns % 4u, 4, 186, 1);
            T(reel ? 58 : 8, 186, &FONT_XS, "TOO TIGHT SNAPS THE LINE", C_MUTE);
            T(reel ? 58 : 8, 202, &FONT_XS, "TOO SLACK THROWS THE HOOK", C_MUTE);
            bw_kit(&bw_art_water[0], ms / 160u % 4u, 0, 224, 1);
            break;
        }
        int32_t depth_y = 52 + (int32_t)tr->depth * 9;
        if (bw_art_river[0].px)
        {
            static const uint8_t panel[BW_NWEATHER] = {0, 0, 1, 2, 0};
            bw_sheet_draw(&bw_art_river[0], panel[bw_weather(&bw)], 0, -60, 0, 0); /* the water, under the header */
            R(0, 92, 240, 148, C_BG);
        }
        else
            R(0, 48, 240, 44, RGB(10, 30, 40));
        R(120, 48, 1, depth_y - 48, C_MUTE);
        if (st != BW_FISH_IDLE)
            bw_draw_bobber(119, 52, k, ms);
        bw_draw_fish(150, depth_y, k, ms / 250u % 4u, st == BW_FISH_IDLE);
        TS(8, 56, &FONT_XS, bw_depth_names[tr->depth], C_TXT);
        TS(8, 72, &FONT_XS, "PRESETS: DEPTH", C_MUTE);
        int32_t left = (int32_t)(bw.deadline[k] - bw.clock);
        t[0] = 0;
        if (st == BW_FISH_WAIT)
        {
            bw_num(t, left > 0 ? left / (int32_t)BW_RATE : 0);
            bw_cat(t, sizeof t, " S");
        }
        T(8, 96, &FONT_B, st == BW_FISH_BITE ? "BITE!" : st == BW_FISH_WAIT ? "WAITING" : "NO LINE", C_TXT);
        TR(232, 96, &FONT_B, t, C_YEL);
        bw_param_n(6, 120, "K1 MIN WAIT S", tr->wait_min, 600, c);
        bw_param_n(122, 120, "K2 MAX WAIT S", tr->wait_max, 600, c);
        bw_param(6, 156, "K3 MODE", tr->manual ? "TIMED" : "AUTO", 0, 0, c);
        bw_param(122, 156, "K4 BAIT", bw_bait_names[tr->bait], 0, 0, c);
        {
            static const uint8_t icon[BW_NBAIT] = {11, 12, 13, 0}; /* worm, crankbait, spinner, jig: the hook */
            bw_prop(icon[tr->bait], 208, 186);
        }
        if (bw_kit(&bw_art_weather[0], bw_weather_icon(), 6, 193, 1))
            T(28, 196, &FONT_XS, bw_weather_names[bw_weather(&bw)], C_YEL);
        else
        {
            T(6, 196, &FONT_XS, "WEATHER", C_MUTE);
            T(70, 196, &FONT_XS, bw_weather_names[bw_weather(&bw)], C_YEL);
        }
        T(6, 214, &FONT_XS, "REC: CAST / PULL IN / SET THE HOOK", C_MUTE);
        break;
    }
    case BW_THEME:
        bw_draw_theme(50, 92, k);
        if (sub == 0)
        {
            bw_param_n(6, 150, "K1 LENGTH", tr->cur.len, 32, c);
            bw_param_n(122, 150, "K2 DENSITY %", tr->density, 100, c);
            bw_param_n(6, 190, "K3 OCTAVE", tr->octave, 0, c);
            T(122, 190, &FONT_XS, "PRESETS: ACTIONS", C_MUTE);
        }
        else
        {
            T(6, 148, &FONT_XS, "TURN RIGHT TO DO:", C_MUTE);
            T(6, 166, &FONT_XS, tr->has_prev ? "K1 RELEASE LAST CATCH" : "K1 (NOTHING TO RELEASE)", C_TXT);
            T(6, 184, &FONT_XS, "K2 BACK TO HOME THEME", C_TXT);
            T(6, 202, &FONT_XS, "K3 MAKE THIS THE HOME", C_TXT);
            T(6, 220, &FONT_XS, bw.rec_arm[k] ? "K4 RECORDING... (LEFT: STOP)" : "K4 RECORD FROM KEYS",
              bw.rec_arm[k] ? C_PINK : C_TXT);
        }
        break;
    case BW_LOG:
        if (sub == 0)
            for (unsigned i = 0; i < 4; i++)
            {
                int32_t y = 48 + (int32_t)i * 48;
                for (unsigned rare = 0; rare < 2; rare++)
                {
                    const bw_species_log *l = &bw.log.s[i + rare * 4];
                    int32_t yy = y + 2 + (int32_t)rare * 22;
                    T(6, yy, &FONT_XS, bw_species_names[i + rare * 4], rare ? C_GOLD : bw_color(i));
                    bw_num(t, (int)l->count);
                    TR(150, yy, &FONT_XS, t, C_TXT);
                    t[0] = 0;
                    if (l->count)
                        bw_length_text(t, l->best_mm);
                    else
                        bw_cat(t, sizeof t, "-");
                    TR(234, yy, &FONT_XS, t, C_YEL);
                    /* the best against the species' usual top size (a trophy runs past it, in gold) */
                    uint32_t mx = bw_species_max_mm(i + rare * 4), b = l->best_mm;
                    int32_t bw_ = (int32_t)(b >= mx ? 70 : b * 70 / mx);
                    R(160, yy + 15, 70, 2, C_BAR);
                    R(160, yy + 15, bw_, 2, b > mx ? C_GOLD : bw_color(i));
                }
            }
        else
        {
            static const char *const names[4] = {"MISSED BITES", "SNAPPED LINES", "THROWN HOOKS", "RELEASED"};
            const uint32_t v[4] = {bw.log.missed, bw.log.snapped, bw.log.thrown, bw.log.released};
            for (unsigned i = 0; i < 4; i++)
            {
                T(6, 56 + (int32_t)i * 40, &FONT_XS, names[i], C_MUTE);
                bw_num(t, (int)v[i]);
                TR(234, 52 + (int32_t)i * 40, &FONT_B, t, C_TXT);
            }
            T(6, 216, &FONT_XS, "BEST SIZES ON PAGE 1 (PRESETS)", C_MUTE);
        }
        break;
    case BW_SCALE:
    {
        static const char *const roman[7] = {"i", "ii", "iii", "iv", "v", "vi", "vii"};
        bw_param(6, 54, "K1 KEY", bw_key_names[s->key], s->key, 11, C_YEL);
        bw_param(122, 54, "K2 SCALE", bw_scale_names[s->scale], s->scale, BW_SCALES - 1, C_YEL);
        bw_param(6, 100, "K3 CHORDS", bw_progression_names[s->progression], 0, 0, C_YEL);
        bw_param_n(122, 100, "K4 SWING %", s->swing, 60, C_YEL);
        T(6, 150, &FONT_XS, "CHORD NOW", C_MUTE);
        int r = bw_chord_root(&bw);
        T(110, 146, &FONT_B, roman[((r % 7) + 7) % 7], C_PINK);
        {
            /* an octave from the key: the scale's notes lit, the chord's root and third and fifth bright */
            int in[12] = {0}, chord[12] = {0};
            for (int d = -14; d < 14; d++)
            {
                int n = bw_note_of(&bw, 2, d, 0) % 12;
                in[n] = 1;
            }
            for (int d = 0; d <= 4; d += 2)
                chord[bw_note_of(&bw, 2, d + r, 0) % 12] = 1;
            /* a piano octave, C to B: seven white keys, five black ones over their gaps */
            static const uint8_t white_pc[7] = {0, 2, 4, 5, 7, 9, 11}, black_after[5] = {0, 1, 3, 4, 5},
                                 black_pc[5] = {1, 3, 6, 8, 10};
            for (int w = 0; w < 7; w++)
            {
                int pc = white_pc[w];
                int32_t x = 8 + w * 32;
                R(x, 176, 30, 44, chord[pc] ? C_PINK : in[pc] ? C_YEL : C_TXT);
                if (pc == s->key)
                    R(x + 11, 212, 8, 4, C_BG); /* the key's own note */
            }
            for (int b = 0; b < 5; b++)
            {
                int pc = black_pc[b];
                int32_t x = 8 + (black_after[b] + 1) * 32 - 10;
                R(x - 1, 176, 20, 28, C_BG);
                R(x + 1, 176, 16, 26, chord[pc] ? C_PINK : in[pc] ? RGB(150, 104, 40) : C_BAR);
                if (pc == s->key)
                    R(x + 5, 196, 6, 3, C_TXT);
            }
            T(8, 224, &FONT_XS, "IN THE SCALE", C_YEL);
            T(100, 224, &FONT_XS, "IN THE CHORD", C_PINK);
        }
        break;
    }
    case BW_EDIT:
    {
        bw_num(t, (int)bw_ui.op + 1);
        T(6, 52, &FONT_XS, "SELECT: OPERATOR", C_MUTE);
        TR(110, 50, &FONT_B, t, C_TXT);
        u[0] = 0;
        bw_cat(u, sizeof u, "PAGE ");
        bw_num(u + 5, (int)sub + 1);
        bw_cat(u, sizeof u, "/4");
        TR(234, 52, &FONT_XS, u, C_MUTE);
        for (unsigned i = 0; i < 6; i++) /* the six operators' levels; carriers bright */
        {
            int32_t x = 6 + (int32_t)i * 38;
            R(x, 72, 34, 14, i == bw_ui.op ? C_PANEL : C_BG);
            R(x + 2, 82, (int32_t)tr->patch.op[i].level * 30 / 100, 3, tr->patch.carrier[i] ? c : C_MUTE);
        }
        if (sub == 0)
        {
            bw_param_n(6, 100, "K1 RATIO /100", o->ratio, 1600, c);
            bw_param_n(122, 100, "K2 LEVEL", o->level, 100, c);
            bw_param_n(6, 146, "K3 FEEDBACK", o->feedback, 100, c);
            bw_param_n(122, 146, "K4 ALGORITHM", tr->patch.algorithm + 1, 8, c);
        }
        else if (sub == 1)
        {
            bw_param(6, 100, "K1 FIXED", o->fixed ? "ON" : "OFF", o->fixed, 1, c);
            bw_param_n(122, 100, "K2 FIXED HZ", o->hz, 16000, c);
            bw_param_n(6, 146, "K3 VELOCITY", o->velocity, 100, c);
            bw_param_n(122, 146, "K4 OUTPUT", tr->patch.carrier[bw_ui.op], 100, c);
        }
        else if (sub == 2)
        {
            bw_param_n(6, 100, "K1 FROM OP", bw_ui.route_source + 1, 6, c);
            bw_param_n(122, 100, "K2 INTO OP", bw_ui.route_dest + 1, 6, c);
            bw_param_n(6, 146, "K3 AMOUNT", tr->patch.route[bw_ui.route_source][bw_ui.route_dest], 100, c);
            bw_param_n(122, 146, "K4 FROM OUT", tr->patch.carrier[bw_ui.route_source], 100, c);
        }
        else
        {
            u[0] = 0;
            bw_cat(u, sizeof u, bw_ui.patch_pick < 4 ? "F" : "U");
            bw_num(u + 1, bw_ui.patch_pick % 4 + 1);
            bw_cat(u, sizeof u, " ");
            bw_cat(u, sizeof u, bw_ui.patch_pick < 4 ? bw_patch_name(k, bw_ui.patch_pick) : "USER");
            bw_param(6, 100, "K1 PATCH", u, bw_ui.patch_pick, 7, c);
            T(122, 100, &FONT_XS, "K2 LOAD (RIGHT)", C_TXT);
            T(122, 122, &FONT_XS, "K3 STORE (RIGHT)", bw_ui.patch_pick >= 4 ? C_TXT : C_MUTE);
            T(6, 150, &FONT_XS, "F1-F4 FACTORY, U1-U4 YOURS", C_MUTE);
        }
        bw_draw_algorithm(&tr->patch, 190, 48, bw_ui.op, c);
        break;
    }
    case BW_ENV:
    {
        bw_num(t, (int)bw_ui.op + 1);
        T(6, 52, &FONT_XS, "SELECT: OPERATOR", C_MUTE);
        TR(110, 50, &FONT_B, t, C_TXT);
        /* the envelope's shape */
        int32_t x0 = 6, y0 = 120, w = 228, hgt = 44;
        float total = (float)o->attack + (float)o->decay + 400.0f + (float)o->release;
        int32_t xa = x0 + (int32_t)(w * (float)o->attack / total), xd = xa + (int32_t)(w * (float)o->decay / total);
        int32_t xs = xd + (int32_t)(w * 400.0f / total), ys = y0 - hgt * o->sustain / 100;
        R(x0, 72, w, hgt + 6, C_PANEL);
        for (int32_t x = x0; x < x0 + w; x++)
        {
            int32_t y = x < xa ? y0 - hgt * (x - x0) / (xa - x0 + 1)
                        : x < xd ? y0 - hgt + (hgt - (y0 - ys)) * (x - xa) / (xd - xa + 1)
                        : x < xs ? ys
                                 : ys + (y0 - ys) * (x - xs) / (x0 + w - xs + 1);
            R(x, y, 1, 2, c);
        }
        bw_param_n(6, 130, "K1 ATTACK MS", o->attack, 10000, c);
        bw_param_n(122, 130, "K2 DECAY MS", o->decay, 10000, c);
        bw_param_n(6, 176, "K3 SUSTAIN %", o->sustain, 100, c);
        bw_param_n(122, 176, "K4 RELEASE MS", o->release, 10000, c);
        break;
    }
    case BW_LFO:
    {
        R(6, 52, 228, 44, C_PANEL);
        for (int32_t x = 0; x < 228; x++)
        {
            float ph = (float)x / 228.0f, v;
            switch (tr->lfo_shape)
            {
            case BW_LFO_TRI:
                v = ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
                break;
            case BW_LFO_SQUARE:
                v = ph < 0.5f ? 1.0f : -1.0f;
                break;
            case BW_LFO_RANDOM:
                v = (float)(((x / 19) * 2654435761u) >> 28) / 7.5f - 1.0f;
                break;
            default:
                v = fm_sinf(ph * FM_TWO_PI);
                break;
            }
            R(6 + x, 74 - (int32_t)(v * 18.0f * (float)tr->lfo_depth / 100.0f), 1, 2, c);
        }
        R(6 + (int32_t)(bw.lfo_phase[k] * 227.0f), 52, 2, 44, C_YEL);
        bw_param_n(6, 108, "K1 RATE", tr->lfo_rate, 100, c);
        bw_param_n(122, 108, "K2 DEPTH", tr->lfo_depth, 100, c);
        bw_param(6, 154, "K3 TARGET", bw_lfo_target_names[tr->lfo_target], 0, 0, c);
        bw_param(122, 154, "K4 SHAPE", bw_lfo_shape_names[tr->lfo_shape], 0, 0, c);
        T(6, 204, &FONT_XS, "EACH FISH HAS ITS OWN LFO", C_MUTE);
        break;
    }
    case BW_FX:
    {
        static const char *const titles[4] = {"SENDS: THIS FISH", "REVERB + ECHO TIME", "ECHO + CHORUS", "MASTER"};
        static const char *const lab[4][4] = {{"K1 REVERB", "K2 ECHO", "K3 CHORUS", "K4 DRIVE"},
                                              {"K1 SIZE", "K2 TONE", "K3 LEVEL", "K4 ECHO TIME"},
                                              {"K1 FEEDBACK", "K2 ECHO LEVEL", "K3 CHORUS RATE", "K4 DEPTH"},
                                              {"K1 TONE", "K2 WIDTH", "", ""}};
        static const char *const dtime[6] = {"1/16", "1/8T", "1/8", "1/8.", "1/4", "1/4."};
        const uint8_t v[4][4] = {{tr->send_rev, tr->send_dly, tr->send_cho, tr->drive},
                                 {s->rev_size, s->rev_tone, s->rev_level, s->dly_time},
                                 {s->dly_feedback, s->dly_level, s->cho_rate, s->cho_depth},
                                 {s->tone, s->width, 0, 0}};
        T(6, 52, &FONT_XS, titles[sub], sub ? C_YEL : c);
        u[0] = 0;
        bw_cat(u, sizeof u, "PAGE ");
        bw_num(u + 5, (int)sub + 1);
        bw_cat(u, sizeof u, "/4");
        TR(234, 52, &FONT_XS, u, C_MUTE);
        for (unsigned i = 0; i < 4; i++)
        {
            if (!lab[sub][i][0])
                continue;
            int32_t y = 76 + (int32_t)i * 38;
            T(6, y, &FONT_XS, lab[sub][i], C_MUTE);
            if (sub == 1 && i == 3)
                bw_text_copy(t, dtime[v[1][3] * 6u / 101u], sizeof t);
            else
                bw_num(t, v[sub][i]);
            TR(234, y - 2, &FONT_B, t, C_TXT);
            bw_bar(6, y + 22, 228, v[sub][i], 100, sub ? C_PINK : c);
        }
        break;
    }
    case BW_ARP:
    {
        static const char *const st[3] = {"OFF", "ON", "LATCH"};
        T(6, 52, &FONT_XS, "ARP BUTTON: OFF / ON / LATCH", C_MUTE);
        T(6, 70, &FONT_M, st[bw.arp], bw.arp ? C_PINK : C_MUTE);
        bw_param(6, 108, "K1 MODE", bw_arp_mode_names[s->arp_mode], 0, 0, c);
        bw_param(122, 108, "K2 RATE", bw_arp_rate_names[s->arp_rate], 0, 0, c);
        bw_param_n(6, 154, "K3 OCTAVES", s->arp_octaves, 3, c);
        bw_param_n(122, 154, "K4 GATE %", s->arp_gate, 100, c);
        T(6, 204, &FONT_XS, "HOLD WHITE KEYS: THE FISH ARPEGGIATES", C_MUTE);
        break;
    }
    case BW_GLOBAL:
        if (sub == 0)
        {
            bw_param_n(6, 56, "K1 TEMPO", s->tempo, 240, C_YEL);
            bw_param(122, 56, "K2 CLOCK", s->sync ? "MIDI" : "INTERNAL", 0, 0, C_YEL);
            bw_param(6, 102, "K3 WEATHER", bw_weather_names[s->weather], 0, 0, C_YEL);
            bw_param(122, 102, "K4 BEST = KEY", s->pb_event ? "ON" : "OFF", 0, 0, C_YEL);
            T(6, 150, &FONT_XS, "NOW", C_MUTE);
            T(50, 150, &FONT_XS, bw_weather_names[bw_weather(&bw)], C_TXT);
            /* the audio's own load (percent of its time) and the times it fell behind */
            u[0] = 0;
            bw_cat(u, sizeof u, "AUDIO LOAD ");
            bw_num(u + 11, (int)plat_cpu_pct());
            bw_cat(u, sizeof u, "%  XRUNS ");
            t[0] = 0;
            bw_num(t, (int)plat_xruns());
            bw_cat(u, sizeof u, t);
            T(6, 176, &FONT_XS, u, plat_cpu_pct() > 85 ? C_PINK : C_MUTE);
        }
        else if (sub == 1)
        {
            bw_param(6, 56, "K1 UNITS", bw_pref.units ? "INCH / LB" : "CM / KG", 0, 0, C_YEL);
            bw_param(122, 56, "K2 FISH-ON CLIP", bw_pref.clip_on ? (bw_clip_present() ? "ON" : "ON (NONE)") : "OFF", 0, 0,
                     C_YEL);
            bw_param_n(6, 102, "K3 CLIP LEVEL", bw_pref.clip_level, 100, C_YEL);
            bw_param(122, 102, "K4 AUTOSAVE", bw_pref.autosave ? "ON" : "OFF", 0, 0, C_YEL);
        }
        else
        {
            bw_num(t, bw_ui.slot_pick + 1);
            bw_param(6, 56, "K1 SLOT", t, bw_ui.slot_pick, 3, C_YEL);
            T(122, 56, &FONT_XS, "K2 LOAD (RIGHT)", C_TXT);
            T(122, 76, &FONT_XS, "K3 SAVE (RIGHT)", C_TXT);
            bw_param(6, 102, "K4 MIDI OUT", bw_pref.midi_out ? "CH 1-4" : "OFF", 0, 0, C_YEL);
            bw_num(t, bw_pref.slot + 1);
            T(6, 150, &FONT_XS, "THIS SESSION IS SLOT", C_MUTE);
            T(150, 146, &FONT_B, t, C_TXT);
        }
        u[0] = 0;
        bw_cat(u, sizeof u, "PAGE ");
        bw_num(u + 5, (int)sub + 1);
        bw_cat(u, sizeof u, "/3  (PRESETS)");
        T(6, 216, &FONT_XS, u, C_MUTE);
        break;
    default:
    {
        static const char *const help[2][9] = {
            {"HOME RIVER / CATCH LOG", "SEQ FISHING / THEME", "REC CAST, PULL IN, SET HOOK", "PLAY START / STOP",
             "ALGORITHM CHOOSE THE FISH", "SELECT PAGES, REEL IN", "PRESETS SUB-PAGE, DEPTH", "SAVE THIS SLOT",
             "MASTER VOLUME"},
            {"WHITE KEYS PLAY IN THE SCALE", "BLACK KEYS: BRIGHT DARK GROWL", "METAL DIVE RISE STUTTER", "HALF CUT OCTAVE THROW",
             "OCT- / OCT+ KEYBOARD OCTAVE", "SCL KEY, SCALE, CHORDS", "FX EFFECTS  LFO  ARP", "EDIT FM  ENV ENVELOPES",
             "GLO TEMPO, PREFS, SLOTS"}};
        for (unsigned i = 0; i < 9; i++)
            T(6, 50 + (int32_t)i * 21, &FONT_XS, help[sub][i], i % 2 ? C_TXT : C_MUTE);
        break;
    }
    }
}
static void bw_draw(void)
{
    uint32_t ms = plat_ms();
    static char right[24];
    uint32_t lock = plat_lock();
    if (bw.event.seq != bw_ui.event_seq)
    {
        bw_ui.event_seq = bw.event.seq;
        bw_ui.popup = bw.event;
        bw_ui.popup_until = ms + 3500u;
        bw_ui.dirty = 1;
    }
    int popup = (int32_t)(bw_ui.popup_until - ms) > 0 && bw_fish_state(&bw, bw_ui.track) != BW_FISH_FIGHT;
    if (bw_fish_on_seq != bw_ui.fish_on_seen)
    {
        bw_ui.fish_on_seen = bw_fish_on_seq;
        bw_ui.fish_on_until = ms + BW_STRIKE_MS;
    }
    int fish_on = (int32_t)(bw_ui.fish_on_until - ms) > 0;
    uint32_t strike_t = BW_STRIKE_MS - (bw_ui.fish_on_until - ms);
    /* the last 350 ms: a wave rises from the bottom; the page beneath it, the strike above */
    int32_t wave = fish_on && strike_t > BW_STRIKE_MS - 350u
                       ? 240 - (int32_t)((strike_t - (BW_STRIKE_MS - 350u)) * 200u / 350u)
                       : 241;
    if (fish_on)
        bw_ui.draw = 1; /* every frame while it plays */
    right[0] = 0;
    bw_text_copy(right, bw.playing ? "PLAYING" : "STOPPED", sizeof right);
    for (unsigned band = 0; band < 5; band++)
    {
        bw_oy = (int32_t)band * 48;
        cv_begin(240, 48, C_BG);
        if (band == 0)
            bw_header(bw_pages[bw_ui.page], bw_ui.page == BW_FX || bw_ui.page == BW_SCALE || bw_ui.page == BW_GLOBAL
                                                ? C_YEL
                                                : bw_color(bw_ui.track),
                      right);
        else
        {
            if (!fish_on || wave <= 240)
            {
                bw_draw_page(ms);
                if (popup)
                    bw_draw_popup();
            }
            if (fish_on)
            {
                bw_ymax = wave < 240 ? wave : 240;
                if (wave < 240)
                    R(0, 48, 240, 192, C_BG); /* (clipped to above the wave: the strike's own background) */
                bw_draw_fish_on(strike_t);
                bw_ymax = 240;
                if (wave < 240)
                    bw_draw_wave(wave, strike_t);
            }
        }
        lcd_blit(0, band * 48, 240, 48, cv_px);
    }
    /* lights: PLAY, SAVE while unsaved, REC at a bite, ARP while on; the white keys walk this fish's steps; held
     * black keys lit */
    static const uint8_t white[16] = PLAT_WHITE_KEYS;
    unsigned k = bw_ui.track;
    uint32_t leds = (bw.playing ? 1u << B_PLAY : 0) | (bw_ui.dirty ? 1u << B_SAVE : 0) |
                    (bw_fish_state(&bw, k) == BW_FISH_BITE ? 1u << B_REC : 0) | (bw.arp ? 1u << B_ARP : 0);
    uint32_t keyleds = bw_ui.keys;
    if (bw.playing)
        keyleds |= 1u << white[(bw.step[k] + bw.track[k].cur.len - 1) % bw.track[k].cur.len % 16];
    plat_unlock(lock);
    plat_leds(leds, keyleds);
    bw_ui.last_draw = ms;
    bw_ui.draw = 0;
}
/* the boot screen: the river at night, the wordmark, the line */
void app_splash(void)
{
    for (unsigned band = 0; band < 5; band++)
    {
        bw_oy = (int32_t)band * 48;
        cv_begin(240, 48, C_BG);
        if (!bw_kit(&bw_art_river[0], 1, 0, 0, 1))
            R(0, 0, 240, 240, C_BG);
        R(0, 70, 240, 92, RGB(6, 14, 12));
        if (!bw_kit(&bw_art_logo[0], 0, 54, 82, 1))
            T(60, 96, &FONT_M, APP_NAME, C_TXT);
        TS(64, 132, &FONT_XS, "GENERATIVE FISH MUSIC", C_YEL);
        lcd_blit(0, band * 48, 240, 48, cv_px);
    }
}
void app_ui_init(void)
{
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
    if (bw_ui.draw || plat_ms() - bw_ui.last_draw >= 80)
        bw_draw();
    if (ann_on() && ann_where())
        ann_say(bw_pages[bw_ui.page], 0, ANN_NOW);
    ann_tick(plat_ms());
}
#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    unsigned k = bw_ui.track;
    const bw_track *t = &bw.track[k];
    unsigned notes = 0, active = 0;
    for (unsigned i = 0; i < t->cur.len; i++)
        notes += t->cur.s[i].gate == BW_GATE_NOTE;
    for (unsigned i = 0; i < BW_VOICES; i++)
        active += bw.voice[i].active;
    const struct
    {
        const char *name;
        int value;
    } checks[] = {{"track", (int)k},
                  {"page", bw_ui.page},
                  {"sub", bw_ui.sub[bw_ui.page]},
                  {"playing", bw.playing},
                  {"state", bw_fish_state(&bw, k)},
                  {"hooked", bw_fish_state(&bw, k) != BW_FISH_IDLE},
                  {"bite", bw_fish_state(&bw, k) == BW_FISH_BITE},
                  {"fight", bw_fish_state(&bw, k) == BW_FISH_FIGHT},
                  {"catches", (int)(bw.log.s[k].count + bw.log.s[k + 4].count)},
                  {"rare", (int)bw.log.s[k + 4].count},
                  {"best_mm", (int)bw.log.s[k].best_mm},
                  {"last_mm", (int)bw.log.s[k].last_mm},
                  {"missed", (int)bw.log.missed},
                  {"snapped", (int)bw.log.snapped},
                  {"thrown", (int)bw.log.thrown},
                  {"released", (int)bw.log.released},
                  {"manual", t->manual},
                  {"wait_min", t->wait_min},
                  {"wait_max", t->wait_max},
                  {"bait", t->bait},
                  {"depth", t->depth},
                  {"level", t->patch.op[bw_ui.op].level},
                  {"algorithm", t->patch.algorithm},
                  {"theme_len", t->cur.len},
                  {"theme_notes", (int)notes},
                  {"has_prev", t->has_prev},
                  {"tempo", bw.song.tempo},
                  {"key", bw.song.key},
                  {"scale", bw.song.scale},
                  {"swing", bw.song.swing},
                  {"send_rev", t->send_rev},
                  {"rev_level", bw.song.rev_level},
                  {"lfo_depth", t->lfo_depth},
                  {"arp", bw.arp},
                  {"macros", (int)bw.macros[k]},
                  {"voices", (int)active},
                  {"slot", bw_pref.slot},
                  {"units", bw_pref.units},
                  {"dirty", bw_ui.dirty},
                  {"valid", bw_track_valid(t)},
                  {"operator", bw_ui.op},
                  {"clip", bw_fish_on.active},
                  {"clip_present", bw_clip_present()},
                  {"clip_plays", (int)bw_clip_plays},
                  {"rec_arm", bw.rec_arm[k]}};
    for (unsigned i = 0; i < sizeof checks / sizeof checks[0]; i++)
        if (strcmp(what, checks[i].name) == 0)
        {
            *got = checks[i].value;
            return 1;
        }
    return 0;
}
#endif /* PLAT_HOST: simulator assertions are not part of the device image. */
