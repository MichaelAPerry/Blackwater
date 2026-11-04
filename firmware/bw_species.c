/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The fish: names, sizes, weights, where and how they bite, and four factory patches for each track's fish.
 * Included by bw_engine.c. Sizes are typical catch ranges (mm, total length); weights from W = a * L^b (grams,
 * L in mm), coefficients fitted to published length-weight figures of each species, rounded: a game's biology,
 * plausible, not a field guide. */
const char *const bw_species_names[BW_SPECIES] = {"BLUEGILL", "LARGEMOUTH BASS", "BROWN TROUT", "CHANNEL CATFISH",
                                                  "CRAPPIE",  "PEACOCK BASS",    "BROOK TROUT", "FLATHEAD CATFISH"};
static const struct
{
    uint16_t min_mm, max_mm;
    float a, b;
    uint8_t depth, bait;
} bw_fish_data[BW_SPECIES] = {
    {90, 280, 6.5e-6f, 3.2f, BW_DEPTH_SHALLOW, BW_BAIT_WORM},    /* bluegill */
    {250, 650, 5.2e-6f, 3.2f, BW_DEPTH_SHALLOW, BW_BAIT_CRANK},  /* largemouth bass */
    {250, 750, 1.12e-5f, 3.0f, BW_DEPTH_MID, BW_BAIT_SPINNER},   /* brown trout */
    {300, 1000, 2.9e-6f, 3.2f, BW_DEPTH_BOTTOM, BW_BAIT_WORM},   /* channel catfish */
    {150, 400, 5.3e-6f, 3.2f, BW_DEPTH_MID, BW_BAIT_JIG},        /* crappie */
    {300, 750, 5.8e-6f, 3.2f, BW_DEPTH_SHALLOW, BW_BAIT_SPINNER}, /* peacock bass */
    {150, 500, 1.1e-5f, 3.0f, BW_DEPTH_DEEP, BW_BAIT_WORM},      /* brook trout */
    {500, 1350, 3.0e-6f, 3.2f, BW_DEPTH_BOTTOM, BW_BAIT_JIG},    /* flathead catfish */
};
uint32_t bw_weight_g(unsigned species, uint32_t mm)
{
    if (species >= BW_SPECIES || !mm)
        return 0;
    float g = bw_fish_data[species].a * bw_pow((float)mm, bw_fish_data[species].b);
    return g < 1.0f ? 1u : (uint32_t)(g + 0.5f);
}

/* a factory patch: the algorithm, then per operator ratio (/100), level, feedback, attack, decay, sustain,
 * release, velocity (ms, %) */
typedef struct
{
    uint16_t ratio;
    uint8_t level, fb;
    uint16_t a, d;
    uint8_t s;
    uint16_t r;
    uint8_t vel;
} bw_op_preset;
static const struct
{
    const char *name;
    uint8_t alg;
    bw_op_preset op[6];
} bw_presets[4][4] = {
    {/* bluegill: the lead, bright and plucked */
     {"PLUCK", 1,
      {{100, 82, 0, 2, 300, 10, 180, 60}, {300, 22, 0, 1, 120, 0, 100, 70}, {200, 45, 0, 2, 250, 5, 150, 60},
       {100, 18, 0, 1, 200, 0, 100, 60}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"BELL", 1,
      {{100, 80, 0, 2, 900, 0, 700, 50}, {350, 18, 0, 1, 600, 0, 500, 60}, {400, 35, 0, 2, 500, 0, 400, 50},
       {141, 20, 0, 1, 450, 0, 300, 60}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"MARIMBA", 1,
      {{100, 85, 0, 1, 420, 0, 200, 60}, {400, 20, 0, 1, 60, 0, 60, 70}, {400, 30, 0, 1, 150, 0, 100, 60},
       {1000, 10, 0, 1, 30, 0, 30, 70}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"GLASS", 3,
      {{100, 80, 0, 10, 800, 30, 600, 50}, {200, 8, 0, 5, 700, 20, 500, 60}, {500, 6, 0, 5, 500, 10, 400, 60},
       {900, 4, 0, 5, 300, 0, 300, 60}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}}},
    {/* largemouth bass: the bass line, punchy */
     {"PUNCH", 2,
      {{100, 85, 0, 1, 350, 55, 120, 50}, {100, 30, 30, 1, 180, 20, 100, 60}, {100, 10, 0, 1, 80, 0, 60, 60},
       {50, 50, 0, 1, 400, 60, 120, 40}, {100, 10, 0, 1, 50, 0, 40, 70}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"GROWL", 0,
      {{100, 85, 0, 2, 300, 70, 150, 40}, {100, 28, 55, 2, 250, 40, 150, 60}, {200, 12, 0, 2, 200, 20, 120, 60},
       {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"PICK", 1,
      {{100, 85, 0, 1, 250, 30, 120, 50}, {200, 30, 0, 1, 90, 5, 80, 70}, {50, 40, 0, 1, 300, 40, 150, 40},
       {100, 12, 0, 1, 60, 0, 60, 70}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"RUBBER", 3,
      {{100, 85, 0, 5, 500, 50, 200, 50}, {100, 25, 0, 5, 300, 10, 150, 60}, {300, 8, 0, 5, 120, 0, 100, 60},
       {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}}},
    {/* brown trout: flowing arpeggios */
     {"FLOW", 1,
      {{100, 75, 0, 4, 1200, 30, 500, 50}, {100, 18, 0, 4, 800, 10, 400, 60}, {200, 35, 0, 4, 1500, 30, 600, 50},
       {1400, 10, 0, 1, 150, 0, 100, 70}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"SHIMMER", 4,
      {{100, 60, 0, 30, 2000, 60, 1200, 40}, {200, 30, 0, 30, 2000, 60, 1200, 40}, {300, 18, 0, 30, 2000, 60, 1200, 40},
       {400, 10, 0, 30, 2000, 60, 1200, 40}, {50, 25, 0, 30, 2000, 60, 1200, 40}, {100, 12, 20, 50, 1500, 60, 1000, 50}}},
     {"REED", 0,
      {{100, 80, 0, 60, 400, 75, 300, 40}, {100, 30, 40, 80, 400, 60, 300, 50}, {300, 8, 0, 60, 300, 40, 300, 50},
       {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"HARP", 1,
      {{100, 80, 0, 1, 1600, 0, 900, 50}, {200, 15, 0, 1, 500, 0, 400, 60}, {200, 30, 0, 1, 1200, 0, 800, 50},
       {300, 10, 0, 1, 300, 0, 300, 60}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}}},
    {/* catfish: the drone underneath */
     {"DEPTH", 2,
      {{100, 85, 0, 400, 2000, 80, 1500, 30}, {100, 15, 10, 800, 3000, 60, 1500, 40}, {50, 6, 0, 600, 2000, 50, 1500, 40},
       {50, 50, 0, 600, 2000, 90, 1500, 30}, {100, 10, 0, 900, 2000, 50, 1500, 40}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"MURK", 0,
      {{100, 85, 0, 300, 1500, 85, 1800, 30}, {100, 22, 35, 500, 1500, 70, 1800, 40}, {150, 10, 0, 1500, 2500, 60, 1800, 40},
       {50, 6, 0, 800, 2000, 50, 1800, 40}, {100, 0, 0, 1, 100, 0, 100, 0}, {100, 0, 0, 1, 100, 0, 100, 0}}},
     {"RUMBLE", 6,
      {{100, 85, 0, 100, 800, 80, 900, 40}, {100, 12, 70, 100, 800, 50, 900, 50}, {50, 15, 0, 200, 900, 60, 900, 40},
       {100, 8, 0, 200, 900, 50, 900, 40}, {700, 6, 0, 1000, 1500, 40, 900, 40}, {100, 5, 0, 300, 900, 40, 900, 40}}},
     {"ABYSS", 5,
      {{100, 60, 0, 1000, 3000, 90, 2500, 30}, {50, 50, 0, 1200, 3000, 90, 2500, 30}, {200, 15, 0, 1500, 3000, 70, 2500, 30},
       {101, 40, 0, 1100, 3000, 90, 2500, 30}, {300, 6, 0, 1500, 3000, 60, 2500, 30}, {400, 3, 0, 1500, 3000, 50, 2500, 30}}}},
};
const char *bw_patch_name(unsigned species, unsigned slot)
{
    return bw_presets[species % 4][slot % 4].name;
}
void bw_patch_factory(bw_patch *p, unsigned species, unsigned slot)
{
    const bw_op_preset *src = bw_presets[species % 4][slot % 4].op;
    bw_algorithm(p, bw_presets[species % 4][slot % 4].alg);
    for (unsigned i = 0; i < BW_OPS; i++)
    {
        bw_operator *o = &p->op[i];
        o->ratio = src[i].ratio;
        o->hz = 440;
        o->level = src[i].level;
        o->feedback = src[i].fb;
        o->attack = src[i].a;
        o->decay = src[i].d;
        o->sustain = src[i].s;
        o->release = src[i].r;
        o->velocity = src[i].vel;
        o->fixed = 0;
    }
}
