/* SPDX-License-Identifier: GPL-3.0-or-later */
/* BLACKWATER's effects bus, after the tracks' own drive, pan and sends (bw_engine.c):
 *   chorus  the chorus send through two taps around 12 ms, swept in opposite phase (left / right)
 *   delay   the delay send, at the tempo (1/16 .. dotted 1/4), its repeats darkening (a low-pass in the
 *           feedback: echoes off the water); the right tap later than the left by the width
 *   reverb  the reverb send and a little of the delay: four damped combs and two all-passes a side, at half
 *           rate (a new sample every other frame, interpolated): size = feedback, tone = damping
 *   master  a one-pole low-pass (TONE, fully open at 100) and stereo width (0 mono .. 100 wide)
 * Levels are the voices' (+-1 each); the lines hold int16 at 1/8192 (+-4 headroom). A stage at level 0 costs
 * nothing; its line is cleared when it comes back. Lines in the pool (PLAT_POOL) on the device: about 150 KB. */
#include <stdint.h>
#include "plat.h" /* PLAT_POOL: the lines in the pool on the device */
#define BW_CN 2048u  /* chorus, 46 ms */
#define BW_DN 65536u /* delay, 1.49 s */
static const uint16_t bw_comb_len[4] = {557, 593, 641, 677}, bw_ap_len[4] = {223, 271, 241, 293}; /* at 22.05 kHz */
#define BW_RN (557u + 593u + 641u + 677u + 223u + 271u + 241u + 293u)
static int16_t bw_cline[BW_CN] PLAT_POOL, bw_dline[BW_DN] PLAT_POOL, bw_rline[BW_RN] PLAT_POOL;
static struct
{
    uint32_t cw, dw, ci[4], ai[4], half, on, quiet_c, quiet_d, quiet_r; /* samples of silence in each */
    float cph, damp[4], dlp, rl, rr, pl, pr, tl, tr;
} bw_fxs;
static void bw_zero(int16_t *b, uint32_t n);
/* everything silent: the lines and the filters (bw_init) */
static void bw_fx_reset(void)
{
    unsigned char *p = (unsigned char *)&bw_fxs;
    for (unsigned i = 0; i < sizeof bw_fxs; i++)
        p[i] = 0;
    bw_zero(bw_cline, BW_CN);
    bw_zero(bw_dline, BW_DN);
    bw_zero(bw_rline, BW_RN);
}
static int16_t bw_q(float x)
{
    x *= 8192.0f;
    return (int16_t)(x > 32767.0f ? 32767 : x < -32767.0f ? -32767 : (int)x);
}
static void bw_zero(int16_t *b, uint32_t n)
{
    while (n--)
        *b++ = 0;
}
static float bw_ctap(float delay)
{
    uint32_t d = (uint32_t)delay;
    float fr = delay - (float)d;
    uint32_t i = (bw_fxs.cw - d) & (BW_CN - 1u), j = (i - 1u) & (BW_CN - 1u);
    return ((float)bw_cline[i] + fr * (float)(bw_cline[j] - bw_cline[i])) * (1.0f / 8192.0f);
}
void bw_fx_process(const bw_song *s, float *l, float *r, const float *rev, const float *dly, const float *cho,
                   unsigned n)
{
    unsigned f, k;
    uint32_t now = (s->cho_depth ? 1u : 0) | (s->dly_level ? 2u : 0) | (s->rev_level ? 4u : 0);
    /* a stage whose input has been silent longer than its tail sleeps (its line keeps nothing worth hearing):
     * chorus 0.1 s, delay 12 repeats of its time, reverb 4 s */
    {
        int c = 0, d = 0, r = 0;
        for (f = 0; f < n; f++)
        {
            c |= cho[f] != 0.0f;
            d |= dly[f] != 0.0f;
            r |= rev[f] != 0.0f;
        }
        bw_fxs.quiet_c = c ? 0 : bw_fxs.quiet_c + n;
        bw_fxs.quiet_d = d ? 0 : bw_fxs.quiet_d + n;
        bw_fxs.quiet_r = r || (bw_fxs.on & 2u && bw_fxs.quiet_d < 4u * BW_RATE) ? 0 : bw_fxs.quiet_r + n;
        if (bw_fxs.quiet_c > BW_RATE / 10u)
            now &= ~1u;
        if (bw_fxs.quiet_d > 13u * BW_DN)
            now &= ~2u;
        if (bw_fxs.quiet_r > 4u * BW_RATE)
            now &= ~4u;
    }
    if ((now & ~bw_fxs.on) & 1u)
        bw_zero(bw_cline, BW_CN);
    if ((now & ~bw_fxs.on) & 2u)
    {
        bw_zero(bw_dline, BW_DN);
        bw_fxs.dlp = 0;
    }
    if ((now & ~bw_fxs.on) & 4u)
    {
        bw_zero(bw_rline, BW_RN);
        for (k = 0; k < 4; k++)
            bw_fxs.damp[k] = 0;
        bw_fxs.rl = bw_fxs.rr = bw_fxs.pl = bw_fxs.pr = 0;
    }
    bw_fxs.on = now;
    /* delay time at the tempo; halved until it fits the line */
    static const float beats[6] = {0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f};
    float dsamp = beats[s->dly_time * 6u / 101u] * 60.0f * (float)BW_RATE / (float)s->tempo;
    while (dsamp > (float)(BW_DN - 2048u))
        dsamp *= 0.5f;
    uint32_t dl = (uint32_t)dsamp, dr = dl + (uint32_t)((float)s->width * 8.8f); /* up to 20 ms later */
    float fb = 0.15f + (float)s->dly_feedback * 0.0065f, dlev = (float)s->dly_level / 100.0f;
    float rfb = 0.70f + (float)s->rev_size * 0.0025f, rdamp = 0.05f + (100.0f - (float)s->rev_tone) * 0.006f;
    float rlev = (float)s->rev_level / 100.0f * 0.6f;
    float cinc = (0.1f + (float)s->cho_rate * 0.039f) / (float)BW_RATE, cdep = (float)s->cho_depth * 1.3f;
    float tone = s->tone >= 99 ? 1.0f : 1.0f - bw_exp(-FM_TWO_PI * 400.0f * bw_exp2((float)s->tone * 0.06f) /
                                                       (float)BW_RATE);
    float width = (float)s->width / 70.0f;
    for (f = 0; f < n; f++)
    {
        float L = l[f], R = r[f];
        if (now & 1u)
        {
            bw_cline[bw_fxs.cw] = bw_q(cho[f]);
            bw_fxs.cph += cinc;
            if (bw_fxs.cph >= 1.0f)
                bw_fxs.cph -= 1.0f;
            float sw = bw_sin(bw_fxs.cph) * cdep;
            L += bw_ctap(529.0f + sw);
            R += bw_ctap(529.0f - sw);
            bw_fxs.cw = (bw_fxs.cw + 1u) & (BW_CN - 1u);
        }
        float echo = 0.0f;
        if (now & 2u)
        {
            float el = (float)bw_dline[(bw_fxs.dw - dl) & (BW_DN - 1u)] * (1.0f / 8192.0f);
            float er = (float)bw_dline[(bw_fxs.dw - dr) & (BW_DN - 1u)] * (1.0f / 8192.0f);
            bw_fxs.dlp += (el - bw_fxs.dlp) * 0.35f; /* the repeats darken */
            bw_dline[bw_fxs.dw] = bw_q(dly[f] + bw_fxs.dlp * fb);
            bw_fxs.dw = (bw_fxs.dw + 1u) & (BW_DN - 1u);
            L += el * dlev;
            R += er * dlev;
            echo = (el + er) * 0.5f;
        }
        if (now & 4u)
        {
            float ol, orr;
            if ((bw_fxs.half ^= 1u) != 0)
            {
                int16_t *p = bw_rline;
                float in = (rev[f] + echo * 0.3f) * 0.25f, sum = 0.0f;
                for (k = 0; k < 4; k++)
                {
                    float o = (float)p[bw_fxs.ci[k]] * (1.0f / 8192.0f);
                    bw_fxs.damp[k] += (o - bw_fxs.damp[k]) * (1.0f - rdamp);
                    p[bw_fxs.ci[k]] = bw_q(in + bw_fxs.damp[k] * rfb);
                    if (++bw_fxs.ci[k] >= bw_comb_len[k])
                        bw_fxs.ci[k] = 0;
                    sum += o;
                    p += bw_comb_len[k];
                }
                ol = orr = sum * 0.5f;
                for (k = 0; k < 4; k++)
                {
                    float *v = k < 2 ? &ol : &orr, b = (float)p[bw_fxs.ai[k]] * (1.0f / 8192.0f);
                    p[bw_fxs.ai[k]] = bw_q(*v + b * 0.5f);
                    *v = b - *v;
                    if (++bw_fxs.ai[k] >= bw_ap_len[k])
                        bw_fxs.ai[k] = 0;
                    p += bw_ap_len[k];
                }
                bw_fxs.pl = bw_fxs.rl;
                bw_fxs.pr = bw_fxs.rr;
                bw_fxs.rl = ol;
                bw_fxs.rr = orr;
                ol = (bw_fxs.pl + bw_fxs.rl) * 0.5f;
                orr = (bw_fxs.pr + bw_fxs.rr) * 0.5f;
            }
            else
            {
                ol = bw_fxs.rl;
                orr = bw_fxs.rr;
            }
            L += ol * rlev;
            R += orr * rlev;
        }
        /* master: tone, then width */
        bw_fxs.tl += (L - bw_fxs.tl) * tone;
        bw_fxs.tr += (R - bw_fxs.tr) * tone;
        float mid = (bw_fxs.tl + bw_fxs.tr) * 0.5f, side = (bw_fxs.tl - bw_fxs.tr) * 0.5f * width;
        l[f] = mid + side;
        r[f] = mid - side;
    }
}
