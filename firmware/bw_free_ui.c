/* SPDX-License-Identifier: GPL-3.0-or-later */
static void bw_draw_free_play(void)
{
    unsigned layer=bw.live_track;
    const bw_patch *patch=&bw.track[layer].patch;
    const bw_operator *op=&patch->op[bw_ui.op];
    T(24,20,&FONT_M,"FREE PLAY",C_TXT);
    T(24,51,&FONT_XS,bw_layers[layer],bw_color(layer));
    char selected[16]="OPERATOR ";bw_num(selected+9,(int)bw_ui.op+1);
    TR(216,51,&FONT_XS,selected,C_YEL);
    bw_draw_algorithm(patch,74,96,bw_ui.op,bw_color(layer));
    T(24,172,&FONT_T,"SELECT / OPERATOR   ALGO / VOICE",C_MUTE);
    for(unsigned key=0;key<BW_KEYS;key++)
    {
        unsigned pc=(53u+key)%12u;
        int black=pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
        int x=24+(int)key*7;
        V(x,black ? 184:181,x,197,bw_ui.keys>>key&1u ? bw_color(layer):C_BAR);
        if(bw_ui.keys>>key&1u) bw_vector_oval(x,black ? 187:192,2,2,C_YEL);
    }
    T(24,201,&FONT_T,"ALL 27 KEYS / PRESET CHOOSES PATCH",C_MUTE);
    bw_knob_cell kc[4];
    bw_kcell(&kc[0],"RATIO",op->ratio,1600,0);
    bw_kcell(&kc[1],"LEVEL",op->level,100,0);
    bw_kcell(&kc[2],"FEEDBK",op->feedback,100,0);
    bw_kcell(&kc[3],"ALGO",patch->algorithm,7,0);bw_num(kc[3].value,patch->algorithm+1);
    bw_knob_bar(kc,bw_color(layer));
}
