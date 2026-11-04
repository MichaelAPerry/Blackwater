/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The clip player against tools/adpcm.py's decoder (tests/run_tests.sh writes both from a synthetic tone). */
#include "../firmware/bw_clip.c"
#include <stdio.h>
#include <stdlib.h>
static int fails;
#define CHECK(c)                                                                                                     \
    do                                                                                                               \
    {                                                                                                                \
        if (!(c))                                                                                                    \
        {                                                                                                            \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c);                                                       \
            fails++;                                                                                                 \
        }                                                                                                            \
    } while (0)
int main(int argc, char **argv)
{
    if (argc < 2)
        return 2;
    FILE *f = fopen(argv[1], "rb"); /* the reference: int16 per 22,050 Hz frame */
    static int16_t ref[BW_CLIP_FRAMES];
    CHECK(f && fread(ref, 2, BW_CLIP_FRAMES, f) == BW_CLIP_FRAMES);
    if (f)
        fclose(f);
    CHECK(bw_clip_present());
    bw_clip_state c = {0};
    static int32_t lr[2 * (2 * BW_CLIP_FRAMES + 64)];
    bw_clip_mix(&c, lr, 64, 4096);
    CHECK(lr[0] == 0 && lr[127] == 0); /* nothing before a trigger */
    bw_clip_trigger(&c);
    CHECK(c.active);
    unsigned total = 2 * BW_CLIP_FRAMES + 64;
    for (unsigned at = 0; at < total; at += 256)
        bw_clip_mix(&c, lr + 2 * at, total - at < 256 ? total - at : 256, 4096);
    int worst = 0;
    for (unsigned k = 0; k < BW_CLIP_FRAMES; k++)
    {
        /* even output frames are the decoded samples themselves, scaled by 128 at full MASTER */
        int d = abs(lr[4 * k] - ref[k] * 128);
        worst = d > worst ? d : worst;
        CHECK(lr[4 * k] == lr[4 * k + 1]); /* the same on both channels */
    }
    CHECK(worst == 0);
    CHECK(!c.active); /* it ends by itself */
    /* a retrigger while playing does not restart it */
    bw_clip_trigger(&c);
    static int32_t a[512];
    bw_clip_mix(&c, a, 200, 4096);
    unsigned n = c.n;
    bw_clip_trigger(&c);
    CHECK(c.n == n);
    /* MASTER at zero: silent */
    static int32_t z[512];
    bw_clip_mix(&c, z, 200, 0);
    for (unsigned k = 0; k < 400; k++)
        CHECK(z[k] == 0);
    /* the sum is clamped to the 24-bit range */
    static int32_t hot[512];
    for (unsigned k = 0; k < 512; k++)
        hot[k] = 8388000;
    bw_clip_mix(&c, hot, 256, 4096);
    for (unsigned k = 0; k < 512; k++)
        CHECK(hot[k] <= 8388607 && hot[k] >= -8388608);
    printf("clip: %u frames, decoder matches the reference (worst %d)\n", (unsigned)BW_CLIP_FRAMES, worst);
    return fails != 0;
}
