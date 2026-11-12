/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Included by bw_engine.c. Physical-key ownership survives layer, octave and page changes. */
enum { BW_KEY_NONE, BW_KEY_NOTE, BW_KEY_MACRO, BW_KEY_ARP };
static const uint8_t bw_panel_white[16]=PLAT_WHITE_KEYS;
static const uint8_t bw_panel_black[11]=PLAT_BLACK_KEYS;

int bw_keyboard_full(const bw_engine *e)
{
    return e->keyboard_unlocked || e->mode==BW_MODE_FREE;
}

void bw_panel_key(bw_engine *e,unsigned key,int down,unsigned velocity)
{
    if(key>=BW_KEYS) return;
    if(!down)
    {
        unsigned kind=e->key_kind[key],note=e->key_note[key],layer=e->key_layer[key];
        if(kind==BW_KEY_NOTE) bw_live_note(e,layer,note,0,0);
        else if(kind==BW_KEY_MACRO) bw_macro(e,layer,note,0);
        else if(kind==BW_KEY_ARP)
        {
            e->arp_keys&=~(1u<<note);
            if(e->arp!=BW_ARP_LATCH) e->arp_held[note]=-1;
        }
        e->key_kind[key]=BW_KEY_NONE;
        return;
    }
    if(e->key_kind[key]) return;
    unsigned layer=e->live_track%4u,index=key;
    int full=bw_keyboard_full(e);
    if(!full)
    {
        for(unsigned i=0;i<11;i++)
            if(bw_panel_black[i]==key)
            {
                e->key_kind[key]=BW_KEY_MACRO;e->key_note[key]=(uint8_t)i;e->key_layer[key]=(uint8_t)layer;
                bw_macro(e,layer,i,1);return;
            }
        for(unsigned i=0;i<16;i++) if(bw_panel_white[i]==key) {index=i;break;}
    }
    e->key_layer[key]=(uint8_t)layer;
    if(e->arp)
    {
        if(e->arp==BW_ARP_LATCH && !e->arp_keys)
            for(unsigned i=0;i<BW_KEYS;i++) e->arp_held[i]=-1;
        e->arp_held[index]=(int8_t)(e->arp_order++&127u);
        e->arp_keys|=1u<<index;
        e->key_kind[key]=BW_KEY_ARP;e->key_note[key]=(uint8_t)index;
    }
    else
    {
        int note=full ? 53+(int)key+e->live_oct*12:bw_note_of(e,layer,bw_white_deg(e,index),0);
        note=note<0 ? 0:note>127 ? 127:note;
        e->key_kind[key]=BW_KEY_NOTE;e->key_note[key]=(uint8_t)note;
        bw_live_note(e,layer,(unsigned)note,1,velocity);
    }
    if(!full) bw_record(e,layer,bw_white_deg(e,index),velocity);
}

void bw_panel_clear(bw_engine *e)
{
    for(unsigned i=0;i<BW_KEYS;i++) bw_panel_key(e,i,0,0);
    for(unsigned i=0;i<BW_KEYS;i++) e->arp_held[i]=-1;
    e->arp_keys=0;
    for(unsigned k=0;k<4;k++)
        if(e->live_note[k]>=0)
        {
            bw_live_note(e,k,(unsigned)e->live_note[k],0,0);
            e->live_note[k]=-1;
        }
}

void bw_keyboard_layer(bw_engine *e,unsigned layer)
{
    e->live_track=(uint8_t)(layer%4u);
    if(bw_keyboard_full(e))
    {
        bw_seq_off(e,e->live_track);
        e->off_at[e->live_track]=0;
    }
}

void bw_keyboard_unlock(bw_engine *e,int unlocked)
{
    bw_panel_clear(e);
    e->keyboard_unlocked=unlocked!=0;
    bw_keyboard_layer(e,e->live_track);
    e->river.note_at[e->live_track]=e->river.ms+1000u;
}

void bw_performance_mode(bw_engine *e,unsigned mode)
{
    mode=mode==BW_MODE_FREE ? BW_MODE_FREE:BW_MODE_RIVER;
    if(e->mode==mode) return;
    bw_panel_clear(e);bw_all_notes_off(e);bw_transport(e,0);
    e->mode=(uint8_t)mode;
    e->river.phase=BW_RIVER_QUIET;e->river.phase_at=e->river.ms;
    e->river.growth=e->river.bloom=e->river.pressure=0;
    for(unsigned k=0;k<4;k++)
    {
        e->hooked_state[k]=BW_FISH_IDLE;
        e->river.note_at[k]=e->river.ms+k*600u;
    }
    if(mode==BW_MODE_RIVER) bw_cast(e,0);
}
