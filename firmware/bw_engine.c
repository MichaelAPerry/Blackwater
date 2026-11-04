/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "bw_engine.h"
#include "fastmath.h"
/* Embedded-safe: fixed allocation, no system calls, allocation, or storage in render. */
static float bw_sine[1025], bw_note_hz[128];
static uint32_t bw_random(bw_engine *e)
{
    uint32_t x = e->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return e->rng = x;
}
static float bw_sin(float cycles)
{
    float x = cycles - fm_floorf(cycles);
    x *= 1024.0f;
    unsigned i = (unsigned)x;
    float f = x - (float)i;
    return bw_sine[i] + f * (bw_sine[i + 1] - bw_sine[i]);
}
void bw_algorithm(bw_track *t, unsigned algorithm)
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
int bw_track_valid(const bw_track *t)
{
    unsigned i, j;
    if (t->algorithm > 7 || t->volume > 100 || t->wait_min > t->wait_max || t->wait_max > 600 ||
        t->manual > 1 || t->variation < 1 || t->variation > 16 || t->root > 96)
        return 0;
    for (i = 0; i < 6; i++)
    {
        const bw_operator *o = &t->op[i];
        if (!o->ratio || o->ratio > 1600 || !o->hz || o->hz > 16000 || o->attack > 10000 ||
            o->decay > 10000 || o->release > 10000 || o->level > 100 || o->sustain > 100 ||
            o->feedback > 100 || o->velocity > 100 || o->fixed > 1 || t->carrier[i] > 100)
            return 0;
        for (j = 0; j < 6; j++)
            if (t->route[i][j] > 100 || (i <= j && t->route[i][j]))
                return 0;
    }
    for (i = 0; i < 16; i++)
        if (t->pitch[i] > 7 || t->gate[i] > 1)
            return 0;
    return 1;
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
    e->tempo = 100;
    e->step = 15;
    for (k = 0; k < 4; k++)
    {
        bw_track *t = &e->track[k];
        t->volume = 70;
        t->wait_min = 8;
        t->wait_max = 32;
        t->variation = 2;
        t->root = (uint8_t)(36 + k * 12);
        for (i = 0; i < 6; i++)
        {
            bw_operator *o = &t->op[i];
            o->ratio = (uint16_t)(100 + i * 100);
            o->hz = 440;
            o->attack = (uint16_t)(5 + k * 15);
            o->decay = (uint16_t)(140 + k * 70);
            o->sustain = (uint8_t)(k == 3 ? 0 : 35);
            o->release = (uint16_t)(100 + k * 90);
            o->level = (uint8_t)(i ? 30 + k * 8 : 85);
            o->velocity = 60;
        }
        bw_algorithm(t, k + 1);
        for (i = 0; i < 6; i++)
            if (t->carrier[i])
                t->op[i].level = 85;
        for (i = 0; i < 16; i++)
        {
            t->pitch[i] = (uint8_t)((i * (k + 1)) % 8);
            t->gate[i] = (uint8_t)(i % (k + 2) == 0);
        }
    }
}
void bw_note_on(bw_engine *e, unsigned k, unsigned note, unsigned velocity)
{
    if (k >= 4)
        return;
    bw_voice *v = &e->voice[k];
    bw_track *t = &e->track[k];
    unsigned i;
    float hz = bw_note_hz[note > 127 ? 127 : note];
    v->velocity = (float)(velocity > 127 ? 127 : velocity) / 127.0f;
    v->active = 1;
    for (i = 0; i < 6; i++)
    {
        bw_operator *o = &t->op[i];
        v->increment[i] =
            fm_minf(o->fixed ? (float)o->hz : hz * ((float)o->ratio / 100.0f), BW_RATE * 0.45f) /
            BW_RATE;
        v->attack_step[i] = 1.0f / (1.0f + (float)o->attack * (BW_RATE / 1000.0f));
        v->decay_step[i] =
            (1.0f - (float)o->sustain / 100.0f) / (1.0f + (float)o->decay * (BW_RATE / 1000.0f));
        v->stage[i] = 1;
        v->phase[i] = v->envelope[i] = v->previous[i] = 0.0f;
    }
}
void bw_note_off(bw_engine *e, unsigned k)
{
    if (k >= 4)
        return;
    unsigned i;
    bw_voice *v = &e->voice[k];
    for (i = 0; i < 6; i++)
        if (v->stage[i] && v->stage[i] != 4)
        {
            v->stage[i] = 4;
            v->release_step[i] =
                v->envelope[i] / (1.0f + e->track[k].op[i].release * (BW_RATE / 1000.0f));
        }
}
static void bw_schedule(bw_engine *e, unsigned k)
{
    bw_track *t = &e->track[k];
    unsigned seconds = t->wait_min + bw_random(e) % (1u + t->wait_max - t->wait_min);
    e->deadline[k] = e->clock + seconds * BW_RATE;
}
void bw_cast(bw_engine *e, unsigned k)
{
    if (k < 4)
    {
        e->hooked[k] = 1;
        e->bite[k] = 0;
        bw_schedule(e, k);
    }
}
void bw_retrieve(bw_engine *e, unsigned k)
{
    if (k < 4)
    {
        e->hooked[k] = e->bite[k] = 0;
        bw_note_off(e, k);
    }
}
static void bw_catch(bw_engine *e, unsigned k)
{
    bw_track *t = &e->track[k];
    unsigned i;
    t->last_mm = 50 + bw_random(e) % 951u;
    if (t->last_mm > t->best_mm)
        t->best_mm = t->last_mm;
    if (t->catches < 0xffffffffu)
        t->catches++;
    for (i = 0; i < t->variation; i++)
    {
        unsigned s = bw_random(e) % 16;
        t->pitch[s] = (uint8_t)(bw_random(e) % 8);
        t->gate[s] = (uint8_t)(bw_random(e) % 4 != 0);
    }
    e->bite[k] = 0;
    bw_schedule(e, k);
}
int bw_land(bw_engine *e, unsigned k)
{
    if (k >= 4 || !e->hooked[k] || !e->bite[k] || !e->playing)
        return 0;
    if ((int32_t)(e->clock - e->bite_until[k]) >= 0)
        return 0;
    bw_catch(e, k);
    return 1;
}
void bw_transport(bw_engine *e, int playing)
{
    unsigned k;
    e->playing = playing != 0;
    if (!playing)
        for (k = 0; k < 4; k++)
            bw_note_off(e, k);
}
static void bw_tick(bw_engine *e)
{
    static const uint8_t scale[8] = {0, 2, 3, 5, 7, 9, 10, 12};
    unsigned k;
    e->step = (uint8_t)((e->step + 1) % 16);
    for (k = 0; k < 4; k++)
    {
        bw_track *t = &e->track[k];
        if (!e->hooked[k])
            continue;
        /* Zero waits resolve at most once per sixteenth, never in a busy loop. */
        if (e->bite[k])
        {
            if ((int32_t)(e->clock - e->bite_until[k]) >= 0)
            {
                e->bite[k] = 0;
                e->missed[k]++;
                bw_schedule(e, k);
            }
        }
        else if ((int32_t)(e->clock - e->deadline[k]) >= 0)
        {
            if (t->manual)
            {
                e->bite[k] = 1;
                e->bite_until[k] = e->clock + 2 * BW_RATE;
            }
            else
                bw_catch(e, k);
        }
        if (t->gate[e->step])
        {
            bw_note_on(e, k, t->root + scale[t->pitch[e->step]], 100);
            e->gate_until[k] = e->clock + (BW_RATE * 60u / (e->tempo * 4u)) * 3u / 4u;
        }
    }
}
static float bw_voice_sample(bw_engine *e, unsigned k)
{
    bw_voice *v = &e->voice[k];
    bw_track *t = &e->track[k];
    float output[6], mix = 0.0f;
    unsigned i, j, active = 0, carriers = 0;
    if (!v->active)
        return 0.0f;
    for (i = 6; i-- > 0;)
    {
        bw_operator *o = &t->op[i];
        float sustain = o->sustain / 100.0f, mod = v->previous[i] * (o->feedback * 0.02f);
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
            if (v->envelope[i] <= sustain)
            {
                v->envelope[i] = sustain;
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
        for (j = i + 1; j < 6; j++)
            mod += output[j] * (t->route[j][i] * 0.02f);
        v->phase[i] += v->increment[i];
        if (v->phase[i] >= 1.0f)
            v->phase[i] -= 1.0f;
        output[i] = bw_sin(v->phase[i] + mod) * v->envelope[i] * (o->level / 100.0f) *
                    (1.0f - o->velocity / 100.0f * (1.0f - v->velocity));
        v->previous[i] = output[i];
        mix += output[i] * (t->carrier[i] / 100.0f);
        if (t->carrier[i])
            carriers++;
    }
    v->active = active != 0;
    return mix * (t->volume / 100.0f) / (float)(carriers ? carriers : 1);
}
void bw_render(bw_engine *e, int32_t *lr, unsigned n, unsigned master)
{
    unsigned frame, k;
    float gain = (master > 4096 ? 4096 : master) * (1600000.0f / 4096.0f);
    for (frame = 0; frame < n; frame++)
    {
        float mix = 0;
        if (e->playing)
        {
            e->clock++;
            e->beat_phase += e->tempo * 4u;
            if (e->beat_phase >= BW_RATE * 60u)
            {
                e->beat_phase -= BW_RATE * 60u;
                bw_tick(e);
            }
            for (k = 0; k < 4; k++)
                if (e->gate_until[k] && (int32_t)(e->clock - e->gate_until[k]) >= 0)
                {
                    bw_note_off(e, k);
                    e->gate_until[k] = 0;
                }
        }
        for (k = 0; k < 4; k++)
            mix += bw_voice_sample(e, k);
        /* Four normalized voices, each <= 1; leave headroom before 24-bit output. */
        e->master_gain += (gain - e->master_gain) * 0.004f;
        lr[frame * 2] = lr[frame * 2 + 1] = (int32_t)(mix * e->master_gain);
    }
}
