#pragma once
#include <JuceHeader.h>
#include <vector>
#include <utility>

namespace Gw
{
struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values; // el resto de parametros vuelve a su valor por defecto
};

// Posiciones de wavetable: 0=Sine .143=Tri .286=Saw .429=Square .571=FormantSaw .714=Vowel .857=Pulse 1=Digital
// Indices de destino: 0 Off,1 Cutoff,2 WT A,3 WT B,4 Warp,5 Pitch,6 Drive,7 Volume,8 Reso
// Divisiones: 0 1/1,1 1/2,2 1/4,3 1/8,4 1/16,5 1/32,6 1/2T,7 1/4T,8 1/8T,9 1/16T,10 1/8D,11 1/16D
inline const std::vector<Preset>& getFactoryPresets()
{
    static const std::vector<Preset> presets = {

        // ---------------- SKRILLEX ----------------
        { "01 Fuze - Hyper Screech Lead (Skrillex)", {
            { "a_wave", .571f }, { "a_wmode", 3.f }, { "a_warp", .45f }, { "a_uni", 5.f }, { "a_detune", .55f },
            { "a_spread", .8f }, { "a_level", .7f },
            { "b_wave", .286f }, { "b_wmode", 2.f }, { "b_warp", .3f }, { "b_oct", 1.f }, { "b_uni", 3.f },
            { "b_detune", .4f }, { "b_level", .4f },
            { "sub_level", .15f }, { "mono", 1.f }, { "glide", .06f },
            { "f_type", 0.f }, { "f_cutoff", 5200.f }, { "f_res", .45f }, { "f_drive", .35f }, { "f_key", .4f },
            { "amp_a", .001f }, { "amp_d", .25f }, { "amp_s", .85f }, { "amp_r", .12f },
            { "me_a", .001f }, { "me_d", .18f }, { "me_s", 0.f }, { "me_r", .1f },
            { "me1_dest", 5.f }, { "me1_amt", .25f }, { "me2_dest", 4.f }, { "me2_amt", .5f },
            { "l1_shape", 0.f }, { "l1_sync", 1.f }, { "l1_div", 4.f }, { "l1_amt", .3f }, { "l1_dest", 4.f },
            { "dist_type", 2.f }, { "dist_drive", .55f }, { "dist_mix", .5f },
            { "ott_depth", .5f }, { "ott_time", .4f }, { "ch_mix", .2f }, { "rv_mix", .08f },
            { "out_gain", -4.f } } },

        { "02 Duro - Dembow Sub Stab (Skrillex)", {
            { "a_wave", .143f }, { "a_level", .6f }, { "sub_level", .7f },
            { "mono", 1.f }, { "glide", .03f },
            { "f_type", 1.f }, { "f_cutoff", 900.f }, { "f_res", .25f }, { "f_drive", .3f },
            { "amp_a", .001f }, { "amp_d", .22f }, { "amp_s", .35f }, { "amp_r", .08f },
            { "me_d", .12f }, { "me_s", 0.f }, { "me1_dest", 1.f }, { "me1_amt", .6f },
            { "dist_type", 0.f }, { "dist_drive", .4f }, { "dist_mix", .6f },
            { "ott_depth", .25f }, { "out_gain", -3.f } } },

        { "03 Thistle - Warehouse Wobble (Skrillex)", {
            { "a_wave", .857f }, { "a_uni", 3.f }, { "a_detune", .25f }, { "a_spread", .5f }, { "a_level", .75f },
            { "b_wave", .286f }, { "b_oct", -1.f }, { "b_level", .5f }, { "sub_level", .3f }, { "mono", 1.f },
            { "f_type", 0.f }, { "f_cutoff", 600.f }, { "f_res", .55f }, { "f_drive", .5f },
            { "l1_shape", 1.f }, { "l1_sync", 1.f }, { "l1_div", 4.f }, { "l1_amt", .65f }, { "l1_dest", 1.f },
            { "l2_shape", 5.f }, { "l2_sync", 1.f }, { "l2_div", 4.f }, { "l2_amt", .2f }, { "l2_dest", 2.f },
            { "dist_type", 2.f }, { "dist_drive", .4f }, { "dist_mix", .5f },
            { "ott_depth", .6f }, { "ott_time", .45f }, { "ott_low", 2.f },
            { "dl_div", 10.f }, { "dl_fb", .4f }, { "dl_mix", .15f }, { "out_gain", -3.f } } },

        { "04 Essa Ta Quente - Baile Sub Slide (Skrillex)", {
            { "a_wave", 0.f }, { "a_oct", -1.f }, { "a_level", .9f },
            { "b_wave", .143f }, { "b_level", .25f }, { "sub_level", .5f },
            { "mono", 1.f }, { "glide", .12f },
            { "f_type", 1.f }, { "f_cutoff", 1800.f }, { "f_res", .15f }, { "f_drive", .2f },
            { "amp_a", .001f }, { "amp_d", 1.2f }, { "amp_s", 0.f }, { "amp_r", .3f },
            { "me_d", .06f }, { "me_s", 0.f }, { "me1_dest", 5.f }, { "me1_amt", .35f },
            { "dist_type", 4.f }, { "dist_drive", .55f }, { "dist_mix", .7f },
            { "ott_depth", .3f }, { "out_gain", -3.f } } },

        { "05 Mirage - UKG Reese (Skrillex)", {
            { "a_wave", .286f }, { "a_oct", -1.f }, { "a_uni", 2.f }, { "a_detune", .35f }, { "a_level", .7f },
            { "b_wave", .286f }, { "b_oct", -1.f }, { "b_fine", 12.f }, { "b_uni", 2.f }, { "b_detune", .35f },
            { "b_level", .6f }, { "sub_level", .35f }, { "mono", 1.f }, { "glide", .08f },
            { "f_type", 0.f }, { "f_cutoff", 700.f }, { "f_res", .3f }, { "f_drive", .4f },
            { "l1_shape", 0.f }, { "l1_sync", 1.f }, { "l1_div", 3.f }, { "l1_amt", .4f }, { "l1_dest", 1.f },
            { "dist_type", 0.f }, { "dist_drive", .35f }, { "dist_mix", .5f },
            { "ch_mix", .3f }, { "ch_rate", .5f }, { "ch_depth", .5f },
            { "ott_depth", .3f }, { "out_gain", -3.f } } },

        { "06 hit me where it hurts - Glass Pad (Skrillex)", {
            { "a_wave", .571f }, { "a_uni", 7.f }, { "a_detune", .6f }, { "a_spread", 1.f }, { "a_level", .55f },
            { "b_wave", .143f }, { "b_oct", 1.f }, { "b_wmode", 1.f }, { "b_warp", .2f }, { "b_uni", 5.f },
            { "b_detune", .45f }, { "b_level", .35f },
            { "f_type", 1.f }, { "f_cutoff", 3600.f }, { "f_res", .15f },
            { "amp_a", .35f }, { "amp_d", .8f }, { "amp_s", .8f }, { "amp_r", 1.2f },
            { "l1_shape", 1.f }, { "l1_sync", 0.f }, { "l1_rate", .25f }, { "l1_amt", .25f }, { "l1_dest", 2.f },
            { "l2_shape", 0.f }, { "l2_sync", 0.f }, { "l2_rate", .4f }, { "l2_amt", .15f }, { "l2_dest", 1.f },
            { "ch_mix", .45f }, { "ch_rate", .6f }, { "ch_depth", .5f },
            { "dl_div", 3.f }, { "dl_fb", .4f }, { "dl_mix", .25f },
            { "rv_size", .75f }, { "rv_damp", .45f }, { "rv_mix", .4f }, { "out_gain", -5.f } } },

        { "07 Yo Yan - Ambient Drone (Skrillex)", {
            { "a_wave", .714f }, { "a_uni", 4.f }, { "a_detune", .35f }, { "a_level", .6f },
            { "b_wave", 1.f }, { "b_oct", 1.f }, { "b_wmode", 3.f }, { "b_warp", .15f }, { "b_level", .25f },
            { "sub_level", .3f },
            { "f_type", 5.f }, { "f_cutoff", 700.f }, { "f_res", .5f },
            { "l1_shape", 0.f }, { "l1_sync", 0.f }, { "l1_rate", .15f }, { "l1_amt", .5f }, { "l1_dest", 1.f },
            { "amp_a", 1.2f }, { "amp_d", 1.f }, { "amp_s", .9f }, { "amp_r", 2.f },
            { "dist_type", 0.f }, { "dist_drive", .25f }, { "dist_mix", .3f },
            { "ch_mix", .4f }, { "dl_div", 2.f }, { "dl_fb", .5f }, { "dl_mix", .35f },
            { "rv_size", .9f }, { "rv_damp", .55f }, { "rv_mix", .55f }, { "out_gain", -5.f } } },

        // ---------------- VIRTUAL RIOT ----------------
        { "08 Burning Out - Vowel Growl (Virtual Riot)", {
            { "a_wave", .286f }, { "a_uni", 3.f }, { "a_detune", .3f }, { "a_level", .8f },
            { "b_wave", .571f }, { "b_oct", -1.f }, { "b_wmode", 1.f }, { "b_warp", .3f }, { "b_level", .5f },
            { "sub_level", .25f }, { "mono", 1.f }, { "glide", .02f },
            { "f_type", 5.f }, { "f_cutoff", 500.f }, { "f_res", .55f }, { "f_drive", .5f },
            { "l1_shape", 1.f }, { "l1_sync", 1.f }, { "l1_div", 7.f }, { "l1_amt", .75f }, { "l1_dest", 1.f },
            { "l2_shape", 0.f }, { "l2_sync", 1.f }, { "l2_div", 3.f }, { "l2_amt", .3f }, { "l2_dest", 2.f },
            { "dist_type", 1.f }, { "dist_drive", .5f }, { "dist_mix", .55f },
            { "ott_depth", .65f }, { "ott_time", .35f }, { "ott_low", 1.f }, { "ott_high", 2.f },
            { "ch_mix", .15f }, { "out_gain", -4.f } } },

        { "09 Yellow Lights - Neon Pluck (Virtual Riot)", {
            { "a_wave", .286f }, { "a_uni", 5.f }, { "a_detune", .4f }, { "a_spread", .9f }, { "a_level", .6f },
            { "b_wave", .429f }, { "b_oct", 1.f }, { "b_uni", 3.f }, { "b_detune", .25f }, { "b_level", .3f },
            { "f_type", 0.f }, { "f_cutoff", 1800.f }, { "f_res", .25f }, { "f_drive", .1f }, { "f_key", .5f },
            { "amp_a", .001f }, { "amp_d", .35f }, { "amp_s", .15f }, { "amp_r", .35f },
            { "me_d", .25f }, { "me_s", 0.f }, { "me1_dest", 1.f }, { "me1_amt", .7f },
            { "ch_mix", .35f }, { "dl_div", 10.f }, { "dl_fb", .45f }, { "dl_mix", .3f },
            { "rv_size", .7f }, { "rv_mix", .3f }, { "ott_depth", .2f }, { "out_gain", -4.f } } },

        { "10 Stealing Fire - Neuro Reese (Virtual Riot)", {
            { "a_wave", .571f }, { "a_wmode", 3.f }, { "a_warp", .3f }, { "a_oct", -1.f }, { "a_uni", 3.f },
            { "a_detune", .35f }, { "a_level", .7f },
            { "b_wave", .714f }, { "b_oct", -1.f }, { "b_fine", 12.f }, { "b_wmode", 2.f }, { "b_warp", .35f },
            { "b_level", .6f }, { "sub_level", .3f }, { "mono", 1.f }, { "glide", .04f },
            { "f_type", 0.f }, { "f_cutoff", 450.f }, { "f_res", .6f }, { "f_drive", .55f },
            { "l1_shape", 1.f }, { "l1_sync", 1.f }, { "l1_div", 4.f }, { "l1_amt", .55f }, { "l1_dest", 1.f },
            { "l2_shape", 3.f }, { "l2_sync", 1.f }, { "l2_div", 7.f }, { "l2_amt", .4f }, { "l2_dest", 4.f },
            { "me_d", .15f }, { "me_s", 0.f }, { "me1_dest", 5.f }, { "me1_amt", .1f },
            { "dist_type", 1.f }, { "dist_drive", .6f }, { "dist_mix", .6f },
            { "ott_depth", .7f }, { "out_gain", -4.f } } }
    };
    return presets;
}
} // namespace Gw
