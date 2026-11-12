/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Small procedural sound sources alongside the existing water texture. No FM voice stealing,
 * allocations, samples or encounter RNG. Actor age drives both the sound and drawing. */
static float bw_nature_window(float age,float length)
{
    float a=fm_clampf(age/180.0f,0,1),b=fm_clampf((length-age)/450.0f,0,1);
    return a*b;
}

static void bw_nature_control(bw_engine *e)
{
    bw_river_state *r=&e->river;
    unsigned weather=bw_weather(e);
    r->daylight+=((float)(weather==BW_WEATHER_DAY)-r->daylight)*0.0012f;
    r->night+=((float)(weather==BW_WEATHER_NIGHT)-r->night)*0.0012f;
    r->storm+=((float)(weather==BW_WEATHER_STORM)-r->storm)*0.0012f;
    for(unsigned i=0;i<BW_NATURE_COUNT;i++) r->nature_target[i]=0;
    r->nature_pan=0.5f;
    float calm=r->phase<BW_RIVER_APPROACH ? 1.0f:0;
    float dusk=fm_clampf(1-r->daylight-r->night-r->storm,0,1);
    /* Four short cricket chirps with a long pause; cicadas swell/rattle in daylight. */
    unsigned call=(r->ms+713u)%4300u;
    r->nature_target[BW_NATURE_CRICKET]=(r->night+dusk*0.55f)*calm*
        (call<660u && call%180u<70u ? 0.018f:0);
    r->nature_target[BW_NATURE_CICADA]=(r->daylight+dusk*0.18f)*calm*
        (0.010f+0.009f*bw_sin((float)r->ms/9700.0f));
    r->nature_step[2]=(4100.0f+170.0f*bw_sin((float)r->ms/13000.0f))/BW_RATE;
    r->nature_step[3]=(5600.0f+210.0f*bw_sin((float)r->ms/23000.0f))/BW_RATE;
    float gust=0.5f+0.5f*bw_sin((float)r->ms/11800.0f);
    r->nature_target[BW_NATURE_AIR]=r->wind*(0.025f+gust*0.055f)*(calm ? 1:0.15f);
    if(!r->life || !calm) {r->splash_stage=0;return;}
    float age=(float)(r->ms-r->life_at),duration=(float)(r->life_until-r->life_at);
    float p=fm_clampf(age/duration,0,1),window=bw_nature_window(age,duration);
    r->nature_pan=(r->life_seed&1u) ? 0.12f+p*0.76f:0.88f-p*0.76f;
    if(r->life==BW_LIFE_JUMP)
    {
        unsigned stage=age<1900 ? 1:2;
        if(r->splash_stage!=stage)
        {
            r->splash_stage=(uint8_t)stage;r->splash_at=r->ms;r->splash_count++;
        }
        float tail=fm_clampf(1-(float)(r->ms-r->splash_at)/550.0f,0,1);
        r->nature_target[BW_NATURE_SPLASH]=tail*tail*(stage==2 ? 0.11f:0.065f);
    }
    else if(r->life==BW_LIFE_LEAF)
        r->nature_target[BW_NATURE_AIR]+=window*(0.045f+gust*0.06f);
    else if(r->life==BW_LIFE_BEE || r->life==BW_LIFE_UFO)
    {
        int bee=r->life==BW_LIFE_BEE;
        r->nature_target[BW_NATURE_BUZZ]=window*(bee ? 0.014f:0.019f);
        r->nature_step[0]=(bee ? 164+18*bw_sin(age/1200):69+29*bw_sin(p*0.5f))/BW_RATE;
    }
    else if(r->life==BW_LIFE_FIREFLY)
    {
        float glow=0.45f+0.55f*(0.5f+0.5f*bw_sin(age*(0.002f/FM_TWO_PI)));
        r->nature_target[BW_NATURE_GLINT]=window*glow*glow*0.007f;
        r->nature_step[1]=(980+420*bw_sin(p*0.5f))/BW_RATE;
    }
    else if(r->life==BW_LIFE_METEOR)
    {
        float sweep=fm_clampf(1-age/3000,0,1);
        r->nature_target[BW_NATURE_AIR]+=window*sweep*0.07f;
        r->nature_target[BW_NATURE_GLINT]=window*sweep*0.006f;
        r->nature_step[1]=(1800-1100*p)/BW_RATE;
    }
}

static void bw_nature_sample(bw_engine *e,float noise,float *left,float *right)
{
    bw_river_state *r=&e->river;
    int enabled=e->playing && e->mode==BW_MODE_RIVER;
    for(unsigned i=0;i<BW_NATURE_COUNT;i++)
        r->nature_level[i]+=((enabled ? r->nature_target[i]:0)-r->nature_level[i])*0.0015f;
    r->nature_hiss+=(noise-r->nature_hiss)*0.19f;
    r->splash_filter+=(noise-r->splash_filter)*0.065f;
    float air=(r->nature_hiss-r->texture_l)*r->nature_level[BW_NATURE_AIR];
    float splash=(r->splash_filter*1.8f+noise*0.25f)*r->nature_level[BW_NATURE_SPLASH];
    float tone[4]={0,0,0,0};
    for(unsigned i=0;i<4;i++)
        if(r->nature_level[i+2]>0.000001f)
        {
            r->nature_phase[i]+=r->nature_step[i];
            if(r->nature_phase[i]>=1) r->nature_phase[i]-=1;
            tone[i]=bw_sin(r->nature_phase[i])*r->nature_level[i+2];
        }
    /* A second buzz harmonic and noisy amplitude modulation give wings/stridulation some texture. */
    if(r->nature_level[BW_NATURE_BUZZ]>0.000001f)
        tone[0]+=bw_sin(r->nature_phase[0]*2)*r->nature_level[BW_NATURE_BUZZ]*0.3f;
    if(r->nature_level[BW_NATURE_CICADA]>0.000001f)
    {
        r->nature_phase[4]+=94.0f/BW_RATE;
        if(r->nature_phase[4]>=1) r->nature_phase[4]-=1;
        tone[3]=(tone[3]*0.6f+noise*r->nature_level[BW_NATURE_CICADA]*0.3f)*
            (0.5f+0.5f*bw_sin(r->nature_phase[4]));
    }
    float moving=air+splash+tone[0]+tone[1];
    /* Equal nature levels across sound worlds; all still pass through the existing master and strike duck. */
    float gain=3.0f*(r->world==BW_WORLD_UNDERTOW ? 2.0f:r->world==BW_WORLD_FIREFLIES ? 8.0f/14.0f:1);
    *left+=gain*(moving*(1.1f-r->nature_pan*0.65f)+tone[2]*0.8f+tone[3]*0.55f);
    *right+=gain*(moving*(0.45f+r->nature_pan*0.65f)+tone[2]*0.55f+tone[3]*0.8f);
}
