/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actor geometry uses the same elapsed-time state as the world/motif scheduler. */
static void bw_vector_life(void)
{
    const bw_river_state *r=&bw.river;
    if(!r->life || bw.mode==BW_MODE_FREE) return;
    unsigned saved=bw_vector_opacity;
    float age=(float)(r->ms-r->life_at),duration=(float)(r->life_until-r->life_at);
    float progress=fm_clampf(age/duration,0,1);
    float ink=bw_vector_ease(age/400)*bw_vector_ease((duration-age)/700);
    bw_vector_opacity=(unsigned)((float)saved*ink*0.85f);
    uint16_t c=bw_vector_music_color(r->life_seed%4u,progress,0.75f);
    int direction=(r->life_seed&1u) ? 1:-1;
    if(r->life==BW_LIFE_METEOR)
    {
        float p=fm_clampf(age/2700,0,1);
        int x=direction>0 ? 29+(int)(p*183):211-(int)(p*183);
        int y=40+(int)(p*33);
        float fade=1-bw_vector_ease((age-2000)/1600);
        bw_vector_opacity=(unsigned)((float)saved*ink*fade);
        V(x-direction*29,y-5,x,y,c);V(x-direction*18,y-6,x-direction*3,y-1,C_MUTE);
        V(x-2,y,x+2,y,C_TXT);V(x,y-2,x,y+2,C_TXT);
    }
    else if(r->life==BW_LIFE_UFO)
    {
        int x=direction>0 ? -18+(int)(progress*276):258-(int)(progress*276);
        int y=52+(int)(fm_sinf(progress*FM_TWO_PI)*6);
        bw_vector_oval(x,y,15,4,c);
        bw_vector_curve(x-8,y-1,x,y-15,x+8,y-1,C_TXT);
        for(int j=-1;j<=1;j++) bw_vector_pixel(x+j*7,y+2,C_YEL,256);
        bw_vector_opacity=(unsigned)((float)saved*ink*0.12f);
        V(x-7,y+5,x-16,y+24,c);V(x+7,y+5,x+16,y+24,c);
    }
    else if(r->life==BW_LIFE_JUMP)
    {
        int start=65+(int)(r->life_seed%80u);
        int x=start+(int)(progress*44)*direction;
        int y=146-(int)(fm_sinf(progress*FM_PI)*28);
        if(progress<0.85f) bw_vector_fish(x-14,y-7,r->life_seed%BW_SPECIES,(unsigned)(age/120),0.6f,c);
        else bw_vector_oval(x,149,4+(int)((progress-0.85f)*95),2,C_MUTE);
        bw_vector_opacity=(unsigned)((float)saved*ink*0.25f);
        bw_vector_curve(start-15,151,start,156,start+15,151,C_MUTE);
    }
    else if(r->life==BW_LIFE_LEAF)
    {
        int x=direction>0 ? 18+(int)(progress*204):222-(int)(progress*204);
        int y=82+(int)(progress*75+fm_sinf(progress*FM_TWO_PI*2)*9*r->wind);
        int tilt=(int)(fm_sinf(progress*FM_TWO_PI*3)*5);
        bw_vector_curve(x-7,y+tilt,x-2,y-10,x+7,y-tilt,c);
        bw_vector_curve(x+7,y-tilt,x+2,y+9,x-7,y+tilt,c);
        V(x-7,y+tilt,x+7,y-tilt,C_MUTE);
    }
    else if(r->life==BW_LIFE_BEE)
    {
        int x=27+(int)(progress*180),y=116+(int)(fm_sinf(progress*FM_TWO_PI*2)*9);
        bw_vector_oval(x,y,4,2,C_YEL);
        V(x,y-2,x,y+2,C_MUTE);
        int wing=2+(int)(2*(0.5f+0.5f*fm_sinf(age*0.027f)));
        bw_vector_oval(x-2,y-wing,2,wing,c);bw_vector_oval(x+2,y-wing,2,wing,c);
    }
    else if(r->life==BW_LIFE_FIREFLY)
    {
        int x=100+(int)(fm_sinf(progress*FM_TWO_PI)*38),y=126+(int)(fm_sinf(progress*FM_TWO_PI*2)*15);
        float glow=0.45f+0.55f*(0.5f+0.5f*fm_sinf(age*0.002f));
        bw_vector_opacity=(unsigned)((float)saved*ink*glow);
        bw_vector_pixel(x,y,C_YEL,256);
        bw_vector_oval(x,y,3,3,c);
        bw_vector_opacity=(unsigned)((float)saved*ink*glow*0.12f);
        bw_vector_oval(x,y,7,7,c);
    }
    bw_vector_opacity=saved;
}
