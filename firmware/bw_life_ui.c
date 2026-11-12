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
        bw_vector_opacity=(unsigned)((float)saved*ink*fade*0.25f);
        V(x-direction*41,y-8,x-direction*9,y-2,c);
        V(x-direction*25,y-8,x-direction*5,y-2,c);
        bw_vector_opacity=(unsigned)((float)saved*ink*fade);
        V(x-2,y,x+2,y,C_TXT);V(x,y-2,x,y+2,C_TXT);
    }
    else if(r->life==BW_LIFE_UFO)
    {
        int x=direction>0 ? -18+(int)(progress*276):258-(int)(progress*276);
        int y=52+(int)(fm_sinf(progress*FM_TWO_PI)*6);
        bw_vector_oval(x,y,15,4,c);
        bw_vector_curve(x-13,y+2,x,y+10,x+13,y+2,C_MUTE);
        bw_vector_curve(x-8,y-1,x,y-15,x+8,y-1,C_TXT);
        V(x-5,y-5,x+4,y-5,C_MUTE);
        for(int j=-1;j<=1;j++)
        {
            bw_vector_pixel(x+j*7,y+2,C_YEL,256);
            bw_vector_oval(x+j*7,y+2,1,1,c);
        }
        bw_vector_opacity=(unsigned)((float)saved*ink*0.12f);
        V(x-7,y+5,x-16,y+24,c);V(x+7,y+5,x+16,y+24,c);
        bw_vector_curve(x-16,y+24,x,y+28,x+16,y+24,c);
    }
    else if(r->life==BW_LIFE_JUMP)
    {
        int start=65+(int)(r->life_seed%80u);
        float flight=fm_clampf(age/1900,0,1);
        int x=start+(int)(flight*44)*direction;
        int y=146-(int)(fm_sinf(flight*FM_PI)*28);
        if(age<1900) bw_vector_fish(x-14,y-7,r->life_seed%BW_SPECIES,(unsigned)(age/90),0.7f,c);
        /* Ripples and airborne droplets share the sound's exact takeoff/landing timestamp. */
        float splash=(float)(r->ms-r->splash_at)/700;
        int sx=age<1900 ? start:start+44*direction;
        for(unsigned j=0;j<3;j++)
        {
            float radius=splash*19+(float)j*4;
            bw_vector_opacity=(unsigned)((float)saved*ink*(1-fm_clampf(splash,0,1))*0.65f/(float)(j+1));
            bw_vector_oval(sx,151,(int)radius,2+(int)(radius/5),c);
        }
        bw_vector_opacity=(unsigned)((float)saved*ink*(1-fm_clampf(splash,0,1)));
        for(int j=-2;j<=2;j++)
        {
            int dx=sx+(int)(splash*18*(float)j/2),dy=148-(int)(fm_sinf(splash*FM_PI)*(12-2*(j<0 ? -j:j)));
            V(dx,dy,dx,dy+2,c);
        }
    }
    else if(r->life==BW_LIFE_LEAF)
    {
        int x=direction>0 ? 18+(int)(progress*204):222-(int)(progress*204);
        int y=82+(int)(progress*75+fm_sinf(progress*FM_TWO_PI*2)*9*r->wind);
        int tilt=(int)(fm_sinf(progress*FM_TWO_PI*3)*5);
        bw_vector_curve(x-7,y+tilt,x-2,y-10,x+7,y-tilt,c);
        bw_vector_curve(x+7,y-tilt,x+2,y+9,x-7,y+tilt,c);
        V(x-7,y+tilt,x+7,y-tilt,C_MUTE);
        V(x-2,y-2,x+1,y+tilt,C_MUTE);V(x+2,y+tilt,x+4,y+3,C_MUTE);
        V(x+7,y-tilt,x+10,y-tilt-2,c);
        bw_vector_opacity=(unsigned)((float)saved*ink*0.3f);
        bw_vector_curve(x-direction*21,y-5,x-direction*13,y-9,x-direction*5,y-6,c);
        bw_vector_curve(x-direction*31,y+7,x-direction*20,y+2,x-direction*11,y+4,c);
    }
    else if(r->life==BW_LIFE_BEE)
    {
        int x=27+(int)(progress*180),y=116+(int)(fm_sinf(progress*FM_TWO_PI*2)*9);
        bw_vector_oval(x,y,5,3,C_YEL);
        V(x-1,y-2,x-1,y+2,C_MUTE);V(x+2,y-2,x+2,y+2,C_MUTE);
        bw_vector_oval(x+5,y-1,2,2,c);
        V(x+6,y-3,x+8,y-5,c);V(x+7,y-1,x+9,y-2,c);
        V(x-2,y+3,x-3,y+5,C_MUTE);V(x+2,y+3,x+3,y+5,C_MUTE);
        int wing=2+(int)(2*(0.5f+0.5f*fm_sinf(age*0.027f)));
        bw_vector_oval(x-2,y-wing,2,wing,c);bw_vector_oval(x+2,y-wing,2,wing,c);
        bw_vector_opacity=(unsigned)((float)saved*ink*0.2f);
        bw_vector_curve(x-23,y+2,x-14,y-4,x-9,y-2,c);
    }
    else if(r->life==BW_LIFE_FIREFLY)
    {
        int x=100+(int)(fm_sinf(progress*FM_TWO_PI)*38),y=126+(int)(fm_sinf(progress*FM_TWO_PI*2)*15);
        float glow=0.45f+0.55f*(0.5f+0.5f*fm_sinf(age*0.002f));
        bw_vector_opacity=(unsigned)((float)saved*ink*glow);
        bw_vector_pixel(x,y,C_YEL,256);
        bw_vector_oval(x,y,3,3,c);
        V(x,y-2,x,y-5,C_MUTE);
        bw_vector_curve(x,y-3,x-5,y-8,x-4,y-2,c);
        bw_vector_curve(x,y-3,x+5,y-8,x+4,y-2,c);
        bw_vector_opacity=(unsigned)((float)saved*ink*glow*0.12f);
        bw_vector_oval(x,y,7,7,c);
    }
    bw_vector_opacity=saved;
}
