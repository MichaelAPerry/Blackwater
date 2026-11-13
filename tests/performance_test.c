/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "bw_engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static bw_engine e;
static uint32_t chromatic_seen;
static uint8_t midi_held[4][128];
void bw_midi_send(uint32_t p)
{
    unsigned status=p>>8&0xF0u,channel=p>>8&15u,note=p>>16&127u;
    if(channel<4 && (status==0x90u || status==0x80u))
        midi_held[channel][note]=(uint8_t)(status==0x90u && (p>>24&127u));
    if(((p>>8)&0xF0u)==0x90u && (p>>24&127u))
    {
        unsigned note=p>>16&127u;
        if(note>=53 && note<=79) chromatic_seen|=1u<<(note-53u);
    }
}
uint32_t bw_midi_room(void){return 64;}
void bw_on_rare(unsigned k){(void)k;}
static double run(unsigned ms)
{
    int32_t lr[512];double power=0;
    uint32_t total=(uint32_t)((uint64_t)ms*BW_RATE/1000u);
    for(uint32_t i=0;i<total;)
    {
        unsigned n=total-i>256u ? 256u:total-i;
        bw_render(&e,lr,n,4096);
        for(unsigned j=0;j<n*2;j++)
        {
            assert(lr[j]>-8388607 && lr[j]<8388607);
            power+=(double)lr[j]*lr[j];
        }
        i+=n;
    }
    return sqrt(power/(total*2u));
}
static void fresh(void)
{
    bw_init(&e);bw_river_enable(&e);bw_river_world(&e,0);chromatic_seen=0;
    memset(midi_held,0,sizeof midi_held);
}
static unsigned held(void)
{
    unsigned n=0;for(unsigned i=0;i<BW_VOICES;i++) n+=e.voice[i].active && e.voice[i].live && e.voice[i].held;
    return n;
}
int main(void)
{
    fresh();
    bw_panel_key(&e,1,1,100);assert(e.macros[0]&1u);
    bw_keyboard_layer(&e,2);bw_panel_key(&e,1,0,0);assert(!e.macros[0]);
    bw_keyboard_unlock(&e,1);
    for(unsigned key=0;key<BW_KEYS;key++)
    {
        bw_panel_key(&e,key,1,100);
        assert(e.key_note[key]==53u+key && held()==1 && !e.macros[2]);
        bw_panel_key(&e,key,0,0);assert(held()==0);
    }
    bw_panel_key(&e,0,1,100);bw_keyboard_layer(&e,3);e.live_oct=1;
    bw_panel_key(&e,0,0,0);assert(held()==0); /* releases its original pitch/layer */
    for(unsigned key=0;key<BW_KEYS;key++) bw_panel_key(&e,key,1,100);
    assert(held()==8);
    bw_keyboard_unlock(&e,0);assert(held()==0); /* map changes cannot orphan any keys */
    for(unsigned layer=0;layer<4;layer++)
        for(unsigned note=0;note<128;note++) assert(!midi_held[layer][note]);

    fresh();bw_performance_mode(&e,BW_MODE_FREE);
    assert(bw_keyboard_full(&e));bw_transport(&e,1);
    assert(run(5000)==0); /* no autonomous river/world/fishing audio */
    assert(e.river.ms==0 && !e.river.life && !e.river.motif_notes);
    bw_keyboard_layer(&e,2);bw_panel_key(&e,1,1,100);
    assert(e.key_note[1]==54 && held()==1 && run(2000)>1000);
    bw_panel_key(&e,1,0,0);assert(!held());
    bw_panel_clear(&e);e.live_oct=0;e.arp=BW_ARP_ON;
    for(unsigned key=0;key<BW_KEYS;key++) bw_panel_key(&e,key,1,100);
    chromatic_seen=0;run(5000);
    assert(chromatic_seen==((1u<<BW_KEYS)-1u)); /* arpeggiator includes black keys too */
    bw_panel_clear(&e);assert(!held());

    fresh();bw_keyboard_unlock(&e,1);bw_keyboard_layer(&e,1);
    bw_cast(&e,0);bw_transport(&e,1);bw_panel_key(&e,0,1,100);run(20000);
    assert(held()==1 && !e.sounding[1]); /* one layer is reserved for the unlocked performer */
    bw_panel_key(&e,0,0,0);assert(!held());
    for(unsigned key=0;key<8;key++) bw_panel_key(&e,key,1,100);
    run(15000);assert(held()==8); /* background notes never steal held performance */
    bw_panel_clear(&e);

    fresh();bw_transport(&e,1);run(45000);
    assert(e.river.life_count[BW_LIFE_METEOR]>=1 && e.river.motif_notes>=4);
    for(unsigned i=0;i<4;i++) assert(e.river.motif[i]>=0 && e.river.motif[i]<7);
    assert(e.river.life_count[BW_LIFE_UFO]==0 && e.river.life_next[BW_LIFE_UFO-1]>=900000);
    /* The rare event's scheduler, geometry lifetime and long recurrence are exercised without a 20-minute wait. */
    bw_world_stop(&e);e.river.life_gap=0;
    for(unsigned i=0;i<6;i++) e.river.life_next[i]=e.river.ms+9000000;
    e.river.life_next[BW_LIFE_UFO-1]=e.river.ms;run(20);
    assert(e.river.life==BW_LIFE_UFO && e.river.life_until-e.river.life_at==12000);
    assert(e.river.life_next[BW_LIFE_UFO-1]-e.river.ms>=1200000);
    bw_performance_mode(&e,BW_MODE_FREE);assert(!e.river.life && !e.river.motif_left);

    /* Independent wildlife RNG/motifs must not change which fish arrives or when. */
    bw_log original_log;uint32_t original_rng;
    for(unsigned pass=0;pass<2;pass++)
    {
        fresh();e.track[0].manual=0;e.track[0].wait_min=e.track[0].wait_max=100;
        if(pass) for(unsigned i=0;i<6;i++) e.river.life_next[i]=9000000;
        bw_cast(&e,0);bw_transport(&e,1);run(300000);
        if(!pass){original_log=e.log;original_rng=e.rng;}
        else {assert(e.rng==original_rng);assert(memcmp(&e.log,&original_log,sizeof e.log)==0);}
    }
    puts("performance: all 27 keys, ownership, full-key arp, free play, meteor motif, rarity and fish independence passed");
    return 0;
}
