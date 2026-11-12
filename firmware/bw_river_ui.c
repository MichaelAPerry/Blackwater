/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A single composition: persistent water curves, traced foreground, eased ink and consistent margins. */
static void bw_draw_river_scene(uint32_t now, int setup)
{
    const bw_river_state *r=&bw.river;
    unsigned phase=r->phase,line=r->line;
    uint32_t t=r->ms,age=t-r->phase_at;
    static float focus_x=120,focus_y=151,reveal_x=120,reveal_y=151;
    static unsigned last_phase;
    if (!bw_oy && last_phase!=phase)
    {
        reveal_x=focus_x;reveal_y=focus_y;last_phase=phase;
    }
    float enter=bw_vector_ease((float)age/700.0f);
    float leave=phase==BW_RIVER_REVEAL ? bw_vector_ease((8000.0f-(float)age)/1200.0f):1;
    float reveal=phase==BW_RIVER_REVEAL ? enter*leave:0;
    float strike=phase==BW_RIVER_STRIKE ? 1:phase==BW_RIVER_FIGHT ? 1-enter:0;
    float focus=phase==BW_RIVER_STRIKE ? 1-enter*0.46f:phase==BW_RIVER_FIGHT ? 0.54f:1;
    if(phase==BW_RIVER_REVEAL) focus=(0.54f-enter*0.41f)*leave+1-leave;
    float sky=phase==BW_RIVER_STRIKE ? 1-enter:phase==BW_RIVER_FIGHT ? 0:
        phase==BW_RIVER_REVEAL ? enter:1;
    bw_vector_sky=(unsigned)(sky*256);
    bw_vector_opacity=(unsigned)(focus*256);
    bw_vector_trace=256;
    bw_vector_river(t,r->bloom,r->world);
    bw_vector_opacity=256;
    int st=bw_fish_state(&bw,line),bx=120,by=151+(int)(fm_sinf((float)t*0.0014f)*1.5f);
    if(st!=BW_FISH_IDLE && phase<BW_RIVER_STRIKE)
    {
        float approach=phase==BW_RIVER_APPROACH ? bw_vector_ease((float)age/12000):0;
        bw_vector_opacity=170;
        bw_vector_oval(bx,by+3,8+(int)(approach*13),2+(int)(approach*3),C_MUTE);
        bw_vector_opacity=256;
        V(bx,by-4,bx,by+3,st==BW_FISH_BITE ? C_YEL:C_TXT);
    }
    if(phase==BW_RIVER_STRIKE || phase==BW_RIVER_FIGHT)
    {
        float tension=bw.track[line].manual ? bw.fight_tension[line]:r->pressure*100;
        float motion=phase==BW_RIVER_FIGHT ? enter:0;
        int running=bw.track[line].manual ? bw.fight_run[line]>0:r->pressure>0.55f;
        float x=120+fm_sinf((float)t*0.0017f)*motion*(8+r->pressure*28),y=151+r->pressure*motion*12;
        if(!bw_oy) {focus_x=x;focus_y=y;}
        bw_vector_opacity=(unsigned)((phase==BW_RIVER_STRIKE ? enter:1)*256);
        bw_vector_trace=(unsigned)((phase==BW_RIVER_STRIKE ? enter:1)*256);
        bw_vector_rod((int)x,(int)y,tension,C_YEL);
        bw_vector_trace=256;
        bw_vector_oval((int)x,(int)y+3,12,3,C_MUTE);
        bw_vector_opacity=256;
        float portrait=bw.fish_rare[line] ? strike:0;
        if(portrait>0.01f)
        {
            bw_vector_opacity=(unsigned)(portrait*(phase==BW_RIVER_STRIKE ? enter:1)*256);
            bw_vector_trace=(unsigned)((phase==BW_RIVER_STRIKE ? enter:1)*256);
            bw_vector_fisherman(22,26,77);
            bw_vector_trace=bw_vector_opacity=256;
        }
        int tx=24+(int)(portrait*90),ty=24+(int)(portrait*14);
        T(tx,ty,&FONT_M,"FISH ON",bw_vector_text_color(C_TXT,phase==BW_RIVER_STRIKE ? enter:1));
        if(phase==BW_RIVER_FIGHT)
        {
            T(24,54,&FONT_XS,running ? "LET IT RUN":"REEL GENTLY",bw_vector_text_color(C_MUTE,enter));
            bw_vector_opacity=(unsigned)(enter*256);
            V(24,216,216,216,C_BAR);
            for(unsigned j=0;j<5;j++) V(24+(int)j*48,213,24+(int)j*48,219,C_MUTE);
            int at=24+(int)(192*fm_clampf(tension/100,0,1));
            bw_vector_oval(at,216,3,3,tension>78 ? C_PINK:C_YEL);
            T(24,227,&FONT_T,"TENSION",bw_vector_text_color(C_MUTE,enter));
            TR(216,227,&FONT_T,"SELECT / K1",bw_vector_text_color(C_MUTE,enter));
        }
        bw_vector_opacity=256;
        return;
    }
    if(phase==BW_RIVER_REVEAL)
    {
        const bw_event *ev=&bw.event;
        if(enter<1)
        {
            bw_vector_opacity=(unsigned)((1-enter)*256);
            bw_vector_rod((int)reveal_x,(int)reveal_y,r->pressure*100,C_YEL);
        }
        bw_vector_opacity=(unsigned)(reveal*256);
        if(ev->result==BW_FIGHT_LANDED)
        {
            float size=0.7f+enter*1.7f,x=reveal_x+(120-reveal_x)*enter,y=reveal_y+(121-reveal_y)*enter;
            bw_vector_fish((int)(x-24*size),(int)(y-12*size),ev->species,10,size,ev->rare ? C_YEL:C_TXT);
            float labels=bw_vector_ease(((float)age-450)/850)*leave;
            T(24,24,&FONT_XS,ev->rare ? "RARE ENCOUNTER":"ENCOUNTER",bw_vector_text_color(C_MUTE,labels));
            int tx=(240-text_w(&FONT_XS,bw_species_names[ev->species]))/2;
            T(tx,161,&FONT_XS,bw_species_names[ev->species],bw_vector_text_color(C_TXT,labels));
            bw_vector_opacity=(unsigned)(labels*256);
            V(24,184,216,184,C_BAR);
            char length[24]="",weight[24]="";
            bw_length_text(length,ev->mm);bw_weight_text(weight,ev->g);
            T(24,196,&FONT_B,length,bw_vector_text_color(C_TXT,labels));
            TR(216,196,&FONT_B,weight,bw_vector_text_color(C_MUTE,labels));
        }
        else
        {
            bw_vector_oval((int)reveal_x,(int)reveal_y,12+(int)(enter*38),3+(int)(enter*8),C_MUTE);
            T(24,24,&FONT_XS,"BACK INTO THE DEEP",bw_vector_text_color(C_MUTE,reveal));
            T(24,196,&FONT_XS,ev->result==BW_FIGHT_SNAPPED ? "THE LINE GAVE WAY":"THE WATER SETTLES",bw_vector_text_color(C_TXT,reveal));
        }
        bw_vector_opacity=bw_vector_trace=256;
        return;
    }
    float controls=!bw.playing || setup || st==BW_FISH_BITE ? 1:
        1-bw_vector_ease(((float)(now-bw_ui.last_input)-2800)/1700);
    if(controls>0.01f)
    {
        T(24,24,&FONT_XS,bw_world_names[r->world],bw_vector_text_color(C_TXT,controls));
        TR(216,24,&FONT_T,bw.playing ? "RIVER / 01":"PLAY TO ENTER",bw_vector_text_color(C_MUTE,controls));
        if(st==BW_FISH_BITE) T(24,196,&FONT_XS,"BITE / PRESS REC",C_YEL);
        else if(setup)
        {
            bw_knob_cell kc[4];const bw_track *tr=&bw.track[bw_ui.track];
            T(24,190,&FONT_T,"REC / PLACE OR LIFT",C_MUTE);
            bw_kcell(&kc[0],"MIN S",tr->wait_min,600,0);bw_kcell(&kc[1],"MAX S",tr->wait_max,600,0);
            bw_kcell(&kc[2],"MODE",tr->manual,1,tr->manual ? "FISH":"LISTEN");
            bw_kcell(&kc[3],"BAIT",tr->bait,BW_NBAIT-1,bw_bait_names[tr->bait]);
            bw_knob_bar(kc,C_YEL);
        }
        else
        {
            T(24,217,&FONT_T,"CURRENT / BED / LIGHT / DISTANCE",bw_vector_text_color(C_MUTE,controls));
            T(24,229,&FONT_T,(int32_t)(bw_ui.message_until-now)>0 ? bw_ui.message:
                "PRESET / WORLD     SEQ / LINE",bw_vector_text_color(C_MUTE,controls));
        }
    }
    bw_vector_opacity=bw_vector_trace=256;
}
