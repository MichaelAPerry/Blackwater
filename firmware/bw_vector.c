/* SPDX-License-Identifier: GPL-3.0-or-later */
/* All artwork is geometry rasterized by the firmware's existing canvas. No image/sprite data. */
typedef struct { int16_t x, y; } bw_vpoint;
#define BW_PATH(p, x, y, size, c) bw_vector_path(p, sizeof(p) / sizeof((p)[0]), x, y, size, c)

/* Fixed-point antialiased strokes blend into the current LCD band, without a full framebuffer. */
static unsigned bw_vector_opacity = 256;
static unsigned bw_vector_trace = 256;
static unsigned bw_vector_sky = 256;
static float bw_vector_ease(float x)
{
    x = fm_clampf(x,0,1);
    return x*x*(3.0f-2.0f*x);
}
static uint16_t bw_vector_mix(uint16_t a, uint16_t b, unsigned n)
{
    unsigned inv=256u-n;
    return (uint16_t)(((((a>>11)*inv+(b>>11)*n)>>8)<<11) |
        (((((a>>5)&63u)*inv+((b>>5)&63u)*n)>>8)<<5) |
        (((a&31u)*inv+(b&31u)*n)>>8));
}
static void bw_vector_pixel(int x,int y,uint16_t c,unsigned coverage)
{
    int row=y-bw_oy;
    if ((unsigned)x>=240u || (unsigned)row>=48u || y>=bw_ymax) return;
    unsigned alpha=coverage*bw_vector_opacity/256u;
    uint16_t back=swap16(cv_px[row*240+x]);
    cv_px[row*240+x]=swap16(bw_vector_mix(back,c,alpha));
}
static void bw_vector_stroke(int x0,int y0,int x1,int y1,uint16_t c)
{
    int lo=(y0<y1 ? y0:y1)/256, hi=(y0>y1 ? y0:y1)/256;
    if (!bw_vector_opacity || !bw_in(lo-1,hi-lo+3)) return;
    int steep=(y1-y0<0 ? y0-y1:y1-y0)>(x1-x0<0 ? x0-x1:x1-x0);
    if (steep) {int v=x0;x0=y0;y0=v;v=x1;x1=y1;y1=v;}
    if (x0>x1) {int v=x0;x0=x1;x1=v;v=y0;y0=y1;y1=v;}
    int dx=x1-x0,dy=y1-y0;
    if (!dx) {bw_vector_pixel(steep ? y0/256:x0/256,steep ? x0/256:y0/256,c,256);return;}
    int first=x0>=0 ? (x0+255)/256:-((-x0)/256);
    int end=x1>=0 ? x1/256:-((-x1+255)/256);
    int gradient=dy*256/dx,fy=y0+(first*256-x0)*dy/dx;
    for(int x=first;x<=end;x++,fy+=gradient)
    {
        int iy=fy>=0 ? fy/256:-((-fy+255)/256),frac=fy-iy*256;
        if (steep) {bw_vector_pixel(iy,x,c,256u-(unsigned)frac);bw_vector_pixel(iy+1,x,c,(unsigned)frac);}
        else {bw_vector_pixel(x,iy,c,256u-(unsigned)frac);bw_vector_pixel(x,iy+1,c,(unsigned)frac);}
    }
}
static void V(int x0,int y0,int x1,int y1,uint16_t c)
{
    bw_vector_stroke(x0*256,y0*256,x1*256,y1*256,c);
}
static uint16_t bw_vector_text_color(uint16_t c,float opacity)
{
    return bw_vector_mix(C_BG,c,(unsigned)(fm_clampf(opacity,0,1)*256.0f));
}

static void bw_vector_box(int x, int y, int w, int h, uint16_t c)
{
    V(x,y,x+w,y,c); V(x+w,y,x+w,y+h,c);
    V(x+w,y+h,x,y+h,c); V(x,y+h,x,y,c);
}

static void bw_vector_path(const bw_vpoint *p, unsigned n, int x, int y, int size, uint16_t c)
{
    for (unsigned i = 1; i < n && i*256u <= (n-1u)*bw_vector_trace; i++)
        V(x + p[i-1].x * size / 100, y + p[i-1].y * size / 100,
          x + p[i].x * size / 100, y + p[i].y * size / 100, c);
}

static void bw_vector_curve(int x0, int y0, int cx, int cy, int x1, int y1, uint16_t c)
{
    int px = x0, py = y0;
    for (int i = 1; i <= 16 && i*16 <= (int)bw_vector_trace; i++)
    {
        int u = 16 - i;
        int x = (u*u*x0 + 2*u*i*cx + i*i*x1) / 256;
        int y = (u*u*y0 + 2*u*i*cy + i*i*y1) / 256;
        V(px, py, x, y, c);
        px = x; py = y;
    }
}

static void bw_vector_oval(int x, int y, int rx, int ry, uint16_t c)
{
    int px = x + rx, py = y;
    for (unsigned i = 1; i <= 32; i++)
    {
        float a = (float)i * (FM_TWO_PI / 32.0f);
        int xx = x + (int)(fm_cosf(a) * rx), yy = y + (int)(fm_sinf(a) * ry);
        V(px, py, xx, yy, c);
        px = xx; py = yy;
    }
}

/* Original line portrait: swept silver hair, strong brow, long face and expedition collar. */
static void bw_vector_fisherman(int x, int y, int size)
{
    static const bw_vpoint hair[] = {{20,43},{14,31},{18,18},{29,9},{48,5},{67,9},{78,20},{79,34},{72,43}};
    static const bw_vpoint sweep[] = {{17,26},{31,19},{45,18},{59,21},{71,28},{74,39}};
    static const bw_vpoint part[] = {{25,17},{40,11},{56,12},{71,20}};
    static const bw_vpoint face[] = {{21,33},{19,48},{23,62},{32,77},{45,83},{57,81},{68,69},{74,51},{72,35}};
    static const bw_vpoint brow_l[] = {{26,39},{34,36},{42,39}};
    static const bw_vpoint brow_r[] = {{51,39},{59,36},{67,40}};
    static const bw_vpoint nose[] = {{47,41},{43,55},{47,59},{53,57}};
    static const bw_vpoint mouth[] = {{35,66},{45,64},{55,65},{60,63}};
    static const bw_vpoint chin[] = {{37,73},{46,76},{56,72}};
    static const bw_vpoint collar[] = {{28,73},{25,87},{10,94},{2,113},{91,113},{83,94},{66,85},{65,74}};
    static const bw_vpoint shirt[] = {{25,85},{35,99},{45,89},{57,99},{67,85}};
    BW_PATH(hair,x,y,size,C_TXT); BW_PATH(sweep,x,y,size,C_TXT); BW_PATH(part,x,y,size,C_MUTE);
    BW_PATH(face,x,y,size,C_TXT); BW_PATH(brow_l,x,y,size,C_TXT); BW_PATH(brow_r,x,y,size,C_TXT);
    BW_PATH(nose,x,y,size,C_TXT); BW_PATH(mouth,x,y,size,C_TXT); BW_PATH(chin,x,y,size,C_MUTE);
    BW_PATH(collar,x,y,size,C_MUTE); BW_PATH(shirt,x,y,size,C_YEL);
    V(x+30*size/100,y+43*size/100,x+36*size/100,y+43*size/100,C_TXT);
    V(x+55*size/100,y+43*size/100,x+61*size/100,y+43*size/100,C_TXT);
}

static void bw_vector_fish(int x, int y, unsigned sp, unsigned frame, float scale, uint16_t c)
{
    sp%=BW_SPECIES;
    unsigned kind = sp % 4u;
    static const int8_t depth[8]={9,6,5,5,10,7,6,4};
    int h=depth[sp];
    int tail = (int)(frame % 3u) - 1;
    /* Coordinates retain the former 48×24 bounding box; the drawing itself is all curves/strokes. */
    bw_vector_curve(x+8*scale,y+12*scale,x+25*scale,y+(12-h*2)*scale,x+44*scale,y+11*scale,c);
    bw_vector_curve(x+44*scale,y+11*scale,x+27*scale,y+(12+h*2)*scale,x+8*scale,y+12*scale,c);
    V(x+8*scale,y+12*scale,x+2*scale,y+(4+tail)*scale,c);
    if(sp==BW_FLATHEAD)
        V(x+2*scale,y+(4+tail)*scale,x+2*scale,y+(20+tail)*scale,c);
    else
    {
        V(x+2*scale,y+(4+tail)*scale,x+5*scale,y+12*scale,c);
        V(x+5*scale,y+12*scale,x+2*scale,y+(20+tail)*scale,c);
    }
    V(x+2*scale,y+(20+tail)*scale,x+8*scale,y+12*scale,c);
    bw_vector_curve(x+33*scale,y+7*scale,x+29*scale,y+12*scale,x+34*scale,y+17*scale,C_MUTE);
    V(x+20*scale,y+(12-h)*scale,x+27*scale,y+(6-h)*scale,c);
    V(x+27*scale,y+(6-h)*scale,x+30*scale,y+(12-h)*scale,c);
    V(x+22*scale,y+13*scale,x+18*scale,y+17*scale,c);
    V(x+18*scale,y+17*scale,x+27*scale,y+14*scale,c);
    bw_vector_oval(x+38*scale,y+10*scale,scale,scale,c);
    if (sp == BW_PEACOCK)
    {
        for(unsigned i=0;i<3;i++)
            V(x+(15+(int)i*6)*scale,y+7*scale,x+(17+(int)i*6)*scale,y+17*scale,C_MUTE);
        bw_vector_oval(x+11*scale,y+12*scale,2*scale,2*scale,c);
    }
    else if(sp==BW_CRAPPIE)
    {
        for(unsigned i=0;i<5;i++)
        {
            V(x+(17+(int)i*3)*scale,y+4*scale,x+(18+(int)i*3)*scale,y+(i%2 ? 0:2)*scale,c);
            V(x+(14+(int)i*4)*scale,y+(i%2 ? 10:14)*scale,x+(15+(int)i*4)*scale,y+(i%2 ? 11:15)*scale,C_MUTE);
        }
    }
    else if(sp==BW_BROOK)
    {
        for(unsigned i=0;i<4;i++)
            bw_vector_curve(x+(13+(int)i*5)*scale,y+9*scale,x+(15+(int)i*5)*scale,y+5*scale,
                x+(17+(int)i*5)*scale,y+10*scale,C_MUTE);
    }
    if (kind == BW_CATFISH)
    {
        V(x+43*scale,y+12*scale,x+48*scale,y+16*scale,c);
        V(x+42*scale,y+13*scale,x+46*scale,y+19*scale,c);
        if(sp==BW_FLATHEAD)
        {
            V(x+36*scale,y+15*scale,x+45*scale,y+14*scale,c);
            V(x+43*scale,y+11*scale,x+48*scale,y+8*scale,c);
        }
    }
    else if (sp == BW_BASS)
        V(x+12*scale,y+12*scale,x+29*scale,y+12*scale,C_MUTE);
    else if (sp == BW_TROUT)
        for (unsigned i=0;i<4;i++) bw_vector_pixel((int)(x+(14+(int)i*5)*scale),(int)(y+(i%2 ? 10:13)*scale),c,256);
    else if(sp==BW_BLUEGILL)
        bw_vector_oval(x+32*scale,y+12*scale,2*scale,3*scale,C_MUTE);
}

static void bw_vector_rod(int tx, int ty, float tension, uint16_t line_color)
{
    int tip_y = 78 + (int)(tension * 0.20f);
    bw_vector_curve(211,201,224,126,176,tip_y,C_TXT);
    bw_vector_curve(208,200,220,128,174,tip_y,C_MUTE);
    V(174,tip_y,tx,ty,line_color);
    bw_vector_oval(208,190,5,5,C_TXT);
    V(209,190,216,187,C_YEL);
    V(211,201,214,208,C_TXT);
    for (int i=0;i<3;i++) V(209+i,202+i*2,214+i,200+i*2,C_MUTE);
}

static void bw_vector_weather(unsigned kind, int x, int y, int scale)
{
    if (kind < 2)
    {
        bw_vector_oval(x+8*scale,y+8*scale,4*scale,4*scale,C_YEL);
        if (!kind)
        {
            V(x+8*scale,y,x+8*scale,y+2*scale,C_YEL);
            V(x+8*scale,y+14*scale,x+8*scale,y+16*scale,C_YEL);
            V(x,y+8*scale,x+2*scale,y+8*scale,C_YEL);
            V(x+14*scale,y+8*scale,x+16*scale,y+8*scale,C_YEL);
        }
    }
    else if (kind == 2)
    {
        bw_vector_curve(x+11*scale,y+2*scale,x-2*scale,y+8*scale,x+11*scale,y+14*scale,C_TXT);
        bw_vector_curve(x+11*scale,y+2*scale,x+6*scale,y+8*scale,x+11*scale,y+14*scale,C_TXT);
    }
    else
    {
        bw_vector_curve(x,y+8*scale,x+8*scale,y-5*scale,x+16*scale,y+8*scale,C_TXT);
        V(x,y+8*scale,x+16*scale,y+8*scale,C_TXT);
        V(x+6*scale,y+10*scale,x+3*scale,y+15*scale,bw_color(2));
        V(x+12*scale,y+10*scale,x+9*scale,y+15*scale,bw_color(2));
    }
}

static void bw_vector_icon(unsigned kind, int x, int y, uint16_t c)
{
    if (kind == BW_RIVER || kind == BW_LFO || kind == BW_FX)
        for (unsigned j=0;j<(kind == BW_RIVER ? 3u:1u);j++)
            bw_vector_curve(x+2,y+5+(int)j*3,x+9,y+(j&1 ? 13:1),x+16,y+8+(int)j*3,c);
    else if (kind == BW_FISHING)
    {
        V(x+10,y+2,x+10,y+10,c);
        bw_vector_curve(x+10,y+10,x+10,y+19,x+4,y+11,c);
    }
    else if (kind == BW_GLOBAL)
    {
        bw_vector_oval(x+9,y+9,6,6,c); V(x+9,y+9,x+12,y+5,c);
    }
    else if (kind == BW_ENV)
    {
        V(x+2,y+14,x+5,y+3,c); V(x+5,y+3,x+9,y+9,c); V(x+9,y+9,x+14,y+9,c); V(x+14,y+9,x+17,y+14,c);
    }
    else if (kind == BW_HELP)
        T(x+6,y+1,&FONT_XS,"?",c);
    else
        for (unsigned i=0;i<3;i++)
        {
            int yy = y+5+(int)((i*3+kind)%8);
            V(x+3+(int)i*5,yy,x+3+(int)i*5,y+14,c);
            V(x+2+(int)i*5,yy,x+5+(int)i*5,yy,c);
        }
}

static uint16_t bw_vector_music_color(unsigned layer,float distance,float saturation)
{
    static const uint16_t palette[3][3]={
        {RGB(104,206,202),RGB(188,151,224),RGB(239,181,108)},
        {RGB(112,156,229),RGB(93,201,181),RGB(203,139,201)},
        {RGB(237,193,115),RGB(176,208,141),RGB(112,205,213)}};
    const bw_river_state *r=&bw.river;
    float phase=r->color_phase-distance*0.55f+r->color_voice[layer%4u]*0.24f;
    while(phase<0) phase+=1;
    while(phase>=1) phase-=1;
    float stop=phase*3;
    unsigned first=(unsigned)stop;
    uint16_t color=bw_vector_mix(palette[r->world][first],palette[r->world][(first+1u)%3u],
        (unsigned)(bw_vector_ease(stop-first)*256));
    return bw_vector_mix(C_TXT,color,(unsigned)(fm_clampf(saturation,0,1)*256));
}

static void bw_vector_atmosphere(uint32_t t,float bloom,unsigned scene_ink)
{
    const bw_river_state *r=&bw.river;
    unsigned sky_ink=scene_ink*bw_vector_sky/256u;
    for(unsigned i=0;i<24;i++)
    {
        unsigned hash=i*2654435761u+0x19381u;
        int x=25+(int)(hash%190u),y=42+(int)((hash>>10)%33u);
        if(x>161 && x<198 && y<66) continue;
        float visibility=i<8 ? 0.34f:bw_vector_ease(bloom-(float)(i%4)*0.08f)*0.72f;
        float breathe=0.80f+0.20f*fm_sinf((float)t*0.00021f+(float)i);
        bw_vector_opacity=(unsigned)((float)sky_ink*visibility*breathe);
        uint16_t color=bw_vector_music_color(i%4u,(float)x/240,bloom*0.6f);
        bw_vector_pixel(x,y,color,256);
        if(i%6u==0)
        {
            V(x-2,y,x+2,y,color);V(x,y-2,x,y+2,color);
        }
        if(i%5u==1 && bloom>0.2f)
        {
            bw_vector_opacity=(unsigned)((float)sky_ink*bloom*0.16f);
            V(x,y,x+9,y+4,C_MUTE);V(x+9,y+4,x+17,y-2,C_MUTE);
        }
    }
    for(unsigned i=0;i<3;i++)
    {
        float gust=0.5f+0.5f*fm_sinf((float)t*0.00015f+i*1.9f);
        int x=22+(int)i*68+(int)(gust*7),y=57+(int)i*7;
        bw_vector_opacity=(unsigned)((float)sky_ink*r->wind*(0.18f+bloom*0.40f)*gust);
        bw_vector_curve(x,y,x+16,y-5,x+32+(int)(r->wind*13),y-2,bw_vector_music_color(i,gust,bloom));
    }
    /* Sparse bank reeds bend with the same wind that opens the audio's air texture. */
    bw_vector_opacity=(unsigned)((float)scene_ink*(0.16f+bloom*0.35f));
    int lean=(int)(r->wind*7*fm_sinf((float)t*0.00036f));
    bw_vector_curve(18,207,14+lean,188,27+lean,172,C_MUTE);
    bw_vector_curve(19,202,29+lean,194,32+lean,187,C_MUTE);
    bw_vector_curve(16,198,8+lean,190,9+lean,184,C_MUTE);
    bw_vector_opacity=scene_ink;
}

static void bw_vector_river(uint32_t t, float bloom, unsigned world)
{
    (void)world;
    unsigned scene_ink=bw_vector_opacity;
    bw_vector_atmosphere(t,bloom,scene_ink);
    bw_vector_opacity=scene_ink*bw_vector_sky/256u*3u/5u;
    bw_vector_oval(179,49,10,10,C_MUTE);
    V(24,83,216,83,C_MUTE);
    bw_vector_opacity=scene_ink;
    /* A fixed family of sixteen curves: new detail fades in between existing water lines.
     * Their positions never reflow when the sound crosses a complexity threshold. */
    static uint16_t water[16][41],stroke_color[16][41];
    for(unsigned j=0;j<16;j++)
    {
        float depth=(float)(j+1)/17.0f;
        int base=(j%4u)==3u;
        float visibility=base ? 0.72f : bw_vector_ease(bloom*1.6f-(float)(j%4u)*0.14f);
        bw_vector_opacity=(unsigned)((float)scene_ink*visibility);
        int width=24+(int)(depth*71.0f),prev_x=0,prev_y=0;
        for(unsigned i=0;i<=40;i++)
        {
            int x=120-width+(int)i*width/20;
            if (!bw_oy)
            {
                float across=((float)x-120.0f)/96.0f;
                float envelope=1.0f-across*across;
                float wave=fm_sinf((float)x*0.027f+(float)t*0.00017f+depth*4.2f);
                float fold=fm_sinf((float)x*0.057f-(float)t*0.00011f-depth*5.0f);
                stroke_color[j][i]=bw_vector_music_color(j%4u,(float)i/40,bloom*(0.55f+bw.river.energy*0.45f));
                if(!base) stroke_color[j][i]=bw_vector_mix(C_MUTE,stroke_color[j][i],180);
                water[j][i]=(uint16_t)((91.0f+depth*depth*115.0f+
                    envelope*depth*(2.0f+bloom*23.0f)*wave+bloom*envelope*6.0f*fold)*256.0f);
            }
            int y=water[j][i];
            if(i)
            {
                bw_vector_stroke(prev_x*256,prev_y,x*256,y,stroke_color[j][i]);
            }
            prev_x=x;prev_y=y;
        }
    }
    bw_vector_opacity=scene_ink;
}
