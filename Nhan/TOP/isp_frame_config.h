#ifndef ISP_FRAME_CONFIG_H_
#define ISP_FRAME_CONFIG_H_


#ifndef ISP_FRAME_WIDTH
#define ISP_FRAME_WIDTH 1920
#endif

#ifndef ISP_FRAME_HEIGHT
#define ISP_FRAME_HEIGHT 1080
#endif

#define ISP_FRAME_PIXELS (ISP_FRAME_WIDTH * ISP_FRAME_HEIGHT)

// So frame toi da trong mot ap_ctrl_hs transaction.
// Testbench dung 2; gioi han nay chi de HLS uoc luong tripcount.
#ifndef ISP_MAX_FRAMES_PER_TRANSACTION
#define ISP_MAX_FRAMES_PER_TRANSACTION 255
#endif

#define ISP_DEMOSAIC_DEFAULT_THRESHOLD 270
#define ISP_DEMOSAIC_DEFAULT_EDGE_MAG  23

// Hai tang cua so 3x3, moi tang can WIDTH+1 beat warm-up.
#define ISP_DEMOSAIC_DELAY (2 * ISP_FRAME_WIDTH + 2)

#if ISP_FRAME_WIDTH < 8
#error "ISP_FRAME_WIDTH must be at least 8 for the direction ping-pong banks"
#endif

#if ISP_FRAME_HEIGHT < 2
#error "ISP_FRAME_HEIGHT must be at least 2"
#endif

#endif
