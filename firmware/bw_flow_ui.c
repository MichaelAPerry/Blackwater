/* SPDX-License-Identifier: GPL-3.0-or-later */
/* One frame of projected travelling crests, reused across the five existing LCD bands. */
#define BW_FLOW_POINTS 25
typedef struct
{
    uint16_t x[BW_FLOW_POINTS],y[BW_FLOW_POINTS],color[BW_FLOW_POINTS];
    unsigned ink,tails;
    float center,base_y,width,tail_center[2],tail_y[2],tail_scale[2];
} bw_flow_strip;
static bw_flow_strip bw_flow_frame[BW_FLOW_WAVES+6];
static unsigned bw_flow_strips;

static void bw_vector_flow_strip(float depth,unsigned layer,unsigned note,float strength,float opacity,
                                float age,int musical,float bloom)
{
    if(depth<=0 || depth>=1 || bw_flow_strips>=BW_FLOW_WAVES+6) return;
    float fade=bw_vector_ease(depth/0.07f)*(1-bw_vector_ease((depth-0.83f)/0.17f));
    if(musical) fade*=bw_vector_ease(age/160);
    unsigned ink=(unsigned)(fm_clampf(opacity*fade,0,1)*256);
    if(!ink) return;
    bw_flow_strip *strip=&bw_flow_frame[bw_flow_strips++];
    float center,width,y;
    bw_flow_project(depth,&center,&width,&y);
    strip->center=center;strip->base_y=y;strip->width=width;
    strip->ink=ink;strip->tails=0;
    float pitch=(float)(note%12u)/12;
    float span=musical ? 0.89f-fm_clampf(((float)note-48)/48,0,1)*0.25f:0.94f;
    float bias=musical ? ((float)(note%7u)-3)*0.025f:0;
    for(unsigned i=0;i<BW_FLOW_POINTS;i++)
    {
        float across=(float)i*2/(BW_FLOW_POINTS-1)-1;
        float envelope=1-across*across;
        float bend=fm_sinf(across*4.2f+pitch*FM_TWO_PI+depth*0.9f);
        float fold=fm_sinf(across*7.0f-pitch*FM_TWO_PI);
        float amplitude=depth*(1+bloom*9)*(0.5f+strength*0.5f);
        float xx=center+width*(across*span+bias);
        float yy=y+envelope*amplitude*(bend+bloom*fold*0.4f);
        float saturation=(musical ? 0.68f:0.2f)+bloom*0.45f+bw.river.reactive*0.2f;
        if(bw.river.phase>=BW_RIVER_STRIKE) saturation*=0.45f;
        uint16_t color=bw_vector_music_color(layer,(float)i/(BW_FLOW_POINTS-1)+pitch*0.24f,saturation);
        strip->x[i]=(uint16_t)(fm_clampf(xx,0,239)*256);
        strip->y[i]=(uint16_t)(fm_clampf(yy,0,239)*256);
        strip->color[i]=musical ? color:bw_vector_mix(C_MUTE,color,180);
    }
    if(musical)
        for(unsigned tail=0;tail<2;tail++)
        {
            float behind=depth-0.035f*(float)(tail+1);
            if(behind<0.1f) break; /* a wake cannot predate the note */
            float tail_width;
            bw_flow_project(behind,&strip->tail_center[tail],&tail_width,&strip->tail_y[tail]);
            strip->tail_scale[tail]=tail_width/width;
            strip->tails++;
        }
}

static void bw_vector_flow(float bloom,unsigned scene_ink)
{
    const bw_river_state *r=&bw.river;
    if(!bw_oy)
    {
        bw_flow_strips=0;
        /* Faint current keeps moving between notes. No water row is anchored to a screen position. */
        for(unsigned j=0;j<6;j++)
        {
            float depth=r->flow_distance+(float)j/6;
            if(depth>=1) depth-=1;
            float opacity=j%2u==0 ? 0.20f+bloom*0.2f:bloom*0.45f;
            bw_vector_flow_strip(depth,j%4u,48+j*5,0.3f,opacity,1000,0,bloom);
        }
        for(unsigned j=0;j<BW_FLOW_WAVES;j++)
        {
            const bw_flow_wave *wave=&r->waves[j];
            if(wave->active)
                bw_vector_flow_strip(wave->depth,wave->layer,wave->note,wave->strength,
                    0.30f+wave->strength*0.7f,(float)(r->ms-wave->born),1,bloom);
        }
    }
    for(unsigned j=0;j<bw_flow_strips;j++)
    {
        const bw_flow_strip *strip=&bw_flow_frame[j];
        bw_vector_opacity=scene_ink*strip->ink/256u;
        for(unsigned i=1;i<BW_FLOW_POINTS;i++)
            bw_vector_stroke(strip->x[i-1],strip->y[i-1],strip->x[i],strip->y[i],strip->color[i]);
        for(unsigned tail=0;tail<strip->tails;tail++)
        {
            bw_vector_opacity=(unsigned)((float)scene_ink*(float)strip->ink/256*
                (0.12f+bloom*0.32f)/(float)(tail+1));
            int px=0,py=0;
            for(unsigned i=0;i<BW_FLOW_POINTS;i++)
            {
                int x=(int)((strip->tail_center[tail]+((float)strip->x[i]/256-strip->center)*strip->tail_scale[tail])*256);
                int y=(int)((strip->tail_y[tail]+((float)strip->y[i]/256-strip->base_y)*strip->tail_scale[tail])*256);
                if(i) bw_vector_stroke(px,py,x,y,strip->color[i]);
                px=x;py=y;
            }
        }
    }
    bw_vector_opacity=scene_ink;
}
