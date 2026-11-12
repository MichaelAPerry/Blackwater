/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Included by the engine. Actual FM attacks launch bounded water impulses; no audio or encounter RNG. */
void bw_flow_project(float depth,float *center,float *width,float *y)
{
    float d=fm_clampf(depth,0,1),perspective=d*d;
    *center=120+bw_sin(perspective)*(1-perspective)*11;
    *width=20+perspective*78;
    *y=88+perspective*119;
}

static void bw_flow_clear(bw_engine *e)
{
    for(unsigned i=0;i<BW_FLOW_WAVES;i++) e->river.waves[i].active=0;
    e->river.flow_kick=0;e->river.flow_speed=0.055f;
}

static void bw_flow_note(bw_engine *e,unsigned layer,unsigned note,unsigned velocity)
{
    if(!e->river.enabled || e->mode!=BW_MODE_RIVER || !e->playing ||
        !bw_track_audible(e,layer) || !e->track[layer].volume || !velocity) return;
    const bw_patch *p=&e->track[layer].patch;
    int carrier=0;
    for(unsigned i=0;i<BW_OPS;i++) carrier|=p->carrier[i] && p->op[i].level;
    if(!carrier) return;
    bw_river_state *r=&e->river;
    float strength=fm_clampf((float)velocity/127,0,1)*(0.3f+(float)e->track[layer].volume*0.007f);
    r->flow_notes++;
    r->flow_kick=fm_minf(1,r->flow_kick+strength*0.45f);
    bw_flow_wave *wave=0;
    /* A chord reinforces one crest rather than drawing several coincident full-width lines. */
    for(unsigned i=0;i<BW_FLOW_WAVES;i++)
        if(r->waves[i].active && r->waves[i].layer==layer && r->ms-r->waves[i].born<100u)
        {
            r->waves[i].strength=fm_minf(1,r->waves[i].strength+strength*0.35f);
            return;
        }
    for(unsigned i=0;i<BW_FLOW_WAVES;i++)
        if(!r->waves[i].active) {wave=&r->waves[i];break;}
    if(!wave)
    {
        wave=&r->waves[0];
        for(unsigned i=1;i<BW_FLOW_WAVES;i++)
            if(r->waves[i].depth>wave->depth) wave=&r->waves[i];
    }
    wave->active=1;wave->layer=(uint8_t)layer;wave->note=(uint8_t)note;
    wave->depth=0.10f;wave->born=r->ms;wave->strength=strength;
}

static void bw_flow_tick(bw_engine *e)
{
    bw_river_state *r=&e->river;
    float target=0.055f+r->reactive*0.025f+r->flow_kick*0.035f;
    r->flow_speed+=(target-r->flow_speed)*0.035f;
    r->flow_kick*=0.98f;
    float step=r->flow_speed*0.01f;
    r->flow_distance+=step;
    if(r->flow_distance>=1) r->flow_distance-=1;
    for(unsigned i=0;i<BW_FLOW_WAVES;i++)
        if(r->waves[i].active)
        {
            r->waves[i].depth+=step;
            if(r->waves[i].depth>=1.06f) r->waves[i].active=0;
        }
}
