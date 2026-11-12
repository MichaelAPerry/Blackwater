/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Included by bw_engine.c. Wildlife has its own random stream and never changes encounter odds/timing. */
static void bw_life_init(bw_engine *e)
{
    static const uint32_t first[6]={18000,900000,37000,9000,53000,27000};
    bw_river_state *r=&e->river;
    r->life_rng=0x73746172u;
    r->motif_voice=r->motif_note=255;
    for(unsigned i=0;i<6;i++)
        r->life_next[i]=r->ms+first[i]+bw_rand(&r->life_rng)%(i==1 ? 300001u:5001u);
}

static void bw_life_note_off(bw_engine *e)
{
    bw_river_state *r=&e->river;
    if(r->motif_note!=255)
    {
        bw_midi_note(e,r->motif_layer,r->motif_note,0,0);
        r->motif_note=255;
    }
    if(r->motif_voice<BW_VOICES)
    {
        bw_voice *v=&e->voice[r->motif_voice];
        if(v->active && v->world && v->age==r->motif_token) bw_voice_off(e,v);
    }
}

void bw_world_stop(bw_engine *e)
{
    bw_life_note_off(e);
    e->river.life=BW_LIFE_NONE;
    e->river.motif_left=0;
    e->river.motif_voice=255;
}

static void bw_life_motif_note(bw_engine *e)
{
    bw_river_state *r=&e->river;
    bw_life_note_off(e);
    unsigned layer=r->motif_layer;
    if(!bw_track_audible(e,layer) || !e->track[layer].volume) return;
    if(bw_keyboard_full(e) && layer==e->live_track) return;
    bw_voice *voice=0;
    if(r->motif_voice<BW_VOICES)
    {
        bw_voice *candidate=&e->voice[r->motif_voice];
        if(candidate->world && candidate->age==r->motif_token && !candidate->held) voice=candidate;
    }
    if(!voice)
        for(int i=BW_VOICES-1;i>=0;i--)
            if(!e->voice[i].active) {voice=&e->voice[i];break;}
    if(!voice) return; /* keyboard performance always wins over a world ornament */
    unsigned note=(unsigned)bw_note_of(e,layer,r->motif[r->motif_pos%4u]+7,1);
    bw_voice_on(e,voice,layer,note,82,1);
    voice->live=0;voice->held=0;voice->world=1;
    r->motif_voice=(uint8_t)(voice-e->voice);
    r->motif_token=voice->age;r->motif_note=(uint8_t)note;r->motif_off=r->ms+350u;
    r->motif_notes++;
    bw_midi_note(e,layer,note,82,1);
}

static void bw_life_tick(bw_engine *e)
{
    static const uint32_t duration[6]={6000,12000,2600,10000,9000,14000};
    static const uint32_t interval[6]={80000,1200000,45000,35000,80000,60000};
    bw_river_state *r=&e->river;
    if(e->mode==BW_MODE_FREE || r->phase>=BW_RIVER_APPROACH)
    {
        if(r->life || r->motif_left) bw_world_stop(e);
        return;
    }
    if(r->motif_note!=255 && (int32_t)(r->ms-r->motif_off)>=0) bw_life_note_off(e);
    if(r->motif_left && (int32_t)(r->ms-r->motif_at)>=0)
    {
        bw_life_motif_note(e);
        r->motif_pos++;r->motif_left--;r->motif_at=r->ms+650u;
    }
    if(r->life && (int32_t)(r->ms-r->life_until)>=0)
    {
        r->life=0;r->life_gap=r->ms+8000u;
    }
    if(r->life || (int32_t)(r->ms-r->life_gap)<0) return;
    unsigned weather=bw_weather(e);
    for(unsigned i=0;i<6;i++)
    {
        if((int32_t)(r->ms-r->life_next[i])<0) continue;
        if((i==4 && (weather==BW_WEATHER_NIGHT || weather==BW_WEATHER_STORM)) ||
           (i==5 && weather==BW_WEATHER_DAY && r->world!=BW_WORLD_FIREFLIES)) continue;
        r->life=(uint8_t)(i+1);r->life_at=r->ms;r->life_until=r->ms+duration[i];
        r->life_seed=bw_rand(&r->life_rng);r->life_count[i+1]++;
        r->life_next[i]=r->life_until+interval[i]+bw_rand(&r->life_rng)%(i==1 ? 900001u:60001u);
        if(i==0)
        {
            r->motif_layer=(uint8_t)(bw_keyboard_full(e) && e->live_track==3 ? 2:3);
            for(unsigned n=0;n<4;n++) r->motif[n]=(int8_t)(bw_rand(&r->life_rng)%7u);
            r->motif_pos=0;r->motif_left=8;r->motif_at=r->ms;
        }
        break;
    }
}
