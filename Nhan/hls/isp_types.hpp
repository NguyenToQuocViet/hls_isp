#pragma once

#include "../../isp_pixel_packet.hpp"

#include <ap_fixed.h>
#include <ap_int.h>
#include <hls_stream.h>

using Raw10Packet = IspPixelPacket<10>;
using Raw12Packet = IspPixelPacket<12>;
using Rgb36Packet = IspPixelPacket<36>;

using Raw10Stream = hls::stream<Raw10Packet>;
using Raw12Stream = hls::stream<Raw12Packet>;
using Rgb36Stream = hls::stream<Rgb36Packet>;

using WbGain = ap_ufixed<16, 4>;
using CcmCoeff = ap_fixed<16, 4>;
