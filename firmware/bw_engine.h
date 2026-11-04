/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stdint.h>
#define BW_TRACKS 4
#define BW_OPS 6
#define BW_STEPS 16
#define BW_RATE 44100u
/* Versioned integer parameters: no pointers or compiler-dependent float data in saves. */
typedef struct
{
    uint16_t ratio, hz, attack, decay, release; /* ratio / 100, Hz, milliseconds */
    uint8_t level, sustain, feedback, velocity, fixed;
} bw_operator;
typedef struct
{
    bw_operator op[BW_OPS];
    uint8_t route[BW_OPS][BW_OPS]; /* source -> destination; only source > destination */
    uint8_t carrier[BW_OPS], algorithm, volume;
    uint16_t wait_min, wait_max; /* seconds, 0..600 */
    uint8_t manual, variation, root, pitch[BW_STEPS], gate[BW_STEPS];
    uint32_t catches, best_mm, last_mm;
} bw_track;
typedef struct
{
    float phase[BW_OPS], envelope[BW_OPS], release_step[BW_OPS], previous[BW_OPS];
    float increment[BW_OPS], attack_step[BW_OPS], decay_step[BW_OPS];
    uint8_t stage[BW_OPS], active;
    float velocity;
} bw_voice;
typedef struct
{
    bw_track track[BW_TRACKS];
    bw_voice voice[BW_TRACKS];
    uint32_t rng, clock, deadline[BW_TRACKS], bite_until[BW_TRACKS];
    uint32_t gate_until[BW_TRACKS], missed[BW_TRACKS];
    uint8_t hooked[BW_TRACKS], bite[BW_TRACKS], step, playing;
    uint16_t tempo;
    uint32_t beat_phase;
    float master_gain;
} bw_engine;
void bw_init(bw_engine *e);
void bw_algorithm(bw_track *t, unsigned algorithm);
int bw_track_valid(const bw_track *t);
void bw_note_on(bw_engine *e, unsigned track, unsigned note, unsigned velocity);
void bw_note_off(bw_engine *e, unsigned track);
void bw_cast(bw_engine *e, unsigned track);
void bw_retrieve(bw_engine *e, unsigned track);
int bw_land(bw_engine *e, unsigned track);
void bw_transport(bw_engine *e, int playing);
void bw_render(bw_engine *e, int32_t *stereo, unsigned frames, unsigned master);
