/* SPDX-License-Identifier: GPL-3.0-or-later */
/* BLACKWATER's engine on the host, under UBSan: sound, music, fishing, performance, effects and MIDI. */
#include "bw_engine.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static int fails;
#define CHECK(c)                                                                                                     \
    do                                                                                                               \
    {                                                                                                                \
        if (!(c))                                                                                                    \
        {                                                                                                            \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c);                                                       \
            fails++;                                                                                                 \
        }                                                                                                            \
    } while (0)
static uint32_t midi_log[4096], midi_n, clocks, rare_hooks;
void bw_midi_send(uint32_t pkt)
{
    if (((pkt >> 8) & 255) == 0xF8)
        clocks++;
    if (midi_n < 4096)
        midi_log[midi_n++] = pkt;
}
uint32_t bw_midi_room(void)
{
    return 64;
}
void bw_on_rare(unsigned k)
{
    (void)k;
    rare_hooks++;
}
static bw_engine e;
static int32_t lr[2 * 256];
static double render(unsigned blocks, double *peak)
{
    double sum = 0, pk = 0;
    for (unsigned b = 0; b < blocks; b++)
    {
        bw_render(&e, lr, 256, 4096);
        for (unsigned i = 0; i < 512; i++)
        {
            double v = lr[i];
            sum += v * v;
            if (fabs(v) > pk)
                pk = fabs(v);
            CHECK(lr[i] <= 8388607 && lr[i] >= -8388608);
        }
    }
    if (peak)
        *peak = pk;
    return sqrt(sum / (blocks * 512.0));
}
static unsigned notes_on(unsigned ch)
{
    unsigned n = 0;
    for (unsigned i = 0; i < midi_n; i++)
        if (((midi_log[i] >> 8) & 0xF0) == 0x90 && ((midi_log[i] >> 8) & 15) == ch && (midi_log[i] >> 24))
            n++;
    return n;
}
static void fresh(void)
{
    bw_init(&e);
    for (unsigned k = 0; k < 4; k++)
    {
        e.track[k].send_rev = e.track[k].send_dly = e.track[k].send_cho = 0;
        e.track[k].manual = 0; /* AUTO unless a test asks for TIMED (the firmware's default is TIMED) */
    }
    midi_n = clocks = rare_hooks = 0;
}
int main(void)
{
    unsigned k, i;
    double pk;
    printf("engine bytes: %u; track bytes: %u; song bytes: %u\n", (unsigned)sizeof(bw_engine),
           (unsigned)sizeof(bw_track), (unsigned)sizeof(bw_song));
    /* defaults are valid and every fish has a theme */
    fresh();
    CHECK(bw_song_valid(&e.song));
    for (k = 0; k < 4; k++)
    {
        unsigned n = 0;
        CHECK(bw_track_valid(&e.track[k]));
        for (i = 0; i < e.track[k].cur.len; i++)
            n += e.track[k].cur.s[i].gate == BW_GATE_NOTE;
        CHECK(n >= 2);
    }
    /* every factory patch sounds, stays bounded, and releases to silence */
    for (k = 0; k < 4; k++)
        for (unsigned slot = 0; slot < 4; slot++)
        {
            fresh();
            bw_patch_factory(&e.track[k].patch, k, slot);
            CHECK(bw_track_valid(&e.track[k]));
            e.live_track = (uint8_t)k;
            bw_live_note(&e, k, 60, 1, 110);
            double on = render(40, &pk);
            bw_live_note(&e, k, 60, 0, 0);
            render(1400, 0); /* 8 s: the longest release */
            double off = render(10, 0);
            if (!(on > 2000 && off < 50))
                printf("  patch %u/%u %s: on %.0f off %.1f\n", k, slot, bw_patch_name(k, slot), on, off);
            CHECK(on > 2000);
            CHECK(off < 50);
        }
    /* the scale: A minor from the bass's register; degree 2 is a minor third up, 7 an octave */
    fresh();
    CHECK(bw_note_of(&e, BW_BASS, 0, 0) == 36 + 9);
    CHECK(bw_note_of(&e, BW_BASS, 2, 0) == 36 + 9 + 3);
    CHECK(bw_note_of(&e, BW_BASS, 7, 0) == 36 + 9 + 12);
    CHECK(bw_note_of(&e, BW_BASS, -1, 0) == 36 + 9 - 2);
    e.song.scale = 6; /* minor pentatonic: five a cycle */
    CHECK(bw_note_of(&e, BW_BASS, 5, 0) == 36 + 9 + 12);
    /* playing: all four fish play, on their MIDI channels, with the clock out at 24 a beat */
    fresh();
    e.song.tempo = 120;
    bw_transport(&e, 1);
    CHECK(render(172, 0) == 0.0); /* no line in the water: silence */
    bw_transport(&e, 0);
    midi_n = clocks = 0;
    for (k = 0; k < 4; k++)
    {
        e.track[k].wait_min = e.track[k].wait_max = 600; /* (no catches meanwhile) */
        bw_cast(&e, k);
    }
    bw_transport(&e, 1);
    double rms = render(172 * 4, &pk); /* 4 s */
    printf("four fish playing: RMS %.0f, peak %.0f (24-bit)\n", rms, pk);
    CHECK(rms > 20000);
    for (k = 0; k < 4; k++)
        CHECK(notes_on(k) >= 2);
    CHECK(notes_on(0) > notes_on(3)); /* the lead busier than the drone */
    printf("clock out: %u in 4 s at 120 BPM\n", (unsigned)clocks);
    CHECK(clocks >= 190 && clocks <= 194);
    bw_transport(&e, 0);
    render(400, 0);
    /* fishing: zero waits, automatic, every fish: sizes in range, weights grow with length, rare ones turn up */
    fresh();
    unsigned rare = 0, common = 0;
    for (k = 0; k < 4; k++)
    {
        e.track[k].wait_min = e.track[k].wait_max = 0;
        bw_cast(&e, k);
    }
    bw_transport(&e, 1);
    uint32_t seq = e.event.seq;
    for (unsigned b = 0; b < 172 * 60 && e.event.seq - seq < 2000; b++)
    {
        uint32_t before = e.event.seq;
        bw_render(&e, lr, 256, 4096);
        if (e.event.seq != before)
        {
            const bw_event *ev = &e.event;
            unsigned sp = ev->species;
            CHECK(sp % 4 == ev->track);
            CHECK(ev->result == BW_FIGHT_LANDED);
            CHECK(ev->mm >= 90 && ev->mm <= 1600);
            CHECK(ev->g > 0);
        }
    }
    for (unsigned sp = 0; sp < BW_SPECIES; sp++)
        *(sp >= 4 ? &rare : &common) += e.log.s[sp].count;
    printf("automatic catches: %u common, %u rare, %u clip triggers\n", common, rare, (unsigned)rare_hooks);
    CHECK(common > 100 && rare > 2 && rare < common / 3);
    CHECK(rare_hooks == rare);
    for (unsigned sp = 0; sp < BW_SPECIES; sp++)
        if (e.log.s[sp].count)
        {
            CHECK(e.log.s[sp].best_mm >= e.log.s[sp].last_mm);
            CHECK(bw_weight_g(sp, 600) > bw_weight_g(sp, 300));
        }
    CHECK(bw_weight_g(BW_FLATHEAD, 1000) > 8000 && bw_weight_g(BW_FLATHEAD, 1000) < 20000);
    CHECK(bw_weight_g(BW_BLUEGILL, 200) > 80 && bw_weight_g(BW_BLUEGILL, 200) < 250);
    for (k = 0; k < 4; k++)
        CHECK(bw_track_valid(&e.track[k]));
    /* a catch changes the theme; release undoes it; home restores the start */
    fresh();
    bw_theme home = e.track[0].cur;
    e.track[0].wait_min = e.track[0].wait_max = 0;
    bw_cast(&e, 0);
    bw_transport(&e, 1);
    int changed = 0;
    for (unsigned n = 0; n < 40 && !changed; n++)
    {
        seq = e.event.seq;
        while (e.event.seq == seq)
            bw_render(&e, lr, 256, 4096);
        changed = memcmp(&e.track[0].cur, &home, sizeof home) != 0;
    }
    CHECK(changed);
    bw_theme now = e.track[0].cur;
    bw_release(&e, 0);
    CHECK(memcmp(&e.track[0].cur, &now, sizeof now) != 0);
    CHECK(e.log.released == 1);
    bw_restore_home(&e, 0);
    CHECK(memcmp(&e.track[0].cur, &e.track[0].home, sizeof home) == 0);
    /* personal bests: the key moves up a fourth */
    fresh();
    e.track[2].wait_min = e.track[2].wait_max = 0;
    bw_cast(&e, 2);
    bw_transport(&e, 1);
    unsigned key_changes = 0;
    uint8_t key = e.song.key;
    for (unsigned b = 0; b < 172 * 30; b++)
    {
        seq = e.event.seq;
        bw_render(&e, lr, 256, 4096);
        if (e.event.seq != seq && e.event.key_change)
        {
            key_changes++;
            CHECK(e.song.key == (key + 5) % 12);
            key = e.song.key;
        }
    }
    printf("personal-best key changes in 30 s: %u\n", key_changes);
    CHECK(key_changes >= 1);
    /* the fight: patient reeling lands fish; cranking through runs snaps lines; doing nothing throws the hook */
    unsigned landed = 0, snapped = 0, thrown = 0, spat = 0, strikes = 0;
    for (unsigned trial = 0; trial < 60; trial++)
    {
        fresh();
        e.rng = 0x1234u + trial * 977u;
        e.track[1].manual = 1;
        e.track[1].wait_min = e.track[1].wait_max = 0;
        bw_cast(&e, 1);
        bw_transport(&e, 1);
        int set = 0;
        for (unsigned tries = 0; tries < 50 && !set; tries++) /* a strike may not stick: wait for the next */
        {
            while (bw_fish_state(&e, 1) != BW_FISH_BITE)
                bw_render(&e, lr, 256, 4096);
            set = bw_hookset(&e, 1);
            spat += !set;
            strikes++;
        }
        CHECK(set);
        CHECK(bw_fish_state(&e, 1) == BW_FISH_FIGHT);
        int r = BW_FIGHT_ON, style = trial % 3;
        for (unsigned t = 0; t < 2000 && r == BW_FIGHT_ON; t++)
        {
            int reel = style == 0 ? (e.fight_run[1] > 0 || e.fight_tension[1] > 70 ? 0 : 1)
                       : style == 1 ? 4
                                    : 0;
            r = bw_fight(&e, 1, reel, 50);
        }
        CHECK(r != BW_FIGHT_ON);
        if (style == 0)
            landed += r == BW_FIGHT_LANDED;
        if (style == 1)
            snapped += r == BW_FIGHT_SNAPPED;
        if (style == 2)
            thrown += r == BW_FIGHT_THROWN;
    }
    printf("fight results (landed/snapped/thrown by style) in the line below\n");
    printf("fights: patient %u/20 landed, cranking %u/20 snapped, idle %u/20 thrown\n", landed, snapped, thrown);
    printf("strikes: %u of %u spat the hook\n", spat, strikes);
    CHECK(spat * 6 >= strikes && spat * 2 <= strikes); /* some get away at the strike, most stick */
    CHECK(landed >= 12 && snapped >= 14 && thrown >= 15); /* (shorter fights: a small fish can be hauled in) */
    /* a missed bite: the window closes */
    fresh();
    e.track[0].manual = 1;
    e.track[0].wait_min = e.track[0].wait_max = 0;
    bw_cast(&e, 0);
    bw_transport(&e, 1);
    render(172 * 5, 0); /* past the three-second window */
    CHECK(e.log.missed >= 1);
    /* live keys, macros, the arpeggiator */
    fresh();
    e.live_track = 0;
    bw_live_key(&e, 7, 1, 100);
    unsigned act = 0;
    for (i = 0; i < BW_VOICES; i++)
        act += e.voice[i].active;
    CHECK(act == 1);
    double plain = render(20, 0);
    bw_macro(&e, 0, BW_M_BRIGHT, 1);
    render(1, 0);
    double bright = render(20, 0);
    bw_macro(&e, 0, BW_M_BRIGHT, 0);
    CHECK(fabs(bright - plain) > plain * 0.01);
    bw_live_key(&e, 7, 0, 0);
    render(600, 0);
    e.arp = BW_ARP_ON;
    midi_n = 0;
    bw_live_key(&e, 0, 1, 100);
    bw_live_key(&e, 2, 1, 100);
    bw_live_key(&e, 4, 1, 100);
    render(172, 0);
    printf("arp: %u notes in 1 s at 1/16, 96 BPM\n", notes_on(0));
    CHECK(notes_on(0) >= 5);
    bw_live_key(&e, 0, 0, 0);
    bw_live_key(&e, 2, 0, 0);
    bw_live_key(&e, 4, 0, 0);
    midi_n = 0;
    render(172, 0);
    CHECK(notes_on(0) <= 1); /* ON stops with the keys */
    e.arp = BW_ARP_LATCH;
    bw_live_key(&e, 3, 1, 100);
    bw_live_key(&e, 3, 0, 0);
    midi_n = 0;
    render(172, 0);
    CHECK(notes_on(0) >= 5); /* LATCH goes on */
    e.arp = BW_ARP_OFF;
    /* the LFO: pitch modulation changes the sound */
    fresh();
    e.track[2].lfo_depth = 0;
    bw_live_note(&e, 2, 64, 1, 100);
    render(10, 0);
    int32_t a[64];
    for (i = 0; i < 64; i++)
        a[i] = 0;
    bw_render(&e, lr, 256, 4096);
    for (i = 0; i < 64; i++)
        a[i] = lr[i * 8];
    fresh();
    e.track[2].lfo_depth = 100;
    e.track[2].lfo_rate = 100;
    bw_live_note(&e, 2, 64, 1, 100);
    render(10, 0);
    bw_render(&e, lr, 256, 4096);
    int differ = 0;
    for (i = 0; i < 64; i++)
        differ |= a[i] != lr[i * 8];
    CHECK(differ);
    /* effects: a reverb and echo tail after the note */
    fresh();
    e.track[0].send_rev = 100;
    e.track[0].send_dly = 100;
    bw_live_note(&e, 0, 72, 1, 120);
    render(30, 0);
    bw_live_note(&e, 0, 72, 0, 0);
    render(60, 0);
    double tail = render(40, 0);
    fresh();
    bw_live_note(&e, 0, 72, 1, 120);
    render(30, 0);
    bw_live_note(&e, 0, 72, 0, 0);
    render(60, 0);
    double dry = render(40, 0);
    printf("tail RMS with effects %.0f, without %.0f\n", tail, dry);
    CHECK(tail > dry * 3 + 100);
    /* MIDI clock in: external sync moves the sequencer only with clock pulses */
    fresh();
    e.song.sync = 1;
    bw_midi_clock(&e, 0xFA);
    CHECK(e.playing);
    render(172, 0);
    unsigned without = (unsigned)e.ticks;
    for (i = 0; i < 24; i++)
    {
        bw_midi_clock(&e, 0xF8);
        bw_render(&e, lr, 256, 4096);
    }
    CHECK(without <= 1);
    CHECK(e.ticks >= 96 && e.ticks <= 98);
    bw_midi_clock(&e, 0xFC);
    CHECK(!e.playing);
    /* everything at once stays inside 24 bits */
    fresh();
    for (k = 0; k < 4; k++)
    {
        e.track[k].volume = 100;
        e.track[k].drive = 100;
        e.track[k].send_rev = e.track[k].send_dly = e.track[k].send_cho = 100;
        e.macros[k] = (1u << BW_M_BRIGHT) | (1u << BW_M_GROWL) | (1u << BW_M_THROW);
    }
    bw_transport(&e, 1);
    for (i = 0; i < 16; i++)
        bw_live_key(&e, i, 1, 127);
    render(172 * 3, &pk);
    printf("everything at full: peak %.0f\n", pk);
    /* determinism */
    fresh();
    bw_cast(&e, 0);
    bw_cast(&e, 3);
    bw_transport(&e, 1);
    render(300, 0);
    int32_t first = lr[100];
    fresh();
    bw_cast(&e, 0);
    bw_cast(&e, 3);
    bw_transport(&e, 1);
    render(300, 0);
    CHECK(first == lr[100]);
    /* Editing the FM ratio / fixed Hz updates a held note without retriggering its envelope. */
    fresh();
    bw_live_note(&e,0,60,1,110);
    render(1,0);
    bw_voice *held=0;
    for(i=0;i<BW_VOICES;i++) if(e.voice[i].active) {held=&e.voice[i];break;}
    CHECK(held != 0);
    if(held)
    {
        float increment=held->inc[0];
        uint32_t age=held->age;
        e.track[0].patch.op[0].ratio*=2;
        render(1,0);
        CHECK(fabsf(held->inc[0]/increment-2.0f)<0.001f);
        CHECK(held->age==age);
        e.track[0].patch.op[0].fixed=1;
        e.track[0].patch.op[0].hz=320;
        render(1,0);
        CHECK(fabsf(held->inc[0]-320.0f/BW_RATE)<0.000001f);
        CHECK(held->age==age);
    }
    printf(fails ? "engine: %d FAILED\n" : "engine: all passed\n", fails);
    return fails != 0;
}
