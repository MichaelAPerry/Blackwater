/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Included by bw_engine.c. One river, one encounter, four musical layers. Fixed allocation throughout. */
const char *const bw_world_names[BW_WORLDS] = {"DUSK", "UNDERTOW", "FIREFLIES"};

void bw_river_enable(bw_engine *e)
{
    e->river.enabled = 1;
    e->river.duck = 1.0f;
    e->river.noise = 0x72697665u;
    e->river.modulation_rng = 0x77617465u;
    bw_life_init(e);
    unsigned weather=bw_weather(e);
    e->river.daylight=weather==BW_WEATHER_DAY;
    e->river.night=weather==BW_WEATHER_NIGHT;
    e->river.storm=weather==BW_WEATHER_STORM;
    for (unsigned k = 0; k < 4; k++)
        e->river.note_at[k] = e->river.ms + k * 2300u;
}

/* A deliberate sound-world load. Saved user patches are never overwritten. Existing sessions retain their
 * exact patch/song/track bytes until this command is used; no save-format/version reset is needed. */
void bw_river_world(bw_engine *e, unsigned world)
{
    e->river.world = (uint8_t)(world % BW_WORLDS);
    static const uint8_t source[3][4] = {{3, 3, 2, 0}, {3, 3, 2, 3}, {3, 2, 0, 2}};
    static const uint8_t slot[3][4] = {{3, 0, 1, 3}, {1, 2, 2, 3}, {3, 1, 1, 3}};
    for (unsigned k = 0; k < 4; k++)
    {
        bw_track *t = &e->track[k];
        bw_patch_factory(&t->patch, source[e->river.world][k], slot[e->river.world][k]);
        /* The world loader is explicit; active voices are released by its UI caller. */
        for (unsigned i = 0; i < BW_OPS; i++)
        {
            bw_operator *o = &t->patch.op[i];
            o->attack = k == 3 ? 120u + i * 40u : 900u + k * 600u + i * 180u;
            o->release = k == 3 ? 2400 : 5000;
            if (k < 3 && o->level)
                o->sustain = 65u + i * 3u;
            if (world == BW_WORLD_UNDERTOW && i == 1)
                o->ratio = 151;
        }
        t->patch.op[0].level = 85;
        /* The layer's register is independent of the fish which approaches it. */
        t->octave = k == 0 ? -2 : k == 1 || k == 2 ? 1 : 0;
        t->patch_slot = 0;
    }
}

void bw_river_reel(bw_engine *e, int clicks)
{
    if (e->river.phase != BW_RIVER_FIGHT)
        return; /* the strike is an input grace period, not an invisible fight */
    int n = e->river.reel + clicks;
    e->river.reel = (int16_t)(n < -32 ? -32 : n > 32 ? 32 : n);
}

static void bw_river_finish(bw_engine *e)
{
    e->river.phase = BW_RIVER_REVEAL;
    e->river.phase_at = e->river.ms;
    e->river.reel = 0;
}

static void bw_river_encounter(bw_engine *e, unsigned line)
{
    const bw_track *t = &e->track[line];
    /* Every species can visit the same line. Habitat and bait weight the distribution; a musical layer
     * is not a species selector. Ordinary/rare classification is preserved for the speech trigger. */
    unsigned weights[BW_SPECIES], total = 0;
    for (unsigned sp = 0; sp < BW_SPECIES; sp++)
    {
        weights[sp] = (sp < 4 ? 12u : 1u) *
            (t->depth == bw_fish_data[sp].depth ? 3u : 1u) *
            (t->bait == bw_fish_data[sp].bait ? 2u : 1u);
        total += weights[sp];
    }
    unsigned pick = bw_random(e) % total, sp = 0;
    while (sp + 1 < BW_SPECIES && pick >= weights[sp])
        pick -= weights[sp++];
    e->fish[line] = (uint8_t)sp;
    e->fish_rare[line] = sp >= 4;
    const float lo = bw_fish_data[sp].min_mm, hi = bw_fish_data[sp].max_mm;
    e->fish_mm[line] = (uint32_t)(lo + (hi - lo) * bw_pow(bw_unit(e), 1.6f));
}

static void bw_river_control(bw_engine *e)
{
    bw_river_state *r = &e->river;
    unsigned k = r->line;
    if(e->mode==BW_MODE_FREE) return;
    r->ms += 10;
    int state = e->hooked_state[k];
    if (r->phase <= BW_RIVER_GROW)
    {
        r->growth = state == BW_FISH_WAIT
            ? fm_clampf((float)(r->ms - r->wait_at) / (float)(r->wait_ms ? r->wait_ms : 1), 0, 1) : 0;
        r->phase = r->growth > 0.15f ? BW_RIVER_GROW : BW_RIVER_QUIET;
        if (state == BW_FISH_WAIT && r->ms - r->wait_at >= r->wait_ms)
        {
            bw_river_encounter(e, k);
            r->phase = BW_RIVER_APPROACH;
            r->phase_at = r->ms;
        }
    }
    else if (r->phase == BW_RIVER_APPROACH && r->ms - r->phase_at >= 12000u && state == BW_FISH_WAIT)
    {
        e->hooked_state[k] = BW_FISH_BITE;
        e->bite_until[k] = e->clock + 8u * BW_RATE;
        /* Listening mode still has an encounter arc, instead of instantly landing a fish. */
        if (!e->track[k].manual)
        {
            e->hooked_state[k] = BW_FISH_FIGHT;
            r->phase = BW_RIVER_STRIKE;
            r->phase_at = r->ms;
            if (e->fish_rare[k])
                bw_on_rare(k);
        }
    }
    if (r->phase == BW_RIVER_APPROACH && e->hooked_state[k] == BW_FISH_BITE &&
        (int32_t)(e->clock - e->bite_until[k]) >= 0)
    {
        e->log.missed++;
        bw_event_set(e, k, BW_FIGHT_THROWN);
        e->hooked_state[k] = BW_FISH_WAIT;
        bw_river_finish(e);
    }
    if (r->phase == BW_RIVER_STRIKE && r->ms - r->phase_at >= 1600u)
    {
        r->phase = BW_RIVER_FIGHT;
        r->phase_at = r->ms;
        r->note_at[3] = r->ms;
    }
    if (r->phase == BW_RIVER_FIGHT)
    {
        if (e->track[k].manual)
        {
            int reel = r->reel;
            r->reel = 0;
            bw_fight(e, k, reel, 10);
        }
        else if (r->ms - r->phase_at >= 16000u)
            bw_catch(e, k);
    }
    if (r->phase == BW_RIVER_REVEAL && r->ms - r->phase_at >= 8000u)
    {
        r->phase = BW_RIVER_RETURN;
        r->phase_at = r->ms;
    }
    if (r->phase == BW_RIVER_RETURN && r->ms - r->phase_at >= 30000u)
    {
        r->phase = BW_RIVER_QUIET;
        r->phase_at = r->ms;
        r->growth = 0;
        if (e->hooked_state[k] != BW_FISH_IDLE)
        {
            e->hooked_state[k] = BW_FISH_WAIT;
            bw_schedule(e, k); /* the wait begins after recovery, never during it */
        }
    }
    float target = r->phase == BW_RIVER_APPROACH ? 1.0f :
                   r->phase == BW_RIVER_STRIKE || r->phase == BW_RIVER_REVEAL ? 0.0f :
                   r->phase == BW_RIVER_FIGHT ? 0.15f :
                   r->phase == BW_RIVER_RETURN ? 0.0f :
                   r->growth * 0.8f;
    r->bloom += (target - r->bloom) * 0.008f;
    float pressure = r->phase == BW_RIVER_FIGHT
        ? (e->track[k].manual ? e->fight_tension[k] * 0.01f :
           0.45f + 0.3f * bw_sin((float)(r->ms - r->phase_at) / 6000.0f)) : 0;
    r->pressure += (pressure - r->pressure) * 0.025f;
    /* Shared audiovisual weather and colour. Notes, FM motion and measured layer output steer colour;
     * the wind also opens the noise filter. No animation RNG enters encounter selection. */
    float energy=0;
    for(unsigned layer=0;layer<4;layer++)
    {
        energy+=e->meter[layer];
        float tone=(float)(r->played_note[layer]%12u)/12.0f+e->lfo_value[layer]*0.12f+
            e->mod_scale[layer]*0.05f+(float)e->track[layer].patch.op[0].feedback*0.001f;
        r->color_voice[layer]+=(tone-r->color_voice[layer])*0.035f;
        r->note_flash[layer]*=0.975f;
    }
    r->energy+=(fm_clampf(energy*1.5f,0,1)-r->energy)*0.015f;
    float motion=fm_clampf(energy*4.0f,0,1);
    r->reactive+=(motion-r->reactive)*(motion>r->reactive ? 0.12f:0.008f);
    unsigned weather=bw_weather(e);
    float wind=(weather==BW_WEATHER_STORM ? 0.55f:weather==BW_WEATHER_NIGHT ? 0.08f:0.18f)+
        (0.5f+0.5f*bw_sin((float)r->ms/47000.0f))*0.20f;
    r->wind+=(wind-r->wind)*0.003f;
    r->color_phase+=0.00014f+r->reactive*0.0011f+r->wind*0.00008f;
    if(r->color_phase>=1) r->color_phase-=1;
    if (e->song.progression && r->ms % 48000u == 0)
        e->chord = (uint8_t)((e->chord + 1) % bw_progressions[e->song.progression].n);
    for (unsigned layer = 0; layer < 4; layer++)
        if ((int32_t)(r->ms - r->note_at[layer]) >= 0)
        {
            if(bw_keyboard_full(e) && layer==e->live_track)
            {
                r->note_at[layer]=r->ms+1000u;
                continue;
            }
            static const uint32_t spacing[4] = {12000u, 17000u, 21000u, 9000u};
            static const int8_t degrees[8] = {0, 4, 7, 2, 0, 9, 4, -3};
            unsigned j = (r->ms / spacing[layer] + layer + r->memory) % 8;
            int degree = layer == 0 ? (r->memory ? degrees[r->memory % 8] : 0) : degrees[j];
            /* Existing user motifs remain editable, but unfold as long gestures instead of a fast loop. */
            const bw_theme *theme = &e->track[layer].cur;
            unsigned step = (r->ms / spacing[layer] + layer) % theme->len;
            if (layer != 0)
                degree = theme->s[step].deg + (r->memory ? degrees[r->memory % 8] : 0);
            e->step[layer] = (uint8_t)((step + 1u) % theme->len);
            if (e->rec_left[layer] && !--e->rec_left[layer])
                e->rec_arm[layer] = 0;
            unsigned note = (unsigned)bw_note_of(e, layer, degree, 1);
            if (bw_track_audible(e, layer) &&
                bw_rand(&r->modulation_rng) % 100u < e->track[layer].density)
                bw_seq_note(e, layer, note, layer == 0 ? 88 : 55u + j * 3u);
            uint32_t hold = layer == 3 ? 2200u : spacing[layer] * 3u / 4u;
            e->off_at[layer] = e->clock + hold * (BW_RATE / 1000u);
            uint32_t interval = spacing[layer];
            if (layer == 3 && r->phase == BW_RIVER_FIGHT)
                interval = 1400u; /* a restrained pulse; the ambient layers remain slow */
            if (e->macros[layer] >> BW_M_STUTTER & 1u)
                interval = 400u;
            if (e->macros[layer] >> BW_M_HALF & 1u)
                interval *= 2u;
            r->note_at[layer] = r->ms + interval;
        }
    bw_life_tick(e);
    bw_nature_control(e);
}

/* Two stereo bands of filtered noise alongside FM. Independent PRNG keeps texture/sample rate from
 * changing fish selection. No sample assets, large delay buffers, or per-sample transcendental functions. */
static void bw_river_texture(bw_engine *e, float *l, float *right)
{
    bw_river_state *r = &e->river;
    float x = (float)(bw_rand(&r->noise) >> 8) / 8388608.0f - 1.0f;
    r->texture_l += (x - r->texture_l) * (0.012f+r->wind*0.018f);
    r->texture_r += (-x - r->texture_r) * (0.009f+r->wind*0.012f);
    r->body_l += (r->texture_l - r->body_l) * 0.001f;
    r->body_r += (r->texture_r - r->body_r) * 0.0013f;
    float level = e->playing && e->mode==BW_MODE_RIVER ? (r->world == BW_WORLD_UNDERTOW ? 0.15f : 0.055f) : 0;
    level *= 0.25f + r->bloom + r->pressure * 0.7f;
    /* Sample-rate smoothing avoids a discontinuity when the strike clears the mix. */
    r->duck += ((r->phase == BW_RIVER_STRIKE ? 0.30f : 1.0f) - r->duck) * 0.00008f;
    bw_nature_sample(e,x,l,right);
    *l = (*l + (r->texture_l - r->body_l) * level) * r->duck;
    *right = (*right + (r->texture_r - r->body_r) * level) * r->duck;
}
