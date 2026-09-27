#ifndef LOCAL_RESNET_MICRO_PARAMS_H
#define LOCAL_RESNET_MICRO_PARAMS_H

#include "isp_cnn_denoise.h"

#define L0_PARAMS_VALID 1


static const q31_t  L0_RAW_Q31 = 1218475039;
static const qexp_t L0_RAW_EXP = -2;

static const weight_word_t L0_WBANK[9][L0_CIN] = {
    {
        (((weight_word_t)0x2c2d04261b1d1814ULL << 64) | (weight_word_t)0x4a10ed0d41face98ULL), (((weight_word_t)0x2ff6db070cae00b4ULL << 64) | (weight_word_t)0xfbe4fc0c37f28d35ULL), (((weight_word_t)0xe5c9d83034390eddULL << 64) | (weight_word_t)0xe2fafb1a11d9e517ULL), (((weight_word_t)0xc94fe6acd86607fbULL << 64) | (weight_word_t)0xef0692ea112cf9dfULL)
    },
    {
        (((weight_word_t)0x12e9e606177ff1f4ULL << 64) | (weight_word_t)0x2341d6da20514d09ULL), (((weight_word_t)0xef9a1a092604ee44ULL << 64) | (weight_word_t)0x03f7e3322c0005c8ULL), (((weight_word_t)0xeb2717c407c10284ULL << 64) | (weight_word_t)0xd2ca01b0f5e634d2ULL), (((weight_word_t)0x3439d281c648d9f6ULL << 64) | (weight_word_t)0xa4f6b4c9f0deb4e3ULL)
    },
    {
        (((weight_word_t)0xf3cdf832e80edb4bULL << 64) | (weight_word_t)0x3008efbe292184b4ULL), (((weight_word_t)0xedf3033ff8e0fff5ULL << 64) | (weight_word_t)0x1ffb12021bfbedb9ULL), (((weight_word_t)0xf801f20410a4c581ULL << 64) | (weight_word_t)0x0ae28bdb15194ef3ULL), (((weight_word_t)0x1199ddde2403e0fcULL << 64) | (weight_word_t)0x0b1b17331ce03cb6ULL)
    },
    {
        (((weight_word_t)0x0bb030167fac2234ULL << 64) | (weight_word_t)0x05205e17f08138e7ULL), (((weight_word_t)0x0210a6db01c21d3cULL << 64) | (weight_word_t)0xeb1b2d20d1f79bdcULL), (((weight_word_t)0xf90500f6c6643ebcULL << 64) | (weight_word_t)0x1de4e1d4e6d8cd9bULL), (((weight_word_t)0xc3577fa40f9d532aULL << 64) | (weight_word_t)0xf9007fe0baf3299cULL)
    },
    {
        (((weight_word_t)0x150341b921caea83ULL << 64) | (weight_word_t)0x41109439ab405bd1ULL), (((weight_word_t)0xd9d8cb87140eb39aULL << 64) | (weight_word_t)0x127d7f44bb73c9bbULL), (((weight_word_t)0x1aac63be8c5f2a96ULL << 64) | (weight_word_t)0x43fe190d8116ab38ULL), (((weight_word_t)0xe890dfd63864c7feULL << 64) | (weight_word_t)0x63f77f81bbdff430ULL)
    },
    {
        (((weight_word_t)0x02ad090ff8258c9dULL << 64) | (weight_word_t)0xb61b3bef07f44cbcULL), (((weight_word_t)0x47f92934fb548a48ULL << 64) | (weight_word_t)0xedf23a2025ff8a28ULL), (((weight_word_t)0x054438e50acc9088ULL << 64) | (weight_word_t)0xbd208c19fcf11209ULL), (((weight_word_t)0xcdd2e2dd28e79d31ULL << 64) | (weight_word_t)0xb5b9146b27383281ULL)
    },
    {
        (((weight_word_t)0x9cb118fce8b158c6ULL << 64) | (weight_word_t)0x511de7fcf29cd820ULL), (((weight_word_t)0x7f08f4f5e0ca7ffdULL << 64) | (weight_word_t)0xe7f722dcdf21bbc3ULL), (((weight_word_t)0xc0c3f749114d6cb5ULL << 64) | (weight_word_t)0xecede4141a0f47ccULL), (((weight_word_t)0x60d62ee403a57c03ULL << 64) | (weight_word_t)0xf12de71100f359b1ULL)
    },
    {
        (((weight_word_t)0x2cd7e235fe464e75ULL << 64) | (weight_word_t)0x0c811409ae2b32cfULL), (((weight_word_t)0x9c81f918c9f6f5c1ULL << 64) | (weight_word_t)0xc7332cf6dcf243f6ULL), (((weight_word_t)0x1c0b1c401b455fd6ULL << 64) | (weight_word_t)0xf202ec00e507843dULL), (((weight_word_t)0x18b8f0e8350323a8ULL << 64) | (weight_word_t)0x2ff137f5fb1d9113ULL)
    },
    {
        (((weight_word_t)0xe0cfc720ef11cde6ULL << 64) | (weight_word_t)0x81bb79092c378102ULL), (((weight_word_t)0x0f21f63720dbbfa1ULL << 64) | (weight_word_t)0xa402e9d64900fd0aULL), (((weight_word_t)0xed1a0757ff1de0d0ULL << 64) | (weight_word_t)0xd2ffef2345f5dc02ULL), (((weight_word_t)0x00bd28fa09ffe548ULL << 64) | (weight_word_t)0x9215c32c49f65504ULL)
    }
};

static const bias_t L0_BIAS[L0_COUT] = {
    -7889, -14680, 759, -495, 1036, 1749, 1329, 469,
    -14714, -687, 710, 676, 404, 922, -14846, -15
};

static const q31_t L0_HEAD_Q31[L0_COUT] = {
    1801917824, 1544124288, 1668412800, 1579242368,
    1838550912, 1233487104, 2104964224, 1182674944,
    1444871040, 2044286336, 1969878400, 1790770048,
    1315074688, 1852432128, 1651359232, 1276817536
};

static const qexp_t L0_HEAD_EXP[L0_COUT] = {
    -9, -9, -8, -8, -8, -8, -8, -9,
    -9, -9, -9, -8, -9, -8, -9, -8
};


#define CNN_BODY_PARAMS_VALID 1

// L1
static const body_weight_word_t L1_WBANK[9][CNN_C] = {
    {
        (((body_weight_word_t)0xfefe421bdbf4f808ULL << 64) | (body_weight_word_t)0xf3ffe84d02196e22ULL), (((body_weight_word_t)0xf1f815f7cd0118fdULL << 64) | (body_weight_word_t)0x00e607f20cf5ce0aULL), (((body_weight_word_t)0x0a0a2ee9f3ed19fdULL << 64) | (body_weight_word_t)0x0ce81153df236745ULL), (((body_weight_word_t)0x3086b741c50c20b5ULL << 64) | (body_weight_word_t)0x973837c6a21bcdfaULL),
        (((body_weight_word_t)0xf51028fe201610f5ULL << 64) | (body_weight_word_t)0x0705d7b8e22828e2ULL), (((body_weight_word_t)0xebeea21131fefaf3ULL << 64) | (body_weight_word_t)0x17ebf9ac05078528ULL), (((body_weight_word_t)0xe4e7f7ece7d6fff5ULL << 64) | (body_weight_word_t)0x01effeab26ed39ffULL), (((body_weight_word_t)0x11fde10c7fe57018ULL << 64) | (body_weight_word_t)0xfdea1ce8fdf4a9e3ULL),
        (((body_weight_word_t)0xf7f746f2f7f4e700ULL << 64) | (body_weight_word_t)0x04fb023a06f12f16ULL), (((body_weight_word_t)0x358cbde3060b0e81ULL << 64) | (body_weight_word_t)0xb2022b0bad36d625ULL), (((body_weight_word_t)0xf4fc16271e0301fcULL << 64) | (body_weight_word_t)0x0fe50ce5eb229922ULL), (((body_weight_word_t)0x0e15450cf3ff0df4ULL << 64) | (body_weight_word_t)0xf9fb0cf106e8b10dULL),
        (((body_weight_word_t)0xf315d651fbe71700ULL << 64) | (body_weight_word_t)0xf21f02f4f616d6d3ULL), (((body_weight_word_t)0x0917e5f7ed180202ULL << 64) | (body_weight_word_t)0x04f2ecb1f7d710e5ULL), (((body_weight_word_t)0xfcfee3f420e6fd04ULL << 64) | (body_weight_word_t)0x08ecfe5315e613f8ULL), (((body_weight_word_t)0xf351df0925080c55ULL << 64) | (body_weight_word_t)0x58f7f7fc7f5bd7efULL)
    },
    {
        (((body_weight_word_t)0x14f91211f405fe04ULL << 64) | (body_weight_word_t)0x0afdfd47fa1631f4ULL), (((body_weight_word_t)0x10f5ccf8f7f3f907ULL << 64) | (body_weight_word_t)0xf8e9e9f404e4261cULL), (((body_weight_word_t)0x031da62a0ff414eeULL << 64) | (body_weight_word_t)0x11e12708fc36b808ULL), (((body_weight_word_t)0x4fde20f0c82833bbULL << 64) | (body_weight_word_t)0x9f336105a1ebb3e1ULL),
        (((body_weight_word_t)0x18fab7ef0af7eeffULL << 64) | (body_weight_word_t)0xfa11fab0e9d958f6ULL), (((body_weight_word_t)0xe40a9d0f151909e6ULL << 64) | (body_weight_word_t)0x0c0719f1f0403e07ULL), (((body_weight_word_t)0xfd131aefd2fe0207ULL << 64) | (body_weight_word_t)0xe9fcf38f16a158d6ULL), (((body_weight_word_t)0x2bf3cb00df1ad406ULL << 64) | (body_weight_word_t)0xed1ebb99051fa83aULL),
        (((body_weight_word_t)0x010abdf0edfd14feULL << 64) | (body_weight_word_t)0xfa130e5009f952eaULL), (((body_weight_word_t)0x348a0003c9161ab1ULL << 64) | (body_weight_word_t)0x9d2f7febcc069ce8ULL), (((body_weight_word_t)0xfb080f1ff72e27e9ULL << 64) | (body_weight_word_t)0x15bf0866ef459c15ULL), (((body_weight_word_t)0x07eaedd330151d06ULL << 64) | (body_weight_word_t)0xfa14b38119f6d510ULL),
        (((body_weight_word_t)0x10f1ff19cff9d9fbULL << 64) | (body_weight_word_t)0xf929314cefeed9d6ULL), (((body_weight_word_t)0x08188d26f0fb15f0ULL << 64) | (body_weight_word_t)0x02130193e5a2f2f0ULL), (((body_weight_word_t)0x0a0f3c010e0cf5faULL << 64) | (body_weight_word_t)0x06122ae405f12819ULL), (((body_weight_word_t)0x4b2bac4c167ffc6bULL << 64) | (body_weight_word_t)0x64253a377f4d14e1ULL)
    },
    {
        (((body_weight_word_t)0xf1f01afb05fae7f8ULL << 64) | (body_weight_word_t)0x0815de4c0304eb09ULL), (((body_weight_word_t)0xf3032019e1131c00ULL << 64) | (body_weight_word_t)0x103bf6ccf41848eaULL), (((body_weight_word_t)0x10fb06fd19e71406ULL << 64) | (body_weight_word_t)0xd2de1322092b1d50ULL), (((body_weight_word_t)0xdca9f30aef2c08bdULL << 64) | (body_weight_word_t)0x9a07fb50d710d40cULL),
        (((body_weight_word_t)0xe2f2a2e6efd7020fULL << 64) | (body_weight_word_t)0xf71a01f0fde1de09ULL), (((body_weight_word_t)0x2d0d35f22206edecULL << 64) | (body_weight_word_t)0xf9e3e78ffa13c104ULL), (((body_weight_word_t)0xff0429efddf0f3f6ULL << 64) | (body_weight_word_t)0x012be231f1e3d9d3ULL), (((body_weight_word_t)0xe1fdd112e1f7c2ffULL << 64) | (body_weight_word_t)0x004ee8b8141a5b00ULL),
        (((body_weight_word_t)0x11fe130ae6fbf909ULL << 64) | (body_weight_word_t)0x0901062605ef30f9ULL), (((body_weight_word_t)0x17d2c71a14110fb2ULL << 64) | (body_weight_word_t)0xa8e41f9a0d16ae0bULL), (((body_weight_word_t)0x05ebb30e0800f3f3ULL << 64) | (body_weight_word_t)0xffa1154111f84b39ULL), (((body_weight_word_t)0xeef196f16900fe02ULL << 64) | (body_weight_word_t)0xe7f002900c00cfd8ULL),
        (((body_weight_word_t)0xe9f7f7fdf3e417edULL << 64) | (body_weight_word_t)0xf90d1ef9070ba0dbULL), (((body_weight_word_t)0x0b0b810ddb0afeefULL << 64) | (body_weight_word_t)0xf332083aeb161905ULL), (((body_weight_word_t)0xeb003af8eaff0602ULL << 64) | (body_weight_word_t)0xf4140226f5e868fbULL), (((body_weight_word_t)0x494f0c20ce4ee169ULL << 64) | (body_weight_word_t)0x5c5410dd5e7510f9ULL)
    },
    {
        (((body_weight_word_t)0x2308ad0aec040ffbULL << 64) | (body_weight_word_t)0xf420254bfcf10e1cULL), (((body_weight_word_t)0x05f8c4f8c6f7f602ULL << 64) | (body_weight_word_t)0x0b1005d107123e18ULL), (((body_weight_word_t)0xfd00c419ea04f8fdULL << 64) | (body_weight_word_t)0xfd080daee6088901ULL), (((body_weight_word_t)0x41e762d2d60b54c0ULL << 64) | (body_weight_word_t)0xb869301f9023ba08ULL),
        (((body_weight_word_t)0x0514e7fbea020310ULL << 64) | (body_weight_word_t)0x13f3e2e0b45bddf1ULL), (((body_weight_word_t)0x0906f029001ee0f2ULL << 64) | (body_weight_word_t)0x07350bd71831ba1fULL), (((body_weight_word_t)0xe3e8c536fadf15f6ULL << 64) | (body_weight_word_t)0x08e712e9260c26edULL), (((body_weight_word_t)0xc7ffae445b2849ffULL << 64) | (body_weight_word_t)0x24ba34d8f8ec650eULL),
        (((body_weight_word_t)0x130145fcdf0b17feULL << 64) | (body_weight_word_t)0xf321024d0408cffdULL), (((body_weight_word_t)0x10d520e7f6eb39ceULL << 64) | (body_weight_word_t)0xd1f31d06d202d40aULL), (((body_weight_word_t)0xecf70d150005e901ULL << 64) | (body_weight_word_t)0x0aef0ea2fee8c1ffULL), (((body_weight_word_t)0xf80ff5e8efe81f0bULL << 64) | (body_weight_word_t)0x0f26efcb00fcde11ULL),
        (((body_weight_word_t)0x07f517d7e21ff000ULL << 64) | (body_weight_word_t)0x1210de41e804e8feULL), (((body_weight_word_t)0x0d0e3c07de080a08ULL << 64) | (body_weight_word_t)0xf94b0ad4fce18d00ULL), (((body_weight_word_t)0x09f14afbeeeb0b07ULL << 64) | (body_weight_word_t)0x0204eaef0107b7f7ULL), (((body_weight_word_t)0x2f4dce31fb1e0145ULL << 64) | (body_weight_word_t)0x5705f9c37938a944ULL)
    },
    {
        (((body_weight_word_t)0xe20114fb02effa03ULL << 64) | (body_weight_word_t)0xf9090c49f11c29f5ULL), (((body_weight_word_t)0x02031809caf9f805ULL << 64) | (body_weight_word_t)0xfae019f30205ad26ULL), (((body_weight_word_t)0x051fe4f2d91eeae9ULL << 64) | (body_weight_word_t)0xf5c836d4f7e4a315ULL), (((body_weight_word_t)0x1dbb28abb11c63c4ULL << 64) | (body_weight_word_t)0xb7545128890ad4cbULL),
        (((body_weight_word_t)0x28fef5d2941be60eULL << 64) | (body_weight_word_t)0x01f0d2f600fe8631ULL), (((body_weight_word_t)0xa1f283fdd5390cffULL << 64) | (body_weight_word_t)0x017f189108162222ULL), (((body_weight_word_t)0xe71133279e474102ULL << 64) | (body_weight_word_t)0xfe0dec1e04a4ccffULL), (((body_weight_word_t)0xce1c451ef91418fdULL << 64) | (body_weight_word_t)0xfccbeb280f21c30aULL),
        (((body_weight_word_t)0x1002e710e30f13ffULL << 64) | (body_weight_word_t)0xfd07042003072e1fULL), (((body_weight_word_t)0x2eb6a91b0f187fc6ULL << 64) | (body_weight_word_t)0xc4f57cada609c700ULL), (((body_weight_word_t)0xe7f034ee850308faULL << 64) | (body_weight_word_t)0x0e0008de1d1c6322ULL), (((body_weight_word_t)0x1c08b9e13aef1801ULL << 64) | (body_weight_word_t)0x00fbe194ecd8ccd8ULL),
        (((body_weight_word_t)0x1d0843a139105bffULL << 64) | (body_weight_word_t)0x08b0fe5905febb65ULL), (((body_weight_word_t)0xe40adf12d7c51300ULL << 64) | (body_weight_word_t)0x104c14f016daef03ULL), (((body_weight_word_t)0xf4090814df0aec09ULL << 64) | (body_weight_word_t)0x00f4f6150a13b3e6ULL), (((body_weight_word_t)0x324bc37f252c2253ULL << 64) | (body_weight_word_t)0x4b3dfa57754dd9c0ULL)
    },
    {
        (((body_weight_word_t)0xff093ef027fa0400ULL << 64) | (body_weight_word_t)0x050df144feef7fd6ULL), (((body_weight_word_t)0x03f914fbef00ef0bULL << 64) | (body_weight_word_t)0x05ec135702014ee4ULL), (((body_weight_word_t)0x60140cb34601db09ULL << 64) | (body_weight_word_t)0xd1db29af261d9103ULL), (((body_weight_word_t)0xe586c00ee60d21bcULL << 64) | (body_weight_word_t)0xa20411fad4f7f8f0ULL),
        (((body_weight_word_t)0x13f40eeb2329dcfdULL << 64) | (body_weight_word_t)0xfbd5e1a718f15d0eULL), (((body_weight_word_t)0xea0ab0fdf8c418ffULL << 64) | (body_weight_word_t)0x115d2ba1feff2dd7ULL), (((body_weight_word_t)0x1a030615f6dc03f8ULL << 64) | (body_weight_word_t)0x0302fe39ffd8d716ULL), (((body_weight_word_t)0xaff854430f12ec1aULL << 64) | (body_weight_word_t)0x150e1bd3062c264aULL),
        (((body_weight_word_t)0xf8ffb40f250615faULL << 64) | (body_weight_word_t)0x0af9f7b301f3caecULL), (((body_weight_word_t)0x01a055df0d051aadULL << 64) | (body_weight_word_t)0xcb21fc240202170fULL), (((body_weight_word_t)0xd9ebbd050bc7d503ULL << 64) | (body_weight_word_t)0x10bbf710f6220717ULL), (((body_weight_word_t)0x22fd9215410e0806ULL << 64) | (body_weight_word_t)0x05fefbe70b03b304ULL),
        (((body_weight_word_t)0xeae611f7d9d5c6fdULL << 64) | (body_weight_word_t)0xf9f09bbef8f2201eULL), (((body_weight_word_t)0xd006d1c522ee0a14ULL << 64) | (body_weight_word_t)0xef7bfcc5fcfc46eeULL), (((body_weight_word_t)0x0700bffffaf319faULL << 64) | (body_weight_word_t)0xf513d210eeff4e0dULL), (((body_weight_word_t)0x1c4820fecc3af752ULL << 64) | (body_weight_word_t)0x591435b25c33d781ULL)
    },
    {
        (((body_weight_word_t)0x0d04a70deefa06faULL << 64) | (body_weight_word_t)0x06faf6c4f1000211ULL), (((body_weight_word_t)0xf20108f8eee711fbULL << 64) | (body_weight_word_t)0x0102fe40fe1f6015ULL), (((body_weight_word_t)0xeff0db1ce3ff0005ULL << 64) | (body_weight_word_t)0x0d08f7eb01f2eff1ULL), (((body_weight_word_t)0x4791351ce20e2db5ULL << 64) | (body_weight_word_t)0xc1221441a40d07fcULL),
        (((body_weight_word_t)0x0007bfea09f51e0fULL << 64) | (body_weight_word_t)0x100ef3bde110c301ULL), (((body_weight_word_t)0x2703facc1a1fd70eULL << 64) | (body_weight_word_t)0xd4080a95011e442dULL), (((body_weight_word_t)0x0debe028e3e9db04ULL << 64) | (body_weight_word_t)0xfcd9042af4f5bbf7ULL), (((body_weight_word_t)0x7fd7b6370240b5f0ULL << 64) | (body_weight_word_t)0xf2e5ef10f5ee9329ULL),
        (((body_weight_word_t)0x07fce1f5e00a0f08ULL << 64) | (body_weight_word_t)0x0a11f7b80cfc3006ULL), (((body_weight_word_t)0x0981022dec061493ULL << 64) | (body_weight_word_t)0x81120600be06ec0aULL), (((body_weight_word_t)0x15ffe3ec3d06d100ULL << 64) | (body_weight_word_t)0xf8cffce210c90e24ULL), (((body_weight_word_t)0xda0227f5c4f6fa02ULL << 64) | (body_weight_word_t)0x1a06012bec189cf5ULL),
        (((body_weight_word_t)0x08031fec2a19fd06ULL << 64) | (body_weight_word_t)0x0300fe0507f6f8d7ULL), (((body_weight_word_t)0x03149b020209f81dULL << 64) | (body_weight_word_t)0xfd32f6d60d0daadbULL), (((body_weight_word_t)0x0505fc05e8e50202ULL << 64) | (body_weight_word_t)0x06e01b4800f0e9e3ULL), (((body_weight_word_t)0x0453e30d291cee3fULL << 64) | (body_weight_word_t)0x7f09115d6d7f0feeULL)
    },
    {
        (((body_weight_word_t)0x0a08b0fbcf1c1700ULL << 64) | (body_weight_word_t)0x09042509f3f5abe5ULL), (((body_weight_word_t)0xf701f617f0e4f8fdULL << 64) | (body_weight_word_t)0x0aeb10e2ed0f41ffULL), (((body_weight_word_t)0xccf7a6dff1f603e9ULL << 64) | (body_weight_word_t)0x17ef3de611d49e46ULL), (((body_weight_word_t)0x17aa204ddb273bb7ULL << 64) | (body_weight_word_t)0x92260851c70db5e6ULL),
        (((body_weight_word_t)0xeaf7cdfe0026110bULL << 64) | (body_weight_word_t)0x07ebcde4e80c58faULL), (((body_weight_word_t)0xf7fbf8a70a10e314ULL << 64) | (body_weight_word_t)0x090403aee71541b9ULL), (((body_weight_word_t)0xe106fff3f91a0b03ULL << 64) | (body_weight_word_t)0xef06f00feea9d434ULL), (((body_weight_word_t)0x16f1e608dbaaf8e5ULL << 64) | (body_weight_word_t)0xe3f3c1b30518bd15ULL),
        (((body_weight_word_t)0xff0611f7ddebea08ULL << 64) | (body_weight_word_t)0x0906030d04e8130bULL), (((body_weight_word_t)0x1eb8e2e3f2060fbbULL << 64) | (body_weight_word_t)0x96d502ff87f5f6edULL), (((body_weight_word_t)0x02060bfcff0be1ffULL << 64) | (body_weight_word_t)0xf3cddeb91a04cf33ULL), (((body_weight_word_t)0x090aaf1e1ce6ee0cULL << 64) | (body_weight_word_t)0x07ebf13ee30318f3ULL),
        (((body_weight_word_t)0x07061e10e8e5f50eULL << 64) | (body_weight_word_t)0x0b1352c004f33401ULL), (((body_weight_word_t)0x0c0397cfe5cdff0cULL << 64) | (body_weight_word_t)0xf9f8252e0e1031faULL), (((body_weight_word_t)0xf112200df9080a03ULL << 64) | (body_weight_word_t)0x040e1217fc162feeULL), (((body_weight_word_t)0xe93c3525553c1655ULL << 64) | (body_weight_word_t)0x51f7fe177d539303ULL)
    },
    {
        (((body_weight_word_t)0xecfdf807fd100a01ULL << 64) | (body_weight_word_t)0x0710e040f7f10de4ULL), (((body_weight_word_t)0xfcfae517380f1101ULL << 64) | (body_weight_word_t)0xf5f617da081aa11eULL), (((body_weight_word_t)0x3e0ac7ed202bd40fULL << 64) | (body_weight_word_t)0xf8c52beb440e3c1aULL), (((body_weight_word_t)0x3d9cbb05f01427b9ULL << 64) | (body_weight_word_t)0xbb1510a5e60ca9d9ULL),
        (((body_weight_word_t)0xe3f5442ff4e71116ULL << 64) | (body_weight_word_t)0x0eeafede18ea27fcULL), (((body_weight_word_t)0xccf62cd02402f511ULL << 64) | (body_weight_word_t)0xf40af49cfcf493f2ULL), (((body_weight_word_t)0x12f9e809c3fbe7f7ULL << 64) | (body_weight_word_t)0xf30ffc02f70055f9ULL), (((body_weight_word_t)0xe6f94707b720e1f7ULL << 64) | (body_weight_word_t)0xfa6bd5b1f60f504aULL),
        (((body_weight_word_t)0x0ff6ddfeff12f609ULL << 64) | (body_weight_word_t)0x040509580005affeULL), (((body_weight_word_t)0x0db4b4f60e04f4a2ULL << 64) | (body_weight_word_t)0x9b07fcea10000c06ULL), (((body_weight_word_t)0xedf0e51d3bc8d0f8ULL << 64) | (body_weight_word_t)0x07e7e606210cee25ULL), (((body_weight_word_t)0x0100c94ffaea0a0aULL << 64) | (body_weight_word_t)0x0c1ef50df01f0930ULL),
        (((body_weight_word_t)0xf205de4810f60306ULL << 64) | (body_weight_word_t)0xf8c7f7140821a857ULL), (((body_weight_word_t)0xf1001dcc3007ea0bULL << 64) | (body_weight_word_t)0xe8ff060b18de4eeeULL), (((body_weight_word_t)0x00fb4a12cbe92400ULL << 64) | (body_weight_word_t)0xfb1bf3cf03025714ULL), (((body_weight_word_t)0x1f46d4fae734074fULL << 64) | (body_weight_word_t)0x5f1ceadc65019a0fULL)
    }
};
static const bias_t L1_BIAS[CNN_C] = {
    -133, -4486, -1794, -52, -7855, -3, -2033, 216,
    -112, -40, -656, 1798, -120, -14539, -216, 212
};
static const q31_t L1_Q31[CNN_C] = {
    1703579392, 1971122560, 1112374784, 1816192384,
    1216187904, 1767840512, 1695111168, 1957504128,
    1365642240, 1091119488, 1122300544, 1397689088,
    1472811136, 1262032256, 2097021952, 1248734720
};
static const qexp_t L1_EXP[CNN_C] = {
    -8, -10, -7, -7, -9, -8, -8, -7,
    -6, -7, -7, -8, -7, -9, -7, -7
};

// L2
static const body_weight_word_t L2_WBANK[9][CNN_C] = {
    {
        (((body_weight_word_t)0x1218e0fccc02f5fbULL << 64) | (body_weight_word_t)0x03f8f7ba2ed510e6ULL), (((body_weight_word_t)0xf514fcf0f4ac12e2ULL << 64) | (body_weight_word_t)0x1a0205be1d2828e5ULL), (((body_weight_word_t)0x0fbb36b925f80744ULL << 64) | (body_weight_word_t)0xb3e31608c62904e1ULL), (((body_weight_word_t)0xd4c1e942d73d0012ULL << 64) | (body_weight_word_t)0x7ffeddfa23f8c828ULL),
        (((body_weight_word_t)0x000307e347ac0ce4ULL << 64) | (body_weight_word_t)0xe4f02ee2220a05f5ULL), (((body_weight_word_t)0xe9f3ee1f0606f1f2ULL << 64) | (body_weight_word_t)0x4cfee51ce80cf624ULL), (((body_weight_word_t)0x1302ea00de0ff806ULL << 64) | (body_weight_word_t)0x24fee9fb3aec10fbULL), (((body_weight_word_t)0xf831cb3b2a6ef4dcULL << 64) | (body_weight_word_t)0x0c04c5cd1be93d4eULL),
        (((body_weight_word_t)0xd604181dcebb1515ULL << 64) | (body_weight_word_t)0x2eff1540f30fceedULL), (((body_weight_word_t)0xf7222beab2e104f3ULL << 64) | (body_weight_word_t)0xe71108c21d7fedf3ULL), (((body_weight_word_t)0xde2bd22ef854e4feULL << 64) | (body_weight_word_t)0xfc0303f91aa10f14ULL), (((body_weight_word_t)0xeeb321a4d6bbeab5ULL << 64) | (body_weight_word_t)0x2eb6eb1bfe56a3f5ULL),
        (((body_weight_word_t)0x278b4091d70ff95aULL << 64) | (body_weight_word_t)0xb6f924ef933d330cULL), (((body_weight_word_t)0xfb01edea3d150bdaULL << 64) | (body_weight_word_t)0xf8def1ae1ffff4e6ULL), (((body_weight_word_t)0x6c7f0b232a091cc0ULL << 64) | (body_weight_word_t)0xd51cfbb57805350cULL), (((body_weight_word_t)0x2ad0f6f449ee2704ULL << 64) | (body_weight_word_t)0x01fe0f3afaac0212ULL)
    },
    {
        (((body_weight_word_t)0xde38f3ecd6d913d9ULL << 64) | (body_weight_word_t)0xe109dbff2bb8e144ULL), (((body_weight_word_t)0x1416f2ecc8060fedULL << 64) | (body_weight_word_t)0x2af11afc2308d735ULL), (((body_weight_word_t)0x06e3efc2f5020522ULL << 64) | (body_weight_word_t)0x0d2306f9fcf72624ULL), (((body_weight_word_t)0xbce9d95dd41b17f7ULL << 64) | (body_weight_word_t)0x2b02f2f8fbd8bc1bULL),
        (((body_weight_word_t)0xfdfa12fad3eb060fULL << 64) | (body_weight_word_t)0x1f1ef1efed271fdeULL), (((body_weight_word_t)0x272dce3235dbe602ULL << 64) | (body_weight_word_t)0x5204e6207fb1e3f6ULL), (((body_weight_word_t)0x05e900f0f109293eULL << 64) | (body_weight_word_t)0x0af90f26f8e42218ULL), (((body_weight_word_t)0xed48fd1a27fef9f7ULL << 64) | (body_weight_word_t)0x2002e381e82220ffULL),
        (((body_weight_word_t)0x1bdde32ab0c4e6ecULL << 64) | (body_weight_word_t)0x15cff007b4e2c93bULL), (((body_weight_word_t)0xf7e00c26eafee625ULL << 64) | (body_weight_word_t)0x0ddc01fe1003eb13ULL), (((body_weight_word_t)0xc8f00b01eac41be2ULL << 64) | (body_weight_word_t)0x0fd5cff2f42102c4ULL), (((body_weight_word_t)0xab000134a8972506ULL << 64) | (body_weight_word_t)0x4cbf324590359e0aULL),
        (((body_weight_word_t)0xfcc13b97d9e0e2deULL << 64) | (body_weight_word_t)0xbc145e449a4b2e13ULL), (((body_weight_word_t)0x18d60af3c91206e0ULL << 64) | (body_weight_word_t)0xe00e1dfeeb0714d0ULL), (((body_weight_word_t)0x253806f61b0be701ULL << 64) | (body_weight_word_t)0x1b17c4a72902e615ULL), (((body_weight_word_t)0x1bf318bce5f8151bULL << 64) | (body_weight_word_t)0xc6130bdebd2cf112ULL)
    },
    {
        (((body_weight_word_t)0xcdcffa13dcf11cf6ULL << 64) | (body_weight_word_t)0xbbe2d5d7f2eec417ULL), (((body_weight_word_t)0xfbf1dbed0cf60728ULL << 64) | (body_weight_word_t)0x1c06e61f0208fc09ULL), (((body_weight_word_t)0xe4f413f508f6d433ULL << 64) | (body_weight_word_t)0xcff71beacf303a1eULL), (((body_weight_word_t)0xfae5de4ee3eddcf7ULL << 64) | (body_weight_word_t)0x1ff7e7b5600881d9ULL),
        (((body_weight_word_t)0xee0d1f18d542fafbULL << 64) | (body_weight_word_t)0x0ef20d21f1041de6ULL), (((body_weight_word_t)0xfdf7df1601d72bc7ULL << 64) | (body_weight_word_t)0x09e7c2ddfae1e33cULL), (((body_weight_word_t)0xefef00f701080c16ULL << 64) | (body_weight_word_t)0x1901edd1aef80cfdULL), (((body_weight_word_t)0xfe4300013f3205e8ULL << 64) | (body_weight_word_t)0xfd13b4cd42f2e562ULL),
        (((body_weight_word_t)0xf4f4d520e7010aa9ULL << 64) | (body_weight_word_t)0x5b09ed0debe6f1c5ULL), (((body_weight_word_t)0xff080b0f0824d1ebULL << 64) | (body_weight_word_t)0x021c1da13b1eebcdULL), (((body_weight_word_t)0x290b15ede4000037ULL << 64) | (body_weight_word_t)0xf604ddf221f90d26ULL), (((body_weight_word_t)0x44f027def9b8c20cULL << 64) | (body_weight_word_t)0x13daeedbf826f720ULL),
        (((body_weight_word_t)0xfabd2de7e5deed5dULL << 64) | (body_weight_word_t)0xc8f05632a93d5316ULL), (((body_weight_word_t)0xe712f40a190cf0e5ULL << 64) | (body_weight_word_t)0xec2011d628fd14dbULL), (((body_weight_word_t)0xecfb16261deaf8e6ULL << 64) | (body_weight_word_t)0x0ee8ecdf1212dcf7ULL), (((body_weight_word_t)0x060307e51a06fd02ULL << 64) | (body_weight_word_t)0xfdf9194805071204ULL)
    },
    {
        (((body_weight_word_t)0x464fe0089e81e3efULL << 64) | (body_weight_word_t)0xf0f4d4e01d05e32cULL), (((body_weight_word_t)0xf82a11e7ed18ee01ULL << 64) | (body_weight_word_t)0xe4d719d6ffef22edULL), (((body_weight_word_t)0xf8b102f460d9db12ULL << 64) | (body_weight_word_t)0xdf1cf910f1381c05ULL), (((body_weight_word_t)0xcfa1dd42aef710f3ULL << 64) | (body_weight_word_t)0x7f0fd037fad8c726ULL),
        (((body_weight_word_t)0x130eea16d9d713eaULL << 64) | (body_weight_word_t)0xf7ded626f8edea29ULL), (((body_weight_word_t)0xfac3d21d4cf40203ULL << 64) | (body_weight_word_t)0xfef7ec081c180c06ULL), (((body_weight_word_t)0x15c0811ec523bcf1ULL << 64) | (body_weight_word_t)0x1bf084f95eefe347ULL), (((body_weight_word_t)0x0d56c522cbf708fbULL << 64) | (body_weight_word_t)0x000539cbfcae4661ULL),
        (((body_weight_word_t)0xc4c6f920ecd1f5eaULL << 64) | (body_weight_word_t)0x50ffe82bf9d92333ULL), (((body_weight_word_t)0xd1180225ddbffa1eULL << 64) | (body_weight_word_t)0x27040a30ca150219ULL), (((body_weight_word_t)0xe521a934b74e3803ULL << 64) | (body_weight_word_t)0x392938a81a982b30ULL), (((body_weight_word_t)0x72910b219e90f581ULL << 64) | (body_weight_word_t)0x3f15ffe557aaf83eULL),
        (((body_weight_word_t)0xf3e03581d1df104aULL << 64) | (body_weight_word_t)0xcbfa183c9a254827ULL), (((body_weight_word_t)0xeee722edea2309f5ULL << 64) | (body_weight_word_t)0xea200dbde004fd21ULL), (((body_weight_word_t)0xe650f11310c618fdULL << 64) | (body_weight_word_t)0xd1f12daa1b14f8f2ULL), (((body_weight_word_t)0xfeeb30d3157017fbULL << 64) | (body_weight_word_t)0xf2e1feccf6a10c16ULL)
    },
    {
        (((body_weight_word_t)0x2508edd6a381f908ULL << 64) | (body_weight_word_t)0x23caeeff12dbd2cdULL), (((body_weight_word_t)0xf7e92213ee3408fcULL << 64) | (body_weight_word_t)0x0a04f9d30fe2cffdULL), (((body_weight_word_t)0x2dd513974acd0d33ULL << 64) | (body_weight_word_t)0xe2190e420a30161dULL), (((body_weight_word_t)0xfba5d802d7182214ULL << 64) | (body_weight_word_t)0x06e310471addcc6dULL),
        (((body_weight_word_t)0xfb0cecf1132705f0ULL << 64) | (body_weight_word_t)0xee251ff923f91ddcULL), (((body_weight_word_t)0x1ff2d6187f1981f5ULL << 64) | (body_weight_word_t)0x0a268deaa7d92120ULL), (((body_weight_word_t)0x14fed602fbecde3aULL << 64) | (body_weight_word_t)0x08cc81e7d616fb48ULL), (((body_weight_word_t)0xf73dec17d5e30b1cULL << 64) | (body_weight_word_t)0x0a06fccc3497edefULL),
        (((body_weight_word_t)0xf48ff323b4e6dfd2ULL << 64) | (body_weight_word_t)0xcce2e934a6eef20cULL), (((body_weight_word_t)0xef2816fa97dba73fULL << 64) | (body_weight_word_t)0xd681e7e49538cc5cULL), (((body_weight_word_t)0xefdd221fdc1718acULL << 64) | (body_weight_word_t)0x469e8d97fde1525eULL), (((body_weight_word_t)0xcaa204c29f16cbfbULL << 64) | (body_weight_word_t)0xc29df6195dfe7fa3ULL),
        (((body_weight_word_t)0xf5d433a035db016aULL << 64) | (body_weight_word_t)0xd6dd34440b1e4369ULL), (((body_weight_word_t)0x08dfece631e3e61cULL << 64) | (body_weight_word_t)0xe2edf22103fc21f0ULL), (((body_weight_word_t)0xc27410cbfdd0fce5ULL << 64) | (body_weight_word_t)0xf821f5b49a1c1702ULL), (((body_weight_word_t)0x03c931babc31f621ULL << 64) | (body_weight_word_t)0xdc297c01c1082fe1ULL)
    },
    {
        (((body_weight_word_t)0xcd03f0cfcce8fddbULL << 64) | (body_weight_word_t)0xf9f7a2a50fdbc812ULL), (((body_weight_word_t)0xfa200bfb1e1d0706ULL << 64) | (body_weight_word_t)0x1df417c8ff2cfa23ULL), (((body_weight_word_t)0xfdcd5dcc5119f359ULL << 64) | (body_weight_word_t)0xd2ea0690f9432af2ULL), (((body_weight_word_t)0x8108d64ae31212a8ULL << 64) | (body_weight_word_t)0x32f0e4d517ebbd05ULL),
        (((body_weight_word_t)0x0cf0140cd326f119ULL << 64) | (body_weight_word_t)0xf321290427d9f9feULL), (((body_weight_word_t)0xfbc5ef013eca11dcULL << 64) | (body_weight_word_t)0xd7f1ed04e9361713ULL), (((body_weight_word_t)0x1bc7eeefe6c4fcc5ULL << 64) | (body_weight_word_t)0xff03d8978e1619f3ULL), (((body_weight_word_t)0x051fd70db50e0423ULL << 64) | (body_weight_word_t)0xc508f7bc0dccea4eULL),
        (((body_weight_word_t)0xd71adf22b4f90da7ULL << 64) | (body_weight_word_t)0xf900f11ceafedc8fULL), (((body_weight_word_t)0xec18080cf206ac09ULL << 64) | (body_weight_word_t)0x0df6ffe7a9d2e204ULL), (((body_weight_word_t)0x0ee2e1db0fff3213ULL << 64) | (body_weight_word_t)0x1938fbf6c0f23df2ULL), (((body_weight_word_t)0x52a7de114715d737ULL << 64) | (body_weight_word_t)0x1a0d2de4d21011edULL),
        (((body_weight_word_t)0xe6c928b539e61940ULL << 64) | (body_weight_word_t)0x81c506f9d34a3e5cULL), (((body_weight_word_t)0x25ea160ae7b8f5e9ULL << 64) | (body_weight_word_t)0x11fef5e3d7f016f0ULL), (((body_weight_word_t)0xc9d31e300fc1ea11ULL << 64) | (body_weight_word_t)0x0fddeba89923db35ULL), (((body_weight_word_t)0x08fd11f61706ed0dULL << 64) | (body_weight_word_t)0xe72423f4232a0ee0ULL)
    },
    {
        (((body_weight_word_t)0x2afcf0eed5d5d4e2ULL << 64) | (body_weight_word_t)0xefe300e5dd17f226ULL), (((body_weight_word_t)0x09cde4f9c3ad0421ULL << 64) | (body_weight_word_t)0x0a27c7b61be9f0ebULL), (((body_weight_word_t)0xefcef2e33d24d532ULL << 64) | (body_weight_word_t)0xefef17dad91f050cULL), (((body_weight_word_t)0xe3f4f85fb9172326ULL << 64) | (body_weight_word_t)0x650fdf10d7c99e51ULL),
        (((body_weight_word_t)0x1fbaf5e4d4def914ULL << 64) | (body_weight_word_t)0xf4c611d9d20938f2ULL), (((body_weight_word_t)0xd230fbdd108c1810ULL << 64) | (body_weight_word_t)0x310b4fddfc33f9ceULL), (((body_weight_word_t)0x18daeeb5e2c0c82cULL << 64) | (body_weight_word_t)0xb1d442b1df1b2321ULL), (((body_weight_word_t)0x0773fe18d9100312ULL << 64) | (body_weight_word_t)0x310d0ee1fdc2f460ULL),
        (((body_weight_word_t)0xef05f744e8e10bf4ULL << 64) | (body_weight_word_t)0x5f07e520caf3e2e3ULL), (((body_weight_word_t)0x0d21d53cd70f0cd7ULL << 64) | (body_weight_word_t)0x3607faf62aede93cULL), (((body_weight_word_t)0xca77ec24db193bfbULL << 64) | (body_weight_word_t)0x0efff048f3c7c3dfULL), (((body_weight_word_t)0x4e28a01abead32eaULL << 64) | (body_weight_word_t)0x250f0cbc52147f1bULL),
        (((body_weight_word_t)0x09d126bb2df7ec27ULL << 64) | (body_weight_word_t)0xc7fe0615b4251c08ULL), (((body_weight_word_t)0x14e9e602e138fdd6ULL << 64) | (body_weight_word_t)0x0e210df8ed201fd2ULL), (((body_weight_word_t)0x0f3b0f1c23393d31ULL << 64) | (body_weight_word_t)0xeef813f13afa121aULL), (((body_weight_word_t)0x13ff0f01091f16feULL << 64) | (body_weight_word_t)0xf701db1019c72b0cULL)
    },
    {
        (((body_weight_word_t)0xeb39e4ffabe711feULL << 64) | (body_weight_word_t)0xd8e116fa03e5fa09ULL), (((body_weight_word_t)0x0708e7eee3af0820ULL << 64) | (body_weight_word_t)0x1d01d11a1bf906d4ULL), (((body_weight_word_t)0x17cd7fc319b6ed0aULL << 64) | (body_weight_word_t)0xba31e6452bf44c2aULL), (((body_weight_word_t)0xbb1dda6de2c6082eULL << 64) | (body_weight_word_t)0x50e0e4edb1e0a87fULL),
        (((body_weight_word_t)0x0e0cf3fe0ddafa25ULL << 64) | (body_weight_word_t)0x1418251d201727f6ULL), (((body_weight_word_t)0xd8e70fc12feb9af4ULL << 64) | (body_weight_word_t)0xd317ee3681002ee2ULL), (((body_weight_word_t)0xda08098493c9f03fULL << 64) | (body_weight_word_t)0xaaf12f19f0ea2a1eULL), (((body_weight_word_t)0x1179e7220d17eaf4ULL << 64) | (body_weight_word_t)0x1309ffc7e1c21146ULL),
        (((body_weight_word_t)0xc9bcea1e02fad521ULL << 64) | (body_weight_word_t)0x21e3ed15cbee0414ULL), (((body_weight_word_t)0x03e3cd51c5c0ece1ULL << 64) | (body_weight_word_t)0x32dbd00e570ee71dULL), (((body_weight_word_t)0xce07f811f081fd33ULL << 64) | (body_weight_word_t)0xfee502e3e807c4c5ULL), (((body_weight_word_t)0x22a1173118175dbdULL << 64) | (body_weight_word_t)0x4f0ede0279337f4dULL),
        (((body_weight_word_t)0xeecd19ce1afbf40fULL << 64) | (body_weight_word_t)0xb004101859154603ULL), (((body_weight_word_t)0xebfd1918e3b1111fULL << 64) | (body_weight_word_t)0x07e7f5d3182ef20fULL), (((body_weight_word_t)0x041202efef0b072cULL << 64) | (body_weight_word_t)0xf41318f5c200efc3ULL), (((body_weight_word_t)0x1b2a0ddd0c160331ULL << 64) | (body_weight_word_t)0xd4f136d0c912ededULL)
    },
    {
        (((body_weight_word_t)0x11e2ffefbbf4f46aULL << 64) | (body_weight_word_t)0xce06f8d6d6fee609ULL), (((body_weight_word_t)0x022cdc0d0d20fe1dULL << 64) | (body_weight_word_t)0x2a0acb30fee4e31cULL), (((body_weight_word_t)0xf3e3fba2f2c9eb30ULL << 64) | (body_weight_word_t)0xcfff7fec023a672eULL), (((body_weight_word_t)0xd0d6cc7bc8fd18ebULL << 64) | (body_weight_word_t)0x4c05b2bd21cfbaedULL),
        (((body_weight_word_t)0x03e1050cdaf206dbULL << 64) | (body_weight_word_t)0x2911d5de04cde9f7ULL), (((body_weight_word_t)0xf8de12dd75e71d21ULL << 64) | (body_weight_word_t)0xe5f8ea28d622f101ULL), (((body_weight_word_t)0xfeb907ab04d80a41ULL << 64) | (body_weight_word_t)0xf1fb25afd6413024ULL), (((body_weight_word_t)0x1b70f4030311fed9ULL << 64) | (body_weight_word_t)0xd216efaedaaf814cULL),
        (((body_weight_word_t)0xe1cfd152c4f301ceULL << 64) | (body_weight_word_t)0x27f5c30529e81210ULL), (((body_weight_word_t)0x16eada33b7f107f3ULL << 64) | (body_weight_word_t)0x55f1edc213e8d0e7ULL), (((body_weight_word_t)0xf2e8f7fdb4ebf60fULL << 64) | (body_weight_word_t)0xf8f0f4d2cc140907ULL), (((body_weight_word_t)0x1df5bff6fae908cdULL << 64) | (body_weight_word_t)0xbce011dad601f20cULL),
        (((body_weight_word_t)0xefe60dae0b051524ULL << 64) | (body_weight_word_t)0x05fe1e46d20d3805ULL), (((body_weight_word_t)0xedfae3f000f803f9ULL << 64) | (body_weight_word_t)0xfbd237fc02130b30ULL), (((body_weight_word_t)0xc6fc0efb201f0814ULL << 64) | (body_weight_word_t)0xf4dd1710970e03ecULL), (((body_weight_word_t)0x0b13f5fd0c1802edULL << 64) | (body_weight_word_t)0xe50403f401e2e7fcULL)
    }
};
static const bias_t L2_BIAS[CNN_C] = {
    -2016, 956, -1310, -100, -2921, -3029, -2951, -986,
    1438, -81, -1814, -1722, -562, -881, -22, -485
};
static const q31_t L2_MAIN_Q31[CNN_C] = {
    1778696960, 1081042688, 1132263936, 1263189376,
    1441819264, 1745399936, 1945350912, 1120792320,
    2137065600, 1108352640, 1303577216, 1487316608,
    1705760256, 1353778560, 1913878528, 1537619200
};
static const qexp_t L2_MAIN_EXP[CNN_C] = {
    -9, -8, -8, -8, -9, -9, -9, -8,
    -9, -7, -9, -9, -8, -8, -9, -8
};
static const q31_t L2_SKIP_Q31 = 1439926547;
static const qexp_t L2_SKIP_EXP = 1;

// L3
static const body_weight_word_t L3_WBANK[9][CNN_C] = {
    {
        (((body_weight_word_t)0xeef21b1619080e10ULL << 64) | (body_weight_word_t)0xe804f244efdff6e9ULL), (((body_weight_word_t)0xb0f823e3ebf403f1ULL << 64) | (body_weight_word_t)0x35f0d6262143cedeULL), (((body_weight_word_t)0xfb2727fd1ece4d67ULL << 64) | (body_weight_word_t)0x3617f61417e70fe6ULL), (((body_weight_word_t)0xda16d80f2ddb1effULL << 64) | (body_weight_word_t)0x08f8fed7f3dcd9feULL),
        (((body_weight_word_t)0xe31b3fd716b50219ULL << 64) | (body_weight_word_t)0x7ffdd9e6f4280bf3ULL), (((body_weight_word_t)0x280439e774f04351ULL << 64) | (body_weight_word_t)0x94054437f0bc01b7ULL), (((body_weight_word_t)0x13f68115812fb998ULL << 64) | (body_weight_word_t)0xcb1afb0ce4ebd13cULL), (((body_weight_word_t)0xf22efdf84ff71ae9ULL << 64) | (body_weight_word_t)0x21320e0f26f50917ULL),
        (((body_weight_word_t)0x00e3cdfbc4f1fb12ULL << 64) | (body_weight_word_t)0xc206e6e5fae2b80cULL), (((body_weight_word_t)0xea0ed5f9fc05f812ULL << 64) | (body_weight_word_t)0x0cef06f607f22513ULL), (((body_weight_word_t)0x1617fff23eff1decULL << 64) | (body_weight_word_t)0xe9022efaf0e3f507ULL), (((body_weight_word_t)0x08091336e3e52907ULL << 64) | (body_weight_word_t)0x04d3e4fa1b10f71aULL),
        (((body_weight_word_t)0xcb2ad80005d60406ULL << 64) | (body_weight_word_t)0x2f3101ed0107e11dULL), (((body_weight_word_t)0x08f2e00fde03f1d0ULL << 64) | (body_weight_word_t)0x1203cb1805c72305ULL), (((body_weight_word_t)0x7f0037120febf4ceULL << 64) | (body_weight_word_t)0x3529ed1be7819f22ULL), (((body_weight_word_t)0x2e0527f0f307320eULL << 64) | (body_weight_word_t)0xee0321300fdae8f8ULL)
    },
    {
        (((body_weight_word_t)0xebeafe08a409f8f2ULL << 64) | (body_weight_word_t)0x12d90414fcf9c2d8ULL), (((body_weight_word_t)0x0bfcf2ff1834f511ULL << 64) | (body_weight_word_t)0x400c022be6f7bdb6ULL), (((body_weight_word_t)0xde32fed6b3083d05ULL << 64) | (body_weight_word_t)0x141702c6fb4ceeeeULL), (((body_weight_word_t)0xf1450d0b3ddb14eeULL << 64) | (body_weight_word_t)0x040308330e9ff843ULL),
        (((body_weight_word_t)0xca1007f6f905c6bbULL << 64) | (body_weight_word_t)0x3fffd6dad32c1911ULL), (((body_weight_word_t)0x3606500afd245149ULL << 64) | (body_weight_word_t)0xfbf5040617d8dd11ULL), (((body_weight_word_t)0x1cead6619bd9c0b3ULL << 64) | (body_weight_word_t)0xa2cdf5c7470cd774ULL), (((body_weight_word_t)0xf9163cf221fd3117ULL << 64) | (body_weight_word_t)0x480402db06f74126ULL),
        (((body_weight_word_t)0xf8e5ece1dceeff1dULL << 64) | (body_weight_word_t)0xd705e70b2513d0d2ULL), (((body_weight_word_t)0x1102020426ca2303ULL << 64) | (body_weight_word_t)0x37f90af307f90c11ULL), (((body_weight_word_t)0x1f22f6d047c52503ULL << 64) | (body_weight_word_t)0x3c05d1dfc70bd1e1ULL), (((body_weight_word_t)0xe8f01fc8bfc408dbULL << 64) | (body_weight_word_t)0xfdeb240b311ef23bULL),
        (((body_weight_word_t)0xe2100df327d714e7ULL << 64) | (body_weight_word_t)0x2b16f9edf92716f7ULL), (((body_weight_word_t)0x14f40000e4d9feb9ULL << 64) | (body_weight_word_t)0x44fdc61226dd34e8ULL), (((body_weight_word_t)0x580afdcdecedf314ULL << 64) | (body_weight_word_t)0x3e1b13e6e2deccb6ULL), (((body_weight_word_t)0x3cf40ed81f061ef3ULL << 64) | (body_weight_word_t)0xedfcf905c30e2fc7ULL)
    },
    {
        (((body_weight_word_t)0xeeee0b01092b17f8ULL << 64) | (body_weight_word_t)0x0ce2d501e71bcadcULL), (((body_weight_word_t)0x3fe6fff0ad4b2c0dULL << 64) | (body_weight_word_t)0x06ced86e0509e4d1ULL), (((body_weight_word_t)0xcc01fb113ffbebd5ULL << 64) | (body_weight_word_t)0xcaf105e91331160fULL), (((body_weight_word_t)0xf8111dfb1cfe2ffeULL << 64) | (body_weight_word_t)0x11f9f400f928291bULL),
        (((body_weight_word_t)0xe0d8f0dadd0ef2ebULL << 64) | (body_weight_word_t)0x16140a12111015faULL), (((body_weight_word_t)0x24002e1d01e3e419ULL << 64) | (body_weight_word_t)0xecea00050417be09ULL), (((body_weight_word_t)0xfae6d1eabbf0531cULL << 64) | (body_weight_word_t)0x0c04c6e60ff8ea4cULL), (((body_weight_word_t)0x0f174a0410f00df1ULL << 64) | (body_weight_word_t)0x0043fce10aed1841ULL),
        (((body_weight_word_t)0x09e704f7fcf6e8f9ULL << 64) | (body_weight_word_t)0xcddb31d8f608c29dULL), (((body_weight_word_t)0x0402f4fed7011df8ULL << 64) | (body_weight_word_t)0x03de04070a1de903ULL), (((body_weight_word_t)0x1227170c4d1db5e6ULL << 64) | (body_weight_word_t)0xcbc948f9fc11d4c4ULL), (((body_weight_word_t)0xfff52016c30410dcULL << 64) | (body_weight_word_t)0xcaf7de0bee06e326ULL),
        (((body_weight_word_t)0xe12def111bce25f6ULL << 64) | (body_weight_word_t)0x3b181eff0900e70aULL), (((body_weight_word_t)0x061417f8cafc5f1fULL << 64) | (body_weight_word_t)0x2ceae56ff710ed45ULL), (((body_weight_word_t)0x2af21012074abfe9ULL << 64) | (body_weight_word_t)0xfe0ef413f81081d5ULL), (((body_weight_word_t)0x07fb280b280b12f3ULL << 64) | (body_weight_word_t)0x1003ff1a0f2615edULL)
    },
    {
        (((body_weight_word_t)0xd4352a0e39ab26b6ULL << 64) | (body_weight_word_t)0x0a24e5dcebca0806ULL), (((body_weight_word_t)0x2ff40be0e11901c2ULL << 64) | (body_weight_word_t)0x110aebfa3dff0fe0ULL), (((body_weight_word_t)0x12f010faef1422dbULL << 64) | (body_weight_word_t)0xeb280df21ae2bcfeULL), (((body_weight_word_t)0x2cf2021b100f0108ULL << 64) | (body_weight_word_t)0x301302fbe4e12917ULL),
        (((body_weight_word_t)0xd3e1161d5c45f9ceULL << 64) | (body_weight_word_t)0x48ecf10cde3f2badULL), (((body_weight_word_t)0xb92e277f2b1f6758ULL << 64) | (body_weight_word_t)0xc704e8f41e547a41ULL), (((body_weight_word_t)0x2d288db9292eebbfULL << 64) | (body_weight_word_t)0xf6161bb5e0cf0c3fULL), (((body_weight_word_t)0xbc1d08184b051cfaULL << 64) | (body_weight_word_t)0x63310fbc0432044bULL),
        (((body_weight_word_t)0x20debdf0cbfad4f8ULL << 64) | (body_weight_word_t)0x0dfd10e00cc7b90cULL), (((body_weight_word_t)0x0c1a16fd330e20ecULL << 64) | (body_weight_word_t)0x14f20b0dffc3933aULL), (((body_weight_word_t)0xf4080a42232309fdULL << 64) | (body_weight_word_t)0x0e1810dde80e1918ULL), (((body_weight_word_t)0x39f0f801f9bc242aULL << 64) | (body_weight_word_t)0x5feef52d1caac9c7ULL),
        (((body_weight_word_t)0xdc23fe09f20adadbULL << 64) | (body_weight_word_t)0xfb23fadf0002e91dULL), (((body_weight_word_t)0xfc04deff0420f517ULL << 64) | (body_weight_word_t)0x17f800cbd8240736ULL), (((body_weight_word_t)0x3fd7321018d8ebe9ULL << 64) | (body_weight_word_t)0x20131939e1aa78dfULL), (((body_weight_word_t)0x222a1402f3140ef6ULL << 64) | (body_weight_word_t)0x2c3f17fa10bbec0fULL)
    },
    {
        (((body_weight_word_t)0x060ff6f015f627d9ULL << 64) | (body_weight_word_t)0xdcf830ba200ae3ccULL), (((body_weight_word_t)0x252affd40921ce32ULL << 64) | (body_weight_word_t)0xda0402d3fda1e33dULL), (((body_weight_word_t)0xfd27dff8eb420220ULL << 64) | (body_weight_word_t)0x40f107e9f13481d1ULL), (((body_weight_word_t)0x180ffb1b29ee2c08ULL << 64) | (body_weight_word_t)0x1d1816e8f8a4bd15ULL),
        (((body_weight_word_t)0xc6def504343ec973ULL << 64) | (body_weight_word_t)0x53e3d4f3921406dcULL), (((body_weight_word_t)0x97810b2d3a7f12faULL << 64) | (body_weight_word_t)0xf0191bf37f5970cfULL), (((body_weight_word_t)0x31f40634fb5dbb31ULL << 64) | (body_weight_word_t)0x29eb7ffc4804e722ULL), (((body_weight_word_t)0xf4ed13ff160f1c12ULL << 64) | (body_weight_word_t)0x40f6f6b9d4e81f39ULL),
        (((body_weight_word_t)0xdef0091411e4ce41ULL << 64) | (body_weight_word_t)0x0bdde31b0bebdf13ULL), (((body_weight_word_t)0x2b22481223fbd9f4ULL << 64) | (body_weight_word_t)0x31120adffa330d2bULL), (((body_weight_word_t)0xf9db2000e7e52781ULL << 64) | (body_weight_word_t)0x0d3f5fdee3eb1ad6ULL), (((body_weight_word_t)0x33f1054723023312ULL << 64) | (body_weight_word_t)0xfbe7943a30b3ab1fULL),
        (((body_weight_word_t)0xff1bef1405f4d80bULL << 64) | (body_weight_word_t)0xeef41bd9fbeb151bULL), (((body_weight_word_t)0xc823f097b50b1f07ULL << 64) | (body_weight_word_t)0x8f67d9ccfc701224ULL), (((body_weight_word_t)0x19ed2825ebe702e4ULL << 64) | (body_weight_word_t)0x2be90ad5e1ea6218ULL), (((body_weight_word_t)0x02041d06f409fa0bULL << 64) | (body_weight_word_t)0x0f15f5e4a5b6f525ULL)
    },
    {
        (((body_weight_word_t)0x1b062b030a321608ULL << 64) | (body_weight_word_t)0xf9ccf80c1af8b4cdULL), (((body_weight_word_t)0x69d815f0f2071125ULL << 64) | (body_weight_word_t)0xc50e1d1afa0df7ecULL), (((body_weight_word_t)0xefd2021330e7a4c6ULL << 64) | (body_weight_word_t)0xb3b932ec2202b825ULL), (((body_weight_word_t)0x5605f1ff32fafbe9ULL << 64) | (body_weight_word_t)0x001128d016e4b427ULL),
        (((body_weight_word_t)0x1828f91dd10d0822ULL << 64) | (body_weight_word_t)0xefd5dffce9cb1feeULL), (((body_weight_word_t)0xeece1fc3c9563523ULL << 64) | (body_weight_word_t)0x0de0e9291d2731f2ULL), (((body_weight_word_t)0x1b09da1dd2313e1bULL << 64) | (body_weight_word_t)0x14d832dddaffdf27ULL), (((body_weight_word_t)0x06010df5350b1ddaULL << 64) | (body_weight_word_t)0x22f10b02f822c303ULL),
        (((body_weight_word_t)0xe10dd8d9ca07141bULL << 64) | (body_weight_word_t)0xb4fb05f612ff0c8fULL), (((body_weight_word_t)0xf7f1eff8271ce1f5ULL << 64) | (body_weight_word_t)0xfe0315081efac2fcULL), (((body_weight_word_t)0xdd0408b25402bbc2ULL << 64) | (body_weight_word_t)0xba05b203f009e701ULL), (((body_weight_word_t)0x24052d05e7f93228ULL << 64) | (body_weight_word_t)0x38e932b2edde9f0aULL),
        (((body_weight_word_t)0x02f02f02e90d09f1ULL << 64) | (body_weight_word_t)0x02123614edf6ee24ULL), (((body_weight_word_t)0xa2ecdfb6c22e7f0dULL << 64) | (body_weight_word_t)0xeef5d57f2664fb12ULL), (((body_weight_word_t)0xe0194b2d3e0ad22fULL << 64) | (body_weight_word_t)0x0dd5190ed7370e35ULL), (((body_weight_word_t)0xfcf84404e002f7faULL << 64) | (body_weight_word_t)0x2a05e74ad7112ef3ULL)
    },
    {
        (((body_weight_word_t)0xdbf0072fecd61c02ULL << 64) | (body_weight_word_t)0x2f2f1012d4d1f3deULL), (((body_weight_word_t)0x03f7c508f417baf4ULL << 64) | (body_weight_word_t)0xf5d8eb0c140728d7ULL), (((body_weight_word_t)0x150536e8f2f6f606ULL << 64) | (body_weight_word_t)0xd9f70b0ff30f38dfULL), (((body_weight_word_t)0x04e0030413fefe08ULL << 64) | (body_weight_word_t)0xfafae8ebf71a3607ULL),
        (((body_weight_word_t)0xeff5e8193704b3f1ULL << 64) | (body_weight_word_t)0x4efdf70f2528e8e8ULL), (((body_weight_word_t)0xc409f134d0d8cd19ULL << 64) | (body_weight_word_t)0xc449e107014a00b4ULL), (((body_weight_word_t)0x034defee15ec3b27ULL << 64) | (body_weight_word_t)0xe252f6ef0f0618c2ULL), (((body_weight_word_t)0xdd11ed0f29eaf1f8ULL << 64) | (body_weight_word_t)0x2ff60cc9f4009a21ULL),
        (((body_weight_word_t)0xee3ccdfa1234dee1ULL << 64) | (body_weight_word_t)0x091122c7fff3323fULL), (((body_weight_word_t)0xf50fdbeb00090312ULL << 64) | (body_weight_word_t)0x1131e5e2081df435ULL), (((body_weight_word_t)0xe50312fa0eedfb2fULL << 64) | (body_weight_word_t)0xf130f82f460225eaULL), (((body_weight_word_t)0x0feddce3d0f60ceaULL << 64) | (body_weight_word_t)0x25f3eddeeaee23f6ULL),
        (((body_weight_word_t)0xf91bd709fff4b2dfULL << 64) | (body_weight_word_t)0xfe140bdfffd015f4ULL), (((body_weight_word_t)0x0be4e814f1fae620ULL << 64) | (body_weight_word_t)0xe20bce0c1c0cffe0ULL), (((body_weight_word_t)0xf117163630f6d9f2ULL << 64) | (body_weight_word_t)0x3a54112117fb7305ULL), (((body_weight_word_t)0xf5033606140ad8fbULL << 64) | (body_weight_word_t)0xed0e240205f72c01ULL)
    },
    {
        (((body_weight_word_t)0xf2e74ef3e0ffff0eULL << 64) | (body_weight_word_t)0xf41915ffe2f8f00bULL), (((body_weight_word_t)0x1819e2e6d2f5dc25ULL << 64) | (body_weight_word_t)0x2b08cef821f821cbULL), (((body_weight_word_t)0xf4ad091ee601591bULL << 64) | (body_weight_word_t)0xe5b72178fc25f438ULL), (((body_weight_word_t)0x390fd8fb05fe01fcULL << 64) | (body_weight_word_t)0x0a02f9de06e4fd0cULL),
        (((body_weight_word_t)0x1a0105470ff8c316ULL << 64) | (body_weight_word_t)0x20ea1dd752f8d8d8ULL), (((body_weight_word_t)0xa0e933f0f5ef98d2ULL << 64) | (body_weight_word_t)0x16654fd0f3252804ULL), (((body_weight_word_t)0xeedde700b80b733aULL << 64) | (body_weight_word_t)0xfd27326dce2f145fULL), (((body_weight_word_t)0x2a1a1ef525202722ULL << 64) | (body_weight_word_t)0xef01f8d0fe11cf09ULL),
        (((body_weight_word_t)0x072cc807feeb04e0ULL << 64) | (body_weight_word_t)0xdffbefeae1f348e3ULL), (((body_weight_word_t)0xf10808051e0113faULL << 64) | (body_weight_word_t)0x3803f2ec10e3b645ULL), (((body_weight_word_t)0xe5f5ea0d210b670cULL << 64) | (body_weight_word_t)0xf85724181c04137fULL), (((body_weight_word_t)0x0c230a31aa20ddcaULL << 64) | (body_weight_word_t)0x02daf5c3b1e9081bULL),
        (((body_weight_word_t)0x04040f23e512d2fdULL << 64) | (body_weight_word_t)0xee04f0eecfff2811ULL), (((body_weight_word_t)0xdbd802fb0bfbdc26ULL << 64) | (body_weight_word_t)0xee5e37cdf942fbcbULL), (((body_weight_word_t)0xdbdeef2622e1ede3ULL << 64) | (body_weight_word_t)0x1849032edd3d50f1ULL), (((body_weight_word_t)0xe51b0dd9ddfcdadeULL << 64) | (body_weight_word_t)0x04f225fde91237e3ULL)
    },
    {
        (((body_weight_word_t)0xd1e32b01010926faULL << 64) | (body_weight_word_t)0x0df004f00a1ed0d1ULL), (((body_weight_word_t)0x0ec5d2ea0130e115ULL << 64) | (body_weight_word_t)0x2521ac2119312be4ULL), (((body_weight_word_t)0x14f940e0d5f403f0ULL << 64) | (body_weight_word_t)0x90810e1c1fee0b1eULL), (((body_weight_word_t)0x17fe0906e2081204ULL << 64) | (body_weight_word_t)0xd4ebf713f102dde6ULL),
        (((body_weight_word_t)0x44fcc2f5e5ee0202ULL << 64) | (body_weight_word_t)0x07ff0df421ecf2dcULL), (((body_weight_word_t)0xe2b3211414eac900ULL << 64) | (body_weight_word_t)0x3b3905cec8141ae4ULL), (((body_weight_word_t)0xe617e4dd1d033a0fULL << 64) | (body_weight_word_t)0xebcb0a58e211ef1cULL), (((body_weight_word_t)0x08ece717010915f6ULL << 64) | (body_weight_word_t)0x1f1be812fe0cbc11ULL),
        (((body_weight_word_t)0xea0dc8f4d31504fdULL << 64) | (body_weight_word_t)0xd7ff07f80de420edULL), (((body_weight_word_t)0x0202f0e903011703ULL << 64) | (body_weight_word_t)0x06f8e3e1fafc4d00ULL), (((body_weight_word_t)0xf11b01ac101e36f8ULL << 64) | (body_weight_word_t)0x11e50b0c2df728f7ULL), (((body_weight_word_t)0xfc1ff6e5f7f70be1ULL << 64) | (body_weight_word_t)0x2fcd02a9b80402b2ULL),
        (((body_weight_word_t)0x0f0bcc08fe06e4e8ULL << 64) | (body_weight_word_t)0x3b1f0ac6e4c21c20ULL), (((body_weight_word_t)0xeed7121ed0f60808ULL << 64) | (body_weight_word_t)0xe2f0e924f5082d43ULL), (((body_weight_word_t)0xac0a3a0e14e8b513ULL << 64) | (body_weight_word_t)0xf8f1eadb45555831ULL), (((body_weight_word_t)0xe7f9461610ed1636ULL << 64) | (body_weight_word_t)0x0a04e0f815214cd9ULL)
    }
};
static const bias_t L3_BIAS[CNN_C] = {
    -210, -1369, -690, -2264, -254, 1900, -217, 904,
    -3087, -18785, -916, 1258, 2193, -14209, 41, -1023
};
static const q31_t L3_Q31[CNN_C] = {
    1798244864, 1670798976, 1785132672, 1828690560,
    1213601664, 2002852736, 2044977408, 1602863616,
    1496851840, 1328290432, 1797194112, 1561657344,
    1818315776, 1387812736, 1195281792, 2101645952
};
static const qexp_t L3_EXP[CNN_C] = {
    -8, -8, -8, -8, -8, -8, -8, -8,
    -8, -8, -8, -8, -8, -8, -7, -8
};

// L4
static const body_weight_word_t L4_WBANK[9][CNN_C] = {
    {
        (((body_weight_word_t)0xe6f22bd79de303baULL << 64) | (body_weight_word_t)0x3811e9fc0120ddceULL), (((body_weight_word_t)0xf0e9f0081df63401ULL << 64) | (body_weight_word_t)0x1efffe23f21c01feULL), (((body_weight_word_t)0x00fde7f4da0afaf6ULL << 64) | (body_weight_word_t)0x15f7f9041b1c01cbULL), (((body_weight_word_t)0xf1d0180f2fd1159aULL << 64) | (body_weight_word_t)0xe0132ce9e4f7f607ULL),
        (((body_weight_word_t)0x07fff82534fb17d4ULL << 64) | (body_weight_word_t)0x1bf8fce2e61bf8e6ULL), (((body_weight_word_t)0xe2c50ed202fd2ee5ULL << 64) | (body_weight_word_t)0x06f7f623f1fbf92dULL), (((body_weight_word_t)0x0e01effbe611e1cfULL << 64) | (body_weight_word_t)0x42112a015cd52034ULL), (((body_weight_word_t)0x0ded021829d21ae6ULL << 64) | (body_weight_word_t)0xfee9393018c730fcULL),
        (((body_weight_word_t)0xf11937e7fa0d151cULL << 64) | (body_weight_word_t)0xf10b0f33d70f15f0ULL), (((body_weight_word_t)0xf8f2310ab4f1f2e5ULL << 64) | (body_weight_word_t)0xc6f41806da1c0e35ULL), (((body_weight_word_t)0xff0b0f05e5debdd3ULL << 64) | (body_weight_word_t)0xe8050c060f0dde07ULL), (((body_weight_word_t)0x171fe00bbb120d09ULL << 64) | (body_weight_word_t)0xdd0712cff6efe630ULL),
        (((body_weight_word_t)0x46eef900414d363fULL << 64) | (body_weight_word_t)0xea1d212d14d92b12ULL), (((body_weight_word_t)0x07d7f8d0f806f7f1ULL << 64) | (body_weight_word_t)0x1a0ef12deb233fe5ULL), (((body_weight_word_t)0xdae6be1613e9211eULL << 64) | (body_weight_word_t)0xc40cbf0ad918f3fbULL), (((body_weight_word_t)0xf41808f6e1e3fae1ULL << 64) | (body_weight_word_t)0xf31212040bfbf8f4ULL)
    },
    {
        (((body_weight_word_t)0xd6d9a61c0b2838dfULL << 64) | (body_weight_word_t)0x0e1c12f4fa3ecb01ULL), (((body_weight_word_t)0xef1cb5e8f5fd20d4ULL << 64) | (body_weight_word_t)0x05010108e1062ff9ULL), (((body_weight_word_t)0xea03ca341004100cULL << 64) | (body_weight_word_t)0x0cfc050808bc19e9ULL), (((body_weight_word_t)0xfafefa192cfbe5d2ULL << 64) | (body_weight_word_t)0xde2d12f1f7fadd35ULL),
        (((body_weight_word_t)0x10e61e0f31f6201fULL << 64) | (body_weight_word_t)0x18e6303edec1dbf3ULL), (((body_weight_word_t)0xeadfe85a01034ff0ULL << 64) | (body_weight_word_t)0x220f3135d8f0233eULL), (((body_weight_word_t)0xfec128febdfefafeULL << 64) | (body_weight_word_t)0x55042feb61b0de26ULL), (((body_weight_word_t)0xee17e32fa0060aeaULL << 64) | (body_weight_word_t)0xc0f817020e1860d1ULL),
        (((body_weight_word_t)0x0e38df1948fe471cULL << 64) | (body_weight_word_t)0x11e93c0b00d6f2d6ULL), (((body_weight_word_t)0x01e41f181ed2ee1aULL << 64) | (body_weight_word_t)0xefc72bfcf7d017f9ULL), (((body_weight_word_t)0xf9ed2ce6e61a22f4ULL << 64) | (body_weight_word_t)0xdb250b26f12801f8ULL), (((body_weight_word_t)0x3cf123b8a6211806ULL << 64) | (body_weight_word_t)0xdcd3df06febdfcd8ULL),
        (((body_weight_word_t)0x5118e8a7d6d6991aULL << 64) | (body_weight_word_t)0x53e7d5e8fcfdc7c8ULL), (((body_weight_word_t)0x0400d0ffe616cc08ULL << 64) | (body_weight_word_t)0x0a18dbe9fc2c12c2ULL), (((body_weight_word_t)0xd418c70f2207f81cULL << 64) | (body_weight_word_t)0xd7ece608e6c70ae2ULL), (((body_weight_word_t)0xfb031d0bd2f839fdULL << 64) | (body_weight_word_t)0xf20911f61b1ed1ffULL)
    },
    {
        (((body_weight_word_t)0xd8eb1d09f9f3265bULL << 64) | (body_weight_word_t)0x14e1e42d49010fc7ULL), (((body_weight_word_t)0xe30e04f317eb1cf3ULL << 64) | (body_weight_word_t)0x38f8ded3e619f5e7ULL), (((body_weight_word_t)0xe8e808e2eafed3e4ULL << 64) | (body_weight_word_t)0x00f9eaee09fce5ffULL), (((body_weight_word_t)0x02e308d606f0d91fULL << 64) | (body_weight_word_t)0x2600fb10090ffe06ULL),
        (((body_weight_word_t)0xfe23d1fa1e070d23ULL << 64) | (body_weight_word_t)0x4e11fd0adfeafbc7ULL), (((body_weight_word_t)0xe5e9b6d6f20d0bf3ULL << 64) | (body_weight_word_t)0x001cd9f4300319eaULL), (((body_weight_word_t)0xfa011ef8c0020df4ULL << 64) | (body_weight_word_t)0x061f49054bf6bd46ULL), (((body_weight_word_t)0xf201c35e5bcbdd8fULL << 64) | (body_weight_word_t)0xdadecb07e5df25e8ULL),
        (((body_weight_word_t)0xdb06cdfcfef00002ULL << 64) | (body_weight_word_t)0x09eff847cddaed09ULL), (((body_weight_word_t)0x02e3d71eb9ffcfdbULL << 64) | (body_weight_word_t)0xfbd9313e23ec0c2aULL), (((body_weight_word_t)0x0ffcfd05e005f392ULL << 64) | (body_weight_word_t)0xf609330bf32309f8ULL), (((body_weight_word_t)0x0fd80ccdf906a849ULL << 64) | (body_weight_word_t)0xfdcef33bcef2ea08ULL),
        (((body_weight_word_t)0xe04b3924fd1b5350ULL << 64) | (body_weight_word_t)0xf4f71d0ab5f42e0aULL), (((body_weight_word_t)0xd9edf215c20215d0ULL << 64) | (body_weight_word_t)0x272bfdf4dfe446dbULL), (((body_weight_word_t)0x1e07d53c34f11217ULL << 64) | (body_weight_word_t)0x074df0b40034fa2bULL), (((body_weight_word_t)0xf511dd1edb0b03f0ULL << 64) | (body_weight_word_t)0x03062227dde3e805ULL)
    },
    {
        (((body_weight_word_t)0xfaed3912e53a1744ULL << 64) | (body_weight_word_t)0xe128e2e6adee1fd3ULL), (((body_weight_word_t)0xf8ebd42744e119c1ULL << 64) | (body_weight_word_t)0x1305db29f806f4e1ULL), (((body_weight_word_t)0xe904db2ce2d71cd2ULL << 64) | (body_weight_word_t)0xe404ec131a0eedddULL), (((body_weight_word_t)0x0d03d94cf405101cULL << 64) | (body_weight_word_t)0x1628dffe33d509f5ULL),
        (((body_weight_word_t)0xc6fb901c4cfe0021ULL << 64) | (body_weight_word_t)0xb72af01942e00bc1ULL), (((body_weight_word_t)0xdf271c810ebacd39ULL << 64) | (body_weight_word_t)0x02f9171d484403ffULL), (((body_weight_word_t)0xe4fefbfcec27bfc7ULL << 64) | (body_weight_word_t)0x2cebf79d14dc4c7aULL), (((body_weight_word_t)0xdf3217bf7fc2c2f5ULL << 64) | (body_weight_word_t)0xebec073300f324e7ULL),
        (((body_weight_word_t)0x052017dcee27feefULL << 64) | (body_weight_word_t)0x33eefd0a270b020bULL), (((body_weight_word_t)0xcb25a71107440701ULL << 64) | (body_weight_word_t)0xf5ddceed0d42fadfULL), (((body_weight_word_t)0xfe15cae9cb2feafeULL << 64) | (body_weight_word_t)0x22d29fe24d18ffcaULL), (((body_weight_word_t)0x2c230deee149e727ULL << 64) | (body_weight_word_t)0xb5d015bf4d0908cdULL),
        (((body_weight_word_t)0xbf1851494f2d4d12ULL << 64) | (body_weight_word_t)0x1b15be1332eb06e0ULL), (((body_weight_word_t)0xe5cde6fe0207ddf3ULL << 64) | (body_weight_word_t)0xeb0d0710c70a0d18ULL), (((body_weight_word_t)0xf4defe15200e043bULL << 64) | (body_weight_word_t)0xc8feec3d0d1ce681ULL), (((body_weight_word_t)0x030a61f40410f519ULL << 64) | (body_weight_word_t)0xf8f1f7edef34f8fcULL)
    },
    {
        (((body_weight_word_t)0x02b7378ef8aaaff9ULL << 64) | (body_weight_word_t)0xe0fcdbdb0e102cffULL), (((body_weight_word_t)0xde1ce5ec63d219f1ULL << 64) | (body_weight_word_t)0xdadcd607e01002fcULL), (((body_weight_word_t)0x250ea0214323330eULL << 64) | (body_weight_word_t)0x2401051c3db13fd6ULL), (((body_weight_word_t)0x580e101d981efadbULL << 64) | (body_weight_word_t)0x28cae67f470f06f5ULL),
        (((body_weight_word_t)0x0e3b482fed123912ULL << 64) | (body_weight_word_t)0x36f6181f0c15cde1ULL), (((body_weight_word_t)0x0bbfe3ce9316c01dULL << 64) | (body_weight_word_t)0x170a0f12f73db2c8ULL), (((body_weight_word_t)0xd2c34805fa19cfd7ULL << 64) | (body_weight_word_t)0x5f29ee010fbf04f9ULL), (((body_weight_word_t)0x131c27265fe2ea1eULL << 64) | (body_weight_word_t)0xf912fb49ab4ffbcbULL),
        (((body_weight_word_t)0xe93734f5c23d22c1ULL << 64) | (body_weight_word_t)0xd418e38ef4ced51aULL), (((body_weight_word_t)0xe3c7d34aa702f32bULL << 64) | (body_weight_word_t)0xfd400c1eed253242ULL), (((body_weight_word_t)0xec1c1a2de90940f5ULL << 64) | (body_weight_word_t)0x058181810f9b42e8ULL), (((body_weight_word_t)0xe4ff19f030c63526ULL << 64) | (body_weight_word_t)0xc5f94154f270c920ULL),
        (((body_weight_word_t)0xe1d881a87f818162ULL << 64) | (body_weight_word_t)0x811b4fe89d7f7fefULL), (((body_weight_word_t)0xfdfd0c25c7f31a32ULL << 64) | (body_weight_word_t)0x0a27d701e92b2de4ULL), (((body_weight_word_t)0x21523a969ce7210aULL << 64) | (body_weight_word_t)0x2ae29bfd2f413537ULL), (((body_weight_word_t)0x05d34afcd800e709ULL << 64) | (body_weight_word_t)0x31fe022edafefa05ULL)
    },
    {
        (((body_weight_word_t)0xfccd64130a3029c9ULL << 64) | (body_weight_word_t)0x432b19d059253900ULL), (((body_weight_word_t)0xf810e42604d50cd9ULL << 64) | (body_weight_word_t)0x05f4c5f5d7f20503ULL), (((body_weight_word_t)0x0a16dff020fa15feULL << 64) | (body_weight_word_t)0x951ad6902923f31eULL), (((body_weight_word_t)0x16132706ee0eee11ULL << 64) | (body_weight_word_t)0xd00ff10e33eef714ULL),
        (((body_weight_word_t)0xee05f25793084a26ULL << 64) | (body_weight_word_t)0x3838fbef06c9e81aULL), (((body_weight_word_t)0xcf0611f204eb2eceULL << 64) | (body_weight_word_t)0xeae8df0915020744ULL), (((body_weight_word_t)0xdbfffacde610cb52ULL << 64) | (body_weight_word_t)0x08cf12f9e8feac02ULL), (((body_weight_word_t)0x31fa2cce1ee7fbeaULL << 64) | (body_weight_word_t)0x8fda0e52bbd70664ULL),
        (((body_weight_word_t)0xf9390142ed133205ULL << 64) | (body_weight_word_t)0x322aff641916f92aULL), (((body_weight_word_t)0xf0e7200301e8100cULL << 64) | (body_weight_word_t)0x02dfe51a17f9fff5ULL), (((body_weight_word_t)0xd1ffeb0918de11dbULL << 64) | (body_weight_word_t)0xf6e8c5bf0befff0bULL), (((body_weight_word_t)0x7f2acbfc07130d29ULL << 64) | (body_weight_word_t)0x3152fe2e27f11798ULL),
        (((body_weight_word_t)0x4611493fe035281cULL << 64) | (body_weight_word_t)0x35fd57fa18c10fcdULL), (((body_weight_word_t)0xf9dc061fc8260916ULL << 64) | (body_weight_word_t)0x2eeb0914ffdc31e7ULL), (((body_weight_word_t)0x13fc28eb36ff14cdULL << 64) | (body_weight_word_t)0x0de84dffdaee00dcULL), (((body_weight_word_t)0xe30df51aa1f41c9fULL << 64) | (body_weight_word_t)0x28cf3d622fddede4ULL)
    },
    {
        (((body_weight_word_t)0x02e75115c9b5f41eULL << 64) | (body_weight_word_t)0xf21e14fe442fc733ULL), (((body_weight_word_t)0x040018d72ee1f4daULL << 64) | (body_weight_word_t)0xdc0d1af9f4fef923ULL), (((body_weight_word_t)0x1ffc00fc2320f3feULL << 64) | (body_weight_word_t)0x0ffcf9e4f31f1001ULL), (((body_weight_word_t)0xf8ed2707f7ecdfcfULL << 64) | (body_weight_word_t)0xe0f3fff5eede0df6ULL),
        (((body_weight_word_t)0xe20df9060503fb37ULL << 64) | (body_weight_word_t)0x140209f90204ec05ULL), (((body_weight_word_t)0x14d8fa0acf36d4ecULL << 64) | (body_weight_word_t)0xe3e2e7070014e4bdULL), (((body_weight_word_t)0x021c1a10f210dddeULL << 64) | (body_weight_word_t)0x110008f00f01290aULL), (((body_weight_word_t)0x0516295f43dd0506ULL << 64) | (body_weight_word_t)0x370cfc01ccee2b03ULL),
        (((body_weight_word_t)0xecf8e31b3d0509e5ULL << 64) | (body_weight_word_t)0xf7f3df190212d7c4ULL), (((body_weight_word_t)0xf002e4f74827eeeaULL << 64) | (body_weight_word_t)0xd0ce080cfd3ef9c4ULL), (((body_weight_word_t)0x170d0ee08ec088ceULL << 64) | (body_weight_word_t)0x0b0731fde822e809ULL), (((body_weight_word_t)0x2f19b3acb90fd7ddULL << 64) | (body_weight_word_t)0xeff31619f228d4d4ULL),
        (((body_weight_word_t)0xe4020ae43d13da7fULL << 64) | (body_weight_word_t)0xd6e5370e19e80766ULL), (((body_weight_word_t)0xfef1b5fffc48eeabULL << 64) | (body_weight_word_t)0x0b04c810ca1ee1faULL), (((body_weight_word_t)0xf00b02df011303f5ULL << 64) | (body_weight_word_t)0xe2f6f209ccfef513ULL), (((body_weight_word_t)0x08fa10023805fbf9ULL << 64) | (body_weight_word_t)0xe3fbfc02111bebdcULL)
    },
    {
        (((body_weight_word_t)0x0981e33434fc14f9ULL << 64) | (body_weight_word_t)0xef3ef9f415511d9aULL), (((body_weight_word_t)0xf2f8d2f436f81a18ULL << 64) | (body_weight_word_t)0xc405f523f1fad8e1ULL), (((body_weight_word_t)0x1412c9deaeda2615ULL << 64) | (body_weight_word_t)0x3d1225cd0ac1261cULL), (((body_weight_word_t)0x1c14f42acc2e01baULL << 64) | (body_weight_word_t)0xd2c8e109111ccd0dULL),
        (((body_weight_word_t)0xfae5ee2fe3fd05daULL << 64) | (body_weight_word_t)0x002410fcff08e9c5ULL), (((body_weight_word_t)0xc6e40eaf4bc1d4f9ULL << 64) | (body_weight_word_t)0x06ec12181d0efc05ULL), (((body_weight_word_t)0xf817fbdc070eec34ULL << 64) | (body_weight_word_t)0x33fd20f610121e20ULL), (((body_weight_word_t)0x4bec44f91d18eaf7ULL << 64) | (body_weight_word_t)0xfafd08e8d2b3100aULL),
        (((body_weight_word_t)0xece47c132e072419ULL << 64) | (body_weight_word_t)0x0925ea3b0a13dd0bULL), (((body_weight_word_t)0xeb0d2122d5f7100cULL << 64) | (body_weight_word_t)0xee20e52924ce09f4ULL), (((body_weight_word_t)0xdd07f629f3e2f5ecULL << 64) | (body_weight_word_t)0x5820eadeafec05e3ULL), (((body_weight_word_t)0x0c2fd2c92d44e2cbULL << 64) | (body_weight_word_t)0x1eb4edc3e0f424faULL),
        (((body_weight_word_t)0x32cdffeccc066932ULL << 64) | (body_weight_word_t)0xac2604eac1e815a5ULL), (((body_weight_word_t)0x1c2998f7fa0fe906ULL << 64) | (body_weight_word_t)0x0f20ecc6ea02f8faULL), (((body_weight_word_t)0x1941f529c9ed2ed9ULL << 64) | (body_weight_word_t)0xe002f9f71a0607fdULL), (((body_weight_word_t)0x07cd141a210de1abULL << 64) | (body_weight_word_t)0x0713c30fd10eecd6ULL)
    },
    {
        (((body_weight_word_t)0xfcf726f2d2e494f6ULL << 64) | (body_weight_word_t)0x0d1de00534fee9c5ULL), (((body_weight_word_t)0x06e2effa1b051704ULL << 64) | (body_weight_word_t)0xf520e6f202fcfedbULL), (((body_weight_word_t)0xe915eededdf937ccULL << 64) | (body_weight_word_t)0xeef01ded16f7da20ULL), (((body_weight_word_t)0x17eedf14d0fedaf7ULL << 64) | (body_weight_word_t)0xedf72afa3f0a0cffULL),
        (((body_weight_word_t)0xedf9de1314170204ULL << 64) | (body_weight_word_t)0x06ecf004e0e2f2deULL), (((body_weight_word_t)0xe3d8f3f7db11d310ULL << 64) | (body_weight_word_t)0xf1ce2222e824f5d7ULL), (((body_weight_word_t)0xfe1b0a09ffea051dULL << 64) | (body_weight_word_t)0x081109e6ea0ee208ULL), (((body_weight_word_t)0x2fed00512eea05feULL << 64) | (body_weight_word_t)0xdd06bd42fc0efdf8ULL),
        (((body_weight_word_t)0x1b1b3e1b0f0435d7ULL << 64) | (body_weight_word_t)0x2339c8ec12adfc07ULL), (((body_weight_word_t)0x11f21cfff60003fdULL << 64) | (body_weight_word_t)0xf2fce4e20dea25ebULL), (((body_weight_word_t)0x22a4eee8e8ddbb22ULL << 64) | (body_weight_word_t)0x2ace131b8134f8d5ULL), (((body_weight_word_t)0x13e916db13447757ULL << 64) | (body_weight_word_t)0xf2c33e161b1234fdULL),
        (((body_weight_word_t)0xf211e0d429fd16c6ULL << 64) | (body_weight_word_t)0x22f8e40cf6c3c029ULL), (((body_weight_word_t)0xf5d90ed12bff0af7ULL << 64) | (body_weight_word_t)0x5128c8ca05fe2dc3ULL), (((body_weight_word_t)0xf211e1df21f356c5ULL << 64) | (body_weight_word_t)0xf716ceef2603e701ULL), (((body_weight_word_t)0x1ce1110e06fccd29ULL << 64) | (body_weight_word_t)0x34060014db0627dbULL)
    }
};
static const bias_t L4_BIAS[CNN_C] = {
    2176, -2157, -1123, -1709, -2075, -1023, -649, 1135,
    -3797, -664, -531, -2962, 2944, -1543, 968, -156
};
static const q31_t L4_MAIN_Q31[CNN_C] = {
    1279619328, 1568877696, 1493913856, 1458191616,
    1102458496, 1230805632, 1627199744, 1545485696,
    1164222592, 1614344192, 1531844736, 1734652544,
    1660044288, 1081614720, 1629808512, 2116389760
};
static const qexp_t L4_MAIN_EXP[CNN_C] = {
    -8, -8, -8, -8, -8, -8, -8, -8,
    -8, -8, -8, -9, -8, -8, -8, -8
};
static const q31_t L4_SKIP_Q31 = 1774843907;
static const qexp_t L4_SKIP_EXP = 1;

// L5
static const body_weight_word_t L5_WBANK[9][CNN_C] = {
    {
        (((body_weight_word_t)0x00020202fefefe00ULL << 64) | (body_weight_word_t)0x01fe000000fefe00ULL), (((body_weight_word_t)0x02fffe0101fefe00ULL << 64) | (body_weight_word_t)0x00fffe0002020102ULL), (((body_weight_word_t)0x00feff00ff01ff02ULL << 64) | (body_weight_word_t)0xff020202fefffeffULL), (((body_weight_word_t)0x0200fe01010200ffULL << 64) | (body_weight_word_t)0x0002fe00fefe0000ULL),
        (((body_weight_word_t)0xff020102010201ffULL << 64) | (body_weight_word_t)0x02ff0202ff000002ULL), (((body_weight_word_t)0xffffff01fffffefeULL << 64) | (body_weight_word_t)0x0001ff020201fefeULL), (((body_weight_word_t)0x02ff0201ff0102ffULL << 64) | (body_weight_word_t)0xffff00fe00ff00ffULL), (((body_weight_word_t)0x02ffffff0000ff01ULL << 64) | (body_weight_word_t)0xff02ff0101fe0202ULL),
        (((body_weight_word_t)0x01ff020100fffe00ULL << 64) | (body_weight_word_t)0x0000020002ff0101ULL), (((body_weight_word_t)0x01ff0002fefefe00ULL << 64) | (body_weight_word_t)0xff00fefefefe01feULL), (((body_weight_word_t)0x000102020100fefeULL << 64) | (body_weight_word_t)0x01ff0102fefe00feULL), (((body_weight_word_t)0xfefe0002feff01feULL << 64) | (body_weight_word_t)0xfeff010200fefffeULL),
        (((body_weight_word_t)0xfffeffff02fe0200ULL << 64) | (body_weight_word_t)0x02ff020202ff0201ULL), (((body_weight_word_t)0x00ffff000001ff01ULL << 64) | (body_weight_word_t)0x01fe00fe0102ffffULL), (((body_weight_word_t)0x02ffff0000010200ULL << 64) | (body_weight_word_t)0x00fe0201ff00fffeULL), (((body_weight_word_t)0x0202ff0000010200ULL << 64) | (body_weight_word_t)0xff02ffffffffff01ULL)
    },
    {
        (((body_weight_word_t)0x01ff02fe0101feffULL << 64) | (body_weight_word_t)0x00fe02000102fefeULL), (((body_weight_word_t)0xfe010000fffefefeULL << 64) | (body_weight_word_t)0x000001fefe0001feULL), (((body_weight_word_t)0x01feff00fefffffeULL << 64) | (body_weight_word_t)0x00fe02ff01ff0202ULL), (((body_weight_word_t)0x02fe010000020001ULL << 64) | (body_weight_word_t)0x020200feff0102feULL),
        (((body_weight_word_t)0x02fe00fffeff0201ULL << 64) | (body_weight_word_t)0x00feff00fefffefeULL), (((body_weight_word_t)0xff01fe02fe02ff02ULL << 64) | (body_weight_word_t)0x010100fe0100fefeULL), (((body_weight_word_t)0x00fefe00ff00ffffULL << 64) | (body_weight_word_t)0xff0001ff00fefeffULL), (((body_weight_word_t)0xff020102ff020002ULL << 64) | (body_weight_word_t)0x01ff010201ff00ffULL),
        (((body_weight_word_t)0xffff000102feff00ULL << 64) | (body_weight_word_t)0x02ffff00ff01fe02ULL), (((body_weight_word_t)0xff01ff01fffe02feULL << 64) | (body_weight_word_t)0x020100ff010101feULL), (((body_weight_word_t)0xfefffe00fefeff01ULL << 64) | (body_weight_word_t)0x00ff01ff00fe0201ULL), (((body_weight_word_t)0x0101020001fe0201ULL << 64) | (body_weight_word_t)0x01fe02fe00020102ULL),
        (((body_weight_word_t)0x0000fe02fe02feffULL << 64) | (body_weight_word_t)0x0200ff0001010200ULL), (((body_weight_word_t)0x000000ff02000100ULL << 64) | (body_weight_word_t)0xff0201fffe000202ULL), (((body_weight_word_t)0x01fe02fe00000102ULL << 64) | (body_weight_word_t)0x01ff01010201ff02ULL), (((body_weight_word_t)0xfffefffe01020002ULL << 64) | (body_weight_word_t)0x02fe000200ff02feULL)
    },
    {
        (((body_weight_word_t)0xff0102ff0002ff00ULL << 64) | (body_weight_word_t)0x0000fffefffe02ffULL), (((body_weight_word_t)0xfffefffe02010100ULL << 64) | (body_weight_word_t)0xfffe02ff0100ff00ULL), (((body_weight_word_t)0xff0200fefffefffeULL << 64) | (body_weight_word_t)0x01000000000202ffULL), (((body_weight_word_t)0x0201000201fe0100ULL << 64) | (body_weight_word_t)0x0101ff00fe010201ULL),
        (((body_weight_word_t)0x020102ff02fe00ffULL << 64) | (body_weight_word_t)0x0200ff01ff020101ULL), (((body_weight_word_t)0xfefe00010202ffffULL << 64) | (body_weight_word_t)0xfffefefe02fffe02ULL), (((body_weight_word_t)0xff00020202000000ULL << 64) | (body_weight_word_t)0xfe01ff00fe010202ULL), (((body_weight_word_t)0x00fe0201fe00ffffULL << 64) | (body_weight_word_t)0xfe010202fe010101ULL),
        (((body_weight_word_t)0xfe0101fefefefe02ULL << 64) | (body_weight_word_t)0x000102ff01ffff01ULL), (((body_weight_word_t)0xfefe00fffe02ff02ULL << 64) | (body_weight_word_t)0x0202fe010202ff00ULL), (((body_weight_word_t)0x0001ff01ff000000ULL << 64) | (body_weight_word_t)0xff0001020100fe00ULL), (((body_weight_word_t)0xfe00fe00020101ffULL << 64) | (body_weight_word_t)0x01ffff00ff01fe00ULL),
        (((body_weight_word_t)0xff000202010202feULL << 64) | (body_weight_word_t)0x000100fe00fffe00ULL), (((body_weight_word_t)0x010101fffe020102ULL << 64) | (body_weight_word_t)0x0100fefe00000001ULL), (((body_weight_word_t)0xfe00fe0201000001ULL << 64) | (body_weight_word_t)0x0002000201020001ULL), (((body_weight_word_t)0x01ffff010000fefeULL << 64) | (body_weight_word_t)0x01fffefe02fffffeULL)
    },
    {
        (((body_weight_word_t)0x0000fe000202ff02ULL << 64) | (body_weight_word_t)0x01ffff0100000201ULL), (((body_weight_word_t)0xff000102ff00fefeULL << 64) | (body_weight_word_t)0x0001fefe0000feffULL), (((body_weight_word_t)0x00fe02fe00ff00feULL << 64) | (body_weight_word_t)0x00fe0202fe00ff01ULL), (((body_weight_word_t)0x01fefffe010000feULL << 64) | (body_weight_word_t)0xfffffe0100ff00ffULL),
        (((body_weight_word_t)0x0202ff0000020000ULL << 64) | (body_weight_word_t)0xff01ff0002fe0100ULL), (((body_weight_word_t)0xfffefffe00000100ULL << 64) | (body_weight_word_t)0x0200020101ff0200ULL), (((body_weight_word_t)0x0100020001020201ULL << 64) | (body_weight_word_t)0x01fe0102fefeff00ULL), (((body_weight_word_t)0x01fefe01feffff02ULL << 64) | (body_weight_word_t)0xff02fe0200010201ULL),
        (((body_weight_word_t)0x0002ff000100ff01ULL << 64) | (body_weight_word_t)0xff02ff0201000000ULL), (((body_weight_word_t)0xfffefefefefe00feULL << 64) | (body_weight_word_t)0x000102ff000001feULL), (((body_weight_word_t)0x020102ffff0101feULL << 64) | (body_weight_word_t)0x00feff01feff0000ULL), (((body_weight_word_t)0x010100ff000200ffULL << 64) | (body_weight_word_t)0xfe02fefe020200feULL),
        (((body_weight_word_t)0xfefe000002feff02ULL << 64) | (body_weight_word_t)0x0201fe0202ff0001ULL), (((body_weight_word_t)0x0102fe0002fe01feULL << 64) | (body_weight_word_t)0xfefe02ff0000ffffULL), (((body_weight_word_t)0x00fe0102ff0101ffULL << 64) | (body_weight_word_t)0xff000102ff020102ULL), (((body_weight_word_t)0xfffffe02000101feULL << 64) | (body_weight_word_t)0xfe010200fe00ff00ULL)
    },
    {
        (((body_weight_word_t)0xffff0200010102ffULL << 64) | (body_weight_word_t)0x010100fffeff0202ULL), (((body_weight_word_t)0xfffffeff010101feULL << 64) | (body_weight_word_t)0xff0201010201fe02ULL), (((body_weight_word_t)0x02020001feff0202ULL << 64) | (body_weight_word_t)0x0001fe0100fe0002ULL), (((body_weight_word_t)0x00ff0200fe00fe02ULL << 64) | (body_weight_word_t)0x00feffff01010001ULL),
        (((body_weight_word_t)0xfe02020002fefe01ULL << 64) | (body_weight_word_t)0x0202feff0101fe01ULL), (((body_weight_word_t)0xfefffe00fffeff01ULL << 64) | (body_weight_word_t)0xfefffe01fefe0100ULL), (((body_weight_word_t)0xfe01fe0202010202ULL << 64) | (body_weight_word_t)0x0201010202feffffULL), (((body_weight_word_t)0x01ff0201fffefe00ULL << 64) | (body_weight_word_t)0xfefe000200feffffULL),
        (((body_weight_word_t)0x0001fe0100fe01feULL << 64) | (body_weight_word_t)0xffff02fefe000202ULL), (((body_weight_word_t)0x02fe000001000100ULL << 64) | (body_weight_word_t)0x01fefe020001ff00ULL), (((body_weight_word_t)0xfefffe0101fe00ffULL << 64) | (body_weight_word_t)0xfefeff0202fffe01ULL), (((body_weight_word_t)0x0102ff02ffff0101ULL << 64) | (body_weight_word_t)0x00ffff0001ffffffULL),
        (((body_weight_word_t)0xfe0101fe0201ff01ULL << 64) | (body_weight_word_t)0x01010000fe02feffULL), (((body_weight_word_t)0x00010201fe0201feULL << 64) | (body_weight_word_t)0xff02020200020200ULL), (((body_weight_word_t)0xfe020100ffff0002ULL << 64) | (body_weight_word_t)0xfe00fe0100000200ULL), (((body_weight_word_t)0x0100ff01ffff0100ULL << 64) | (body_weight_word_t)0xfffe020201020002ULL)
    },
    {
        (((body_weight_word_t)0xffff020002ff00feULL << 64) | (body_weight_word_t)0x0000ff0102020202ULL), (((body_weight_word_t)0x0101010101feffffULL << 64) | (body_weight_word_t)0xfe0100fe02fffe02ULL), (((body_weight_word_t)0x02fe01ffffff0201ULL << 64) | (body_weight_word_t)0xfeff02020000ff02ULL), (((body_weight_word_t)0x0101ff02ff02fe00ULL << 64) | (body_weight_word_t)0xff0002fe01fe0002ULL),
        (((body_weight_word_t)0x00020101ff00ff02ULL << 64) | (body_weight_word_t)0x00020101fe020202ULL), (((body_weight_word_t)0xff0002ff01ff0000ULL << 64) | (body_weight_word_t)0x02010100fe02ff00ULL), (((body_weight_word_t)0x0102fe0200020101ULL << 64) | (body_weight_word_t)0x00ff02010000ff00ULL), (((body_weight_word_t)0xfefe020000000102ULL << 64) | (body_weight_word_t)0xff01ff02fffe0100ULL),
        (((body_weight_word_t)0x0102fefefefe0100ULL << 64) | (body_weight_word_t)0x00000100ff02ff02ULL), (((body_weight_word_t)0xfe0101fe0102ffffULL << 64) | (body_weight_word_t)0x0001ff01ff010000ULL), (((body_weight_word_t)0x0100ff00ff00fe02ULL << 64) | (body_weight_word_t)0x020102fe01ff02feULL), (((body_weight_word_t)0x0001fe01fefe0201ULL << 64) | (body_weight_word_t)0xfe02fe0000020000ULL),
        (((body_weight_word_t)0xff01fe000002ffffULL << 64) | (body_weight_word_t)0xff010200fe000100ULL), (((body_weight_word_t)0x0202010202000000ULL << 64) | (body_weight_word_t)0x020001ff02000001ULL), (((body_weight_word_t)0x01fe01feffff0202ULL << 64) | (body_weight_word_t)0x000000fe0201ff00ULL), (((body_weight_word_t)0x00feff00fffe0102ULL << 64) | (body_weight_word_t)0x02fe010102fe02ffULL)
    },
    {
        (((body_weight_word_t)0x01fe00fe00ff0202ULL << 64) | (body_weight_word_t)0x010202ff01ff0000ULL), (((body_weight_word_t)0xffff0002ffff01feULL << 64) | (body_weight_word_t)0xfe00feff02000201ULL), (((body_weight_word_t)0x01fefe02fffffffeULL << 64) | (body_weight_word_t)0xfe010002fe02ff00ULL), (((body_weight_word_t)0x00fe010001010101ULL << 64) | (body_weight_word_t)0x0002000000020200ULL),
        (((body_weight_word_t)0xfe0001fffe02ff02ULL << 64) | (body_weight_word_t)0x0001ff0100fffe00ULL), (((body_weight_word_t)0xfe00fefffffffefeULL << 64) | (body_weight_word_t)0x01020001feff0202ULL), (((body_weight_word_t)0x00fe0100fffe0002ULL << 64) | (body_weight_word_t)0xffff01fffeff0202ULL), (((body_weight_word_t)0xff020101fefe0101ULL << 64) | (body_weight_word_t)0x0100ffff010002ffULL),
        (((body_weight_word_t)0x020000000001ff01ULL << 64) | (body_weight_word_t)0x01fe00fe01feffffULL), (((body_weight_word_t)0xfe0200ff01000201ULL << 64) | (body_weight_word_t)0x00010001020100feULL), (((body_weight_word_t)0xfe0001010000ff01ULL << 64) | (body_weight_word_t)0xfe00010100ff00ffULL), (((body_weight_word_t)0xffff0202fe0002ffULL << 64) | (body_weight_word_t)0x01fe02ff00fffefeULL),
        (((body_weight_word_t)0x020100fe0100fefeULL << 64) | (body_weight_word_t)0xfefe01ff01000202ULL), (((body_weight_word_t)0x0002020200ff00feULL << 64) | (body_weight_word_t)0x02fffffe00000200ULL), (((body_weight_word_t)0x02ff0200ff00fffeULL << 64) | (body_weight_word_t)0x00ff0100fefe02ffULL), (((body_weight_word_t)0xff00ff000202ff00ULL << 64) | (body_weight_word_t)0x0201ff0200010201ULL)
    },
    {
        (((body_weight_word_t)0x000200fe0001ff02ULL << 64) | (body_weight_word_t)0xfe00ff0002ffff01ULL), (((body_weight_word_t)0x000001feff01fe00ULL << 64) | (body_weight_word_t)0xff0202ff000101ffULL), (((body_weight_word_t)0x02ff0200fffe0001ULL << 64) | (body_weight_word_t)0x0000fe0002fefe02ULL), (((body_weight_word_t)0x00fefeffff0200ffULL << 64) | (body_weight_word_t)0x020100020001fffeULL),
        (((body_weight_word_t)0x02ffff020200ffffULL << 64) | (body_weight_word_t)0xfefffe00fe0200feULL), (((body_weight_word_t)0xfffe0000fe02fffeULL << 64) | (body_weight_word_t)0x0102010202fe0200ULL), (((body_weight_word_t)0x01fffefefe01feffULL << 64) | (body_weight_word_t)0x010200feffffff02ULL), (((body_weight_word_t)0xfffe02000000ffffULL << 64) | (body_weight_word_t)0xfffffe0002ff00feULL),
        (((body_weight_word_t)0x0102000100010200ULL << 64) | (body_weight_word_t)0x00ff00fffeff0001ULL), (((body_weight_word_t)0xff020200020100ffULL << 64) | (body_weight_word_t)0xffff00ff02fefe02ULL), (((body_weight_word_t)0x02feff02fe010100ULL << 64) | (body_weight_word_t)0x02fffefefffe0200ULL), (((body_weight_word_t)0x02fffefe0201ff01ULL << 64) | (body_weight_word_t)0xfe02fe00020100feULL),
        (((body_weight_word_t)0xff00fe0102020101ULL << 64) | (body_weight_word_t)0x000201ff0202feffULL), (((body_weight_word_t)0x01fe000202000002ULL << 64) | (body_weight_word_t)0x0002ff02000100feULL), (((body_weight_word_t)0xfe00fe00ff0202feULL << 64) | (body_weight_word_t)0x0102020200fe0000ULL), (((body_weight_word_t)0x02ff02ffff01fefeULL << 64) | (body_weight_word_t)0x02000000ff0202feULL)
    },
    {
        (((body_weight_word_t)0x0001ff0102fe0001ULL << 64) | (body_weight_word_t)0xff02010000000202ULL), (((body_weight_word_t)0xff01ff00fffffe02ULL << 64) | (body_weight_word_t)0x00fffeff010201ffULL), (((body_weight_word_t)0xfe02ff02fe0001ffULL << 64) | (body_weight_word_t)0xffffff02000101ffULL), (((body_weight_word_t)0x0100020002010102ULL << 64) | (body_weight_word_t)0x00fe02fe00020101ULL),
        (((body_weight_word_t)0xff02ff01fefeff00ULL << 64) | (body_weight_word_t)0xff020002feff0100ULL), (((body_weight_word_t)0x0001fffefeff02ffULL << 64) | (body_weight_word_t)0xff00fe010201feffULL), (((body_weight_word_t)0x00020100ff010102ULL << 64) | (body_weight_word_t)0xfffffffe0200ff00ULL), (((body_weight_word_t)0x00010200fe020101ULL << 64) | (body_weight_word_t)0x0202ffff0200fffeULL),
        (((body_weight_word_t)0xfe02ff02020000ffULL << 64) | (body_weight_word_t)0xff0202fffe0201ffULL), (((body_weight_word_t)0x0200020201010201ULL << 64) | (body_weight_word_t)0xfe00000101fffefeULL), (((body_weight_word_t)0x00ff0001ff00ff00ULL << 64) | (body_weight_word_t)0xff00fffe0000ff02ULL), (((body_weight_word_t)0x0002ffffffff0102ULL << 64) | (body_weight_word_t)0x00020000fe000102ULL),
        (((body_weight_word_t)0x00fe0100010101ffULL << 64) | (body_weight_word_t)0x0000fe0101ff0002ULL), (((body_weight_word_t)0x0200fe0001000000ULL << 64) | (body_weight_word_t)0x010201fe01020202ULL), (((body_weight_word_t)0x00fefeff0002ffffULL << 64) | (body_weight_word_t)0x0001fefffefffffeULL), (((body_weight_word_t)0x02ff0101fe0000ffULL << 64) | (body_weight_word_t)0x01fe010202020201ULL)
    }
};
static const bias_t L5_BIAS[CNN_C] = {
    84, 41, -127, 46, 92, -85, 54, -20, -33, 30, 33, 93, 85, 91, -10, 68
};
static const q31_t L5_Q31[CNN_C] = {
    1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824
};
static const qexp_t L5_EXP[CNN_C] = {
    -3, -4, -5, -6, -3, -4, -5, -6, -3, -4, -5, -6, -3, -4, -5, -6
};

// L6
static const body_weight_word_t L6_WBANK[9][CNN_C] = {
    {
        (((body_weight_word_t)0x010100fe01fe00feULL << 64) | (body_weight_word_t)0x0102feff000102feULL), (((body_weight_word_t)0xfe01010002ff0002ULL << 64) | (body_weight_word_t)0xfffe0100fefe01feULL), (((body_weight_word_t)0x02fffeffff02fe02ULL << 64) | (body_weight_word_t)0x00fe000202ff0002ULL), (((body_weight_word_t)0xfe02fe02feff0100ULL << 64) | (body_weight_word_t)0x00020100ff000001ULL),
        (((body_weight_word_t)0xfeff020102fefefeULL << 64) | (body_weight_word_t)0xff01fefe00ff0201ULL), (((body_weight_word_t)0x02fe0000fe0202ffULL << 64) | (body_weight_word_t)0x0001ff0200feff01ULL), (((body_weight_word_t)0xff00000002fefe01ULL << 64) | (body_weight_word_t)0xfe00fefe0101ff02ULL), (((body_weight_word_t)0xfefffe01fffefe02ULL << 64) | (body_weight_word_t)0x000202fefeff01ffULL),
        (((body_weight_word_t)0x00010202fffe00feULL << 64) | (body_weight_word_t)0x0002020100fffefeULL), (((body_weight_word_t)0xfe01ff00fffe0202ULL << 64) | (body_weight_word_t)0x01ff01fe01fe0101ULL), (((body_weight_word_t)0xff00fe01fefffe02ULL << 64) | (body_weight_word_t)0x0001ff010002ff02ULL), (((body_weight_word_t)0x00fe0202fe01fefeULL << 64) | (body_weight_word_t)0x00fe0002fe01ff01ULL),
        (((body_weight_word_t)0x01fffffefe0102feULL << 64) | (body_weight_word_t)0xfffefeff02000000ULL), (((body_weight_word_t)0xfe01fe00fe02ffffULL << 64) | (body_weight_word_t)0xffff0200ff00ff01ULL), (((body_weight_word_t)0xff01fefe01fefefeULL << 64) | (body_weight_word_t)0x02020002ff00feffULL), (((body_weight_word_t)0xfe0100ff010000ffULL << 64) | (body_weight_word_t)0x01010002fe000001ULL)
    },
    {
        (((body_weight_word_t)0xfe00fe01010100ffULL << 64) | (body_weight_word_t)0x020000fefe0100ffULL), (((body_weight_word_t)0x02ff0101ff010000ULL << 64) | (body_weight_word_t)0x00fe0200feff00feULL), (((body_weight_word_t)0x0101ff01fefe0000ULL << 64) | (body_weight_word_t)0xffffff02ffffff00ULL), (((body_weight_word_t)0x02000001fe0201ffULL << 64) | (body_weight_word_t)0xfffffe0202fe0100ULL),
        (((body_weight_word_t)0xfe00ff00010001ffULL << 64) | (body_weight_word_t)0x00000001fe00ffffULL), (((body_weight_word_t)0xff0102fefeff0000ULL << 64) | (body_weight_word_t)0xff02ff020100fe02ULL), (((body_weight_word_t)0x000101feff010002ULL << 64) | (body_weight_word_t)0x010200ff020201ffULL), (((body_weight_word_t)0x0201fe01fe0101ffULL << 64) | (body_weight_word_t)0xff0001fe02000000ULL),
        (((body_weight_word_t)0xfe00010000fffe02ULL << 64) | (body_weight_word_t)0xfffeff0200010100ULL), (((body_weight_word_t)0xff01fffe02fe0100ULL << 64) | (body_weight_word_t)0x02ff01ff02ff02ffULL), (((body_weight_word_t)0xff000100010002ffULL << 64) | (body_weight_word_t)0x0101020000010000ULL), (((body_weight_word_t)0x02000201020002feULL << 64) | (body_weight_word_t)0xfe0202ff02000001ULL),
        (((body_weight_word_t)0xfeff0000fffffe00ULL << 64) | (body_weight_word_t)0x020000fe000101ffULL), (((body_weight_word_t)0x02ff020201020100ULL << 64) | (body_weight_word_t)0xfffeff00fefeffffULL), (((body_weight_word_t)0xfe010102fe00fe01ULL << 64) | (body_weight_word_t)0x0201ff020202fefeULL), (((body_weight_word_t)0x00ff000200feff00ULL << 64) | (body_weight_word_t)0x00fffefe0102ff02ULL)
    },
    {
        (((body_weight_word_t)0xfeffff02feffff00ULL << 64) | (body_weight_word_t)0xfe020100ff010100ULL), (((body_weight_word_t)0x01ffff02ff01feffULL << 64) | (body_weight_word_t)0xfeff0100ff0100ffULL), (((body_weight_word_t)0x00010000fe020202ULL << 64) | (body_weight_word_t)0x010002010002fefeULL), (((body_weight_word_t)0xfe0000ff01fe01ffULL << 64) | (body_weight_word_t)0x0202010200feff00ULL),
        (((body_weight_word_t)0xfe02010200010000ULL << 64) | (body_weight_word_t)0x0102fffe02000002ULL), (((body_weight_word_t)0x0001fffeff0200feULL << 64) | (body_weight_word_t)0x00fe01020002fe00ULL), (((body_weight_word_t)0xfefe0002020201feULL << 64) | (body_weight_word_t)0x02fefffe000000feULL), (((body_weight_word_t)0x00fe020202fe00ffULL << 64) | (body_weight_word_t)0xffff010201020200ULL),
        (((body_weight_word_t)0xffff02ff00020202ULL << 64) | (body_weight_word_t)0xff01fefeff010201ULL), (((body_weight_word_t)0x02fe0202fe0201feULL << 64) | (body_weight_word_t)0xfe01fe0200010001ULL), (((body_weight_word_t)0x01fe02fefefefe02ULL << 64) | (body_weight_word_t)0xff00020201fffe02ULL), (((body_weight_word_t)0x01ff000000020201ULL << 64) | (body_weight_word_t)0xfe00020100fefefeULL),
        (((body_weight_word_t)0xfe01000001fffeffULL << 64) | (body_weight_word_t)0xffff010202ff0001ULL), (((body_weight_word_t)0x01000101010101feULL << 64) | (body_weight_word_t)0x0002fe01ff01feffULL), (((body_weight_word_t)0xfeff02fe010201ffULL << 64) | (body_weight_word_t)0x01fe00fffefe0102ULL), (((body_weight_word_t)0xfefffe010102ff01ULL << 64) | (body_weight_word_t)0x0002fe020100ffffULL)
    },
    {
        (((body_weight_word_t)0xffff0200fe01fe00ULL << 64) | (body_weight_word_t)0x0000fffefe000201ULL), (((body_weight_word_t)0x01ff02feff01ff00ULL << 64) | (body_weight_word_t)0xff00020201fffffeULL), (((body_weight_word_t)0x00fe02fe02010101ULL << 64) | (body_weight_word_t)0x00fffefffe010201ULL), (((body_weight_word_t)0xfefefefe0202fe01ULL << 64) | (body_weight_word_t)0x00ff00fefefeff01ULL),
        (((body_weight_word_t)0xfe01fe02feff0102ULL << 64) | (body_weight_word_t)0x02fefffffffe0101ULL), (((body_weight_word_t)0xff0202fe00fe0200ULL << 64) | (body_weight_word_t)0xfe0002fe01000000ULL), (((body_weight_word_t)0xfeffffffff01fffeULL << 64) | (body_weight_word_t)0xfffe02feffff02feULL), (((body_weight_word_t)0xff01ff02010201feULL << 64) | (body_weight_word_t)0xfe00fefe0100fe02ULL),
        (((body_weight_word_t)0x0000fffe0201fe02ULL << 64) | (body_weight_word_t)0x01ff02ff020101ffULL), (((body_weight_word_t)0x010200ff0200ff01ULL << 64) | (body_weight_word_t)0x000102fffffeff01ULL), (((body_weight_word_t)0x02fe0101ff010001ULL << 64) | (body_weight_word_t)0x0002000200000001ULL), (((body_weight_word_t)0xfe0001ffffffff01ULL << 64) | (body_weight_word_t)0x020101fe0201fe01ULL),
        (((body_weight_word_t)0x0100ff0100ff0100ULL << 64) | (body_weight_word_t)0x00fffe00ffff0102ULL), (((body_weight_word_t)0x01fe02ffff010200ULL << 64) | (body_weight_word_t)0x00020101feff00feULL), (((body_weight_word_t)0xfe02feffff010001ULL << 64) | (body_weight_word_t)0x01fe02020001ffffULL), (((body_weight_word_t)0x0100ffffff0102ffULL << 64) | (body_weight_word_t)0xfffe0102fefffffeULL)
    },
    {
        (((body_weight_word_t)0x0200ff0102ff0001ULL << 64) | (body_weight_word_t)0xfeff00fefeff00ffULL), (((body_weight_word_t)0x0101ff02fffe01feULL << 64) | (body_weight_word_t)0x02000202020100ffULL), (((body_weight_word_t)0xfefe02ff0102fe01ULL << 64) | (body_weight_word_t)0x010002ff0102fe00ULL), (((body_weight_word_t)0x0102fe02feffff00ULL << 64) | (body_weight_word_t)0xfefefe00ffff02feULL),
        (((body_weight_word_t)0xffff0001fe0201ffULL << 64) | (body_weight_word_t)0x000201fffe0102feULL), (((body_weight_word_t)0x01ffff00ff02feffULL << 64) | (body_weight_word_t)0xff0201fefe010202ULL), (((body_weight_word_t)0xfe000000feff00feULL << 64) | (body_weight_word_t)0x0202010002ffff01ULL), (((body_weight_word_t)0x0002ff0200010001ULL << 64) | (body_weight_word_t)0x02fe01fe01ffff01ULL),
        (((body_weight_word_t)0x01020101000002feULL << 64) | (body_weight_word_t)0x01000202000001feULL), (((body_weight_word_t)0x02ff02fe02fefe02ULL << 64) | (body_weight_word_t)0xfe0202fe02ffff01ULL), (((body_weight_word_t)0x01020201fe00feffULL << 64) | (body_weight_word_t)0x020001ff02010100ULL), (((body_weight_word_t)0xfe02ff01ff020000ULL << 64) | (body_weight_word_t)0xff01fe0202ffff02ULL),
        (((body_weight_word_t)0x0001000002020202ULL << 64) | (body_weight_word_t)0x000101fe01fefe02ULL), (((body_weight_word_t)0xff02fffefefe0101ULL << 64) | (body_weight_word_t)0x0102feff02fe0202ULL), (((body_weight_word_t)0xff000000fe000001ULL << 64) | (body_weight_word_t)0xffff000000020101ULL), (((body_weight_word_t)0x010102fe02fe02feULL << 64) | (body_weight_word_t)0x000202fffeff01feULL)
    },
    {
        (((body_weight_word_t)0x0000fe02feff02ffULL << 64) | (body_weight_word_t)0x010001ffffff01feULL), (((body_weight_word_t)0xfe01010202ff0000ULL << 64) | (body_weight_word_t)0x02ffffff0102ffffULL), (((body_weight_word_t)0x0100ff00ff01ff00ULL << 64) | (body_weight_word_t)0x02fe0001fffefeffULL), (((body_weight_word_t)0x02fe02fefefe00ffULL << 64) | (body_weight_word_t)0x01010002020202feULL),
        (((body_weight_word_t)0x01fe020102ff00ffULL << 64) | (body_weight_word_t)0xfefe02feff010201ULL), (((body_weight_word_t)0xfefefeff02000200ULL << 64) | (body_weight_word_t)0xff01fe0200000001ULL), (((body_weight_word_t)0x00ffff02ff00fffeULL << 64) | (body_weight_word_t)0xfe0002000201fefeULL), (((body_weight_word_t)0x0200fe01fffefe01ULL << 64) | (body_weight_word_t)0xfe00ff0001ff0002ULL),
        (((body_weight_word_t)0x01020200fe02fffeULL << 64) | (body_weight_word_t)0xffff01010102ff00ULL), (((body_weight_word_t)0x00ffffff0202fffeULL << 64) | (body_weight_word_t)0xfe0102020002fefeULL), (((body_weight_word_t)0xfe02010101ffff00ULL << 64) | (body_weight_word_t)0xfefe02fe01fe02ffULL), (((body_weight_word_t)0xfefe0101ffff02feULL << 64) | (body_weight_word_t)0x00fe00fefe01ffffULL),
        (((body_weight_word_t)0xfefe01fffe010102ULL << 64) | (body_weight_word_t)0xff02fe0100fe0000ULL), (((body_weight_word_t)0x010100020101fe00ULL << 64) | (body_weight_word_t)0x0002020002000102ULL), (((body_weight_word_t)0x0000fefe00ffff01ULL << 64) | (body_weight_word_t)0xfefefe000201fe01ULL), (((body_weight_word_t)0xfefffefefffefeffULL << 64) | (body_weight_word_t)0xfefe0102000200feULL)
    },
    {
        (((body_weight_word_t)0xfefe000000fffe01ULL << 64) | (body_weight_word_t)0xffff00fe00ff0100ULL), (((body_weight_word_t)0x00000001ff000101ULL << 64) | (body_weight_word_t)0xfe010002fe01fefeULL), (((body_weight_word_t)0x01fe0001010202ffULL << 64) | (body_weight_word_t)0x01fffe01ff00ff00ULL), (((body_weight_word_t)0x000202fe01ffffffULL << 64) | (body_weight_word_t)0xfe0000fffe00ff01ULL),
        (((body_weight_word_t)0x02fffe00fffffefeULL << 64) | (body_weight_word_t)0xff020100fe0102ffULL), (((body_weight_word_t)0xfeff01fe00fe0100ULL << 64) | (body_weight_word_t)0x00fe01fffffffe00ULL), (((body_weight_word_t)0x01000200fefe0102ULL << 64) | (body_weight_word_t)0x0201010201fe00ffULL), (((body_weight_word_t)0xff02fefefeff0102ULL << 64) | (body_weight_word_t)0xfefefeffff0001ffULL),
        (((body_weight_word_t)0xfeff02ff00ff0000ULL << 64) | (body_weight_word_t)0xfefffe020201ff02ULL), (((body_weight_word_t)0xff02010001020100ULL << 64) | (body_weight_word_t)0xff000002fffe0000ULL), (((body_weight_word_t)0xfeff000201ff0101ULL << 64) | (body_weight_word_t)0xffff0201ff00ff01ULL), (((body_weight_word_t)0x01fffe02fe00fe00ULL << 64) | (body_weight_word_t)0x01fe02fffeff0102ULL),
        (((body_weight_word_t)0xfe01fffe010000ffULL << 64) | (body_weight_word_t)0xfffe01fe00fe0201ULL), (((body_weight_word_t)0x010202feffffff02ULL << 64) | (body_weight_word_t)0xff00ffff02ff0000ULL), (((body_weight_word_t)0xff000000ff0100ffULL << 64) | (body_weight_word_t)0xfe02ff0200fffe02ULL), (((body_weight_word_t)0xfe020202fe0201ffULL << 64) | (body_weight_word_t)0xff000000fefffe02ULL)
    },
    {
        (((body_weight_word_t)0xfe0002fe02000202ULL << 64) | (body_weight_word_t)0x020100fefeff0002ULL), (((body_weight_word_t)0x0000020100ff01ffULL << 64) | (body_weight_word_t)0xfe010101feff0000ULL), (((body_weight_word_t)0xff0201fefffe0100ULL << 64) | (body_weight_word_t)0xfffffeffffff0101ULL), (((body_weight_word_t)0x0102fffefe01fe00ULL << 64) | (body_weight_word_t)0xff01ff020001fe02ULL),
        (((body_weight_word_t)0x00ff0201ffff00ffULL << 64) | (body_weight_word_t)0xff0000fefe0002feULL), (((body_weight_word_t)0xff02fe0002fefefeULL << 64) | (body_weight_word_t)0xfe020202ff000102ULL), (((body_weight_word_t)0x00fe0200feff0202ULL << 64) | (body_weight_word_t)0x00ff02ffff0000ffULL), (((body_weight_word_t)0x02fe00fefffefe02ULL << 64) | (body_weight_word_t)0xff00fe0200fffe02ULL),
        (((body_weight_word_t)0xff01fe02ff0100ffULL << 64) | (body_weight_word_t)0xfefe0101010200feULL), (((body_weight_word_t)0x0202fe01fe02ff00ULL << 64) | (body_weight_word_t)0x01fe02000200fe01ULL), (((body_weight_word_t)0xff02fe020201fe02ULL << 64) | (body_weight_word_t)0xff00000001fffeffULL), (((body_weight_word_t)0x0100020202ffff00ULL << 64) | (body_weight_word_t)0x00fe000101ff0002ULL),
        (((body_weight_word_t)0x01fefe0201010001ULL << 64) | (body_weight_word_t)0xffff01fe02fffe02ULL), (((body_weight_word_t)0xff020102feff0101ULL << 64) | (body_weight_word_t)0x0002000100010000ULL), (((body_weight_word_t)0x01ff020100000001ULL << 64) | (body_weight_word_t)0x0200ff01fefffe01ULL), (((body_weight_word_t)0xfefffefffffe0002ULL << 64) | (body_weight_word_t)0x01010001fe010201ULL)
    },
    {
        (((body_weight_word_t)0x00ffff010002ffffULL << 64) | (body_weight_word_t)0xffff01fefeffff01ULL), (((body_weight_word_t)0x0102fefffe000001ULL << 64) | (body_weight_word_t)0xff000100ff01fe02ULL), (((body_weight_word_t)0x02ff0001fe01ffffULL << 64) | (body_weight_word_t)0x02fe02000102ffffULL), (((body_weight_word_t)0xff0100010101ff01ULL << 64) | (body_weight_word_t)0x02ff01fefe0100feULL),
        (((body_weight_word_t)0x000201000101ffffULL << 64) | (body_weight_word_t)0xff0001000001ffffULL), (((body_weight_word_t)0xfefe000002000202ULL << 64) | (body_weight_word_t)0xfe0200ff02000101ULL), (((body_weight_word_t)0xfe0100ff0201fefeULL << 64) | (body_weight_word_t)0x01fe01ff01ffff02ULL), (((body_weight_word_t)0xfefe010202fe02ffULL << 64) | (body_weight_word_t)0x02020100fefe0102ULL),
        (((body_weight_word_t)0x020202fe000000ffULL << 64) | (body_weight_word_t)0x02ff020202000201ULL), (((body_weight_word_t)0x00fe020200020200ULL << 64) | (body_weight_word_t)0xfe02000102000101ULL), (((body_weight_word_t)0x00ff00fffffeff02ULL << 64) | (body_weight_word_t)0x02010002010202ffULL), (((body_weight_word_t)0x000100010100fe01ULL << 64) | (body_weight_word_t)0x020200ff00ff0001ULL),
        (((body_weight_word_t)0xfe0202ff01ff00feULL << 64) | (body_weight_word_t)0x01fe02ffffff01ffULL), (((body_weight_word_t)0x000100000002fe01ULL << 64) | (body_weight_word_t)0xff000001ff02fffeULL), (((body_weight_word_t)0x02ffffffff01feffULL << 64) | (body_weight_word_t)0xfefffe0200feff00ULL), (((body_weight_word_t)0xff02020001000201ULL << 64) | (body_weight_word_t)0xfe01000102010001ULL)
    }
};
static const bias_t L6_BIAS[CNN_C] = {
    56, -31, 52, -112, 101, 7, -66, 128, -115, -38, 118, -48, 63, -90, -38, -22
};
static const q31_t L6_MAIN_Q31[CNN_C] = {
    1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824
};
static const qexp_t L6_MAIN_EXP[CNN_C] = {
    -3, -4, -5, -6, -3, -4, -5, -6, -3, -4, -5, -6, -3, -4, -5, -6
};
static const q31_t L6_SKIP_Q31 = 1073741824;
static const qexp_t L6_SKIP_EXP = 1;

// L7
static const body_weight_word_t L7_WBANK[9][CNN_C] = {
    {
        (((body_weight_word_t)0xfe02fe01ff0202feULL << 64) | (body_weight_word_t)0x0002020102020201ULL), (((body_weight_word_t)0xff02ff000101ffffULL << 64) | (body_weight_word_t)0xff02010101020201ULL), (((body_weight_word_t)0xff020000ff02fe00ULL << 64) | (body_weight_word_t)0x020101fefefeff01ULL), (((body_weight_word_t)0x00fffe0100fe01feULL << 64) | (body_weight_word_t)0x02ff00ff0100ffffULL),
        (((body_weight_word_t)0x02fe000201ffff00ULL << 64) | (body_weight_word_t)0x0002fe020101ff00ULL), (((body_weight_word_t)0x0200fefefe0202feULL << 64) | (body_weight_word_t)0x0202000102ff00feULL), (((body_weight_word_t)0x01000000ff020200ULL << 64) | (body_weight_word_t)0x010000ffff02ffffULL), (((body_weight_word_t)0xffff02ff02ff00ffULL << 64) | (body_weight_word_t)0xfeff0000ff0000ffULL),
        (((body_weight_word_t)0xfe020001ffff0102ULL << 64) | (body_weight_word_t)0x00fe02fe02fe00feULL), (((body_weight_word_t)0xffffffff02fe02feULL << 64) | (body_weight_word_t)0x02fffeff01ff0100ULL), (((body_weight_word_t)0x02fffe01feff01feULL << 64) | (body_weight_word_t)0x01ff0100010101feULL), (((body_weight_word_t)0x0102fe020202fe01ULL << 64) | (body_weight_word_t)0x01000101fefe01feULL),
        (((body_weight_word_t)0x020200fe01ff00feULL << 64) | (body_weight_word_t)0x0002fe00000102feULL), (((body_weight_word_t)0x0101fe01ff010201ULL << 64) | (body_weight_word_t)0x02fffe0001000002ULL), (((body_weight_word_t)0x01010001020200feULL << 64) | (body_weight_word_t)0xfe01000202000201ULL), (((body_weight_word_t)0x00ffff01fffeff00ULL << 64) | (body_weight_word_t)0x0002ffff01ffff01ULL)
    },
    {
        (((body_weight_word_t)0xff02fe0002ff01feULL << 64) | (body_weight_word_t)0x00fefe0102020202ULL), (((body_weight_word_t)0x0102000001ff01feULL << 64) | (body_weight_word_t)0xfe020002fffe0101ULL), (((body_weight_word_t)0xfefeff020101fffeULL << 64) | (body_weight_word_t)0x0100fefe0200fe01ULL), (((body_weight_word_t)0xfeff02020200fe02ULL << 64) | (body_weight_word_t)0xfe0001ff020002feULL),
        (((body_weight_word_t)0x0200fffe00fefffeULL << 64) | (body_weight_word_t)0x01fe00fffe0001feULL), (((body_weight_word_t)0x01ff0101ff00ffffULL << 64) | (body_weight_word_t)0xfeffffff02000202ULL), (((body_weight_word_t)0x01feff0101fefe02ULL << 64) | (body_weight_word_t)0xff00010002fefe00ULL), (((body_weight_word_t)0xfe01fe02fe020002ULL << 64) | (body_weight_word_t)0x000001fe00fefffeULL),
        (((body_weight_word_t)0x000100010000feffULL << 64) | (body_weight_word_t)0x01feff000201fe00ULL), (((body_weight_word_t)0x01010100fe000101ULL << 64) | (body_weight_word_t)0xffff01fffe0102ffULL), (((body_weight_word_t)0x00ff0102fe00fe01ULL << 64) | (body_weight_word_t)0xfffe01ffffff02feULL), (((body_weight_word_t)0x02fefe0002020200ULL << 64) | (body_weight_word_t)0xff01ff01ff0001ffULL),
        (((body_weight_word_t)0x0101fefffefe0002ULL << 64) | (body_weight_word_t)0x0000ff02fe000200ULL), (((body_weight_word_t)0xfffffe0101fe01feULL << 64) | (body_weight_word_t)0x0002fe02ff00fffeULL), (((body_weight_word_t)0xfe000001010200ffULL << 64) | (body_weight_word_t)0xfe020000fe00fe02ULL), (((body_weight_word_t)0x0001010101fe01ffULL << 64) | (body_weight_word_t)0x010202fe010202feULL)
    },
    {
        (((body_weight_word_t)0x0202ff0200000202ULL << 64) | (body_weight_word_t)0x000200ff02020001ULL), (((body_weight_word_t)0x01fffffffe02fe00ULL << 64) | (body_weight_word_t)0xfe0102fffefeff01ULL), (((body_weight_word_t)0x00fe0200020200feULL << 64) | (body_weight_word_t)0x00ff000200fe0200ULL), (((body_weight_word_t)0x00ff02fe020201ffULL << 64) | (body_weight_word_t)0xff00000001020200ULL),
        (((body_weight_word_t)0x01feff0201fe0102ULL << 64) | (body_weight_word_t)0xfe02feff0200ff01ULL), (((body_weight_word_t)0xfffefe01ff00fe02ULL << 64) | (body_weight_word_t)0xfe000101ff0102feULL), (((body_weight_word_t)0x0001000000fefeffULL << 64) | (body_weight_word_t)0x0200ff000101ffffULL), (((body_weight_word_t)0xfe000002fe01fe02ULL << 64) | (body_weight_word_t)0x02ff02fffe02fefeULL),
        (((body_weight_word_t)0x000002feff000001ULL << 64) | (body_weight_word_t)0xff0101ff00fe0101ULL), (((body_weight_word_t)0xfe00fefe020101feULL << 64) | (body_weight_word_t)0x0202fe00ff02ff00ULL), (((body_weight_word_t)0x00ff0201ff000001ULL << 64) | (body_weight_word_t)0x01fffe0201fefe01ULL), (((body_weight_word_t)0x00ff01fefffffefeULL << 64) | (body_weight_word_t)0x02feff0200fefeffULL),
        (((body_weight_word_t)0x00fe02ff000000ffULL << 64) | (body_weight_word_t)0xfffe0000fefe00feULL), (((body_weight_word_t)0xfe0100fffe02fefeULL << 64) | (body_weight_word_t)0x01fefeffff00fffeULL), (((body_weight_word_t)0xfe00fe0202ffffffULL << 64) | (body_weight_word_t)0xffff01ff00ff0102ULL), (((body_weight_word_t)0x01fe00ff02ff01ffULL << 64) | (body_weight_word_t)0xff0001fefe00ff00ULL)
    },
    {
        (((body_weight_word_t)0x00feff01ff000202ULL << 64) | (body_weight_word_t)0x01fe000001020001ULL), (((body_weight_word_t)0x01010201ff02ffffULL << 64) | (body_weight_word_t)0xfefefe02ff000202ULL), (((body_weight_word_t)0xfe01ff0102020102ULL << 64) | (body_weight_word_t)0x000001020102fe00ULL), (((body_weight_word_t)0x00fffe0202fffeffULL << 64) | (body_weight_word_t)0xffff02fe02020001ULL),
        (((body_weight_word_t)0x01feff02fefe0001ULL << 64) | (body_weight_word_t)0xfefefe00ffff02ffULL), (((body_weight_word_t)0xff0201fe00020102ULL << 64) | (body_weight_word_t)0x01010002ff010201ULL), (((body_weight_word_t)0x020001ff02000101ULL << 64) | (body_weight_word_t)0x0000fe00fe01ffffULL), (((body_weight_word_t)0xfeff010200fefe00ULL << 64) | (body_weight_word_t)0xfe0202ff02fe0100ULL),
        (((body_weight_word_t)0x0102fe0000fffe00ULL << 64) | (body_weight_word_t)0x00ff02ff000001feULL), (((body_weight_word_t)0xfffefefffefe00feULL << 64) | (body_weight_word_t)0x0200fffffeff0200ULL), (((body_weight_word_t)0x0000ff00fe0101ffULL << 64) | (body_weight_word_t)0xfe02ff01ff020001ULL), (((body_weight_word_t)0xfe01010002fefeffULL << 64) | (body_weight_word_t)0xff010101fffefe01ULL),
        (((body_weight_word_t)0xff02ff00fffefeffULL << 64) | (body_weight_word_t)0x02fe0101fe00fffeULL), (((body_weight_word_t)0x0101fe00ff02fe00ULL << 64) | (body_weight_word_t)0x02ff000000fe01ffULL), (((body_weight_word_t)0xfefeffffffff00feULL << 64) | (body_weight_word_t)0xff0101fffe010100ULL), (((body_weight_word_t)0x01fe0200fffefffeULL << 64) | (body_weight_word_t)0x0102ff0000fefefeULL)
    },
    {
        (((body_weight_word_t)0xfefefe0201fffe01ULL << 64) | (body_weight_word_t)0xfe010102010001ffULL), (((body_weight_word_t)0x00ff0202000100feULL << 64) | (body_weight_word_t)0x020200ffff000101ULL), (((body_weight_word_t)0x0200feffff000101ULL << 64) | (body_weight_word_t)0x01ff00000102ff01ULL), (((body_weight_word_t)0x00010102fe02fefeULL << 64) | (body_weight_word_t)0x0102fffffffe02ffULL),
        (((body_weight_word_t)0x010102000101ff01ULL << 64) | (body_weight_word_t)0xfe00fe01fefffffeULL), (((body_weight_word_t)0x02ff02fe000200ffULL << 64) | (body_weight_word_t)0x00fe02000202ff02ULL), (((body_weight_word_t)0x0002fefeff010102ULL << 64) | (body_weight_word_t)0x0002ff01000100feULL), (((body_weight_word_t)0xfffffefe02fefe02ULL << 64) | (body_weight_word_t)0x0002000201010002ULL),
        (((body_weight_word_t)0xff0002ff0100feffULL << 64) | (body_weight_word_t)0xfe00fffeff02ffffULL), (((body_weight_word_t)0x0202010001010002ULL << 64) | (body_weight_word_t)0x02ff020202fe0202ULL), (((body_weight_word_t)0x010202fffffffe02ULL << 64) | (body_weight_word_t)0xfe02ffff02fe02feULL), (((body_weight_word_t)0xff0202ff0000ffffULL << 64) | (body_weight_word_t)0xfe00ff020000feffULL),
        (((body_weight_word_t)0x01ff020102fffe01ULL << 64) | (body_weight_word_t)0xfe0202fe00ff01ffULL), (((body_weight_word_t)0x0102fffefefefe00ULL << 64) | (body_weight_word_t)0x00fefffe02010000ULL), (((body_weight_word_t)0xfe00ff0201fe00ffULL << 64) | (body_weight_word_t)0x01ff0002ffffff00ULL), (((body_weight_word_t)0xfe02ff0001fefe01ULL << 64) | (body_weight_word_t)0x0100fe0101000101ULL)
    },
    {
        (((body_weight_word_t)0x0000ff0002020100ULL << 64) | (body_weight_word_t)0xfefefe01feffffffULL), (((body_weight_word_t)0x000000fe02fe0102ULL << 64) | (body_weight_word_t)0xff0202fffefe00ffULL), (((body_weight_word_t)0x00fe02010002ff02ULL << 64) | (body_weight_word_t)0x00010101010101ffULL), (((body_weight_word_t)0xfffffe0002fefffeULL << 64) | (body_weight_word_t)0xfffffffe020102ffULL),
        (((body_weight_word_t)0xfffe000000010101ULL << 64) | (body_weight_word_t)0x02fe01fe01feff01ULL), (((body_weight_word_t)0xfefe01ffff0000ffULL << 64) | (body_weight_word_t)0x0002fe01fe01ff01ULL), (((body_weight_word_t)0xfefe0001fe000102ULL << 64) | (body_weight_word_t)0x0000010102ff0100ULL), (((body_weight_word_t)0x00020100ff01fe02ULL << 64) | (body_weight_word_t)0x0102ff02fe00ff02ULL),
        (((body_weight_word_t)0x02fefe02fe020002ULL << 64) | (body_weight_word_t)0xffff0200ff000100ULL), (((body_weight_word_t)0xfe02feffff0101feULL << 64) | (body_weight_word_t)0x01ff00ff00fefe02ULL), (((body_weight_word_t)0x0100010001ff0100ULL << 64) | (body_weight_word_t)0xff0001ffffff0102ULL), (((body_weight_word_t)0xfe0001ffff0202feULL << 64) | (body_weight_word_t)0x02ff02ff01fe01ffULL),
        (((body_weight_word_t)0xfeff02ff000200ffULL << 64) | (body_weight_word_t)0xff0102020101fe01ULL), (((body_weight_word_t)0x000002ffffff02ffULL << 64) | (body_weight_word_t)0x020002020100ff00ULL), (((body_weight_word_t)0xff000100fffeff02ULL << 64) | (body_weight_word_t)0x020102020202fffeULL), (((body_weight_word_t)0x00000100ff01fe00ULL << 64) | (body_weight_word_t)0x0102fe00010000feULL)
    },
    {
        (((body_weight_word_t)0x0201fe02feff0201ULL << 64) | (body_weight_word_t)0xfe00fe01010201ffULL), (((body_weight_word_t)0x0102fe01ff00ff02ULL << 64) | (body_weight_word_t)0xfffe02ff010202ffULL), (((body_weight_word_t)0x010001ffff02ff02ULL << 64) | (body_weight_word_t)0x00000101fe01ff01ULL), (((body_weight_word_t)0xfffe02020001ff01ULL << 64) | (body_weight_word_t)0x0100fe0001020200ULL),
        (((body_weight_word_t)0x0101ffff00ff0001ULL << 64) | (body_weight_word_t)0xfe0002ff00feff02ULL), (((body_weight_word_t)0x010202fe02fffefeULL << 64) | (body_weight_word_t)0xff02000000010001ULL), (((body_weight_word_t)0xff01fefeff02ff00ULL << 64) | (body_weight_word_t)0x02fe01010001fe00ULL), (((body_weight_word_t)0xfe02fffefeff0002ULL << 64) | (body_weight_word_t)0xff0100ff01fe0200ULL),
        (((body_weight_word_t)0x0102fefffffeffffULL << 64) | (body_weight_word_t)0xfffeff00ff01ff02ULL), (((body_weight_word_t)0x020102fe02fefe01ULL << 64) | (body_weight_word_t)0xfe0200fe0101fffeULL), (((body_weight_word_t)0xfe0200fefffe0000ULL << 64) | (body_weight_word_t)0x0002feffff01feffULL), (((body_weight_word_t)0x000002fe0100fe02ULL << 64) | (body_weight_word_t)0x02feff01ff00fe01ULL),
        (((body_weight_word_t)0x01010101ff010202ULL << 64) | (body_weight_word_t)0x01ff00feff0001ffULL), (((body_weight_word_t)0x00ff00ffff000201ULL << 64) | (body_weight_word_t)0x0100000100ff0201ULL), (((body_weight_word_t)0x0201020202fe00ffULL << 64) | (body_weight_word_t)0xff00020201fffe01ULL), (((body_weight_word_t)0x0100fe02ff02ff02ULL << 64) | (body_weight_word_t)0x01fffe0102fe02ffULL)
    },
    {
        (((body_weight_word_t)0x010102ff00fe00ffULL << 64) | (body_weight_word_t)0xfffe00fffefe01feULL), (((body_weight_word_t)0xfe0200fe0102ff00ULL << 64) | (body_weight_word_t)0x01ffffff01ff0102ULL), (((body_weight_word_t)0x00fe010000fe0201ULL << 64) | (body_weight_word_t)0x02ff00ff01feff01ULL), (((body_weight_word_t)0xfe0100ff020001feULL << 64) | (body_weight_word_t)0xff02ffff010202ffULL),
        (((body_weight_word_t)0x0201fe0002fe00feULL << 64) | (body_weight_word_t)0xfe010001fefe01ffULL), (((body_weight_word_t)0xff02010201000101ULL << 64) | (body_weight_word_t)0x020200feff0002feULL), (((body_weight_word_t)0x0200ff00fe00feffULL << 64) | (body_weight_word_t)0x01fefefe00ffff00ULL), (((body_weight_word_t)0x010200ff01ff0100ULL << 64) | (body_weight_word_t)0x01ff000101ffff01ULL),
        (((body_weight_word_t)0x01fe0100fffffefeULL << 64) | (body_weight_word_t)0xff02ff01ff000002ULL), (((body_weight_word_t)0x00fe01ff01feff02ULL << 64) | (body_weight_word_t)0xff02fefffefe0200ULL), (((body_weight_word_t)0xff020200fefe02feULL << 64) | (body_weight_word_t)0x02ff000101fffe02ULL), (((body_weight_word_t)0x0102fe0102ff0201ULL << 64) | (body_weight_word_t)0xfefffeff02fe01ffULL),
        (((body_weight_word_t)0xff02fffe010201feULL << 64) | (body_weight_word_t)0x01ff020102fe02feULL), (((body_weight_word_t)0x0000ff000202ff00ULL << 64) | (body_weight_word_t)0x02ff020202fe0100ULL), (((body_weight_word_t)0x00fe01ff00ff00feULL << 64) | (body_weight_word_t)0xff02000201fe02ffULL), (((body_weight_word_t)0x0102fe0202fe0101ULL << 64) | (body_weight_word_t)0x02fffeffff0000feULL)
    },
    {
        (((body_weight_word_t)0xfe01020100ff02feULL << 64) | (body_weight_word_t)0x0001fe0201fffefeULL), (((body_weight_word_t)0x01fffefe0001ffffULL << 64) | (body_weight_word_t)0x02ff0000fffe0200ULL), (((body_weight_word_t)0x020000fffefe02ffULL << 64) | (body_weight_word_t)0x02ff00ff00fffe00ULL), (((body_weight_word_t)0xff01ff02fe0102ffULL << 64) | (body_weight_word_t)0xfffe010002010000ULL),
        (((body_weight_word_t)0x02000000010101ffULL << 64) | (body_weight_word_t)0xff00ffff00010001ULL), (((body_weight_word_t)0x020202020102ff02ULL << 64) | (body_weight_word_t)0x01ffff02020001feULL), (((body_weight_word_t)0x0200fe010202fe02ULL << 64) | (body_weight_word_t)0xfe02fefe00020201ULL), (((body_weight_word_t)0xfe010200000001feULL << 64) | (body_weight_word_t)0xfe0102ff01fefe01ULL),
        (((body_weight_word_t)0x00ff01ff01010200ULL << 64) | (body_weight_word_t)0xff01feff0201fe02ULL), (((body_weight_word_t)0x02feff0001fe0200ULL << 64) | (body_weight_word_t)0x00ff0102fe000002ULL), (((body_weight_word_t)0xfeff02ff010101feULL << 64) | (body_weight_word_t)0x0001020202fe0102ULL), (((body_weight_word_t)0xff00010202fffeffULL << 64) | (body_weight_word_t)0x0201010002fe02ffULL),
        (((body_weight_word_t)0xfefe0200fffffe01ULL << 64) | (body_weight_word_t)0x00feff02fffe02ffULL), (((body_weight_word_t)0x02000100ff0001ffULL << 64) | (body_weight_word_t)0x02ffffff010001ffULL), (((body_weight_word_t)0xfeff0001fffe0001ULL << 64) | (body_weight_word_t)0x0002fefefeff0200ULL), (((body_weight_word_t)0xfffe000101010102ULL << 64) | (body_weight_word_t)0x0000ff02fe02fffeULL)
    }
};
static const bias_t L7_BIAS[CNN_C] = {
    66, 43, -26, 25, -124, -32, 67, 111, 37, 28, -70, -119, -31, 30, -99, 53
};
static const q31_t L7_Q31[CNN_C] = {
    1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824
};
static const qexp_t L7_EXP[CNN_C] = {
    -3, -4, -5, -6, -3, -4, -5, -6, -3, -4, -5, -6, -3, -4, -5, -6
};

// L8
static const body_weight_word_t L8_WBANK[9][CNN_C] = {
    {
        (((body_weight_word_t)0x00fe01fe0102ff01ULL << 64) | (body_weight_word_t)0x02010201feffff00ULL), (((body_weight_word_t)0xfeff010001ffff02ULL << 64) | (body_weight_word_t)0x02fe01000200fffeULL), (((body_weight_word_t)0xfe0201fe020002ffULL << 64) | (body_weight_word_t)0x0101ff01fffe02ffULL), (((body_weight_word_t)0x0102ff02fffe02ffULL << 64) | (body_weight_word_t)0x0102fffefe01ff00ULL),
        (((body_weight_word_t)0x0100000101000201ULL << 64) | (body_weight_word_t)0x010202fe0002ff01ULL), (((body_weight_word_t)0x02fffffeffff0102ULL << 64) | (body_weight_word_t)0x0002ff0102020002ULL), (((body_weight_word_t)0xfe00000002010101ULL << 64) | (body_weight_word_t)0x01fffffe00000201ULL), (((body_weight_word_t)0xfe0100ff0100fefeULL << 64) | (body_weight_word_t)0xff0201fe01feffffULL),
        (((body_weight_word_t)0xfeff00ff00fffefeULL << 64) | (body_weight_word_t)0x00fffe010002ffffULL), (((body_weight_word_t)0x0200ff0202000102ULL << 64) | (body_weight_word_t)0xfefefefeff01ffffULL), (((body_weight_word_t)0xff010200020102feULL << 64) | (body_weight_word_t)0xfffe010101ff0202ULL), (((body_weight_word_t)0x00010202ff0201feULL << 64) | (body_weight_word_t)0xfffe01fefefe0102ULL),
        (((body_weight_word_t)0x0002ff0100fffeffULL << 64) | (body_weight_word_t)0x02000200fe010001ULL), (((body_weight_word_t)0x01fe01fe01fe0202ULL << 64) | (body_weight_word_t)0x0102ffff000002ffULL), (((body_weight_word_t)0xfffe01fe02ff0101ULL << 64) | (body_weight_word_t)0x0200fffffeff01ffULL), (((body_weight_word_t)0x01ff020001010002ULL << 64) | (body_weight_word_t)0x02fe00fffe01fffeULL)
    },
    {
        (((body_weight_word_t)0xff00fefffe02fffeULL << 64) | (body_weight_word_t)0xfffe020201fefefeULL), (((body_weight_word_t)0xfefefffefe000200ULL << 64) | (body_weight_word_t)0x0202020202fe00feULL), (((body_weight_word_t)0x010202feff02feffULL << 64) | (body_weight_word_t)0xff0000010002ff01ULL), (((body_weight_word_t)0x0100ff00fffeff02ULL << 64) | (body_weight_word_t)0x010201fe020201feULL),
        (((body_weight_word_t)0x01ff0100000202ffULL << 64) | (body_weight_word_t)0xff02fe0101ff0201ULL), (((body_weight_word_t)0x020102feffffff02ULL << 64) | (body_weight_word_t)0x00ff0100fe01fffeULL), (((body_weight_word_t)0xfefffefefffffeffULL << 64) | (body_weight_word_t)0xff0000fefe010100ULL), (((body_weight_word_t)0x0001ff0102fe0201ULL << 64) | (body_weight_word_t)0x020101fe01020200ULL),
        (((body_weight_word_t)0xfe01010001020202ULL << 64) | (body_weight_word_t)0x01fe00fffeffff01ULL), (((body_weight_word_t)0x00fefe0201fe0200ULL << 64) | (body_weight_word_t)0x00ff01ff01010202ULL), (((body_weight_word_t)0xfe00fe02fe0200feULL << 64) | (body_weight_word_t)0x01ff010100feffffULL), (((body_weight_word_t)0x0001ff00000001ffULL << 64) | (body_weight_word_t)0x02ff0002fffffe02ULL),
        (((body_weight_word_t)0xff01feff0002ff00ULL << 64) | (body_weight_word_t)0xff0200fefffefe00ULL), (((body_weight_word_t)0x0101feffff0102ffULL << 64) | (body_weight_word_t)0xfe02010001fe00feULL), (((body_weight_word_t)0x020202ff0101feffULL << 64) | (body_weight_word_t)0x02fe000001020102ULL), (((body_weight_word_t)0xff00fe020000fe02ULL << 64) | (body_weight_word_t)0x010002ff01010002ULL)
    },
    {
        (((body_weight_word_t)0x020001000200fefeULL << 64) | (body_weight_word_t)0xfeffff02010002feULL), (((body_weight_word_t)0xfe0202010102fefeULL << 64) | (body_weight_word_t)0xfe01fe02ff02ff02ULL), (((body_weight_word_t)0xff01010001020202ULL << 64) | (body_weight_word_t)0x00fefefe020201feULL), (((body_weight_word_t)0x01010000ff0000ffULL << 64) | (body_weight_word_t)0xfefe000001feff01ULL),
        (((body_weight_word_t)0xff0000fffffe0101ULL << 64) | (body_weight_word_t)0xffff00feff0100ffULL), (((body_weight_word_t)0xff0200feffff0000ULL << 64) | (body_weight_word_t)0x0101000102fe0001ULL), (((body_weight_word_t)0x000001000201ffffULL << 64) | (body_weight_word_t)0x02ff0202fe0001feULL), (((body_weight_word_t)0xff01fefe01020002ULL << 64) | (body_weight_word_t)0xfeff00ff02fe01ffULL),
        (((body_weight_word_t)0x01fefe0202010101ULL << 64) | (body_weight_word_t)0x00ff01fe02010102ULL), (((body_weight_word_t)0x0002ff00feff00ffULL << 64) | (body_weight_word_t)0x00010002fffffe00ULL), (((body_weight_word_t)0x01fffeffffffff01ULL << 64) | (body_weight_word_t)0xff0002fe01020200ULL), (((body_weight_word_t)0x0100fffe0001ff00ULL << 64) | (body_weight_word_t)0x0101fefe00ff01feULL),
        (((body_weight_word_t)0x01feff000001fe01ULL << 64) | (body_weight_word_t)0x01fe02fe00000101ULL), (((body_weight_word_t)0xff0202fe02010201ULL << 64) | (body_weight_word_t)0x0201fe000101ff02ULL), (((body_weight_word_t)0xfe01fffe0002fffeULL << 64) | (body_weight_word_t)0x0201fe0100fefe00ULL), (((body_weight_word_t)0x00020201ff0000feULL << 64) | (body_weight_word_t)0xfe01020202ff00feULL)
    },
    {
        (((body_weight_word_t)0xff010200ff020002ULL << 64) | (body_weight_word_t)0x01ffff0101020101ULL), (((body_weight_word_t)0x01fe010202fe02feULL << 64) | (body_weight_word_t)0x00ff01ffff0000ffULL), (((body_weight_word_t)0xff01ff000101fffeULL << 64) | (body_weight_word_t)0x0001feff0102ff02ULL), (((body_weight_word_t)0x02020100ffff0000ULL << 64) | (body_weight_word_t)0x01ff0100fefe0201ULL),
        (((body_weight_word_t)0xff0001fffeff02feULL << 64) | (body_weight_word_t)0xff02ff00ff000202ULL), (((body_weight_word_t)0x01fefefe000000feULL << 64) | (body_weight_word_t)0x000201feffff01ffULL), (((body_weight_word_t)0x0201fffefe02fffeULL << 64) | (body_weight_word_t)0x01010002ff010200ULL), (((body_weight_word_t)0xfe02ff020200fe02ULL << 64) | (body_weight_word_t)0x000201ff00020102ULL),
        (((body_weight_word_t)0x010000fe02feff01ULL << 64) | (body_weight_word_t)0x02ff02ff01feff02ULL), (((body_weight_word_t)0xff000002010001feULL << 64) | (body_weight_word_t)0x01fffefefe02feffULL), (((body_weight_word_t)0xfffefeffff000001ULL << 64) | (body_weight_word_t)0x0001ff0200010101ULL), (((body_weight_word_t)0xff000100ff0100feULL << 64) | (body_weight_word_t)0x02fe0100fe01fefeULL),
        (((body_weight_word_t)0x0102fffffe0002feULL << 64) | (body_weight_word_t)0x01ff000102020101ULL), (((body_weight_word_t)0x02fefe010002fe00ULL << 64) | (body_weight_word_t)0x0101ff0000000002ULL), (((body_weight_word_t)0xfffe01fffefffffeULL << 64) | (body_weight_word_t)0x0200fe000100fe02ULL), (((body_weight_word_t)0xff000200fe000000ULL << 64) | (body_weight_word_t)0x00feffff02020102ULL)
    },
    {
        (((body_weight_word_t)0x00fffe0200fe0201ULL << 64) | (body_weight_word_t)0x01000200ff00fe01ULL), (((body_weight_word_t)0x02fe01ff00fefefeULL << 64) | (body_weight_word_t)0x02ff02fe0000feffULL), (((body_weight_word_t)0x0000000201fffffeULL << 64) | (body_weight_word_t)0x01fe02fe02feff01ULL), (((body_weight_word_t)0xfeffff00fe00fe01ULL << 64) | (body_weight_word_t)0xfffe00fffffffefeULL),
        (((body_weight_word_t)0xfffefe00ff010101ULL << 64) | (body_weight_word_t)0xffff00feff01ff01ULL), (((body_weight_word_t)0xfe01fffeff00fe02ULL << 64) | (body_weight_word_t)0x000100ffff0101ffULL), (((body_weight_word_t)0xfeff020100020100ULL << 64) | (body_weight_word_t)0x0101fffe01fe00feULL), (((body_weight_word_t)0x02ff0101ffff0101ULL << 64) | (body_weight_word_t)0xfe01fffeff0000feULL),
        (((body_weight_word_t)0x01ff0002ff0102feULL << 64) | (body_weight_word_t)0xffff01000001fe01ULL), (((body_weight_word_t)0xfe00ff010202feffULL << 64) | (body_weight_word_t)0x01fe0200ff00ff01ULL), (((body_weight_word_t)0xfe0000feffff0001ULL << 64) | (body_weight_word_t)0xfe00ff0001fffe02ULL), (((body_weight_word_t)0x00000101fffefefeULL << 64) | (body_weight_word_t)0xfe01fffefefefffeULL),
        (((body_weight_word_t)0x01fe0001fffe01ffULL << 64) | (body_weight_word_t)0x00ff00fefe000202ULL), (((body_weight_word_t)0x02fe020200010001ULL << 64) | (body_weight_word_t)0xfffefeff0101fe00ULL), (((body_weight_word_t)0x0201fefe000200ffULL << 64) | (body_weight_word_t)0x00010000010002feULL), (((body_weight_word_t)0x02000200fefe01feULL << 64) | (body_weight_word_t)0x000201020100ff02ULL)
    },
    {
        (((body_weight_word_t)0x00ff0102ffff0100ULL << 64) | (body_weight_word_t)0xfeff02ff0101ffffULL), (((body_weight_word_t)0x01fe00fe00fe0202ULL << 64) | (body_weight_word_t)0x0201ffff0102ffffULL), (((body_weight_word_t)0x0201ff01ff0101feULL << 64) | (body_weight_word_t)0x00ff01ff0000feffULL), (((body_weight_word_t)0x01020000ffff01ffULL << 64) | (body_weight_word_t)0x02ff0102ff02fe01ULL),
        (((body_weight_word_t)0x01fefeffff0201feULL << 64) | (body_weight_word_t)0x00fffe0000000100ULL), (((body_weight_word_t)0x00fe00ff02fffe01ULL << 64) | (body_weight_word_t)0x0000020000ff0202ULL), (((body_weight_word_t)0x0200ff00fffffffeULL << 64) | (body_weight_word_t)0xff01000202fe0001ULL), (((body_weight_word_t)0xff00ff0102feff01ULL << 64) | (body_weight_word_t)0xfe0102ff00010001ULL),
        (((body_weight_word_t)0x0102fefefe0201ffULL << 64) | (body_weight_word_t)0x02ff00ff0200fe01ULL), (((body_weight_word_t)0x0001fe02ffff0200ULL << 64) | (body_weight_word_t)0xfeff02010201fefeULL), (((body_weight_word_t)0xfe020100fe00fe02ULL << 64) | (body_weight_word_t)0x00fe0000fe01ffffULL), (((body_weight_word_t)0x00020200fe0101feULL << 64) | (body_weight_word_t)0x0002ff0200000101ULL),
        (((body_weight_word_t)0xfefffe00fe010101ULL << 64) | (body_weight_word_t)0xff02ff0000fefefeULL), (((body_weight_word_t)0xfffe0101fe01fe00ULL << 64) | (body_weight_word_t)0x0102fe02000002feULL), (((body_weight_word_t)0x0100ffffff01ffffULL << 64) | (body_weight_word_t)0x01000001ff010101ULL), (((body_weight_word_t)0xfefffefe01fffe00ULL << 64) | (body_weight_word_t)0x0102feff010000feULL)
    },
    {
        (((body_weight_word_t)0xfe01ff01fefe01ffULL << 64) | (body_weight_word_t)0xfeff000202fe0202ULL), (((body_weight_word_t)0x00ff00feffff0100ULL << 64) | (body_weight_word_t)0x02fe01fefe020202ULL), (((body_weight_word_t)0xff000100010201feULL << 64) | (body_weight_word_t)0x020200fe02ff0200ULL), (((body_weight_word_t)0x0100fffffefefefeULL << 64) | (body_weight_word_t)0xff01fefe0001fe01ULL),
        (((body_weight_word_t)0xfe01fefe01fffe02ULL << 64) | (body_weight_word_t)0x0100020102010200ULL), (((body_weight_word_t)0xfe020202fe02feffULL << 64) | (body_weight_word_t)0x000102ff01020102ULL), (((body_weight_word_t)0x0002fe01010100feULL << 64) | (body_weight_word_t)0xff0101fefe0101ffULL), (((body_weight_word_t)0xfefffe0101000201ULL << 64) | (body_weight_word_t)0x00010201fe0000feULL),
        (((body_weight_word_t)0x0200fe020201fe02ULL << 64) | (body_weight_word_t)0x00ff00fe01ff00feULL), (((body_weight_word_t)0xfeff0201ffff0202ULL << 64) | (body_weight_word_t)0x02fefe010101ff02ULL), (((body_weight_word_t)0x00fe01ffff0001ffULL << 64) | (body_weight_word_t)0xfe02fefffe01fe00ULL), (((body_weight_word_t)0x02fefe010102ffffULL << 64) | (body_weight_word_t)0x0001000101fe0200ULL),
        (((body_weight_word_t)0xffffff00fe020102ULL << 64) | (body_weight_word_t)0xff02fe0000020202ULL), (((body_weight_word_t)0x0202fefe0100fe01ULL << 64) | (body_weight_word_t)0x010100fffeffff01ULL), (((body_weight_word_t)0x0002fe01fe02ff01ULL << 64) | (body_weight_word_t)0xfeff01fffeff0002ULL), (((body_weight_word_t)0xfefefe0202ff0200ULL << 64) | (body_weight_word_t)0xfffe0101ff0002feULL)
    },
    {
        (((body_weight_word_t)0x0100fe020200fffeULL << 64) | (body_weight_word_t)0x02ff0102fe000202ULL), (((body_weight_word_t)0x00000200000102feULL << 64) | (body_weight_word_t)0x0201fefeffff0002ULL), (((body_weight_word_t)0x0002fefe020000ffULL << 64) | (body_weight_word_t)0xff02fefefefffeffULL), (((body_weight_word_t)0xfe0002fffffe0100ULL << 64) | (body_weight_word_t)0xff01fefe01000201ULL),
        (((body_weight_word_t)0xfe020100010000feULL << 64) | (body_weight_word_t)0xff0102ff0002fe00ULL), (((body_weight_word_t)0xfe00fe01feffff00ULL << 64) | (body_weight_word_t)0x01000200fe020100ULL), (((body_weight_word_t)0x0000fefffe01ff02ULL << 64) | (body_weight_word_t)0xfe00fffe01fe0202ULL), (((body_weight_word_t)0xfefe0101feff00feULL << 64) | (body_weight_word_t)0xfefefe00010100ffULL),
        (((body_weight_word_t)0x01ffffffff00fefeULL << 64) | (body_weight_word_t)0xff0100fffe0000feULL), (((body_weight_word_t)0xff0101ff01ff0000ULL << 64) | (body_weight_word_t)0x0102feff02020201ULL), (((body_weight_word_t)0x01000201020202ffULL << 64) | (body_weight_word_t)0x000201020201ff01ULL), (((body_weight_word_t)0x0100fffe020101ffULL << 64) | (body_weight_word_t)0xfffe0102000200feULL),
        (((body_weight_word_t)0xff0000010101fe02ULL << 64) | (body_weight_word_t)0x00ff01fe02010202ULL), (((body_weight_word_t)0x020001020102ff00ULL << 64) | (body_weight_word_t)0xfffefeff0000ffffULL), (((body_weight_word_t)0xff02ff01ff010202ULL << 64) | (body_weight_word_t)0x0200fe0000ff0100ULL), (((body_weight_word_t)0xfe01020201fe0201ULL << 64) | (body_weight_word_t)0x0201fefe02fffffeULL)
    },
    {
        (((body_weight_word_t)0x0100010000010100ULL << 64) | (body_weight_word_t)0x0002fe0002010201ULL), (((body_weight_word_t)0xfefe0101fffe01ffULL << 64) | (body_weight_word_t)0x02ffff010200fffeULL), (((body_weight_word_t)0x010002020202fe02ULL << 64) | (body_weight_word_t)0xffffff01fefffefeULL), (((body_weight_word_t)0xffff0102fefffe01ULL << 64) | (body_weight_word_t)0x00fffffe01000200ULL),
        (((body_weight_word_t)0x0100fe020002ff00ULL << 64) | (body_weight_word_t)0xfe000200feff00feULL), (((body_weight_word_t)0x0201ff0201ff02feULL << 64) | (body_weight_word_t)0xfe01ff00010100ffULL), (((body_weight_word_t)0xff000202fe000201ULL << 64) | (body_weight_word_t)0x01fefe01fe02fe00ULL), (((body_weight_word_t)0xfefefe0001000201ULL << 64) | (body_weight_word_t)0x000101010101ff01ULL),
        (((body_weight_word_t)0x00020001ff00fe00ULL << 64) | (body_weight_word_t)0xfeff0200ff01ff00ULL), (((body_weight_word_t)0x0102fffefeff02ffULL << 64) | (body_weight_word_t)0xfe000102fe0101ffULL), (((body_weight_word_t)0x02ff0001000101feULL << 64) | (body_weight_word_t)0x01fe01feff010002ULL), (((body_weight_word_t)0x01fe020000fe00ffULL << 64) | (body_weight_word_t)0x000000fe0002fffeULL),
        (((body_weight_word_t)0x01ff01ff02010102ULL << 64) | (body_weight_word_t)0x02ff00fe01000002ULL), (((body_weight_word_t)0x0101fe0202fe01feULL << 64) | (body_weight_word_t)0x0000fefe01010202ULL), (((body_weight_word_t)0xff020200fffe0100ULL << 64) | (body_weight_word_t)0x010202fffefe0001ULL), (((body_weight_word_t)0x0101ff0102fe0201ULL << 64) | (body_weight_word_t)0x0100feffff0002ffULL)
    }
};
static const bias_t L8_BIAS[CNN_C] = {
    128, 55, 97, -72, -99, 34, -114, -17, -110, 113, 69, 97, -75, -16, -57, -66
};
static const q31_t L8_MAIN_Q31[CNN_C] = {
    1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824, 1073741824
};
static const qexp_t L8_MAIN_EXP[CNN_C] = {
    -3, -4, -5, -6, -3, -4, -5, -6, -3, -4, -5, -6, -3, -4, -5, -6
};
static const q31_t L8_SKIP_Q31 = 1073741824;
static const qexp_t L8_SKIP_EXP = 1;

// L9
static const tail_weight_word_t L9_WBANK[9][CNN_C] = {
    {
        (tail_weight_word_t)0xfc11fef7U, (tail_weight_word_t)0xfe170519U, (tail_weight_word_t)0x0d09f8f0U, (tail_weight_word_t)0x04040cf6U,
        (tail_weight_word_t)0xfeec12f6U, (tail_weight_word_t)0x17040adbU, (tail_weight_word_t)0xe9030418U, (tail_weight_word_t)0x0507f911U,
        (tail_weight_word_t)0xf602ebefU, (tail_weight_word_t)0x0002fcfcU, (tail_weight_word_t)0x06f51ef9U, (tail_weight_word_t)0x1311f705U,
        (tail_weight_word_t)0xf4f60100U, (tail_weight_word_t)0xf0ff0408U, (tail_weight_word_t)0x04170102U, (tail_weight_word_t)0xfcf80901U
    },
    {
        (tail_weight_word_t)0x15f426f1U, (tail_weight_word_t)0xf61125e9U, (tail_weight_word_t)0xeb1c24f4U, (tail_weight_word_t)0xfd101008U,
        (tail_weight_word_t)0xf1f31103U, (tail_weight_word_t)0xec1ae7dbU, (tail_weight_word_t)0x1eeadd7fU, (tail_weight_word_t)0xff0912faU,
        (tail_weight_word_t)0xfaf2f20eU, (tail_weight_word_t)0x040708ffU, (tail_weight_word_t)0x100505e4U, (tail_weight_word_t)0xe4e71b0dU,
        (tail_weight_word_t)0xfe140805U, (tail_weight_word_t)0xfbf3fc20U, (tail_weight_word_t)0x14ea2aefU, (tail_weight_word_t)0x061ffaeaU
    },
    {
        (tail_weight_word_t)0xf5f1e110U, (tail_weight_word_t)0xf809f6e9U, (tail_weight_word_t)0x00e1f81dU, (tail_weight_word_t)0xfcfe02feU,
        (tail_weight_word_t)0xf0ee100aU, (tail_weight_word_t)0x1303ea15U, (tail_weight_word_t)0xe30f09c8U, (tail_weight_word_t)0xf0fae7ffU,
        (tail_weight_word_t)0xfa070203U, (tail_weight_word_t)0xfa030b0fU, (tail_weight_word_t)0x1df20c1bU, (tail_weight_word_t)0x10f6171cU,
        (tail_weight_word_t)0xf2f817ffU, (tail_weight_word_t)0xf6fdf0ddU, (tail_weight_word_t)0xf312e5fcU, (tail_weight_word_t)0x08ff0213U
    },
    {
        (tail_weight_word_t)0xeceb08fcU, (tail_weight_word_t)0xeef30219U, (tail_weight_word_t)0xe0111c09U, (tail_weight_word_t)0x02eafff8U,
        (tail_weight_word_t)0xa7f0f104U, (tail_weight_word_t)0x035ef5deU, (tail_weight_word_t)0x44d12806U, (tail_weight_word_t)0x101d0910U,
        (tail_weight_word_t)0x0df31316U, (tail_weight_word_t)0x03fa00ffU, (tail_weight_word_t)0x0c29ddf1U, (tail_weight_word_t)0x03e91508U,
        (tail_weight_word_t)0xf510fd0dU, (tail_weight_word_t)0x08d9d505U, (tail_weight_word_t)0xf7ea0912U, (tail_weight_word_t)0xf4170d0fU
    },
    {
        (tail_weight_word_t)0x07fce534U, (tail_weight_word_t)0x0e3f07ffU, (tail_weight_word_t)0x1bd597d7U, (tail_weight_word_t)0xfbfd0cf3U,
        (tail_weight_word_t)0x7fdcc7ccU, (tail_weight_word_t)0xc2c4a258U, (tail_weight_word_t)0xe9ef81e5U, (tail_weight_word_t)0xe2cb10d2U,
        (tail_weight_word_t)0x174203f2U, (tail_weight_word_t)0xfaddfc06U, (tail_weight_word_t)0xcca91145U, (tail_weight_word_t)0xc97ffeffU,
        (tail_weight_word_t)0x0bd6f4fdU, (tail_weight_word_t)0x119b33c6U, (tail_weight_word_t)0xe61ce5deU, (tail_weight_word_t)0x0ec4f4c2U
    },
    {
        (tail_weight_word_t)0xf30c0be1U, (tail_weight_word_t)0x06e80efbU, (tail_weight_word_t)0x18371f58U, (tail_weight_word_t)0xf7fdef06U,
        (tail_weight_word_t)0x18370604U, (tail_weight_word_t)0xbb11e9b9U, (tail_weight_word_t)0xfe2af8edU, (tail_weight_word_t)0x01000c22U,
        (tail_weight_word_t)0x09f0e306U, (tail_weight_word_t)0xf713fa07U, (tail_weight_word_t)0x39bc1418U, (tail_weight_word_t)0xfa2004c2U,
        (tail_weight_word_t)0x0a1b08f4U, (tail_weight_word_t)0xb2142200U, (tail_weight_word_t)0x22def41eU, (tail_weight_word_t)0x0c0001fcU
    },
    {
        (tail_weight_word_t)0x01dd09f9U, (tail_weight_word_t)0x0604f803U, (tail_weight_word_t)0x08def5f4U, (tail_weight_word_t)0x0f0e0103U,
        (tail_weight_word_t)0x0315081cU, (tail_weight_word_t)0xed440007U, (tail_weight_word_t)0xee14fa05U, (tail_weight_word_t)0x0409fa14U,
        (tail_weight_word_t)0x0f00fe07U, (tail_weight_word_t)0x0704f4f8U, (tail_weight_word_t)0x032c1717U, (tail_weight_word_t)0xedf20508U,
        (tail_weight_word_t)0xfbeffc03U, (tail_weight_word_t)0x0b0efafeU, (tail_weight_word_t)0xfbd907feU, (tail_weight_word_t)0xf8020204U
    },
    {
        (tail_weight_word_t)0x0c2ffee5U, (tail_weight_word_t)0xf7f20009U, (tail_weight_word_t)0xfe312de7U, (tail_weight_word_t)0xfee4fbffU,
        (tail_weight_word_t)0xf15df528U, (tail_weight_word_t)0x42042513U, (tail_weight_word_t)0x133d1abeU, (tail_weight_word_t)0x1a19ff16U,
        (tail_weight_word_t)0xecdf06feU, (tail_weight_word_t)0x01fbfa06U, (tail_weight_word_t)0x0037fec1U, (tail_weight_word_t)0x24c4ea03U,
        (tail_weight_word_t)0x1c0501f6U, (tail_weight_word_t)0x0fede621U, (tail_weight_word_t)0xe900f70aU, (tail_weight_word_t)0x0900f118U
    },
    {
        (tail_weight_word_t)0x07e2f40fU, (tail_weight_word_t)0x090be8feU, (tail_weight_word_t)0xe9090cf2U, (tail_weight_word_t)0xf7f1f1f7U,
        (tail_weight_word_t)0xf9e0f4f6U, (tail_weight_word_t)0x38f80719U, (tail_weight_word_t)0xfef61c00U, (tail_weight_word_t)0xf702fef4U,
        (tail_weight_word_t)0xff16f410U, (tail_weight_word_t)0x0507fc05U, (tail_weight_word_t)0xe1fd1d10U, (tail_weight_word_t)0x1ac5ee11U,
        (tail_weight_word_t)0x1ceef510U, (tail_weight_word_t)0x203a23feU, (tail_weight_word_t)0xfb2a160cU, (tail_weight_word_t)0xfcfcfffeU
    }
};
static const bias_t L9_BIAS[4] = {107, 435, -79, -12};
static const q31_t L9_TAIL_Q31[4] = {1355755264, 1666328192, 1306489344, 1617399680};
static const qexp_t L9_TAIL_EXP[4] = {-10, -10, -10, -10};
static const q31_t L9_GLOBAL_SKIP_Q31 = 2053364429;
static const qexp_t L9_GLOBAL_SKIP_EXP = 0;


static const q31_t L9_OUT_RAW_Q31 = 1979141884;
static const qexp_t L9_OUT_RAW_EXP = 3;

#endif
