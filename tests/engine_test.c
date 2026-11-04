/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "bw_engine.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int32_t block[512];
static void advance(bw_engine *e, unsigned frames)
{
    while (frames)
    {
        unsigned n = frames > 256 ? 256 : frames;
        bw_render(e, block, n, 4096);
        frames -= n;
    }
}
static void audio(void)
{
    bw_engine e;
    bw_init(&e);
    for (unsigned t = 0; t < 4; t++)
        assert(bw_track_valid(&e.track[t]));
    for (unsigned algorithm = 0; algorithm < 8; algorithm++)
    {
        bw_algorithm(&e.track[0], algorithm);
        bw_note_on(&e, 0, 60, 110);
        int64_t energy = 0;
        for (unsigned b = 0; b < 20; b++)
        {
            bw_render(&e, block, 256, 4096);
            for (unsigned i = 0; i < 512; i++)
            {
                assert(block[i] >= -8388608 && block[i] <= 8388607);
                energy += llabs(block[i]);
            }
        }
        assert(energy > 100000);
        bw_note_off(&e, 0);
        advance(&e, 2 * BW_RATE);
        assert(!e.voice[0].active);
        for (unsigned i = 0; i < 512; i++)
            assert(!block[i]);
    }
    /* A changed operator must change actual samples, not just the UI value. */
    bw_init(&e);
    bw_note_on(&e, 0, 60, 100);
    bw_render(&e, block, 256, 4096);
    int32_t reference[512];
    memcpy(reference, block, sizeof block);
    bw_init(&e);
    e.track[0].op[1].ratio = 713;
    bw_note_on(&e, 0, 60, 100);
    bw_render(&e, block, 256, 4096);
    assert(memcmp(reference, block, sizeof block) != 0);
    /* All four high-feedback, six-operator voices stay inside the hardware sample range. */
    for (unsigned k = 0; k < 4; k++)
    {
        for (unsigned o = 0; o < 6; o++)
        {
            e.track[k].op[o].feedback = 100;
            e.track[k].op[o].level = 100;
        }
        bw_note_on(&e, k, 80, 127);
    }
    for (unsigned b = 0; b < 100; b++)
    {
        bw_render(&e, block, 256, 4096);
        for (unsigned i = 0; i < 512; i++)
            assert(block[i] >= -8388608 && block[i] <= 8388607);
    }
    puts("PASS: eight algorithms, four-voice headroom, operator edit, release to silence");
}
static void fishing(void)
{
    bw_engine e;
    bw_init(&e);
    e.tempo = 120;
    e.track[0].wait_min = e.track[0].wait_max = 0;
    bw_cast(&e, 0);
    advance(&e, BW_RATE);
    assert(!e.track[0].catches && e.clock == 0);
    bw_transport(&e, 1);
    advance(&e, 5512);
    assert(!e.track[0].catches);
    advance(&e, 1);
    assert(e.track[0].catches == 1);
    assert(e.track[0].last_mm >= 50 && e.track[0].last_mm <= 1000);
    advance(&e, 5512);
    assert(e.track[0].catches == 2);
    assert(e.track[0].best_mm >= e.track[0].last_mm);
    bw_retrieve(&e, 0);
    advance(&e, BW_RATE);
    assert(e.track[0].catches == 2);
    bw_init(&e);
    e.track[0].manual = 1;
    e.track[0].wait_min = e.track[0].wait_max = 0;
    bw_cast(&e, 0);
    bw_transport(&e, 1);
    advance(&e, BW_RATE / 4);
    assert(e.bite[0] && !e.track[0].catches);
    assert(bw_land(&e, 0));
    assert(e.track[0].catches == 1);
    assert(!bw_land(&e, 0));
    advance(&e, 3 * BW_RATE);
    assert(e.missed[0] >= 1);
    assert(e.track[0].catches == 1);
    /* Far future timer, modular clock rollover, and selected-track isolation. */
    bw_init(&e);
    e.track[0].wait_min = e.track[0].wait_max = 600;
    e.clock = 0xfffffff0u;
    bw_cast(&e, 0);
    assert(e.deadline[0] == e.clock + 600u * BW_RATE);
    bw_transport(&e, 1);
    advance(&e, 2 * BW_RATE);
    assert(!e.track[0].catches);
    for (unsigned k = 1; k < 4; k++)
        assert(!e.hooked[k] && !e.track[k].catches);
    bw_track bad = e.track[0];
    bad.op[0].ratio = 0;
    assert(!bw_track_valid(&bad));
    bad = e.track[0];
    bad.route[0][5] = 1;
    assert(!bw_track_valid(&bad));
    puts("PASS: automatic/timed catches, missed bite, zero/600s waits, rollover, track isolation");
}
static void deterministic(void)
{
    bw_engine a, b;
    bw_init(&a);
    bw_init(&b);
    for (unsigned k = 0; k < 4; k++)
    {
        a.track[k].wait_min = b.track[k].wait_min = 0;
        a.track[k].wait_max = b.track[k].wait_max = 0;
        bw_cast(&a, k);
        bw_cast(&b, k);
    }
    bw_transport(&a, 1);
    bw_transport(&b, 1);
    int32_t other[512];
    for (unsigned i = 0; i < 1000; i++)
    {
        bw_render(&a, block, 256, 4096);
        bw_render(&b, other, 256, 4096);
        assert(!memcmp(block, other, sizeof block));
    }
    assert(a.track[0].catches > 0);
    assert(!memcmp(a.track, b.track, sizeof a.track));
    puts("PASS: repeatable four-track generative audio and catches");
}
int main(void)
{
    audio();
    fishing();
    deterministic();
    printf("engine bytes: %zu; track bytes: %zu\n", sizeof(bw_engine), sizeof(bw_track));
    return 0;
}
