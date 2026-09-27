/*
Project: Adaptive Directional BPC and BLC
Module: HLS ISP Frame Dimensions
Description: Define frame dimensions shared by the BLC and BPC HLS blocks.
Author: Viet Nguyen To Quoc
*/

#ifndef ISP_FRAME_WIDTH
#define ISP_FRAME_WIDTH 1920
#endif

#ifndef ISP_FRAME_HEIGHT
#define ISP_FRAME_HEIGHT 1080
#endif

constexpr int FRAME_WIDTH  = ISP_FRAME_WIDTH;
constexpr int FRAME_HEIGHT = ISP_FRAME_HEIGHT;
