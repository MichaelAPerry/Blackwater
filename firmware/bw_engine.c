/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "bw_engine.h"
#include "fastmath.h"
/* Embedded-safe: fixed allocation, no system calls, no allocation and no storage in the render. */
static float bw_sine[1025], bw_note_hz[128];
/* fastmath's exp2 shifts a negative exponent (fine on the PI32, undefined in C): only non-negative ones here */
static float bw_exp2(float x)
{
    return x < 0.0f ? 1.0f / fm_exp2f(-x) : fm_exp2f(x);
}
static float bw_pow(float x, float y)
{
    return x <= 0.0f ? 0.0f : bw_exp2(y * fm_log2f(x));
}
static float bw_exp(float x)
{
    return bw_exp2(x * FM_LOG2E);
}
static float pan_l[4], pan_r[4], vol[4], srev[4], sdly[4], scho[4], drv[4]; /* per track, every 32 frames */
static int bw_gains_set;
static void bw_algorithm_impl(bw_patch *t, unsigned algorithm);
static void bw_voice_prepare(bw_engine *e, bw_voice *v);
void bw_algorithm(bw_patch *p, unsigned algorithm)
{
    bw_algorithm_impl(p, algorithm);
}
#include "bw_species.c"

const char *const bw_scale_names[BW_SCALES] = {"MINOR",  "DORIAN",   "MAJOR",       "MIXOLYDIAN",
                                               "HARM MINOR", "PHRYGIAN", "MINOR PENTA", "BLUES"};
static const struct
{
    uint8_t n, semi[7];
} bw_scales[BW_SCALES] = {{7, {0, 2, 3, 5, 7, 8, 10}}, {7, {0, 2, 3, 5, 7, 9, 10}}, {7, {0, 2, 4, 5, 7, 9, 11}},
                          {7, {0, 2, 4, 5, 7, 9, 10}}, {7, {0, 2, 3, 5, 7, 8, 11}}, {7, {0, 1, 3, 5, 7, 8, 10}},
                          {5, {0, 3, 5, 7, 10, 0, 0}}, {6, {0, 3, 5, 6, 7, 10, 0}}};
const char *const bw_progression_names[BW_PROGRESSIONS] = {"OFF", "i VI III VII", "i iv v i", "i VII VI VII",
                                                           "I V vi IV", "12 BAR"};
static const struct
{
    uint8_t n;
    int8_t root[12];
} bw_progressions[BW_PROGRESSIONS] = {{1, {0}},          {4, {0, 5, 2, 6}}, {4, {0, 3, 4, 0}},
                                      {4, {0, 6, 5, 6}}, {4, {0, 4, 5, 3}}, {12, {0, 0, 0, 0, 3, 3, 0, 0, 4, 3, 0, 4}}};
const char *const bw_key_names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const char *const bw_bait_names[BW_NBAIT] = {"WORM", "CRANKBAIT", "SPINNER", "JIG"};
const char *const bw_depth_names[BW_NDEPTH] = {"SHALLOW", "MID", "DEEP", "BOTTOM"};
const char *const bw_weather_names[BW_NWEATHER] = {"AUTO", "DUSK", "NIGHT", "STORM", "DAY"};
const char *const bw_lfo_target_names[BW_NLFO_TARGET] = {"PITCH", "FM INDEX", "VOLUME", "PAN"};
const char *const bw_lfo_shape_names[BW_NLFO_SHAPE] = {"SINE", "TRIANGLE", "SQUARE", "RANDOM"};
const char *const bw_arp_mode_names[BW_NARP_MODE] = {"UP", "DOWN", "UP-DOWN", "RANDOM", "AS PLAYED"};
const char *const bw_arp_rate_names[5] = {"1/4", "1/8", "1/8T", "1/16", "1/32"};
const char *const bw_macro_names[BW_NMACRO] = {"BRIGHT", "DARK", "GROWL", "METAL", "DIVE", "RISE",
                                               "STUTTER", "HALF", "CUT", "OCTAVE", "THROW"};
static const uint8_t bw_arp_ticks[5] = {96, 48, 32, 24, 12};
/* each fish's register (MIDI note of its root at octave 0), steps per beat, gate (% of the step) and range */
static const uint8_t bw_base_note[4] = {60, 36, 48, 48}, bw_spb[4] = {4, 4, 3, 2}, bw_gate_pct[4] = {50, 65, 95, 98};
static const int8_t bw_deg_lo[4] = {-3, -7, -4, -7}, bw_deg_hi[4] = {14, 9, 14, 7};
static const uint8_t bw_mono[4] = {0, 1, 0, 1};

static uint32_t bw_rand(uint32_t *s)
{
    uint32_t x = *s ? *s : 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}
static uint32_t bw_random(bw_engine *e)
{
    return bw_rand(&e->rng);
}
static float bw_unit(bw_engine *e)
{
    return (float)(bw_random(e) >> 8) * (1.0f / 16777216.0f);
}
static float bw_sin(float cycles)
{
    float x = cycles - fm_floorf(cycles);
    x *= 1024.0f;
    unsigned i = (unsigned)x;
    if (i > 1023)
        i = 1023;
    float f = x - (float)i;
    return bw_sine[i] + f * (bw_sine[i + 1] - bw_sine[i]);
}
#include "bw_fx.c"
static void bw_algorithm_impl(bw_patch *t, unsigned algorithm)
{
    unsigned i, j;
    t->algorithm = (uint8_t)(algorithm % 8);
    for (i = 0; i < 6; i++)
    {
        t->carrier[i] = 0;
        for (j = 0; j < 6; j++)
            t->route[i][j] = 0;
    }
    switch (t->algorithm)
    {
    case 0:
        for (i = 1; i < 6; i++)
            t->route[i][i - 1] = 100;
        t->carrier[0] = 100;
        break;
    case 1:
        for (i = 0; i < 6; i += 2)
        {
            t->route[i + 1][i] = 100;
            t->carrier[i] = 100;
        }
        break;
    case 2:
        t->route[5][4] = t->route[4][3] = t->route[2][1] = t->route[1][0] = 100;
        t->carrier[0] = t->carrier[3] = 100;
        break;
    case 3:
        for (i = 1; i < 6; i++)
            t->route[i][0] = 100;
        t->carrier[0] = 100;
        break;
    case 4:
        for (i = 0; i < 5; i++)
        {
            t->route[5][i] = 100;
            t->carrier[i] = 100;
        }
        break;
    case 5:
        for (i = 0; i < 6; i++)
            t->carrier[i] = 100;
        break;
    case 6:
        t->route[5][4] = t->route[4][0] = t->route[3][2] = t->route[2][0] = t->route[1][0] = 100;
        t->carrier[0] = 100;
        break;
    default:
        t->route[5][2] = t->route[4][2] = t->route[3][1] = t->route[2][0] = 100;
        t->carrier[0] = t->carrier[1] = 100;
        break;
    }
}

/* ------------------------------------------------------------------ validation */
static int bw_theme_valid(const bw_theme *th)
{
    if (th->len < 4 || th->len > BW_STEPS)
        return 0;
    for (unsigned i = 0; i < BW_STEPS; i++)
        if (th->s[i].deg < -21 || th->s[i].deg > 28 || th->s[i].gate > BW_GATE_TIE || th->s[i].vel > 127)
            return 0;
    return 1;
}
int bw_track_valid(const bw_track *t)
{
    unsigned i, j;
    const bw_patch *p = &t->patch;
    if (p->algorithm > 7 || t->volume > 100 || t->pan > 100 || t->mute > 1 || t->solo > 1 || t->density > 100 ||
        t->octave < -3 || t->octave > 3 || t->send_rev > 100 || t->send_dly > 100 || t->send_cho > 100 ||
        t->drive > 100 || t->lfo_rate > 100 || t->lfo_depth > 100 || t->lfo_target >= BW_NLFO_TARGET ||
        t->lfo_shape >= BW_NLFO_SHAPE || t->wait_min > t->wait_max || t->wait_max > 600 || t->manual > 1 ||
        t->bait >= BW_NBAIT || t->depth >= BW_NDEPTH || t->has_prev > 1 || t->patch_slot > 7)
        return 0;
    for (i = 0; i < 6; i++)
    {
        const bw_operator *o = &p->op[i];
        if (!o->ratio || o->ratio > 1600 || !o->hz || o->hz > 16000 || o->attack > 10000 || o->decay > 10000 ||
            o->release > 10000 || o->level > 100 || o->sustain > 100 || o->feedback > 100 || o->velocity > 100 ||
            o->fixed > 1 || p->carrier[i] > 100)
            return 0;
        for (j = 0; j < 6; j++)
            if (p->route[i][j] > 100 || (i <= j && p->route[i][j]))
                return 0;
    }
    return bw_theme_valid(&t->home) && bw_theme_valid(&t->cur) && bw_theme_valid(&t->prev);
}
int bw_song_valid(const bw_song *s)
{
    return s->tempo >= 30 && s->tempo <= 240 && s->key < 12 && s->scale < BW_SCALES &&
           s->progression < BW_PROGRESSIONS && s->swing <= 60 && s->pb_event <= 1 && s->weather < BW_NWEATHER &&
           s->sync <= 1 && s->rev_size <= 100 && s->rev_tone <= 100 && s->rev_level <= 100 && s->dly_time <= 100 &&
           s->dly_feedback <= 100 && s->dly_level <= 100 && s->cho_rate <= 100 && s->cho_depth <= 100 &&
           s->tone <= 100 && s->width <= 100 && s->arp_mode < BW_NARP_MODE && s->arp_rate < 5 &&
           s->arp_octaves >= 1 && s->arp_octaves <= 3 && s->arp_gate >= 5 && s->arp_gate <= 100;
}

/* ------------------------------------------------------------------ pitch */
unsigned bw_steps_per_beat(unsigned k)
{
    return bw_spb[k % 4];
}
int bw_note_of(const bw_engine *e, unsigned k, int deg, int with_chord)
{
    const bw_song *s = &e->song;
    unsigned n = bw_scales[s->scale % BW_SCALES].n;
    if (with_chord)
        deg += bw_progressions[s->progression % BW_PROGRESSIONS].root[e->chord % 12];
    int q = deg >= 0 ? deg / (int)n : -((-deg + (int)n - 1) / (int)n);
    int r = deg - q * (int)n;
    int note = bw_base_note[k % 4] + e->track[k % 4].octave * 12 + s->key + q * 12 +
               bw_scales[s->scale % BW_SCALES].semi[r];
    return note < 0 ? 0 : note > 127 ? 127 : note;
}

int bw_chord_root(const bw_engine *e)
{
    return bw_progressions[e->song.progression % BW_PROGRESSIONS].root[e->chord % 12];
}

/* ------------------------------------------------------------------ themes */
static void bw_step_set(bw_step *st, int deg, unsigned gate, unsigned vel)
{
    st->deg = (int8_t)deg;
    st->gate = (uint8_t)gate;
    st->vel = (uint8_t)(vel < 1 ? 1 : vel > 127 ? 127 : vel);
}
void bw_theme_generate(bw_engine *e, unsigned k, uint32_t seed)
{
    bw_theme *th = &e->track[k].cur;
    uint32_t r = seed | 1u;
    unsigned i;
    for (i = 0; i < BW_STEPS; i++)
        bw_step_set(&th->s[i], 0, BW_GATE_REST, 90);
    switch (k % 4)
    {
    case BW_BLUEGILL:
    {
        /* the lead: a busy pentatonic-leaning line, short notes */
        static const uint16_t rhythm[4] = {0xB5B4, 0xDAD6, 0xADAA, 0xB6D5};
        uint16_t rh = rhythm[bw_rand(&r) % 4];
        int d = (int)(bw_rand(&r) % 5);
        static const int8_t walk[8] = {-2, -1, -1, 1, 1, 2, 3, -3};
        th->len = 16;
        for (i = 0; i < 16; i++)
            if (rh >> (15 - i) & 1)
            {
                d += walk[bw_rand(&r) % 8];
                d = d < -2 ? -2 : d > 9 ? 9 : d;
                bw_step_set(&th->s[i], d, BW_GATE_NOTE, i % 4 == 0 ? 110 : 85 + bw_rand(&r) % 20);
            }
        break;
    }
    case BW_BASS:
    {
        /* the bass: root on the one, fifths and octaves, syncopated */
        static const uint16_t rhythm[4] = {0x9292, 0x9A52, 0x8925, 0x9249};
        static const int8_t degs[8] = {0, 0, 4, 7, -3, 2, 4, 5};
        uint16_t rh = rhythm[bw_rand(&r) % 4];
        th->len = 16;
        for (i = 0; i < 16; i++)
            if (rh >> (15 - i) & 1)
                bw_step_set(&th->s[i], i ? degs[bw_rand(&r) % 8] : 0, BW_GATE_NOTE, i % 4 == 0 ? 120 : 95);
        break;
    }
    case BW_TROUT:
    {
        /* the trout: a triplet arpeggio over the chord, legato */
        static const int8_t shapes[4][12] = {{0, 2, 4, 7, 4, 2, 0, 2, 4, 7, 9, 7},
                                             {0, 4, 7, 9, 7, 4, 2, 4, 7, 11, 7, 4},
                                             {7, 4, 2, 0, 2, 4, 7, 4, 2, 0, -1, 0},
                                             {0, 2, 4, 2, 4, 7, 4, 7, 9, 7, 4, 2}};
        unsigned sh = bw_rand(&r) % 4;
        th->len = 12;
        for (i = 0; i < 12; i++)
            bw_step_set(&th->s[i], shapes[sh][i], BW_GATE_NOTE, i % 3 == 0 ? 100 : 80);
        break;
    }
    default:
    {
        /* the catfish: long held notes, two bars of eighths */
        static const int8_t second[4] = {0, -3, 4, -2};
        th->len = 16;
        if (bw_rand(&r) & 1)
        {
            bw_step_set(&th->s[0], 0, BW_GATE_NOTE, 110);
            bw_step_set(&th->s[8], second[bw_rand(&r) % 4], BW_GATE_NOTE, 100);
        }
        else
        {
            bw_step_set(&th->s[0], 0, BW_GATE_NOTE, 110);
            bw_step_set(&th->s[4], -3, BW_GATE_NOTE, 90);
            bw_step_set(&th->s[8], 0, BW_GATE_NOTE, 100);
            bw_step_set(&th->s[12], second[bw_rand(&r) % 4], BW_GATE_NOTE, 90);
        }
        for (i = 1; i < 16; i++)
            if (th->s[i].gate == BW_GATE_REST)
                th->s[i].gate = BW_GATE_TIE;
        break;
    }
    }
}
/* development moves: each fish has its favourites, the bait picks the kind */
enum { MV_ORNAMENT, MV_TRANSPOSE, MV_INVERT, MV_RETRO, MV_DISPLACE, MV_ADD, MV_REST, MV_EXTEND, MV_LEAP, MV_ACCENT,
       NMV };
static const uint8_t bw_mv_species[4][NMV] = {
    {5, 2, 1, 1, 3, 4, 1, 1, 4, 2}, /* bluegill: ornaments, added notes, leaps */
    {2, 3, 1, 1, 5, 3, 3, 1, 1, 3}, /* bass: syncopation, rests, transposition */
    {2, 4, 4, 4, 1, 1, 1, 3, 2, 1}, /* trout: inversion, retrograde, extension */
    {1, 4, 1, 1, 1, 1, 3, 3, 3, 1}, /* catfish: slow shifts, space, depth */
};
static const uint8_t bw_mv_bait[BW_NBAIT][NMV] = {
    {4, 4, 1, 1, 1, 2, 1, 1, 1, 1}, /* worm: melodic */
    {1, 1, 1, 1, 4, 4, 3, 1, 1, 4}, /* crankbait: rhythmic */
    {1, 3, 4, 2, 1, 1, 1, 1, 4, 1}, /* spinner: pitch */
    {1, 1, 1, 4, 1, 1, 3, 4, 1, 1}, /* jig: structure */
};
static int bw_pick_step(bw_engine *e, const bw_theme *th, int note)
{
    unsigned start = bw_random(e) % th->len;
    for (unsigned i = 0; i < th->len; i++)
    {
        unsigned j = (start + i) % th->len;
        if ((th->s[j].gate == BW_GATE_NOTE) == (note != 0))
            return (int)j;
    }
    return -1;
}
static int bw_clamp_deg(unsigned k, int d)
{
    return d < bw_deg_lo[k % 4] ? bw_deg_lo[k % 4] : d > bw_deg_hi[k % 4] ? bw_deg_hi[k % 4] : d;
}
static void bw_move(bw_engine *e, unsigned k, unsigned mv)
{
    bw_theme *th = &e->track[k].cur;
    unsigned len = th->len, seg = (bw_random(e) % (len / 4)) * 4, i;
    int j = bw_pick_step(e, th, 1);
    bw_step tmp[4];
    switch (mv)
    {
    case MV_ORNAMENT:
        if (j >= 0)
            th->s[j].deg = (int8_t)bw_clamp_deg(k, th->s[j].deg + ((bw_random(e) & 1) ? 1 : -1));
        break;
    case MV_TRANSPOSE:
    {
        static const int8_t by[4] = {-4, -2, 2, 4};
        int d = by[bw_random(e) % 4];
        for (i = seg; i < seg + 4; i++)
            if (th->s[i].gate == BW_GATE_NOTE)
                th->s[i].deg = (int8_t)bw_clamp_deg(k, th->s[i].deg + d);
        break;
    }
    case MV_INVERT:
    {
        int pivot = 0, have = 0;
        for (i = seg; i < seg + 4; i++)
            if (th->s[i].gate == BW_GATE_NOTE)
            {
                if (!have)
                    pivot = th->s[i].deg, have = 1;
                th->s[i].deg = (int8_t)bw_clamp_deg(k, 2 * pivot - th->s[i].deg);
            }
        break;
    }
    case MV_RETRO:
        for (i = 0; i < 4; i++)
            tmp[i] = th->s[seg + 3 - i];
        for (i = 0; i < 4; i++)
            th->s[seg + i] = tmp[i];
        if (th->s[seg].gate == BW_GATE_TIE)
            th->s[seg].gate = BW_GATE_NOTE;
        break;
    case MV_DISPLACE:
        tmp[0] = th->s[seg + 3];
        for (i = 3; i > 0; i--)
            th->s[seg + i] = th->s[seg + i - 1];
        th->s[seg] = tmp[0];
        if (th->s[seg].gate == BW_GATE_TIE)
            th->s[seg].gate = BW_GATE_NOTE;
        break;
    case MV_ADD:
    {
        int r = bw_pick_step(e, th, 0);
        if (r >= 0)
        {
            int near = j >= 0 ? th->s[j].deg : 0;
            bw_step_set(&th->s[r], bw_clamp_deg(k, near + (int)(bw_random(e) % 5) - 2), BW_GATE_NOTE,
                        80 + bw_random(e) % 30);
        }
        break;
    }
    case MV_REST:
        /* never the downbeat of the bass or the catfish: they anchor the time */
        if (j > 0 || (j == 0 && (k % 4) != BW_BASS && (k % 4) != BW_CATFISH))
            th->s[j].gate = (k % 4) == BW_CATFISH ? BW_GATE_TIE : BW_GATE_REST;
        break;
    case MV_EXTEND:
        if (len * 2 <= BW_STEPS)
        {
            for (i = 0; i < len; i++)
                th->s[len + i] = th->s[i];
            th->len = (uint8_t)(len * 2);
            /* the answer differs from the call: one ornament in the new half */
            int a = bw_pick_step(e, th, 1);
            if (a >= 0 && (unsigned)a < len)
                a += (int)len;
            if (a >= (int)len && th->s[a].gate == BW_GATE_NOTE)
                th->s[a].deg = (int8_t)bw_clamp_deg(k, th->s[a].deg + 2);
        }
        else
            bw_move(e, k, MV_TRANSPOSE);
        break;
    case MV_LEAP:
        if (j >= 0)
        {
            int up = th->s[j].deg + 7, dn = th->s[j].deg - 7;
            th->s[j].deg = (int8_t)bw_clamp_deg(k, ((bw_random(e) & 1) && up <= bw_deg_hi[k % 4]) ? up : dn);
        }
        break;
    default:
        if (j >= 0)
            th->s[j].vel = (uint8_t)(th->s[j].vel > 100 ? 75 : 120);
        break;
    }
}
static unsigned bw_choose_move(bw_engine *e, unsigned k)
{
    unsigned w[NMV], sum = 0, i, bait = e->track[k].bait % BW_NBAIT;
    for (i = 0; i < NMV; i++)
        sum += w[i] = (unsigned)bw_mv_species[k % 4][i] * bw_mv_bait[bait][i];
    unsigned x = bw_random(e) % sum;
    for (i = 0; i < NMV; i++)
    {
        if (x < w[i])
            return i;
        x -= w[i];
    }
    return MV_ORNAMENT;
}

/* ------------------------------------------------------------------ setup */
void bw_track_defaults(bw_engine *e, unsigned k)
{
    static const uint8_t pan[4] = {38, 50, 64, 50}, rev[4] = {25, 8, 35, 30}, dly[4] = {20, 0, 15, 5},
                         cho[4] = {0, 0, 30, 10}, drive[4] = {0, 12, 0, 8}, vol[4] = {72, 80, 68, 82};
    static const uint16_t wmin[4] = {6, 9, 8, 14}, wmax[4] = {20, 28, 24, 40};
    bw_track *t = &e->track[k];
    unsigned char *p = (unsigned char *)t;
    for (unsigned i = 0; i < sizeof *t; i++)
        p[i] = 0;
    bw_patch_factory(&t->patch, k, 0);
    t->volume = vol[k];
    t->pan = pan[k];
    t->density = 100;
    t->send_rev = rev[k];
    t->send_dly = dly[k];
    t->send_cho = cho[k];
    t->drive = drive[k];
    if (k == BW_CATFISH)
    {
        t->lfo_rate = 15;
        t->lfo_depth = 25;
        t->lfo_target = BW_LFO_FM;
    }
    else if (k == BW_TROUT)
    {
        t->lfo_rate = 55;
        t->lfo_depth = 8;
        t->lfo_target = BW_LFO_PITCH;
    }
    t->wait_min = wmin[k];
    t->wait_max = wmax[k];
    t->manual = 1; /* TIMED: the bobber dips, you set the hook and fight it */
    t->bait = bw_fish_data[k].bait;
    t->depth = bw_fish_data[k].depth;
    bw_theme_generate(e, k, 0x5EED0000u + k * 7919u);
    t->home = t->cur;
    t->prev = t->cur;
}
void bw_defaults(bw_engine *e)
{
    bw_song *s = &e->song;
    s->tempo = 96;
    s->key = 9; /* A */
    s->scale = 0;
    s->progression = 1;
    s->swing = 12;
    s->pb_event = 1;
    s->weather = BW_WEATHER_AUTO;
    s->sync = 0;
    s->rev_size = 60;
    s->rev_tone = 45;
    s->rev_level = 70;
    s->dly_time = 60;
    s->dly_feedback = 40;
    s->dly_level = 60;
    s->cho_rate = 30;
    s->cho_depth = 50;
    s->tone = 85;
    s->width = 70;
    s->arp_mode = BW_ARP_UP;
    s->arp_rate = 3;
    s->arp_octaves = 1;
    s->arp_gate = 60;
    for (unsigned k = 0; k < 4; k++)
        bw_track_defaults(e, k);
}
void bw_init(bw_engine *e)
{
    unsigned i, k;
    unsigned char *p = (unsigned char *)e;
    for (i = 0; i < sizeof *e; i++)
        p[i] = 0;
    for (i = 0; i <= 1024; i++)
        bw_sine[i] = fm_sinf((float)i * (FM_TWO_PI / 1024.0f));
    bw_note_hz[0] = 8.175798916f;
    for (i = 1; i < 128; i++)
        bw_note_hz[i] = bw_note_hz[i - 1] * 1.059463094f;
    e->rng = 0x42574d31u;
    bw_defaults(e);
    for (k = 0; k < 4; k++)
    {
        e->mod_scale[k] = e->pitch_scale[k] = e->amp_scale[k] = e->metal[k] = 1.0f;
        e->live_note[k] = -1;
        e->last_voice[k] = 0xFF;
    }
    for (i = 0; i < 16; i++)
        e->arp_held[i] = -1;
    e->midi_out = 1;
    bw_gains_set = 0;
    bw_fx_reset();
}

/* ------------------------------------------------------------------ voices */
static void bw_midi_note(bw_engine *e, unsigned k, unsigned note, unsigned vel, int on)
{
    if (!e->midi_out)
        return;
    if (on && bw_midi_room() < 4)
        return;
    unsigned st = (on ? 0x90u : 0x80u) | (k & 15u);
    bw_midi_send((st >> 4) | st << 8 | (note & 127u) << 16 | (on ? (vel & 127u) : 0u) << 24);
}
static bw_voice *bw_alloc(bw_engine *e, unsigned k, int live)
{
    bw_voice *best = 0;
    uint32_t oldest = 0;
    unsigned i;
    if (!live && bw_mono[k % 4] && e->last_voice[k] < BW_VOICES)
    {
        bw_voice *v = &e->voice[e->last_voice[k]];
        if (v->track == k && !v->live && v->active)
            return v;
    }
    for (i = 0; i < BW_VOICES; i++)
        if (!e->voice[i].active)
            return &e->voice[i];
    /* steal: the oldest, preferring this track's own, never a held live note while another will do */
    for (int pass = 0; pass < 3 && !best; pass++)
        for (i = 0; i < BW_VOICES; i++)
        {
            bw_voice *v = &e->voice[i];
            if (pass == 0 && (v->track != k || v->held))
                continue;
            if (pass == 1 && v->held)
                continue;
            if (!best || e->age - v->age > oldest)
            {
                best = v;
                oldest = e->age - v->age;
            }
        }
    return best;
}
static void bw_voice_on(bw_engine *e, bw_voice *v, unsigned k, unsigned note, unsigned velocity, int legato)
{
    const bw_patch *p = &e->track[k].patch;
    float hz = bw_note_hz[note > 127 ? 127 : note];
    unsigned i;
    v->velocity = (float)(velocity > 127 ? 127 : velocity) / 127.0f;
    v->track = (uint8_t)k;
    v->note = (uint8_t)note;
    v->age = ++e->age;
    for (i = 0; i < 6; i++)
    {
        const bw_operator *o = &p->op[i];
        v->increment[i] = fm_minf(o->fixed ? (float)o->hz : hz * ((float)o->ratio / 100.0f), BW_RATE * 0.45f) / BW_RATE;
        v->attack_step[i] = 1.0f / (1.0f + (float)o->attack * (BW_RATE / 1000.0f));
        v->decay_step[i] = (1.0f - (float)o->sustain / 100.0f) / (1.0f + (float)o->decay * (BW_RATE / 1000.0f));
        v->sustain[i] = (float)o->sustain * 0.01f;
        /* an operator at level 0 is silent and modulates nothing: it never starts (it costs nothing) */
        v->stage[i] = o->level ? 1 : 0;
        if (!legato || !v->active || !o->level)
            v->phase[i] = v->envelope[i] = v->previous[i] = 0.0f;
    }
    v->active = 1;
    bw_voice_prepare(e, v);
}
static void bw_voice_off(bw_engine *e, bw_voice *v)
{
    const bw_patch *p = &e->track[v->track].patch;
    for (unsigned i = 0; i < 6; i++)
        if (v->stage[i] && v->stage[i] != 4)
        {
            v->stage[i] = 4;
            v->release_step[i] = v->envelope[i] / (1.0f + p->op[i].release * (BW_RATE / 1000.0f));
        }
    v->held = 0;
}
static int bw_track_audible(const bw_engine *e, unsigned k)
{
    int solo = 0;
    for (unsigned i = 0; i < 4; i++)
        solo |= e->track[i].solo;
    return !e->track[k].mute && (!solo || e->track[k].solo) && !(e->macros[k] >> BW_M_CUT & 1);
}
/* the sequencer's note for track k (its previous one released, a mono fish's voice reused) */
static void bw_seq_note(bw_engine *e, unsigned k, unsigned note, unsigned vel)
{
    if (e->last_voice[k] < BW_VOICES)
    {
        bw_voice *old = &e->voice[e->last_voice[k]];
        if (old->track == k && !old->live && old->active && !bw_mono[k % 4])
            bw_voice_off(e, old);
    }
    if (e->sounding[k])
        bw_midi_note(e, k, e->midi_notes[k], 0, 0);
    bw_voice *v = bw_alloc(e, k, 0);
    if (!v)
        return;
    int legato = bw_mono[k % 4] && v->active && v->track == k && !v->live;
    v->live = 0;
    v->held = 0;
    bw_voice_on(e, v, k, note, vel, legato);
    e->last_voice[k] = (uint8_t)(v - e->voice);
    e->last_note[k] = (uint8_t)note;
    e->sounding[k] = 1;
    e->midi_notes[k] = (uint8_t)note;
    bw_midi_note(e, k, note, vel, 1);
}
static void bw_seq_off(bw_engine *e, unsigned k)
{
    if (e->last_voice[k] < BW_VOICES)
    {
        bw_voice *v = &e->voice[e->last_voice[k]];
        if (v->track == k && !v->live && v->active)
            bw_voice_off(e, v);
    }
    if (e->sounding[k])
        bw_midi_note(e, k, e->midi_notes[k], 0, 0);
    e->sounding[k] = 0;
}
void bw_all_notes_off(bw_engine *e)
{
    for (unsigned k = 0; k < 4; k++)
    {
        bw_seq_off(e, k);
        e->off_at[k] = 0;
    }
    for (unsigned i = 0; i < BW_VOICES; i++)
        if (e->voice[i].active)
            bw_voice_off(e, &e->voice[i]);
}
/* a live note on track k (keys, MIDI in): its own voice, held until released */
void bw_live_note(bw_engine *e, unsigned k, unsigned note, int down, unsigned velocity)
{
    unsigned i;
    if (k >= 4)
        return;
    if (down)
    {
        bw_voice *v = bw_alloc(e, k, 1);
        if (!v)
            return;
        v->live = 1;
        v->held = 1;
        bw_voice_on(e, v, k, note, velocity, 0);
        bw_midi_note(e, k, note, velocity, 1);
        return;
    }
    for (i = 0; i < BW_VOICES; i++)
    {
        bw_voice *v = &e->voice[i];
        if (v->active && v->live && v->held && v->track == k && v->note == note)
        {
            bw_voice_off(e, v);
            bw_midi_note(e, k, note, 0, 0);
        }
    }
}
static unsigned bw_step_ticks(const bw_engine *e, unsigned k)
{
    unsigned t = BW_PPQN / bw_spb[k % 4];
    return e->macros[k] >> BW_M_HALF & 1 ? t * 2 : t;
}
static int bw_white_deg(const bw_engine *e, unsigned white)
{
    return (int)white - 7 + e->live_oct * 7;
}
/* record: a live note while armed and playing writes the step it is nearest */
static void bw_record(bw_engine *e, unsigned k, int deg, unsigned vel)
{
    bw_theme *th = &e->track[k].cur;
    unsigned st = bw_step_ticks(e, k), i = e->step[k];
    if (!e->playing || !e->rec_arm[k])
        return;
    /* the step that just played, or the next one past its middle */
    if (e->tcount[k] < st / 2)
        i = (i + 1) % th->len;
    bw_step_set(&th->s[i], bw_clamp_deg(k, deg), BW_GATE_NOTE, vel);
}
void bw_live_key(bw_engine *e, unsigned white, int down, unsigned velocity)
{
    unsigned k = e->live_track % 4, i;
    int deg = bw_white_deg(e, white);
    if (white >= 16)
        return;
    if (e->arp)
    {
        if (down)
        {
            /* LATCH: a new chord after every key was let go replaces the old one */
            if (e->arp == BW_ARP_LATCH && !e->arp_keys)
                for (i = 0; i < 16; i++)
                    e->arp_held[i] = -1;
            e->arp_held[white] = (int8_t)(e->arp_order++ & 127u);
            e->arp_keys |= 1u << white;
            bw_record(e, k, deg, velocity);
        }
        else
        {
            e->arp_keys &= ~(1u << white);
            if (e->arp != BW_ARP_LATCH)
                e->arp_held[white] = -1;
        }
        return;
    }
    bw_live_note(e, k, (unsigned)bw_note_of(e, k, deg, 0), down, velocity);
    if (down)
        bw_record(e, k, deg, velocity);
}
void bw_macro(bw_engine *e, unsigned k, unsigned macro, int down)
{
    if (k >= 4 || macro >= BW_NMACRO)
        return;
    if (down)
        e->macros[k] |= 1u << macro;
    else
        e->macros[k] &= ~(1u << macro);
}

/* ------------------------------------------------------------------ fishing */
unsigned bw_weather(const bw_engine *e)
{
    static const uint8_t cycle[4] = {BW_WEATHER_DAY, BW_WEATHER_DUSK, BW_WEATHER_NIGHT, BW_WEATHER_STORM};
    if (e->song.weather != BW_WEATHER_AUTO)
        return e->song.weather;
    return cycle[(e->clock / (180u * BW_RATE)) % 4]; /* three minutes of play each */
}
static float bw_wait_factor(const bw_engine *e, unsigned k)
{
    const bw_track *t = &e->track[k];
    int dd = (int)t->depth - (int)bw_fish_data[k].depth;
    float f = dd == 0 ? 0.75f : (dd == 1 || dd == -1) ? 1.2f : 1.5f;
    unsigned w = bw_weather(e);
    if (t->bait == bw_fish_data[k].bait)
        f *= 0.8f;
    if (w == BW_WEATHER_DUSK)
        f *= 0.8f;
    else if (w == BW_WEATHER_NIGHT)
        f *= (k == BW_CATFISH || k == BW_TROUT) ? 0.7f : 1.3f;
    else if (w == BW_WEATHER_STORM)
        f *= k == BW_BASS ? 0.7f : 1.1f;
    else if (w == BW_WEATHER_DAY)
        f *= k == BW_BLUEGILL ? 0.8f : 1.2f;
    return f;
}
static void bw_schedule(bw_engine *e, unsigned k)
{
    bw_track *t = &e->track[k];
    unsigned seconds = t->wait_min + bw_random(e) % (1u + t->wait_max - t->wait_min);
    float s = (float)seconds * bw_wait_factor(e, k);
    if (s > 900.0f)
        s = 900.0f;
    e->deadline[k] = e->clock + (uint32_t)(s * (float)BW_RATE);
}
/* which fish takes the bait, and how big it is */
static void bw_encounter(bw_engine *e, unsigned k)
{
    const bw_track *t = &e->track[k];
    unsigned rare = k + 4, w = bw_weather(e);
    float chance = 0.06f;
    if (t->depth == bw_fish_data[rare].depth)
        chance *= 2.0f;
    if (t->bait == bw_fish_data[rare].bait)
        chance *= 1.5f;
    if (w == BW_WEATHER_STORM || (w == BW_WEATHER_NIGHT && k == BW_CATFISH))
        chance *= 1.5f;
    unsigned sp = bw_unit(e) < chance ? rare : k;
    float u = bw_unit(e), p = t->depth == bw_fish_data[sp].depth ? 1.5f : 2.2f;
    if (w == BW_WEATHER_STORM && k == BW_BASS)
        p *= 0.8f;
    float lo = bw_fish_data[sp].min_mm, hi = bw_fish_data[sp].max_mm;
    float mm = lo + (hi - lo) * bw_pow(u, p);
    if (bw_unit(e) < 0.03f) /* a trophy: past the usual range */
        mm = hi * (1.0f + 0.15f * bw_unit(e));
    e->fish[k] = (uint8_t)sp;
    e->fish_rare[k] = sp >= 4;
    e->fish_mm[k] = (uint32_t)(mm + 0.5f);
}
void bw_cast(bw_engine *e, unsigned k)
{
    if (k >= 4)
        return;
    e->fish[k] = (uint8_t)k;
    e->fish_rare[k] = 0;
    e->fish_mm[k] = 0;
    e->fight_dist[k] = e->fight_tension[k] = e->fight_run[k] = e->fight_max[k] = 0;
    e->fight_slack_ms[k] = 0;
    e->hooked_state[k] = BW_FISH_WAIT;
    bw_schedule(e, k);
}
void bw_retrieve(bw_engine *e, unsigned k)
{
    if (k < 4)
    {
        e->hooked_state[k] = BW_FISH_IDLE;
        bw_seq_off(e, k);
        e->off_at[k] = 0;
    }
}
int bw_fish_state(const bw_engine *e, unsigned k)
{
    return k < 4 ? e->hooked_state[k] : BW_FISH_IDLE;
}
static void bw_event_set(bw_engine *e, unsigned k, unsigned result)
{
    e->event.seq++;
    e->event.track = (uint8_t)k;
    e->event.species = e->fish[k];
    e->event.rare = e->fish_rare[k];
    e->event.mm = e->fish_mm[k];
    e->event.g = bw_weight_g(e->fish[k], e->fish_mm[k]);
    e->event.result = (uint8_t)result;
    e->event.best = 0;
    e->event.moves = 0;
    e->event.key_change = 0;
}
static void bw_catch(bw_engine *e, unsigned k)
{
    bw_track *t = &e->track[k];
    unsigned sp = e->fish[k];
    bw_species_log *l = &e->log.s[sp];
    uint32_t mm = e->fish_mm[k], g = bw_weight_g(sp, mm);
    bw_event_set(e, k, BW_FIGHT_LANDED);
    int best = mm > l->best_mm;
    if (l->count < 0xffffffffu)
        l->count++;
    l->last_mm = mm;
    l->last_g = g;
    if (best)
    {
        l->best_mm = mm;
        l->best_g = g;
    }
    /* the music: size decides how far the theme moves; a rare fish starts a new section */
    t->prev = t->cur;
    t->has_prev = 1;
    float lo = bw_fish_data[sp].min_mm, hi = bw_fish_data[sp].max_mm, frac = ((float)mm - lo) / (hi - lo);
    unsigned moves = frac < 0.33f ? 1 : frac < 0.66f ? 2 : frac <= 1.0f ? 3 : 4;
    if (e->fish_rare[k])
    {
        bw_theme_generate(e, k, bw_random(e));
        bw_move(e, k, MV_EXTEND);
        moves = 2;
    }
    for (unsigned i = 0; i < moves; i++)
        bw_move(e, k, bw_choose_move(e, k));
    e->event.moves = (uint8_t)moves;
    /* a personal best (not the first fish of its kind): the river changes key, up a fourth */
    if (best && l->count > 1 && e->song.pb_event)
    {
        e->song.key = (uint8_t)((e->song.key + 5) % 12);
        e->event.key_change = 1;
    }
    e->event.best = (uint8_t)(best && l->count > 1);
    e->hooked_state[k] = BW_FISH_WAIT;
    bw_schedule(e, k);
}
int bw_hookset(bw_engine *e, unsigned k)
{
    if (k >= 4 || e->hooked_state[k] != BW_FISH_BITE || !e->playing)
        return 0;
    if ((int32_t)(e->clock - e->bite_until[k]) >= 0)
        return 0;
    float lo = bw_fish_data[e->fish[k]].min_mm, hi = bw_fish_data[e->fish[k]].max_mm;
    float frac = fm_clampf(((float)e->fish_mm[k] - lo) / (hi - lo), 0.0f, 1.2f);
    e->hooked_state[k] = BW_FISH_FIGHT;
    e->fight_dist[k] = 25.0f + 40.0f * frac + (e->fish_rare[k] ? 15.0f : 0.0f);
    e->fight_max[k] = e->fight_dist[k] * 1.6f + 10.0f;
    e->fight_tension[k] = 30.0f;
    e->fight_run[k] = 0;
    e->fight_slack_ms[k] = 0;
    if (e->fish_rare[k])
        bw_on_rare(k);
    return 1;
}
/* one step of the fight: reel = clicks wound in (negative: line given), ms since the last step */
int bw_fight(bw_engine *e, unsigned k, int reel, unsigned ms)
{
    if (k >= 4 || e->hooked_state[k] != BW_FISH_FIGHT)
        return BW_FIGHT_ON;
    float dt = (float)(ms > 500 ? 500 : ms) / 1000.0f;
    float lo = bw_fish_data[e->fish[k]].min_mm, hi = bw_fish_data[e->fish[k]].max_mm;
    float frac = fm_clampf(((float)e->fish_mm[k] - lo) / (hi - lo), 0.0f, 1.2f);
    float strength = 0.6f + 1.4f * frac + (e->fish_rare[k] ? 0.5f : 0.0f);
    float *d = &e->fight_dist[k], *tn = &e->fight_tension[k], *run = &e->fight_run[k];
    /* the fish's runs: they start at random, last about a second */
    if (*run > 0.0f)
        *run -= dt;
    else if (bw_unit(e) < 0.8f * strength * dt)
        *run = 0.5f + bw_unit(e);
    float pull = strength * (0.4f + (*run > 0.0f ? 1.6f : 0.0f));
    if (reel > 0) /* each click of the reel pulls harder on a stronger fish */
    {
        *tn += (2.0f + strength) * (float)reel;
        *d -= (float)reel;
    }
    else if (reel < 0)
    {
        *tn += 2.0f * (float)reel;
        *d -= 0.5f * (float)reel;
    }
    *tn += (pull * 15.0f - 40.0f) * dt;
    if (reel <= 0)
        *d += pull * 1.2f * dt;
    if (*tn > 85.0f) /* the drag slips: line pays out, the strain eases */
    {
        *d += (1.0f + pull * 3.0f) * dt;
        *tn -= 60.0f * dt;
    }
    *tn = fm_maxf(*tn, 0.0f);
    e->fight_slack_ms[k] = *tn < 5.0f ? e->fight_slack_ms[k] + ms : 0;
    int result = BW_FIGHT_ON;
    if (*tn >= 100.0f)
        result = BW_FIGHT_SNAPPED;
    else if (*d >= e->fight_max[k] || e->fight_slack_ms[k] > 2000)
        result = BW_FIGHT_THROWN;
    else if (*d <= 0.0f)
        result = BW_FIGHT_LANDED;
    if (result == BW_FIGHT_LANDED)
        bw_catch(e, k);
    else if (result != BW_FIGHT_ON)
    {
        bw_event_set(e, k, (unsigned)result);
        if (result == BW_FIGHT_SNAPPED)
            e->log.snapped++;
        else
            e->log.thrown++;
        e->hooked_state[k] = BW_FISH_WAIT;
        bw_schedule(e, k);
    }
    return result;
}
void bw_release(bw_engine *e, unsigned k)
{
    if (k < 4 && e->track[k].has_prev)
    {
        e->track[k].cur = e->track[k].prev;
        e->track[k].has_prev = 0;
        e->log.released++;
    }
}
void bw_restore_home(bw_engine *e, unsigned k)
{
    if (k < 4)
    {
        e->track[k].prev = e->track[k].cur;
        e->track[k].has_prev = 1;
        e->track[k].cur = e->track[k].home;
    }
}
void bw_make_home(bw_engine *e, unsigned k)
{
    if (k < 4)
        e->track[k].home = e->track[k].cur;
}
/* a sixteenth's fishing: bites, misses and automatic catches */
static void bw_fishing(bw_engine *e)
{
    for (unsigned k = 0; k < 4; k++)
    {
        int st = e->hooked_state[k];
        if (st == BW_FISH_BITE && (int32_t)(e->clock - e->bite_until[k]) >= 0)
        {
            e->hooked_state[k] = BW_FISH_WAIT;
            e->log.missed++;
            bw_schedule(e, k);
        }
        else if (st == BW_FISH_WAIT && (int32_t)(e->clock - e->deadline[k]) >= 0)
        {
            bw_encounter(e, k);
            if (e->track[k].manual)
            {
                e->hooked_state[k] = BW_FISH_BITE;
                e->bite_until[k] = e->clock + 3 * BW_RATE; /* three seconds to answer */
            }
            else
            {
                if (e->fish_rare[k])
                    bw_on_rare(k);
                bw_catch(e, k);
            }
        }
    }
}

/* ------------------------------------------------------------------ the clock */
static void bw_play_step(bw_engine *e, unsigned k)
{
    bw_track *t = &e->track[k];
    bw_theme *th = &t->cur;
    unsigned i = e->step[k] % th->len;
    const bw_step *s = &th->s[i];
    if (e->rec_left[k])
    {
        if (!--e->rec_left[k])
            e->rec_arm[k] = 0;
    }
    if (s->gate != BW_GATE_NOTE)
        return;
    if (e->hooked_state[k] == BW_FISH_IDLE) /* a fish is heard while its line is in the water */
        return;
    if (t->density < 100 && i && bw_random(e) % 100 >= t->density)
        return;
    if (!bw_track_audible(e, k))
        return;
    unsigned ties = 0;
    while (ties + 1 < th->len && th->s[(i + 1 + ties) % th->len].gate == BW_GATE_TIE)
        ties++;
    uint32_t spt = 60u * BW_RATE / ((uint32_t)e->song.tempo * BW_PPQN);
    uint32_t len = (ties + 1) * bw_step_ticks(e, k) * spt * bw_gate_pct[k % 4] / 100u;
    bw_seq_note(e, k, (unsigned)bw_note_of(e, k, s->deg, 1), s->vel);
    e->off_at[k] = e->clock + (len ? len : 1);
}
static unsigned bw_swing_ticks(const bw_engine *e, unsigned k)
{
    unsigned spb = bw_spb[k % 4];
    if (spb == 3)
        return 0; /* triplets do not swing */
    return bw_step_ticks(e, k) * e->song.swing / 200u;
}
static void bw_arp_tick(bw_engine *e)
{
    unsigned k = e->live_track % 4, i, n = 0;
    int list[16], order[16];
    if (!e->arp)
        return;
    unsigned period = bw_arp_ticks[e->song.arp_rate % 5];
    if (e->ticks % period)
        return;
    for (i = 0; i < 16; i++)
        if (e->arp_held[i] >= 0)
        {
            list[n] = (int)i;
            order[n++] = e->arp_held[i];
        }
    if (e->live_note[k] >= 0)
    {
        bw_live_note(e, k, (unsigned)e->live_note[k], 0, 0);
        e->live_note[k] = -1;
    }
    if (!n)
        return;
    if (e->song.arp_mode == BW_ARP_PLAYED)
        for (i = 1; i < n; i++) /* insertion sort by the order they were pressed */
            for (unsigned j = i; j > 0 && order[j] < order[j - 1]; j--)
            {
                int a = order[j], b = list[j];
                order[j] = order[j - 1], list[j] = list[j - 1];
                order[j - 1] = a, list[j - 1] = b;
            }
    unsigned total = n * e->song.arp_octaves, idx;
    switch (e->song.arp_mode)
    {
    case BW_ARP_DOWN:
        idx = total - 1 - e->arp_pos % total;
        break;
    case BW_ARP_UPDOWN:
    {
        unsigned cyc = total > 1 ? 2 * total - 2 : 1, p = e->arp_pos % cyc;
        idx = p < total ? p : cyc - p;
        break;
    }
    case BW_ARP_RANDOM:
        idx = bw_random(e) % total;
        break;
    default:
        idx = e->arp_pos % total;
        break;
    }
    e->arp_pos++;
    int deg = bw_white_deg(e, (unsigned)list[idx % n]) + (int)(idx / n) * 7;
    unsigned note = (unsigned)bw_note_of(e, k, deg, 0);
    bw_live_note(e, k, note, 1, 100);
    e->live_note[k] = (int8_t)note;
    e->arp_gate_left = period * e->song.arp_gate / 100u; /* ticks until the arp note's release */
}
static void bw_tick(bw_engine *e)
{
    unsigned k;
    e->ticks++;
    bw_arp_tick(e);
    if (e->arp_gate_left && !--e->arp_gate_left)
    {
        unsigned lk = e->live_track % 4;
        if (e->live_note[lk] >= 0)
        {
            bw_live_note(e, lk, (unsigned)e->live_note[lk], 0, 0);
            e->live_note[lk] = -1;
        }
    }
    if (!e->playing)
        return;
    if (e->ticks % 4 == 0 && e->song.sync == 0 && e->midi_out && bw_midi_room())
        bw_midi_send(0x0F | 0xF8u << 8);
    for (k = 0; k < 4; k++)
    {
        /* STUTTER: the note again every thirty-second */
        if ((e->macros[k] >> BW_M_STUTTER & 1) && e->ticks % 12 == 0 && e->sounding[k])
            bw_seq_note(e, k, e->last_note[k], 100);
        if (e->tcount[k])
        {
            e->tcount[k]--;
            continue;
        }
        bw_play_step(e, k);
        unsigned st = bw_step_ticks(e, k), sw = bw_swing_ticks(e, k);
        unsigned dur = (e->step[k] & 1) ? st - sw : st + sw;
        e->tcount[k] = dur ? dur - 1 : 0;
        e->step[k] = (uint8_t)((e->step[k] + 1) % e->track[k].cur.len);
    }
    if (e->ticks % 24 == 0)
        bw_fishing(e);
    if (e->ticks % (BW_PPQN * 4) == 0)
    {
        e->bar++;
        e->chord = (uint8_t)(e->bar % bw_progressions[e->song.progression % BW_PROGRESSIONS].n);
    }
}
void bw_transport(bw_engine *e, int playing)
{
    unsigned k;
    if (playing && !e->playing)
    {
        e->ticks = 0;
        e->bar = 0;
        e->chord = 0;
        e->tick_acc = 60u * BW_RATE; /* the first tick at once */
        for (k = 0; k < 4; k++)
        {
            e->tcount[k] = 0;
            e->step[k] = 0;
        }
        if (e->midi_out && e->song.sync == 0 && bw_midi_room())
            bw_midi_send(0x0F | 0xFAu << 8);
    }
    else if (!playing && e->playing)
    {
        for (k = 0; k < 4; k++)
            bw_seq_off(e, k);
        if (e->midi_out && e->song.sync == 0 && bw_midi_room())
            bw_midi_send(0x0F | 0xFCu << 8);
    }
    e->playing = playing != 0;
}
void bw_midi_clock(bw_engine *e, unsigned status)
{
    if (!e->song.sync)
        return;
    if (status == 0xF8)
    {
        if (e->playing)
            e->ext_ticks += 4;
        e->ext_seen = 1;
    }
    else if (status == 0xFA || status == 0xFB)
    {
        bw_transport(e, 1);
        e->tick_acc = 0;
        e->ext_ticks = 1; /* the first step at once */
    }
    else if (status == 0xFC)
        bw_transport(e, 0);
}

/* ------------------------------------------------------------------ modulation */
static void bw_modulation(bw_engine *e, unsigned frames)
{
    for (unsigned k = 0; k < 4; k++)
    {
        const bw_track *t = &e->track[k];
        uint32_t m = e->macros[k];
        float hz = 0.05f * bw_exp2((float)t->lfo_rate * 0.0864f), v; /* 0.05 .. 20 Hz */
        e->lfo_phase[k] += hz * (float)frames / (float)BW_RATE;
        if (e->lfo_phase[k] >= 1.0f)
        {
            e->lfo_phase[k] -= fm_floorf(e->lfo_phase[k]);
            e->lfo_hold[k] = bw_unit(e) * 2.0f - 1.0f;
        }
        float ph = e->lfo_phase[k];
        switch (t->lfo_shape)
        {
        case BW_LFO_TRI:
            v = ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
            break;
        case BW_LFO_SQUARE:
            v = ph < 0.5f ? 1.0f : -1.0f;
            break;
        case BW_LFO_RANDOM:
            v = e->lfo_hold[k];
            break;
        default:
            v = bw_sin(ph);
            break;
        }
        float d = (float)t->lfo_depth / 100.0f;
        e->lfo_value[k] = v * d;
        float ms = 1.0f, pitch = 0.0f, amp = 1.0f, pan = 0.0f;
        switch (t->lfo_target)
        {
        case BW_LFO_PITCH:
            pitch = v * d * 2.0f; /* semitones */
            break;
        case BW_LFO_FM:
            ms = fm_maxf(0.0f, 1.0f + v * d);
            break;
        case BW_LFO_AMP:
            amp = 1.0f - d * (0.5f + 0.5f * v);
            break;
        default:
            pan = v * d * 0.5f;
            break;
        }
        if (m >> BW_M_BRIGHT & 1)
            ms *= 2.5f;
        if (m >> BW_M_DARK & 1)
            ms *= 0.2f;
        if (e->hooked_state[k] == BW_FISH_FIGHT) /* the line's tension brightens the fish */
            ms *= 1.0f + e->fight_tension[k] * 0.01f;
        /* DIVE / RISE: glide an octave down / up while held, back when let go */
        float target = (m >> BW_M_DIVE & 1) ? -12.0f : (m >> BW_M_RISE & 1) ? 12.0f : 0.0f;
        float rate = (target != 0.0f ? 12.0f : 36.0f) * (float)frames / (float)BW_RATE;
        if (e->bend[k] < target)
            e->bend[k] = fm_minf(target, e->bend[k] + rate);
        else
            e->bend[k] = fm_maxf(target, e->bend[k] - rate);
        if (m >> BW_M_OCTAVE & 1)
            pitch += 12.0f;
        e->mod_scale[k] = ms;
        e->pitch_scale[k] = bw_exp2((pitch + e->bend[k]) * (1.0f / 12.0f));
        e->amp_scale[k] = amp;
        e->pan_mod[k] = pan;
        e->fb_add[k] = (m >> BW_M_GROWL & 1) ? 0.6f : 0.0f;
        e->metal[k] = (m >> BW_M_METAL & 1) ? 1.414f : 1.0f;
        e->send_boost[k] = (m >> BW_M_THROW & 1) ? 1.0f : 0.0f;
    }
}
/* the voice's per-operator constants: gains, carrier weights, feedback, routes and pitch, from its patch and its
 * track's modulation (every 32 frames and at each note: edits are heard within a millisecond) */
static void bw_voice_prepare(bw_engine *e, bw_voice *v)
{
    static const float inv[7] = {1.0f, 1.0f, 0.5f, 1.0f / 3.0f, 0.25f, 0.2f, 1.0f / 6.0f};
    unsigned k = v->track & 3u, i, j, carriers = 0;
    const bw_patch *p = &e->track[k].patch;
    float ms = e->mod_scale[k] * 0.012f, pm = e->pitch_scale[k], metal = e->metal[k], fba = e->fb_add[k];
    for (i = 0; i < 6; i++)
        carriers += p->carrier[i] != 0;
    for (i = 0; i < 6; i++)
    {
        const bw_operator *o = &p->op[i];
        v->gain[i] = (float)o->level * 0.01f * (1.0f - (float)o->velocity * 0.01f * (1.0f - v->velocity));
        v->cw[i] = (float)p->carrier[i] * 0.01f * inv[carriers];
        v->fb[i] = (float)o->feedback * 0.012f + fba;
        v->inc[i] = v->increment[i] * pm * (p->carrier[i] ? 1.0f : metal);
        v->nsrc[i] = 0;
        for (j = i + 1; j < 6; j++)
            if (p->route[j][i] && p->op[j].level)
            {
                v->src[i][v->nsrc[i]] = (uint8_t)j;
                v->amt[i][v->nsrc[i]++] = (float)p->route[j][i] * ms;
            }
    }
}
static float bw_voice_sample(bw_engine *e, bw_voice *v)
{
    float output[6], mix = 0.0f;
    unsigned i, j, active = 0;
    (void)e;
    for (i = 6; i-- > 0;)
    {
        if (!v->stage[i])
        {
            output[i] = 0.0f;
            continue;
        }
        switch (v->stage[i])
        {
        case 1:
            v->envelope[i] += v->attack_step[i];
            if (v->envelope[i] >= 1.0f)
            {
                v->envelope[i] = 1.0f;
                v->stage[i] = 2;
            }
            break;
        case 2:
            v->envelope[i] -= v->decay_step[i];
            if (v->envelope[i] <= v->sustain[i])
            {
                v->envelope[i] = v->sustain[i];
                v->stage[i] = 3;
            }
            break;
        case 4:
            v->envelope[i] -= v->release_step[i];
            if (v->envelope[i] <= 0.000001f)
            {
                v->envelope[i] = 0;
                v->stage[i] = 0;
            }
            break;
        default:
            break;
        }
        active |= v->stage[i];
        float mod = v->previous[i] * v->fb[i];
        for (j = 0; j < v->nsrc[i]; j++)
            mod += output[v->src[i][j]] * v->amt[i][j];
        v->phase[i] += v->inc[i];
        if (v->phase[i] >= 1.0f)
            v->phase[i] -= 1.0f;
        output[i] = bw_sin(v->phase[i] + mod) * v->envelope[i] * v->gain[i];
        v->previous[i] = output[i];
        mix += output[i] * v->cw[i];
    }
    v->active = active != 0;
    return mix;
}
#define BW_BLOCK 256u
static float bw_l[BW_BLOCK], bw_r[BW_BLOCK], bw_srev[BW_BLOCK], bw_sdly[BW_BLOCK], bw_scho[BW_BLOCK];
static float bw_drive(float x, float amount)
{
    if (amount <= 0.0f)
        return x;
    float g = 1.0f + amount * 8.0f, u = fm_clampf(x * g, -1.5f, 1.5f);
    return (u - u * u * u * (4.0f / 27.0f)) * (1.4f / (1.0f + amount * 3.0f));
}
void bw_render(bw_engine *e, int32_t *lr, unsigned n, unsigned master)
{
    float gain = (master > 4096 ? 4096 : master) * (1600000.0f / 4096.0f);
    unsigned base = 0;
    while (base < n)
    {
        unsigned m = n - base > BW_BLOCK ? BW_BLOCK : n - base, f, k, i;
        for (f = 0; f < m; f++)
        {
            if ((e->lfo_count++ & 31u) == 0 || !bw_gains_set)
            {
                bw_modulation(e, 32);
                bw_gains_set = 1;
                for (i = 0; i < BW_VOICES; i++)
                    if (e->voice[i].active)
                        bw_voice_prepare(e, &e->voice[i]);
                for (k = 0; k < 4; k++)
                {
                    const bw_track *t = &e->track[k];
                    float pan = fm_clampf((float)t->pan / 100.0f + e->pan_mod[k], 0.0f, 1.0f);
                    pan_l[k] = fm_sqrtf(1.0f - pan) * 1.41421f;
                    pan_r[k] = fm_sqrtf(pan) * 1.41421f;
                    vol[k] = (float)t->volume / 100.0f * e->amp_scale[k];
                    srev[k] = fm_minf(1.0f, (float)t->send_rev / 100.0f + e->send_boost[k]);
                    sdly[k] = fm_minf(1.0f, (float)t->send_dly / 100.0f + e->send_boost[k]);
                    scho[k] = (float)t->send_cho / 100.0f;
                    drv[k] = (float)t->drive / 100.0f;
                }
            }
            /* time */
            if (e->song.sync && e->ext_ticks)
            {
                while (e->ext_ticks)
                {
                    e->ext_ticks--;
                    bw_tick(e);
                }
            }
            else if (!e->song.sync || !e->playing)
            {
                e->tick_acc += (uint32_t)e->song.tempo * BW_PPQN;
                while (e->tick_acc >= 60u * BW_RATE)
                {
                    e->tick_acc -= 60u * BW_RATE;
                    bw_tick(e);
                }
            }
            if (e->playing)
            {
                e->clock++;
                for (k = 0; k < 4; k++)
                    if (e->off_at[k] && (int32_t)(e->clock - e->off_at[k]) >= 0)
                    {
                        bw_seq_off(e, k);
                        e->off_at[k] = 0;
                    }
            }
            /* voices, by track */
            float tr[4] = {0, 0, 0, 0}, l = 0, r = 0, sr = 0, sd = 0, sc = 0;
            for (i = 0; i < BW_VOICES; i++)
                if (e->voice[i].active)
                    tr[e->voice[i].track & 3] += bw_voice_sample(e, &e->voice[i]);
            for (k = 0; k < 4; k++)
            {
                float x = bw_drive(tr[k], drv[k]) * vol[k], ax = x < 0.0f ? -x : x;
                e->meter[k] = ax > e->meter[k] ? ax : e->meter[k] * 0.9997f; /* peak, about 75 ms to fall 10 dB */
                l += x * pan_l[k];
                r += x * pan_r[k];
                sr += x * srev[k];
                sd += x * sdly[k];
                sc += x * scho[k];
            }
            bw_l[f] = l;
            bw_r[f] = r;
            bw_srev[f] = sr;
            bw_sdly[f] = sd;
            bw_scho[f] = sc;
        }
        bw_fx_process(&e->song, bw_l, bw_r, bw_srev, bw_sdly, bw_scho, m);
        for (f = 0; f < m; f++)
        {
            e->master_gain += (gain - e->master_gain) * 0.004f;
            float a = bw_l[f] * e->master_gain, b = bw_r[f] * e->master_gain;
            lr[(base + f) * 2] = (int32_t)fm_clampf(a, -8388607.0f, 8388607.0f);
            lr[(base + f) * 2 + 1] = (int32_t)fm_clampf(b, -8388607.0f, 8388607.0f);
        }
        base += m;
    }
}
