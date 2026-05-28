#ifndef TERRAFORGE_MUSIC_H
#define TERRAFORGE_MUSIC_H
#include <stdint.h>

/*
 * TerraForge 32 original soundtrack data.
 *
 * The true stereo master lives in assets/wav/terraforge32_stereo_loop.wav.
 * This compact score is the cartridge-safe teaching representation used by
 * game.c when only the minimal PRG32 note/beep API is available.  Even on a
 * mono target the left/right fields remain useful: students can inspect how
 * counterpoint and panning are represented before converting the WAV/MIDI
 * asset to a PRG32 AUDIO block for PRG32 Audio Plus hardware.
 */
typedef struct {
    uint16_t left_hz;
    uint16_t right_hz;
    uint8_t frames;
    uint8_t pan_hint; /* 0 left, 128 center, 255 right */
} tf_stereo_step_t;

static const tf_stereo_step_t tf_stereo_score[] = {
    {110, 294, 16,  32}, {147, 370, 16, 224}, {165, 330, 16,  64}, {196, 277, 16, 192},
    {220, 247, 16,  96}, {247, 220, 16, 160}, {294, 185, 16,  48}, {330, 147, 16, 208},
    {196, 370, 12,  24}, {165, 294, 12, 232}, {147, 247, 12,  88}, {110, 196, 20, 168},
    {147, 277, 16, 224}, {196, 330, 16,  32}, {220, 370, 16, 192}, {294, 247, 16,  64},
    {330, 220, 16, 160}, {294, 185, 16, 224}, {247, 147, 16,  32}, {220, 196, 24, 128}
};
#define TF_STEREO_SCORE_LEN ((uint16_t)(sizeof(tf_stereo_score)/sizeof(tf_stereo_score[0])))
#endif
