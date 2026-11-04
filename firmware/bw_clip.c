/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The "fish on" clip: one-shot playback of tools/make_clip.py's IMA ADPCM (22,050 Hz, mono), mixed into the
 * synth's 44.1 kHz stereo output at twice its rate by linear interpolation. Integer arithmetic only; a
 * trigger while the clip plays is ignored, so fast catches never stutter it. With no clip built in
 * (BW_CLIP_FRAMES 0) every call is a no-op. */
#include "bw_clip.h"
#include <stdint.h>
typedef struct
{
    uint32_t n;          /* next 22,050 Hz frame to decode */
    int32_t pred, s0, s1; /* decoder state; the two samples being interpolated */
    uint8_t idx, half, active;
} bw_clip_state;
int bw_clip_present(void)
{
    return BW_CLIP_FRAMES > 0;
}
#if !BW_CLIP_FRAMES
void bw_clip_trigger(bw_clip_state *c)
{
    (void)c;
}
void bw_clip_mix(bw_clip_state *c, int32_t *lr, unsigned n, uint32_t master_q12)
{
    (void)c, (void)lr, (void)n, (void)master_q12;
}
#else
static const uint16_t bw_clip_st[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,
    31,    34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,   494,
    544,   598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,
    9493,  10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
static const int8_t bw_clip_ix[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
static int32_t bw_clip_next(bw_clip_state *c)
{
    uint32_t k = c->n;
    if (k >= BW_CLIP_FRAMES)
        return 0;
    if ((k & 255u) == 0)
    {
        /* each 256-sample block starts from its own header: decoding resynchronises there */
        const uint8_t *h = bw_clip_adpcm + (k >> 8) * 132u;
        c->pred = (int16_t)(h[0] | h[1] << 8);
        c->idx = h[2] > 88 ? 88 : h[2];
    }
    unsigned b = bw_clip_adpcm[(k >> 8) * 132u + 4u + (k & 255u) / 2u];
    unsigned code = k & 1u ? b >> 4 : b & 15u;
    int32_t s = bw_clip_st[c->idx], d = s >> 3;
    if (code & 4)
        d += s;
    if (code & 2)
        d += s >> 1;
    if (code & 1)
        d += s >> 2;
    if (code & 8)
        d = -d;
    d += c->pred;
    c->pred = d < -32768 ? -32768 : d > 32767 ? 32767 : d;
    int i = c->idx + bw_clip_ix[code & 7];
    c->idx = (uint8_t)(i < 0 ? 0 : i > 88 ? 88 : i);
    c->n = k + 1;
    return c->pred;
}
void bw_clip_trigger(bw_clip_state *c)
{
    if (c->active)
        return;
    c->n = 0;
    c->half = 0;
    c->s0 = 0;
    c->s1 = bw_clip_next(c);
    c->active = 1;
}
/* adds the clip into n stereo frames; master_q12 as the synth's (0..4096): full scale is +-2^22 of the
 * 24-bit output, half its range, leaving headroom for the four voices beneath it */
void bw_clip_mix(bw_clip_state *c, int32_t *lr, unsigned n, uint32_t master_q12)
{
    if (!c->active)
        return;
    int32_t m = (int32_t)(master_q12 > 4096u ? 4096u : master_q12);
    for (unsigned f = 0; f < n; f++)
    {
        if (!c->half)
        {
            if (c->n >= BW_CLIP_FRAMES && c->s1 == 0 && c->s0 == 0)
            {
                c->active = 0;
                return;
            }
            c->s0 = c->s1;
            c->s1 = c->n < BW_CLIP_FRAMES ? bw_clip_next(c) : 0;
        }
        int32_t v = c->half ? (c->s0 + c->s1) >> 1 : c->s0; /* 22,050 -> 44,100 Hz */
        c->half ^= 1u;
        v = (v * m) >> 5; /* +-32767 * 4096 / 32 = +-2^22 */
        for (unsigned ch = 0; ch < 2; ch++)
        {
            int32_t o = lr[f * 2 + ch] + v;
            lr[f * 2 + ch] = o > 8388607 ? 8388607 : o < -8388608 ? -8388608 : o;
        }
    }
}
#endif
