/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "bw_engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static bw_engine e;
void bw_midi_send(uint32_t p){(void)p;}
uint32_t bw_midi_room(void){return 64;}
void bw_on_rare(unsigned k){(void)k;}
static void run(unsigned ms)
{
    int32_t lr[512];
    unsigned frames=(unsigned)((uint64_t)ms*BW_RATE/1000);
    for(unsigned f=0;f<frames;)
    {
        unsigned n=frames-f>256 ? 256:frames-f;
        bw_render(&e,lr,n,4096);
        for(unsigned i=0;i<n*2;i++) assert(lr[i]>-8388607 && lr[i]<8388607);
        f+=n;
    }
}
static unsigned crests(void)
{
    unsigned n=0;
    for(unsigned i=0;i<BW_FLOW_WAVES;i++)
    {
        assert(isfinite(e.river.waves[i].depth));
        n+=e.river.waves[i].active;
    }
    return n;
}
static void fresh(void)
{
    bw_init(&e);bw_river_enable(&e);bw_river_world(&e,0);
    for(unsigned k=0;k<4;k++) e.track[k].density=0;
    bw_keyboard_unlock(&e,1);bw_keyboard_layer(&e,2);bw_transport(&e,1);
}
int main(void)
{
    fresh();run(2000);
    assert(!crests() && !e.river.flow_notes && e.river.flow_distance>0); /* a quiet current, no phantom notes */
    bw_panel_key(&e,0,1,100);
    assert(crests()==1 && e.river.flow_notes==1);
    float depth=e.river.waves[0].depth,c0,w0,y0,c1,w1,y1;
    bw_flow_project(depth,&c0,&w0,&y0);
    run(2000);bw_flow_project(e.river.waves[0].depth,&c1,&w1,&y1);
    assert(e.river.waves[0].depth>depth && y1>y0+3 && w1>w0+2); /* downstream, towards the viewer */
    float speed=e.river.flow_speed;
    bw_panel_key(&e,7,1,127);run(200);
    assert(crests()==2 && e.river.flow_speed>speed); /* the new note also pushes the existing current */
    bw_panel_clear(&e);bw_transport(&e,0);
    bw_flow_wave paused[BW_FLOW_WAVES];memcpy(paused,e.river.waves,sizeof paused);
    float distance=e.river.flow_distance;run(500);
    assert(e.river.flow_distance==distance && !memcmp(paused,e.river.waves,sizeof paused));
    bw_transport(&e,1);run(20000);assert(!crests()); /* dissolves instead of wrapping back upstream */
    bw_panel_key(&e,1,1,100);assert(crests()==1);
    bw_performance_mode(&e,BW_MODE_FREE);assert(!crests());
    unsigned before=e.river.flow_notes;bw_panel_key(&e,2,1,100);run(1000);
    assert(e.river.flow_notes==before && !crests());

    fresh();e.track[2].mute=1;bw_panel_key(&e,0,1,100);
    assert(!crests() && !e.river.flow_notes);bw_panel_clear(&e);
    e.track[2].mute=0;e.track[2].volume=0;bw_panel_key(&e,0,1,100);assert(!crests());
    fresh();
    for(unsigned key=0;key<3;key++) bw_panel_key(&e,key,1,100);
    assert(e.river.flow_notes==3 && crests()==1); /* simultaneous chord reinforces one travelling crest */
    fresh();
    for(unsigned i=0;i<100;i++) {bw_live_note(&e,2,40+i%48,1,127);run(120);}
    assert(e.river.flow_notes==100 && crests()<=BW_FLOW_WAVES);
    puts("flow: note-driven crests move downstream/widen, accelerate current, pause, expire, coalesce and stay bounded; free/mute honoured");
    return 0;
}
