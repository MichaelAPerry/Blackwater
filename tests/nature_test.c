/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "bw_engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static bw_engine e;
static float maximum[BW_NATURE_COUNT];
void bw_midi_send(uint32_t p){(void)p;}
uint32_t bw_midi_room(void){return 64;}
void bw_on_rare(unsigned k){(void)k;}
static double run(unsigned ms)
{
    int32_t lr[512];double power=0;
    uint32_t frames=(uint32_t)((uint64_t)ms*BW_RATE/1000u);
    memset(maximum,0,sizeof maximum);
    for(uint32_t f=0;f<frames;)
    {
        unsigned n=frames-f>256 ? 256:frames-f;
        bw_render(&e,lr,n,4096);
        for(unsigned i=0;i<n*2;i++)
        {
            assert(lr[i]>-8388607 && lr[i]<8388607);
            power+=(double)lr[i]*lr[i];
        }
        for(unsigned i=0;i<BW_NATURE_COUNT;i++)
        {
            assert(isfinite(e.river.nature_level[i]));
            if(e.river.nature_level[i]>maximum[i]) maximum[i]=e.river.nature_level[i];
        }
        f+=n;
    }
    return sqrt(power/(frames*2u));
}
static void fresh(unsigned weather)
{
    bw_init(&e);e.song.weather=(uint8_t)weather;
    bw_river_enable(&e);bw_river_world(&e,0);
    /* Isolate the nature bed while exercising actual rendering and its scheduler. */
    for(unsigned k=0;k<4;k++) e.track[k].mute=1;
    for(unsigned i=0;i<6;i++) e.river.life_next[i]=9000000;
    bw_transport(&e,1);
}
int main(void)
{
    fresh(BW_WEATHER_DAY);
    assert(e.river.daylight==1 && !e.river.night);
    double day=run(5000);
    assert(day>5000 && maximum[BW_NATURE_CICADA]>0.01f);
    assert(maximum[BW_NATURE_CRICKET]<0.000001f);
    e.song.weather=BW_WEATHER_NIGHT;run(1000);
    assert(e.river.daylight>0.5f && e.river.night<0.5f); /* a fade, not a weather cut */
    run(49000);double night=run(5000);
    assert(e.river.night>0.99f && e.river.daylight<0.01f && night>5000);
    assert(maximum[BW_NATURE_CRICKET]>0.01f && maximum[BW_NATURE_CICADA]<0.001f);
    e.song.weather=BW_WEATHER_STORM;run(50000);run(5000);
    assert(e.river.storm>0.99f && maximum[BW_NATURE_CRICKET]<0.0001f && maximum[BW_NATURE_CICADA]<0.0001f);

    fresh(BW_WEATHER_DUSK);
    for(unsigned kind=BW_LIFE_METEOR;kind<BW_LIFE_COUNT;kind++)
    {
        e.river.life=(uint8_t)kind;e.river.life_at=e.river.ms;
        e.river.life_until=e.river.ms+6000;e.river.life_seed=123;
        run(2800);
        if(kind==BW_LIFE_JUMP) assert(e.river.splash_count==2 && maximum[BW_NATURE_SPLASH]>0.03f);
        if(kind==BW_LIFE_LEAF || kind==BW_LIFE_METEOR) assert(maximum[BW_NATURE_AIR]>0.05f);
        if(kind==BW_LIFE_BEE || kind==BW_LIFE_UFO) assert(maximum[BW_NATURE_BUZZ]>0.01f);
        if(kind==BW_LIFE_FIREFLY) assert(maximum[BW_NATURE_GLINT]>0.001f);
        bw_world_stop(&e);run(1000);
    }
    /* Approach quiets wildlife; FREE and pause leave no autonomous nature audio after the short fade. */
    e.river.phase=BW_RIVER_APPROACH;run(1000);
    assert(maximum[BW_NATURE_BUZZ]<0.00001f);
    bw_transport(&e,0);run(1000);assert(run(100)==0);
    fresh(BW_WEATHER_NIGHT);bw_performance_mode(&e,BW_MODE_FREE);bw_transport(&e,1);
    assert(run(5000)==0);

    /* AUTO retains the established three-minute day/dusk/night/storm schedule. */
    fresh(BW_WEATHER_AUTO);
    static const unsigned weather[4]={BW_WEATHER_DAY,BW_WEATHER_DUSK,BW_WEATHER_NIGHT,BW_WEATHER_STORM};
    for(unsigned i=0;i<4;i++) {e.clock=i*180u*BW_RATE;assert(bw_weather(&e)==weather[i]);}
    puts("nature: day/night insect audio, smooth lighting, all event sound sources, two splashes, pause/free silence passed");
    return 0;
}
