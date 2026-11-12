/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "bw_engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bw_engine e;
static unsigned rare_calls;
void bw_midi_send(uint32_t packet) { (void)packet; }
uint32_t bw_midi_room(void) { return 64; }
void bw_on_rare(unsigned k) { (void)k; rare_calls++; }

static double run(unsigned ms)
{
    int32_t lr[512];
    uint32_t frames = (uint32_t)((uint64_t)ms * BW_RATE / 1000u);
    double power = 0;
    for (uint32_t f = 0; f < frames;)
    {
        unsigned n = frames - f > 256u ? 256u : frames - f;
        bw_render(&e, lr, n, 4096);
        for (unsigned i = 0; i < n * 2; i++)
        {
            assert(lr[i] > -8388607 && lr[i] < 8388607); /* no clipping, not just a clamped bound */
            power += (double)lr[i] * lr[i];
        }
        f += n;
    }
    assert(isfinite(e.river.bloom) && isfinite(e.river.pressure));
    return sqrt(power / (frames * 2u));
}

static void fresh(unsigned world)
{
    bw_init(&e);
    bw_river_enable(&e);
    bw_river_world(&e, world);
    e.song.progression = 0;
    bw_transport(&e, 1);
}

int main(void)
{
    /* Three genuinely different firmware renders. All four layers sound with no line in the water. */
    double rms[3];
    for (unsigned w = 0; w < BW_WORLDS; w++)
    {
        fresh(w);
        for (unsigned k = 0; k < 4; k++)
            assert(bw_track_valid(&e.track[k]));
        rms[w] = run(24000);
        assert(rms[w] > 1000);
        for (unsigned k = 0; k < 4; k++)
            assert(e.hooked_state[k] == BW_FISH_IDLE);
    }
    assert(fabs(rms[0] - rms[1]) > 100 && fabs(rms[1] - rms[2]) > 100);
    printf("river worlds RMS: %.0f %.0f %.0f\n", rms[0], rms[1], rms[2]);
    /* The shared pool and output headroom also hold with live keys over sustained river/effect tails. */
    fresh(BW_WORLD_FIREFLIES);
    for (unsigned k = 0; k < 4; k++)
    {
        e.track[k].volume = 100;
        e.track[k].send_rev = e.track[k].send_dly = e.track[k].send_cho = 100;
        for (unsigned n = 0; n < 4; n++) bw_live_note(&e, k, 48 + n * 4, 1, 127);
    }
    run(5000);

    /* Encounter clock is independent of tempo and missing external MIDI pulses. No fast-forwarded clock. */
    for (unsigned tempo = 30; tempo <= 240; tempo += 210)
    {
        fresh(0);
        e.song.tempo = (uint16_t)tempo;
        e.song.sync = 1;
        e.track[0].wait_min = e.track[0].wait_max = 1;
        bw_cast(&e, 0);
        run(990);
        assert(e.river.phase != BW_RIVER_APPROACH);
        run(20);
        assert(e.river.phase == BW_RIVER_APPROACH);
        run(11990);
        assert(e.hooked_state[0] == BW_FISH_BITE);
        run(7990);
        assert(e.hooked_state[0] == BW_FISH_BITE);
        run(20);
        assert(e.river.phase == BW_RIVER_REVEAL && e.log.missed == 1);
        run(8000);
        assert(e.river.phase == BW_RIVER_RETURN);
        uint32_t at = e.river.phase_at;
        bw_cast(&e, 2); /* replacing the line cannot skip the global recovery */
        assert(e.river.phase_at == at && e.river.line == 0);
        run(30000);
        assert(e.river.phase == BW_RIVER_QUIET);
    }

    /* One line only, any species on it; rare hook invokes speech once. Deterministic fight fixture. */
    fresh(0);
    bw_cast(&e, 0);
    bw_cast(&e, 2);
    assert(e.hooked_state[0] == BW_FISH_IDLE && e.hooked_state[2] == BW_FISH_WAIT);
    unsigned line = 2;
    e.fish[line] = BW_FLATHEAD;
    e.fish_rare[line] = 1;
    e.fish_mm[line] = 700;
    e.hooked_state[line] = BW_FISH_BITE;
    e.bite_until[line] = e.clock + BW_RATE * 8;
    /* Pick a seed whose hook sticks; still exercises the public hook/strike/fight path. */
    e.rng = 1;
    unsigned before = rare_calls;
    assert(bw_hookset(&e, line));
    assert(rare_calls == before + 1 && e.river.phase == BW_RIVER_STRIKE);
    float distance = e.fight_dist[line];
    bw_river_reel(&e, 10);
    run(1590);
    assert(e.fight_dist[line] == distance && e.river.phase == BW_RIVER_STRIKE);
    run(20);
    assert(e.river.phase == BW_RIVER_FIGHT);
    /* Fight progresses with render time, even with no UI/input frame calls. */
    run(500);
    assert(e.fight_dist[line] != distance);
    uint32_t paused = e.river.ms;
    bw_transport(&e, 0);
    run(1000);
    assert(e.river.ms == paused);
    bw_transport(&e, 1);
    e.fight_dist[line] = 1;
    e.fight_tension[line] = 20;
    bw_river_reel(&e, 2);
    run(10);
    assert(e.river.phase == BW_RIVER_REVEAL && e.log.s[BW_FLATHEAD].count == 1);
    assert(e.river.memory == BW_FLATHEAD + 1 && e.event.species == BW_FLATHEAD);

    /* A full uncompressed ten-minute listening session: no overlap, repeatable, finite and unclipped. */
    fresh(BW_WORLD_UNDERTOW);
    e.track[0].manual = 0;
    e.track[0].wait_min = e.track[0].wait_max = 100;
    bw_cast(&e, 0);
    for (unsigned second = 0; second < 600; second++)
    {
        run(1000);
        unsigned active = 0;
        for (unsigned k = 0; k < 4; k++)
            active += e.hooked_state[k] != BW_FISH_IDLE;
        assert(active == 1);
    }
    unsigned total = 0;
    for (unsigned sp = 0; sp < BW_SPECIES; sp++) total += e.log.s[sp].count;
    assert(total >= 3 && total <= 4);
    printf("river: all passed; %u encounters in ten minutes\n", total);
    return 0;
}
