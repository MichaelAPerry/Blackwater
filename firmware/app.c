/* SPDX-License-Identifier: GPL-3.0-or-later */
/* BLACKWATER's application: controls, screens, saves, the "fish on" clip. The engine is bw_engine.c. */
#include "bw_engine.h"
#include "fastmath.h"
#include "bw_clip.c"
static bw_engine bw;
static bw_clip_state bw_fish_on;

/* ------------------------------------------------------------------ saved objects */
#define BW_MAGIC 0x42573033u /* "BW03": the sounds and the river changed; older sessions start fresh */
#define BW_VERSION 3u
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
static const char *const bw_layers[4] = {"CURRENT", "BED", "LIGHT", "DISTANCE"};
static const uint8_t bw_subpages[BW_NPAGES] = {1, 1, 2, 3, 1, 4, 1, 1, 4, 1, 4, 2};
static struct
{
    uint32_t buttons, keys, last_draw, message_until, popup_until, last_input, event_seq, fight_ms;
    uint32_t reel_turns;
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
/* Line-art palette: graphite, chalk, and restrained instrument accents. */
#define C_BG RGB(9, 12, 15)       /* deep water */
#define C_PANEL C_BG             /* open canvas, no filled panels */
#define C_BAR RGB(44, 54, 61)     /* slack line */
#define C_YEL RGB(232, 170, 70)   /* lantern amber: global values, the time */
#define C_PINK RGB(222, 78, 58)   /* bobber red: alerts, recording */
#define C_GOLD RGB(240, 204, 96)  /* trophy: rare fish */
#define C_TXT RGB(224, 231, 230)  /* chalk */
#define C_MUTE RGB(113, 130, 137) /* graphite labels */

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
void bw_on_rare(unsigned k)
{
    (void)k;
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
        bw_panel_clear(&bw);
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
    unsigned world=bw_pref.pad[0]&3u;
    bw.river.world=(uint8_t)(world<BW_WORLDS ? world:0);
    bw.keyboard_unlocked=(bw_pref.pad[0]>>2)&1u;
    bw.mode=(bw_pref.pad[0]>>3)&1u;
    bw.live_track=(bw_pref.pad[0]>>4)&3u;
    bw.river.memory = bw_pref.pad[1] <= BW_SPECIES ? bw_pref.pad[1] : 0;
    if (!bw_load_slot(bw_pref.slot))
    {
        bw_river_world(&bw, bw.river.world);
        bw.song.progression = 0;
        bw.song.pb_event = 0;
        for (unsigned k = 0; k < 4; k++)
        {
            bw.track[k].wait_min = 100;
            bw.track[k].wait_max = 240;
        }
    }
    bw.midi_out = bw_pref.midi_out;
    bw_ui.track=bw.live_track;
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
    bw_pref.pad[0]=(uint8_t)(bw.river.world | bw.keyboard_unlocked<<2 | bw.mode<<3 | bw.live_track<<4);
    bw_pref.pad[1] = bw.river.memory;
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
    bw_river_enable(&bw);
    plat_unlock(lock);
    bw_load_all();
    bw_ui.dirty = 0;
    bw_ui.draw = 1;
}

/* ------------------------------------------------------------------ init and render */
void app_init(void)
{
    bw_init(&bw);
    bw_river_enable(&bw);
    bw_load_all();
    /* simulated reboots call app_init again in the same process: all UI state starts over */
    memset(&bw_ui, 0, sizeof bw_ui);
    for (unsigned k = 0; k < 4; k++)
        bw_ui.preview[k] = -1;
    memset(&bw_fish_on, 0, sizeof bw_fish_on);
    bw_clip_plays = 0;
    bw_ui.track=bw.live_track;
    bw_ui.route_source = 1;
    bw_ui.draw = 1;
    bw_ui.event_seq = bw.event.seq;
    if(bw.mode==BW_MODE_RIVER) bw_cast(&bw,0);
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
        if(bw.mode==BW_MODE_FREE)
        {
            if(knob==0) o->ratio=(uint16_t)bw_step_val(o->ratio,d*5,5,1600);
            if(knob==1) o->level=(uint8_t)bw_step_val(o->level,d,0,100);
            if(knob==2) o->feedback=(uint8_t)bw_step_val(o->feedback,d,0,100);
            if(knob==3) bw_algorithm(p,(unsigned)bw_step_val(p->algorithm,d,0,7));
        }
        else bw.track[knob].volume=(uint8_t)bw_step_val(bw.track[knob].volume,d*2,0,100);
        break;
    case BW_FISHING:
        if (knob == 0)
        {
            t->wait_min = (uint16_t)bw_step_val(t->wait_min, d * 5, 0, 600);
            if (t->wait_max < t->wait_min)
                t->wait_max = t->wait_min;
        }
        if (knob == 1)
            t->wait_max = (uint16_t)bw_step_val(t->wait_max, d * 5, t->wait_min, 600);
        if (knob == 2 && (!bw.river.enabled || bw.river.phase <= BW_RIVER_GROW))
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
            if (knob == 3)
                t->mute = (uint8_t)bw_step_val(t->mute, d, 0, 1);
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
                            {&s->tone, &s->width, &t->pan, &t->volume}};
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
        else if(sub==2)
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
        if(sub==3)
        {
            if(knob==0)
            {
                bw_performance_mode(&bw,(unsigned)bw_step_val(bw.mode,d,0,1));
                bw_ui.track=bw.live_track;bw_fish_on.active=0;
            }
            if(knob==1) bw_keyboard_unlock(&bw,bw_step_val(bw.keyboard_unlocked,d,0,1));
            if(knob==2)
            {
                bw_keyboard_layer(&bw,(unsigned)bw_step_val(bw.live_track,d,0,3));
                bw_ui.track=bw.live_track;
            }
            if(knob==3) bw.live_oct=(int8_t)bw_step_val(bw.live_oct,d,-2,2);
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
    int fighting = bw_fish_state(&bw, bw.river.enabled ? bw.river.line : k) == BW_FISH_FIGHT;
    int reel_page = bw_ui.page == BW_RIVER || bw_ui.page == BW_FISHING;
    uint32_t lock = plat_lock();
    if ((e = turn[EN_ALGO]) && (!fighting || bw_keyboard_full(&bw)))
    {
        /* the held macros and live notes stay with the track they started on */
        bw_ui.track = (uint8_t)clamp(bw_ui.track + e, 0, 3);
        bw_keyboard_layer(&bw,bw_ui.track);
    }
    if (fighting && reel_page)
    {
        /* the fight: SELECT or K1 reels in */
        unsigned dt = now - bw_ui.fight_ms;
        bw_ui.fight_ms = now;
        int r = BW_FIGHT_ON;
        if (bw.river.enabled)
            bw_river_reel(&bw, turn[EN_SELECT] + turn[EN_K1]);
        else
            r = bw_fight(&bw, k, turn[EN_SELECT] + turn[EN_K1], dt);
        if (turn[EN_SELECT] + turn[EN_K1] > 0)
            bw_ui.reel_turns += (uint32_t)(turn[EN_SELECT] + turn[EN_K1]);
        if (r != BW_FIGHT_ON)
            bw_ui.draw = 1;
        turn[EN_SELECT] = turn[EN_K1] = 0;
    }
    if ((e = turn[EN_SELECT]))
    {
        if(bw.mode==BW_MODE_FREE && (bw_ui.page==BW_RIVER || bw_ui.page==BW_EDIT || bw_ui.page==BW_ENV))
            bw_ui.op=(uint8_t)clamp(bw_ui.op+e,0,5);
        else bw_ui.page=(uint8_t)clamp(bw_ui.page+e,0,BW_NPAGES-1);
    }
    if ((e = turn[EN_PRESET]))
    {
        if (bw_ui.page == BW_EDIT || bw_ui.page == BW_ENV) /* the operator (EDIT again: its next page) */
            bw_ui.op = (uint8_t)clamp(bw_ui.op + e, 0, 5);
        else if (bw_ui.page == BW_FISHING)
        {
            bw.track[k].depth = (uint8_t)clamp(bw.track[k].depth + e, 0, BW_NDEPTH - 1);
            bw_ui.dirty = 1;
        }
        else if (bw_ui.page == BW_RIVER)
        {
            if(bw.mode==BW_MODE_FREE)
            {
                bw_ui.patch_pick=(uint8_t)clamp(bw.track[k].patch_slot+e,0,7);
                if(bw_ui.patch_pick<4) bw_patch_factory(&bw.track[k].patch,k,bw_ui.patch_pick);
                else bw.track[k].patch=bw_user.user[k][bw_ui.patch_pick-4u];
                bw.track[k].patch_slot=bw_ui.patch_pick;bw_ui.dirty=1;
            }
            else if (bw.playing)
                app_say("STOP TO CHANGE WORLD", "PRESET: DUSK / UNDERTOW / FIREFLIES");
            else
            {
                unsigned world = (bw.river.world + BW_WORLDS + (e > 0 ? 1 : -1)) % BW_WORLDS;
                bw_all_notes_off(&bw);
                bw_river_world(&bw, world);
                bw_pref.pad[0]=(uint8_t)((bw_pref.pad[0]&~3u)|world);
                app_say("WORLD", bw_world_names[world]);
                bw_ui.dirty = 1;
            }
        }
        else if (bw_ui.page == BW_SCALE)
        {
            bw.song.key = (uint8_t)((bw.song.key + 12 + (e > 0 ? 1 : -1)) % 12);
            bw_ui.dirty = 1;
        }
        else if (bw_ui.page == BW_LFO)
        {
            bw.track[k].lfo_target = (uint8_t)clamp(bw.track[k].lfo_target + e, 0, BW_NLFO_TARGET - 1);
            bw_ui.dirty = 1;
        }
        else if (bw_ui.page == BW_ARP)
        {
            bw.song.arp_mode = (uint8_t)clamp(bw.song.arp_mode + e, 0, BW_NARP_MODE - 1);
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
        if ((pressed >> b & 1) && page_of[b] != 0xFF) /* (during a fight too: it goes on, REC still reels) */
        {
            if (b == B_ARP && bw_ui.page == BW_ARP)
            {
                bw_panel_clear(&bw);
                bw.arp = (uint8_t)((bw.arp + 1) % 3);
                if (!bw.arp)
                {
                    for (unsigned i = 0; i < BW_KEYS; i++)
                        bw.arp_held[i] = -1;
                    if (bw.live_note[k] >= 0)
                        bw_live_note(&bw, k, (unsigned)bw.live_note[k], 0, 0);
                    bw.live_note[k] = -1;
                }
                static const char *const arp_state[3] = {"ARP OFF", "ARP ON", "ARP LATCH"};
                app_say(arp_state[bw.arp], "");
            }
            else if(bw.mode==BW_MODE_FREE && b==B_HOME) bw_ui.page=BW_RIVER;
            else if(bw.mode==BW_MODE_FREE && b==B_SEQ) bw_page(BW_THEME);
            else bw_page(page_of[b]);
        }
    if (pressed & (1u << B_PLAY))
        bw_transport(&bw, !bw.playing);
    if ((pressed & (1u << B_REC)) && bw.mode==BW_MODE_RIVER)
    {
        /* a bite on any line: REC answers it (the chosen fish first), and that fish becomes the chosen one */
        int biting = -1;
        for (unsigned i = 0; i < 4; i++)
            if (bw_fish_state(&bw, (k + i) % 4) == BW_FISH_BITE)
            {
                biting = (int)((k + i) % 4);
                break;
            }
        int st = bw_fish_state(&bw, k);
        if (fighting)
        {
            if (bw.river.enabled)
                bw_river_reel(&bw, 2);
            else
                bw_fight(&bw, k, 4, now - bw_ui.fight_ms);
            bw_ui.fight_ms = now;
            bw_ui.reel_turns += 4;
        }
        else if (biting >= 0)
        {
            if (bw_hookset(&bw, (unsigned)biting))
            {
                if(!bw_keyboard_full(&bw))
                {
                    bw_ui.track=(uint8_t)biting;
                    bw_keyboard_layer(&bw,bw_ui.track);
                }
                bw_ui.page = BW_FISHING;
                bw_ui.fight_ms = now;
                app_say(bw.fish_rare[biting] ? "FISH ON!" : "HOOK SET", "REEL: SELECT OR REC");
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
    uint32_t changed=keys^bw_ui.keys;
    for(unsigned key=0;key<BW_KEYS;key++)
        if(changed>>key&1u) bw_panel_key(&bw,key,(keys>>key)&1u,100);
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
#include "bw_vector.c"

static void bw_bar(int32_t x, int32_t y, int32_t w, int value, int max, uint16_t c)
{
    int32_t f = max > 0 ? w * clamp(value, 0, max) / max : 0;
    L(x,y+2,x+w,y+2,C_BAR);
    bw_vector_oval(x+f,y+2,2,2,c);
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
static unsigned bw_weather_icon(void) /* the icons: day, dusk, night, storm */
{
    static const uint8_t cell[BW_NWEATHER] = {0, 1, 2, 3, 0};
    return cell[bw_weather(&bw)];
}
#ifndef APP_VERSION
#define APP_VERSION "DEV"
#endif
/* what PRESET does on this page, for the header */
static const char *bw_preset_hint(void)
{
    static const char *const sub_n[5] = {"", "PAGE 1/2", "PAGE 2/2", "", ""};
    static const char *const sub4[4] = {"PAGE 1/4", "PAGE 2/4", "PAGE 3/4", "PAGE 4/4"};
    unsigned sub = bw_ui.sub[bw_ui.page];
    switch (bw_ui.page)
    {
    case BW_RIVER:
        return "PRESET: WORLD";
    case BW_FISHING:
        return "PRESET: DEPTH";
    case BW_THEME:
        return sub ? "PRESET: SHAPE" : "PRESET: ACTIONS";
    case BW_LOG:
        return sub==2 ? "FIELD GUIDE" : sub ? "PRESET: TOTALS" : "PRESET: BEST SIZES";
    case BW_SCALE:
        return "PRESET: KEY";
    case BW_EDIT:
        return sub == 0 ? "PRESET: OP  EDIT: PAGE 1/4" : sub == 1 ? "PRESET: OP  EDIT: PAGE 2/4"
               : sub == 2 ? "PRESET: OP  EDIT: PAGE 3/4" : "PRESET: OP  EDIT: PAGE 4/4";
    case BW_ENV:
        return "PRESET: OPERATOR";
    case BW_LFO:
        return "PRESET: TARGET";
    case BW_FX:
        return sub4[sub % 4];
    case BW_ARP:
        return "PRESET: MODE";
    case BW_GLOBAL:
        return sub4[sub % 4];
    default:
        return sub_n[1 + sub % 2];
    }
}
/* the header: the pages as a row of icons (this one lit), the title and the transport, then the chosen fish (or
 * the latest catch, or a message) and what PRESET does here */
static void bw_header(const char *title, uint16_t c, const char *right)
{
    char t[48];
    uint32_t now = plat_ms();
    R(0, 0, 240, 48, C_BG);
    for (unsigned p = 0; p < BW_NPAGES; p++)
    {
        int32_t x = 3 + (int32_t)p * 19;
        if (p == bw_ui.page)
            R(x - 1, 0, 18, 17, C_PANEL);
        bw_vector_icon(p, x, 0, p == bw_ui.page ? c : C_MUTE);
        if (p == bw_ui.page)
            R(x - 1, 16, 18, 2, c);
    }
    T(4, 17, &FONT_B, title, c);
    TR(234, 18, &FONT_XS, right, bw.playing ? C_YEL : C_MUTE);
    bw_vector_weather(bw_weather_icon(), 234 - text_w(&FONT_XS, right) - 19, 16, 1);
    if (bw_ui.dirty)
        T(234 - text_w(&FONT_XS, right) - 28, 18, &FONT_XS, "*", C_PINK);
    t[0] = 0;
    uint16_t lc = bw_color(bw_ui.track);
    if ((int32_t)(bw_ui.popup_until - now) > 0)
    {
        /* the latest catch, in one line */
        const bw_event *ev = &bw_ui.popup;
        if (ev->result == BW_FIGHT_LANDED)
        {
            char l[16] = "";
            bw_cat(t, sizeof t, bw_species_names[ev->species]);
            bw_cat(t, sizeof t, " ");
            bw_length_text(l, ev->mm);
            bw_cat(t, sizeof t, l);
            if (ev->best)
                bw_cat(t, sizeof t, " BEST!");
        }
        else
            bw_cat(t, sizeof t, ev->result == BW_FIGHT_SNAPPED ? "LINE SNAPPED" : "IT GOT AWAY");
        lc = ev->rare ? C_GOLD : bw_color(ev->track);
    }
    else if (bw_ui.message[0] && (int32_t)(bw_ui.message_until - now) > 0)
    {
        bw_cat(t, sizeof t, bw_ui.message);
        lc = C_PINK;
    }
    else
    {
        bw_cat(t, sizeof t, bw.river.enabled ? bw_layers[bw_ui.track] : bw_species_names[bw_ui.track]);
        bw_cat(t, sizeof t, "  ");
        char n[8];
        bw_num(n, bw.song.tempo);
        bw_cat(t, sizeof t, n);
        bw_cat(t, sizeof t, " ");
        bw_cat(t, sizeof t, bw_key_names[bw.song.key]);
    }
    T(4, 33, &FONT_XS, t, lc);
    TR(236, 35, &FONT_T, bw_preset_hint(), C_MUTE);
    R(0, 47, 240, 1, C_BAR);
}
/* the knob bar: what K1-K4 do on this page, and their values, at the bottom of every screen */
typedef struct
{
    const char *label;
    char value[14];
    int v, max; /* max 0: no line */
} bw_knob_cell;
static void bw_kcell(bw_knob_cell *kc, const char *label, int v, int max, const char *text)
{
    kc->label = label;
    kc->v = v;
    kc->max = max;
    kc->value[0] = 0;
    if (text)
        bw_text_copy(kc->value, text, sizeof kc->value);
    else
        bw_num(kc->value, v);
}
static void bw_knob_bar(const bw_knob_cell *kc, uint16_t c)
{
    R(0, 211, 240, 29, C_PANEL);
    R(0, 211, 240, 1, C_BAR);
    for (unsigned i = 0; i < 4; i++)
    {
        int32_t x = (int32_t)i * 60;
        if (i)
            R(x, 214, 1, 24, C_BAR);
        if (!kc[i].label)
        {
            T(x + 5, 220, &FONT_T, "-", C_BAR);
            continue;
        }
        char k[4] = {'K', (char)('1' + i), 0, 0};
        T(x + 4, 212, &FONT_T, k, c);
        T(x + 18, 212, &FONT_T, kc[i].label, C_MUTE);
        const felucca_font_t *value_font=text_w(&FONT_XS,kc[i].value)>52 ? &FONT_T:&FONT_XS;
        char fitted[11];
        bw_text_copy(fitted,kc[i].value,sizeof fitted);
        if(text_w(value_font,kc[i].value)>52)
            fitted[7]=fitted[8]=fitted[9]='.';
        T(x + 4, 223, value_font, fitted, C_TXT);
        if (kc[i].max > 0)
        {
            int32_t w = 52 * clamp(kc[i].v, 0, kc[i].max) / kc[i].max;
            R(x + 4, 237, 52, 1, C_BAR);
            R(x + 4, 236, w, 2, c);
        }
    }
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
            bw_vector_box(lastx,lasty,cw-2,rowh>4 ? 3:rowh,bw_color(k));
        }
        else if (s->gate == BW_GATE_TIE && lastx >= 0)
            L(6+(int32_t)i*cw-1,lasty,6+(int32_t)(i+1)*cw-1,lasty,bw_color(k));
    }
    if (bw.playing)
    {
        unsigned st = (bw.step[k] + th->len - 1) % th->len;
        R(6 + (int32_t)st * cw, y0, cw, 2, C_YEL);
    }
}
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
        int32_t w = (int32_t)p->op[i].level * 20 / 100;
        bw_vector_box(xs[i],ys[i]-3,22,13,i == (int)sel ? C_YEL:C_BAR);
        char number[2]={(char)('1'+i),0};
        T(xs[i]+8,ys[i]-1,&FONT_T,number,p->carrier[i] ? c:C_MUTE);
        L(xs[i]+1,ys[i]+9,xs[i]+1+w,ys[i]+9,p->carrier[i] ? c:C_MUTE);
        if (p->op[i].feedback)
            R(xs[i] + 9, ys[i] - 2, 4, 1, C_PINK); /* feedback */
        if (p->carrier[i])
            L(xs[i] + 22, ys[i] + 3, 232, ys[i] + 3, C_BAR); /* to the output */
    }
}
#include "bw_river_ui.c"
#include "bw_free_ui.c"

static void bw_draw_page(uint32_t ms)
{
    char t[24];
    unsigned k = bw_ui.track, sub = bw_ui.sub[bw_ui.page];
    const bw_track *tr = &bw.track[k];
    const bw_song *s = &bw.song;
    const bw_operator *o = &tr->patch.op[bw_ui.op];
    uint16_t c = bw_color(k);
    bw_knob_cell kc[4];
    memset(kc, 0, sizeof kc);
    switch (bw_ui.page)
    {
    case BW_RIVER:
    case BW_FISHING:
        bw_draw_river_scene(ms, bw_ui.page == BW_FISHING);
        return;
    case BW_THEME:
        bw_draw_theme(52, 120, k);
        if (sub == 0)
        {
            T(6, 178, &FONT_XS, tr->has_prev ? "LAST CATCH CHANGED IT" : "AS IT STARTED", C_MUTE);
            T(6, 194, &FONT_T, "PRESET: RELEASE, HOME, RECORD", C_MUTE);
            bw_kcell(&kc[0], "LENGTH", tr->cur.len, 32, 0);
            bw_kcell(&kc[1], "DENSITY", tr->density, 100, 0);
            bw_kcell(&kc[2], "OCTAVE", tr->octave + 3, 6, 0);
            bw_num(kc[2].value, tr->octave);
            bw_kcell(&kc[3], "MUTE", tr->mute, 1, tr->mute ? "MUTED" : "OFF");
        }
        else
        {
            T(6, 178, &FONT_XS, "TURN A KNOB RIGHT TO DO IT", C_TXT);
            T(6, 194, &FONT_T, bw.rec_arm[k] ? "RECORDING: PLAY THE WHITE KEYS (K4 LEFT STOPS)" : "", C_PINK);
            bw_kcell(&kc[0], "RELEASE", 0, 0, tr->has_prev ? "UNDO" : "-");
            bw_kcell(&kc[1], "BACK TO", 0, 0, "HOME");
            bw_kcell(&kc[2], "KEEP AS", 0, 0, "HOME");
            bw_kcell(&kc[3], "RECORD", 0, 0, bw.rec_arm[k] ? "ON" : "KEYS");
        }
        break;
    case BW_LOG:
        if(sub==2)
        {
            for(unsigned cell=0;cell<BW_SPECIES;cell++)
            {
                unsigned sp=cell/2u+(cell%2u)*4u;
                int x=8+(int)(cell%2u)*116,y=52+(int)(cell/2u)*42;
                bw_vector_fish(x+18,y+6,sp,10,1.25f,C_TXT);
                int tx=x+(110-text_w(&FONT_T,bw_species_names[sp]))/2;
                T(tx,y+33,&FONT_T,bw_species_names[sp],sp>=4 ? C_YEL:C_MUTE);
            }
            return;
        }
        if (sub == 0)
            for (unsigned i = 0; i < 8; i++)
            {
                unsigned sp = i / 2 + (i % 2) * 4;
                const bw_species_log *l = &bw.log.s[sp];
                int32_t y = 50 + (int32_t)i * 20;
                T(6, y, &FONT_XS, bw_species_names[sp], sp >= 4 ? C_GOLD : bw_color(sp % 4));
                bw_num(t, (int)l->count);
                TR(132, y, &FONT_XS, t, C_TXT);
                uint32_t mx = bw_species_max_mm(sp), b = l->best_mm;
                L(140,y+8,230,y+8,C_BAR);
                bw_vector_oval(140+(int)(b>=mx ? 90:b*90/mx),y+8,2,2,b>mx ? C_GOLD:bw_color(sp%4));
            }
        else
        {
            static const char *const names[4] = {"MISSED BITES", "SNAPPED LINES", "THROWN HOOKS", "RELEASED"};
            const uint32_t v[4] = {bw.log.missed, bw.log.snapped, bw.log.thrown, bw.log.released};
            for (unsigned i = 0; i < 4; i++)
            {
                T(6, 58 + (int32_t)i * 36, &FONT_XS, names[i], C_MUTE);
                bw_num(t, (int)v[i]);
                TR(234, 54 + (int32_t)i * 36, &FONT_B, t, C_TXT);
            }
        }
        T(4, 218, &FONT_XS, sub ? "BARS: BEST VS. USUAL TOP SIZE" : "COUNT, AND THE BEST AS A BAR", C_MUTE);
        return;
    case BW_SCALE:
    {
        static const char *const roman[7] = {"i", "ii", "iii", "iv", "v", "vi", "vii"};
        int r = bw_chord_root(&bw);
        T(6, 54, &FONT_XS, "CHORD NOW", C_MUTE);
        T(90, 50, &FONT_B, roman[((r % 7) + 7) % 7], C_PINK);
        T(6, 74, &FONT_XS, bw_progression_names[s->progression], C_TXT);
        int in[12] = {0}, chord[12] = {0};
        for (int d = -14; d < 14; d++)
            in[bw_note_of(&bw, 2, d, 0) % 12] = 1;
        for (int d = 0; d <= 4; d += 2)
            chord[bw_note_of(&bw, 2, d + r, 0) % 12] = 1;
        static const uint8_t white_pc[7] = {0, 2, 4, 5, 7, 9, 11}, black_after[5] = {0, 1, 3, 4, 5},
                             black_pc[5] = {1, 3, 6, 8, 10};
        for (int w = 0; w < 7; w++)
        {
            int pc = white_pc[w];
            int32_t x = 8 + w * 32;
            bw_vector_box(x,100,30,72,chord[pc] ? C_PINK:in[pc] ? C_YEL:C_MUTE);
            if (pc == s->key)
                bw_vector_oval(x+15,164,3,3,C_YEL);
        }
        for (int b = 0; b < 5; b++)
        {
            int pc = black_pc[b];
            int32_t x = 8 + (black_after[b] + 1) * 32 - 10;
            R(x - 1, 100, 20, 44, C_BG);
            bw_vector_box(x+1,100,16,42,chord[pc] ? C_PINK:in[pc] ? C_YEL:C_MUTE);
        }
        T(8, 182, &FONT_XS, "IN THE SCALE", C_YEL);
        T(110, 182, &FONT_XS, "IN THE CHORD", C_PINK);
        bw_kcell(&kc[0], "KEY", s->key, 11, bw_key_names[s->key]);
        bw_kcell(&kc[1], "SCALE", s->scale, BW_SCALES - 1, bw_scale_names[s->scale]);
        bw_kcell(&kc[2], "CHORDS", s->progression, BW_PROGRESSIONS - 1, s->progression ? "ON" : "OFF");
        if (s->progression)
            bw_text_copy(kc[2].value, bw_progression_names[s->progression], sizeof kc[2].value);
        bw_kcell(&kc[3], "SWING", s->swing, 60, 0);
        bw_knob_bar(kc, C_YEL);
        return;
    }
    case BW_EDIT:
    {
        static const char *const subn[4] = {"OPERATOR", "FREQUENCY", "ROUTING", "PATCHES"};
        bw_num(t, (int)bw_ui.op + 1);
        T(6, 52, &FONT_XS, subn[sub], C_TXT);
        T(120, 52, &FONT_XS, "OPERATOR", C_MUTE);
        TR(200, 50, &FONT_B, t, C_YEL);
        T(204, 54, &FONT_T, "PRESET", C_MUTE);
        bw_draw_algorithm(&tr->patch, 72, 96, bw_ui.op, c);
        if (sub == 3)
        {
            char u[24] = "";
            bw_cat(u, sizeof u, bw_ui.patch_pick < 4 ? "FACTORY: " : "YOURS: ");
            bw_cat(u, sizeof u, bw_ui.patch_pick < 4 ? bw_patch_name(k, bw_ui.patch_pick) : "USER");
            T(6, 176, &FONT_B, u, C_TXT);
            T(6, 194, &FONT_T, "K2 RIGHT LOADS IT. K3 RIGHT STORES TO A USER SLOT", C_MUTE);
        }
        else
            T(6, 180, &FONT_T, "KEYS PLAY / BLACK KEYS BEND FM", C_MUTE);
        if (sub == 0)
        {
            bw_kcell(&kc[0], "RATIO", o->ratio, 1600, 0);
            bw_kcell(&kc[1], "LEVEL", o->level, 100, 0);
            bw_kcell(&kc[2], "FEEDBK", o->feedback, 100, 0);
            bw_kcell(&kc[3], "ALGO", tr->patch.algorithm, 7, 0);
            bw_num(kc[3].value, tr->patch.algorithm + 1);
        }
        else if (sub == 1)
        {
            bw_kcell(&kc[0], "FIXED", o->fixed, 1, o->fixed ? "ON" : "OFF");
            bw_kcell(&kc[1], "HZ", o->hz, 16000, 0);
            bw_kcell(&kc[2], "VELOC", o->velocity, 100, 0);
            bw_kcell(&kc[3], "OUTPUT", tr->patch.carrier[bw_ui.op], 100, 0);
        }
        else if (sub == 2)
        {
            bw_kcell(&kc[0], "FROM", bw_ui.route_source, 5, 0);
            bw_num(kc[0].value, bw_ui.route_source + 1);
            bw_kcell(&kc[1], "INTO", bw_ui.route_dest, 5, 0);
            bw_num(kc[1].value, bw_ui.route_dest + 1);
            bw_kcell(&kc[2], "AMOUNT", tr->patch.route[bw_ui.route_source][bw_ui.route_dest], 100, 0);
            bw_kcell(&kc[3], "OUT", tr->patch.carrier[bw_ui.route_source], 100, 0);
        }
        else
        {
            char u[14] = "";
            bw_cat(u, sizeof u, bw_ui.patch_pick < 4 ? "F" : "U");
            bw_num(u + 1, bw_ui.patch_pick % 4 + 1);
            bw_kcell(&kc[0], "PATCH", bw_ui.patch_pick, 7, u);
            bw_kcell(&kc[1], "LOAD", 0, 0, "RIGHT");
            bw_kcell(&kc[2], "STORE", 0, 0, bw_ui.patch_pick >= 4 ? "RIGHT" : "U1-U4");
        }
        break;
    }
    case BW_ENV:
    {
        bw_num(t, (int)bw_ui.op + 1);
        T(6, 52, &FONT_XS, "OPERATOR", C_MUTE);
        TR(90, 50, &FONT_B, t, C_YEL);
        int32_t x0 = 6, y0 = 196, w = 228, hgt = 120;
        float total = (float)o->attack + (float)o->decay + 400.0f + (float)o->release;
        int32_t xa = x0 + (int32_t)(w * (float)o->attack / total), xd = xa + (int32_t)(w * (float)o->decay / total);
        int32_t xs = xd + (int32_t)(w * 400.0f / total), ys = y0 - hgt * o->sustain / 100;
        R(x0, y0 - hgt - 4, w, hgt + 8, C_PANEL);
        for (int32_t x = x0; x < x0 + w; x++)
        {
            int32_t y = x < xa ? y0 - hgt * (x - x0) / (xa - x0 + 1)
                        : x < xd ? y0 - hgt + (hgt - (y0 - ys)) * (x - xa) / (xd - xa + 1)
                        : x < xs ? ys
                                 : ys + (y0 - ys) * (x - xs) / (x0 + w - xs + 1);
            R(x, y, 1, 2, c);
        }
        bw_kcell(&kc[0], "ATTACK", o->attack, 10000, 0);
        bw_kcell(&kc[1], "DECAY", o->decay, 10000, 0);
        bw_kcell(&kc[2], "SUSTAIN", o->sustain, 100, 0);
        bw_kcell(&kc[3], "RELEASE", o->release, 10000, 0);
        break;
    }
    case BW_LFO:
    {
        R(6, 56, 228, 130, C_PANEL);
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
            R(6 + x, 121 - (int32_t)(v * 56.0f * (float)tr->lfo_depth / 100.0f), 1, 2, c);
        }
        R(6 + (int32_t)(bw.lfo_phase[k] * 227.0f), 56, 2, 130, C_YEL);
        T(8, 192, &FONT_XS, bw_lfo_target_names[tr->lfo_target], C_TXT);
        bw_kcell(&kc[0], "RATE", tr->lfo_rate, 100, 0);
        bw_kcell(&kc[1], "DEPTH", tr->lfo_depth, 100, 0);
        bw_kcell(&kc[2], "TARGET", tr->lfo_target, BW_NLFO_TARGET - 1, bw_lfo_target_names[tr->lfo_target]);
        bw_kcell(&kc[3], "SHAPE", tr->lfo_shape, BW_NLFO_SHAPE - 1, bw_lfo_shape_names[tr->lfo_shape]);
        break;
    }
    case BW_FX:
    {
        static const char *const titles[4] = {"LAYER SENDS", "REVERB, ECHO TIME", "ECHO, CHORUS", "MASTER, LAYER PAN"};
        static const char *const lab[4][4] = {{"REVERB", "ECHO", "CHORUS", "DRIVE"},
                                              {"SIZE", "TONE", "LEVEL", "TIME"},
                                              {"FEEDBK", "LEVEL", "RATE", "DEPTH"},
                                              {"TONE", "WIDTH", "PAN", "LEVEL"}};
        static const char *const dtime[6] = {"1/16", "1/8T", "1/8", "1/8.", "1/4", "1/4."};
        const uint8_t v[4][4] = {{tr->send_rev, tr->send_dly, tr->send_cho, tr->drive},
                                 {s->rev_size, s->rev_tone, s->rev_level, s->dly_time},
                                 {s->dly_feedback, s->dly_level, s->cho_rate, s->cho_depth},
                                 {s->tone, s->width, tr->pan, tr->volume}};
        T(6, 52, &FONT_XS, titles[sub], sub == 0 || sub == 3 ? c : C_YEL);
        /* the four as columns: a mixer */
        for (unsigned i = 0; i < 4; i++)
        {
            int32_t x = 14 + (int32_t)i * 58, h = 120 * v[sub][i] / 100;
            L(x+15,72,x+15,192,C_BAR);
            for (int j=0;j<=4;j++) L(x+11,72+j*30,x+19,72+j*30,C_MUTE);
            bw_vector_oval(x+15,192-h,5,5,sub == 0 || sub == 3 ? c:C_YEL);
            bw_kcell(&kc[i], lab[sub][i], v[sub][i], 100, 0);
        }
        bw_text_copy(kc[sub == 1 ? 3 : 0].value, sub == 1 ? dtime[v[1][3] * 6u / 101u] : kc[0].value, sizeof kc[0].value);
        break;
    }
    case BW_ARP:
    {
        static const char *const st[3] = {"OFF", "ON", "LATCH"};
        T(6, 52, &FONT_XS, "ARP BUTTON AGAIN: OFF / ON / LATCH", C_MUTE);
        T(6, 68, &FONT_M, st[bw.arp], bw.arp ? C_PINK : C_MUTE);
        /* the pattern: eight steps of the chosen mode over three held notes */
        static const int8_t pat[BW_NARP_MODE][8] = {{0, 1, 2, 0, 1, 2, 0, 1}, {2, 1, 0, 2, 1, 0, 2, 1},
                                                    {0, 1, 2, 1, 0, 1, 2, 1}, {1, 0, 2, 2, 0, 1, 0, 2},
                                                    {0, 2, 1, 0, 2, 1, 0, 2}};
        for (unsigned i = 0; i < 8; i++)
            bw_vector_box(10+(int32_t)i*28,180-pat[s->arp_mode][i]*26-14,22*s->arp_gate/100+2,12,c);
        T(6, 190, &FONT_T, "HOLD WHITE KEYS: THE CHOSEN LAYER ARPEGGIATES", C_MUTE);
        bw_kcell(&kc[0], "MODE", s->arp_mode, BW_NARP_MODE - 1, bw_arp_mode_names[s->arp_mode]);
        bw_kcell(&kc[1], "RATE", s->arp_rate, 4, bw_arp_rate_names[s->arp_rate]);
        bw_kcell(&kc[2], "OCTAVES", s->arp_octaves, 3, 0);
        bw_kcell(&kc[3], "GATE", s->arp_gate, 100, 0);
        break;
    }
    case BW_GLOBAL:
        if (sub == 0)
        {
            bw_num(t, s->tempo);
            T(6, 52, &FONT_L, t, C_YEL);
            T(70, 64, &FONT_XS, "BPM", C_MUTE);
            bw_vector_weather(bw_weather_icon(), 140, 56, 2);
            T(176, 66, &FONT_XS, bw_weather_names[bw_weather(&bw)], C_TXT);
            char u[32] = "AUDIO LOAD ";
            bw_num(t, (int)plat_cpu_pct());
            bw_cat(u, sizeof u, t);
            bw_cat(u, sizeof u, "%  XRUNS ");
            bw_num(t, (int)plat_xruns());
            bw_cat(u, sizeof u, t);
            T(6, 110, &FONT_XS, u, plat_cpu_pct() > 85 ? C_PINK : C_MUTE);
            T(6, 130, &FONT_XS, "BLACKWATER " APP_VERSION, C_MUTE);
            bw_kcell(&kc[0], "TEMPO", s->tempo, 240, 0);
            bw_kcell(&kc[1], "CLOCK", s->sync, 1, s->sync ? "MIDI" : "INT");
            bw_kcell(&kc[2], "WEATHER", s->weather, BW_NWEATHER - 1, bw_weather_names[s->weather]);
            bw_kcell(&kc[3], "BEST=KEY", s->pb_event, 1, s->pb_event ? "ON" : "OFF");
        }
        else if (sub == 1)
        {
            T(6, 56, &FONT_XS, "UNITS, THE FISH-ON CLIP, AUTOSAVE", C_MUTE);
            T(6, 80, &FONT_B, bw_pref.units ? "INCHES AND POUNDS" : "CENTIMETRES AND KILOS", C_TXT);
            bw_kcell(&kc[0], "UNITS", bw_pref.units, 1, bw_pref.units ? "IN/LB" : "CM/KG");
            bw_kcell(&kc[1], "CLIP", bw_pref.clip_on, 1, bw_pref.clip_on ? "ON" : "OFF");
            bw_kcell(&kc[2], "CLIP LVL", bw_pref.clip_level, 100, 0);
            bw_kcell(&kc[3], "AUTOSAVE", bw_pref.autosave, 1, bw_pref.autosave ? "ON" : "OFF");
        }
        else if(sub==2)
        {
            /* the four slots as boxes: this one lit, the chosen one outlined */
            for (unsigned i = 0; i < 4; i++)
            {
                int32_t x = 8 + (int32_t)i * 58;
                bw_vector_box(x,64,50,60,i == bw_ui.slot_pick ? C_YEL:C_BAR);
                bw_num(t, (int)i + 1);
                T(x + 19, 82, &FONT_B, t, i == bw_pref.slot ? C_YEL : C_MUTE);
            }
            T(6, 136, &FONT_XS, "K1 PICKS A SLOT; K2 RIGHT LOADS IT,", C_MUTE);
            T(6, 152, &FONT_XS, "K3 RIGHT SAVES THIS SESSION INTO IT", C_MUTE);
            bw_num(t, bw_ui.slot_pick + 1);
            bw_kcell(&kc[0], "SLOT", bw_ui.slot_pick, 3, t);
            bw_kcell(&kc[1], "LOAD", 0, 0, "RIGHT");
            bw_kcell(&kc[2], "SAVE", 0, 0, "RIGHT");
            bw_kcell(&kc[3], "MIDI OUT", bw_pref.midi_out, 1, bw_pref.midi_out ? "CH 1-4" : "OFF");
        }
        if(sub==3)
        {
            T(24,62,&FONT_B,"PERFORMANCE",C_TXT);
            T(24,92,&FONT_XS,"RIVER / ONE KEYBOARD VOICE",C_MUTE);
            T(24,114,&FONT_XS,"FREE / SELECT AN OPERATOR",C_MUTE);
            T(24,148,&FONT_T,"FREE PLAY USES ALL 27 KEYS",C_MUTE);
            bw_kcell(&kc[0],"MODE",bw.mode,1,bw.mode==BW_MODE_FREE ? "FREE":"RIVER");
            bw_kcell(&kc[1],"RIVERKEY",bw.keyboard_unlocked,1,bw.keyboard_unlocked ? "ALL 27":"SCALE");
            bw_kcell(&kc[2],"VOICE",bw.live_track,3,bw_layers[bw.live_track]);
            bw_kcell(&kc[3],"OCTAVE",bw.live_oct+2,4,0);bw_num(kc[3].value,bw.live_oct);
        }
        bw_knob_bar(kc, C_YEL);
        return;
    default:
    {
        static const char *const help[2][8] = {
            {"SELECT / PAGE", "PRESET / SEE HEADER", "K1-K4 / PARAMETERS", "ALGO / SOUND LAYER",
             "PLAY / START OR PAUSE", "REC / PLACE, LIFT OR HOOK", "SELECT OR K1 / REEL", "SAVE / KEEP SESSION"},
            {"HOME / RIVER, THEN CATCH LOG", "LOG + PRESET / FIELD GUIDE", "SEQ / LINE, THEN MOTIF",
             "EDIT / FM, AGAIN FOR NEXT PAGE", "ENV / ENVELOPE   LFO / MODULATION", "FX / SPACE   ARP / ARPEGGIATOR",
             "GLO PAGE 4 / FREE PLAY, ALL KEYS", "FREE SELECT / OP   ALGO / VOICE"}};
        for (unsigned i = 0; i < 8; i++)
            T(6, 52 + (int32_t)i * 20, &FONT_XS, help[sub][i], i % 2 ? C_TXT : C_MUTE);
        T(4, 218, &FONT_XS, "BLACKWATER " APP_VERSION, C_MUTE);
        return;
    }
    }
    bw_knob_bar(kc, c);
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
    static uint8_t was_biting[4];
    int any_bite = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        int b = bw_fish_state(&bw, i) == BW_FISH_BITE;
        if (b && !was_biting[i])
            app_say("BITE!", bw.river.enabled ? "REC SETS THE HOOK" : bw_species_names[i]);
        was_biting[i] = (uint8_t)b;
        any_bite |= b;
    }
    right[0] = 0;
    bw_text_copy(right, bw.playing ? "PLAYING" : "STOPPED", sizeof right);
    for (unsigned band = 0; band < 5; band++)
    {
        bw_oy = (int32_t)band * 48;
        cv_begin(240, 48, C_BG);
        if(bw.mode==BW_MODE_FREE && bw_ui.page==BW_RIVER)
            bw_draw_free_play();
        else if (bw.river.enabled && (bw_ui.page == BW_RIVER || bw_ui.page == BW_FISHING))
            bw_draw_river_scene(ms, bw_ui.page == BW_FISHING);
        else if (band == 0)
            bw_header(bw_pages[bw_ui.page], bw_ui.page == BW_FX || bw_ui.page == BW_SCALE || bw_ui.page == BW_GLOBAL
                                                ? C_YEL
                                                : bw_color(bw_ui.track),
                      right);
        else
            bw_draw_page(ms);
        lcd_blit(0, band * 48, 240, 48, cv_px);
    }
    /* lights: PLAY, SAVE while unsaved, REC at a bite, ARP while on; the white keys walk this fish's steps; held
     * black keys lit */
    static const uint8_t white[16] = PLAT_WHITE_KEYS;
    unsigned k = bw_ui.track;
    uint32_t leds = (bw.playing ? 1u << B_PLAY : 0) | (bw_ui.dirty ? 1u << B_SAVE : 0) |
                    (any_bite && (ms / 150u) % 2u ? 1u << B_REC : 0) | (bw.arp ? 1u << B_ARP : 0);
    uint32_t keyleds = bw_ui.keys;
    if (bw.playing && bw.mode==BW_MODE_RIVER)
        keyleds |= 1u << white[(bw.step[k] + bw.track[k].cur.len - 1) % bw.track[k].cur.len % 16];
    plat_unlock(lock);
    plat_leds(leds, keyleds);
    bw_ui.last_draw = ms;
    bw_ui.draw = 0;
}
/* the boot screen: the river at night, the wordmark, the line */
void app_splash(void)
{
    bw_vector_opacity=bw_vector_trace=bw_vector_sky=256;
    for (unsigned band = 0; band < 5; band++)
    {
        bw_oy = (int32_t)band * 48;
        cv_begin(240, 48, C_BG);
        bw_vector_river(0, 0, 0);
        R(20, 75, 200, 81, C_BG);
        L(25, 76, 215, 76, C_BAR);
        T(40, 90, &FONT_M, APP_NAME, C_TXT);
        T(56, 125, &FONT_XS, "A RIVER IN SOUND", C_MUTE);
        TR(213, 151, &FONT_T, APP_VERSION, C_MUTE);
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
    if (bw_ui.draw || plat_ms() - bw_ui.last_draw >= 40)
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
    unsigned notes = 0, active = 0, held = 0;
    for (unsigned i = 0; i < t->cur.len; i++)
        notes += t->cur.s[i].gate == BW_GATE_NOTE;
    for (unsigned i = 0; i < BW_VOICES; i++)
    {
        active += bw.voice[i].active;
        held += bw.voice[i].active && bw.voice[i].live && bw.voice[i].held;
    }
    uint32_t catches = 0, rares = 0;
    for (unsigned i = 0; i < BW_SPECIES; i++)
    {
        catches += bw.log.s[i].count;
        if (i >= 4) rares += bw.log.s[i].count;
    }
    const struct
    {
        const char *name;
        int value;
    } checks[] = {{"track", (int)k},
                  {"mode",bw.mode}, {"unlocked",bw.keyboard_unlocked}, {"keyboard_full",bw_keyboard_full(&bw)},
                  {"keyboard_layer",bw.live_track}, {"life",bw.river.life},
                  {"held",(int)held}, {"physical_note",bw.key_note[0]}, {"black_note",bw.key_note[1]},
                  {"meteors",(int)bw.river.life_count[BW_LIFE_METEOR]},
                  {"ufos",(int)bw.river.life_count[BW_LIFE_UFO]},
                  {"jumps",(int)bw.river.life_count[BW_LIFE_JUMP]},
                  {"leaves",(int)bw.river.life_count[BW_LIFE_LEAF]},
                  {"bees",(int)bw.river.life_count[BW_LIFE_BEE]},
                  {"fireflies",(int)bw.river.life_count[BW_LIFE_FIREFLY]},
                  {"motif_notes",(int)bw.river.motif_notes},
                  {"river_phase", bw.river.phase},
                  {"river_bloom", (int)(bw.river.bloom * 100)},
                  {"world", bw.river.world},
                  {"river_line", bw.river.line},
                  {"line_state",bw_fish_state(&bw,bw.river.line)},
                  {"river_memory", bw.river.memory},
                  {"page", bw_ui.page},
                  {"sub", bw_ui.sub[bw_ui.page]},
                  {"playing", bw.playing},
                  {"state", bw_fish_state(&bw, k)},
                  {"hooked", bw_fish_state(&bw, k) != BW_FISH_IDLE},
                  {"bite", bw_fish_state(&bw, k) == BW_FISH_BITE},
                  {"fight", bw_fish_state(&bw, k) == BW_FISH_FIGHT},
                  {"catches", (int)catches},
                  {"rare", (int)rares},
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
                  {"ratio", t->patch.op[bw_ui.op].ratio},
                  {"fixed", t->patch.op[bw_ui.op].fixed},
                  {"hz", t->patch.op[bw_ui.op].hz},
                  {"feedback", t->patch.op[bw_ui.op].feedback},
                  {"route", t->patch.route[bw_ui.route_source][bw_ui.route_dest]},
                  {"attack", t->patch.op[bw_ui.op].attack},
                  {"release_ms", t->patch.op[bw_ui.op].release},
                  {"volume", t->volume},
                  {"weather", bw.song.weather},
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
