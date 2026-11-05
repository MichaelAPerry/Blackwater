/* SPDX-License-Identifier: GPL-3.0-or-later */
/* BLACKWATER's engine: four fish tracks of six-operator FM, a theme each that catches develop, the fishing that
 * drives them, and the performance layer (live keys, FM macros, arpeggiator, LFOs, MIDI). Saved data (bw_song,
 * bw_track, bw_patch, bw_theme) is plain integers; runtime state (voices, timers) is separate and never saved. */
#pragma once
#include <stdint.h>
#define BW_TRACKS 4
#define BW_OPS 6
#define BW_STEPS 32
#define BW_VOICES 8
#define BW_RATE 44100u
#define BW_PPQN 96u
#define BW_SPECIES 8 /* the four fish, then their rare cousins: species k + 4 */

enum { BW_BLUEGILL, BW_BASS, BW_TROUT, BW_CATFISH, BW_CRAPPIE, BW_PEACOCK, BW_BROOK, BW_FLATHEAD };
enum { BW_GATE_REST, BW_GATE_NOTE, BW_GATE_TIE };
enum { BW_BAIT_WORM, BW_BAIT_CRANK, BW_BAIT_SPINNER, BW_BAIT_JIG, BW_NBAIT };
enum { BW_DEPTH_SHALLOW, BW_DEPTH_MID, BW_DEPTH_DEEP, BW_DEPTH_BOTTOM, BW_NDEPTH };
enum { BW_WEATHER_AUTO, BW_WEATHER_DUSK, BW_WEATHER_NIGHT, BW_WEATHER_STORM, BW_WEATHER_DAY, BW_NWEATHER };
enum { BW_LFO_PITCH, BW_LFO_FM, BW_LFO_AMP, BW_LFO_PAN, BW_NLFO_TARGET };
enum { BW_LFO_SINE, BW_LFO_TRI, BW_LFO_SQUARE, BW_LFO_RANDOM, BW_NLFO_SHAPE };
enum { BW_ARP_UP, BW_ARP_DOWN, BW_ARP_UPDOWN, BW_ARP_RANDOM, BW_ARP_PLAYED, BW_NARP_MODE };
enum { BW_ARP_OFF, BW_ARP_ON, BW_ARP_LATCH };
enum { BW_SCALES = 8, BW_PROGRESSIONS = 6 };
/* black-key FM macros, held */
enum { BW_M_BRIGHT, BW_M_DARK, BW_M_GROWL, BW_M_METAL, BW_M_DIVE, BW_M_RISE, BW_M_STUTTER, BW_M_HALF, BW_M_CUT,
       BW_M_OCTAVE, BW_M_THROW, BW_NMACRO };
/* fishing states per track */
enum { BW_FISH_IDLE, BW_FISH_WAIT, BW_FISH_BITE, BW_FISH_FIGHT };
/* fight results */
enum { BW_FIGHT_ON, BW_FIGHT_LANDED, BW_FIGHT_SNAPPED, BW_FIGHT_THROWN };

typedef struct
{
    uint16_t ratio, hz, attack, decay, release; /* ratio / 100, Hz, milliseconds */
    uint8_t level, sustain, feedback, velocity, fixed;
} bw_operator;
typedef struct
{
    bw_operator op[BW_OPS];
    uint8_t route[BW_OPS][BW_OPS]; /* source -> destination; only source > destination */
    uint8_t carrier[BW_OPS], algorithm;
} bw_patch;
typedef struct
{
    int8_t deg;   /* scale degree from the track's root */
    uint8_t gate; /* BW_GATE_* */
    uint8_t vel;  /* 1..127 */
} bw_step;
typedef struct
{
    bw_step s[BW_STEPS];
    uint8_t len; /* 4..32 */
} bw_theme;
typedef struct
{
    bw_patch patch;
    bw_theme home, cur, prev;
    uint8_t volume, pan, mute, solo, density; /* 0..100, pan 50 centre */
    int8_t octave;                              /* -3..3 from the fish's register */
    uint8_t send_rev, send_dly, send_cho, drive; /* 0..100 */
    uint8_t lfo_rate, lfo_depth, lfo_target, lfo_shape;
    uint16_t wait_min, wait_max; /* seconds, 0..600 */
    uint8_t manual, bait, depth, has_prev, patch_slot;
} bw_track;
typedef struct
{
    uint16_t tempo;                         /* 30..240 */
    uint8_t key, scale, progression, swing; /* key 0..11 (C..B), swing 0..60 % */
    uint8_t pb_event, weather, sync;        /* personal best: 1 = a key change; MIDI clock: 0 internal, 1 external */
    uint8_t rev_size, rev_tone, rev_level, dly_time, dly_feedback, dly_level, cho_rate, cho_depth, tone, width;
    uint8_t arp_mode, arp_rate, arp_octaves, arp_gate;
} bw_song;
typedef struct
{
    uint32_t count, best_mm, best_g, last_mm, last_g;
} bw_species_log;
typedef struct
{
    bw_species_log s[BW_SPECIES];
    uint32_t missed, snapped, thrown, released;
} bw_log;

typedef struct
{
    float phase[BW_OPS], envelope[BW_OPS], release_step[BW_OPS], previous[BW_OPS];
    float increment[BW_OPS], attack_step[BW_OPS], decay_step[BW_OPS];
    float velocity, glide;
    /* per operator, refreshed every 32 frames from the patch and the modulation (bw_voice_prepare) */
    float gain[BW_OPS], cw[BW_OPS], sustain[BW_OPS], fb[BW_OPS], inc[BW_OPS], amt[BW_OPS][BW_OPS - 1];
    uint8_t nsrc[BW_OPS], src[BW_OPS][BW_OPS - 1];
    uint32_t age;
    uint8_t stage[BW_OPS], active, track, note, held, live;
} bw_voice;
/* a catch, for the screen: what was landed (or lost) and what it did */
typedef struct
{
    uint32_t seq; /* changes with each event */
    uint32_t mm, g;
    uint8_t track, species, rare, best, result, moves, key_change;
} bw_event;
typedef struct
{
    bw_song song;
    bw_track track[BW_TRACKS];
    bw_log log;
    bw_voice voice[BW_VOICES];
    /* runtime */
    uint32_t rng, clock, age, tick_acc, ticks, bar;
    uint8_t playing, chord;
    /* per track: step position, its next step's tick count, the voice it plays, held note-off time */
    uint32_t tcount[BW_TRACKS], off_at[BW_TRACKS], deadline[BW_TRACKS], bite_until[BW_TRACKS];
    uint8_t step[BW_TRACKS], last_voice[BW_TRACKS], last_note[BW_TRACKS], sounding[BW_TRACKS];
    uint8_t fish[BW_TRACKS], fish_rare[BW_TRACKS], hooked_state[BW_TRACKS];
    uint32_t fish_mm[BW_TRACKS];
    /* fight (TIMED mode, after the hook is set): distance left, line tension, the fish's run */
    float fight_dist[BW_TRACKS], fight_tension[BW_TRACKS], fight_run[BW_TRACKS], fight_max[BW_TRACKS];
    uint32_t fight_slack_ms[BW_TRACKS];
    /* performance */
    uint32_t macros[BW_TRACKS]; /* BW_M_* bits held */
    float lfo_phase[BW_TRACKS], lfo_value[BW_TRACKS], lfo_hold[BW_TRACKS];
    float mod_scale[BW_TRACKS], pitch_scale[BW_TRACKS], amp_scale[BW_TRACKS], pan_mod[BW_TRACKS];
    float bend[BW_TRACKS], fb_add[BW_TRACKS], metal[BW_TRACKS], send_boost[BW_TRACKS];
    int8_t live_oct;
    float meter[BW_TRACKS]; /* each fish's output level, for the screen */
    uint32_t lfo_count;
    uint8_t live_track, arp, arp_n, arp_pos, arp_dir, rec_arm[BW_TRACKS], rec_left[BW_TRACKS];
    int8_t arp_held[16], live_note[BW_TRACKS];
    uint32_t arp_keys, arp_order, arp_gate_left;
    uint32_t ext_ticks, ext_seen; /* MIDI clock: ticks owed by external clock pulses */
    uint32_t midi_clock_div;
    uint8_t midi_out, midi_notes[BW_TRACKS];
    bw_event event;
    float master_gain;
} bw_engine;

void bw_init(bw_engine *e);
void bw_defaults(bw_engine *e);
void bw_track_defaults(bw_engine *e, unsigned k);
void bw_algorithm(bw_patch *p, unsigned algorithm);
int bw_track_valid(const bw_track *t);
int bw_song_valid(const bw_song *s);
void bw_patch_factory(bw_patch *p, unsigned species, unsigned slot); /* 4 per fish */
const char *bw_patch_name(unsigned species, unsigned slot);
void bw_theme_generate(bw_engine *e, unsigned k, uint32_t seed);
unsigned bw_steps_per_beat(unsigned k);
int bw_note_of(const bw_engine *e, unsigned k, int deg, int with_chord);
int bw_chord_root(const bw_engine *e); /* the chord now: its root as a scale degree */
/* playing */
void bw_transport(bw_engine *e, int playing);
void bw_render(bw_engine *e, int32_t *lr, unsigned n, unsigned master);
void bw_live_key(bw_engine *e, unsigned white, int down, unsigned velocity); /* white key 0..15 */
void bw_live_note(bw_engine *e, unsigned k, unsigned note, int down, unsigned velocity); /* MIDI in, audition */
void bw_macro(bw_engine *e, unsigned k, unsigned macro, int down);
void bw_all_notes_off(bw_engine *e);
void bw_midi_clock(bw_engine *e, unsigned status); /* 0xF8, 0xFA, 0xFB, 0xFC */
/* fishing */
void bw_cast(bw_engine *e, unsigned k);
void bw_retrieve(bw_engine *e, unsigned k);
int bw_fish_state(const bw_engine *e, unsigned k);
int bw_hookset(bw_engine *e, unsigned k); /* REC during a bite: 1 = the fight starts */
int bw_fight(bw_engine *e, unsigned k, int reel, unsigned ms);
void bw_release(bw_engine *e, unsigned k); /* the last catch's change undone */
void bw_restore_home(bw_engine *e, unsigned k);
void bw_make_home(bw_engine *e, unsigned k);
unsigned bw_weather(const bw_engine *e);
uint32_t bw_weight_g(unsigned species, uint32_t mm);
uint32_t bw_species_max_mm(unsigned species); /* the top of its usual range */
/* names and data for the screen */
extern const char *const bw_species_names[BW_SPECIES];
extern const char *const bw_scale_names[BW_SCALES];
extern const char *const bw_progression_names[BW_PROGRESSIONS];
extern const char *const bw_key_names[12];
extern const char *const bw_bait_names[BW_NBAIT];
extern const char *const bw_depth_names[BW_NDEPTH];
extern const char *const bw_weather_names[BW_NWEATHER];
extern const char *const bw_lfo_target_names[BW_NLFO_TARGET];
extern const char *const bw_lfo_shape_names[BW_NLFO_SHAPE];
extern const char *const bw_arp_mode_names[BW_NARP_MODE];
extern const char *const bw_arp_rate_names[5];
extern const char *const bw_macro_names[BW_NMACRO];
/* MIDI out (app.c: plat_midi_send; a test stub elsewhere): a USB-MIDI packet; room = free slots */
void bw_midi_send(uint32_t pkt);
uint32_t bw_midi_room(void);
/* the clip hook: called from the render when a rare fish is hooked (app.c plays it) */
void bw_on_rare(unsigned k);
/* the effects bus hook: the render hands the dry stereo mix and the three sends (app.c's bw_fx) */
void bw_fx_process(const bw_song *s, float *l, float *r, const float *rev, const float *dly, const float *cho,
                   unsigned n);
