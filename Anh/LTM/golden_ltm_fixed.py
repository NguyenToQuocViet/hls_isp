import argparse
import glob
import os
import struct
import numpy as np

def box_filter_sum_exact(img):
    padded = np.pad(img, ((3, 3), (3, 3)), mode='edge')
    row_sum = np.zeros_like(padded)
    for i in range(7):
        row_sum[:, 3:-3] += padded[:, i:padded.shape[1]-6+i]
    col_sum = np.zeros((img.shape[0], img.shape[1]), dtype=np.int64)
    for i in range(7):
        col_sum += row_sum[i:padded.shape[0]-6+i, 3:-3]
    return col_sum

def cpp_div(a, b):
    # C++ integer division truncates towards zero
    res = (np.abs(a) // np.abs(b)) * np.sign(a) * np.sign(b)
    return res

def process_frame(img_in):
    # img_in is (H, W, 3) uint16
    H, W, _ = img_in.shape
    
    # 0. Generate LUTs exactly as in C++
    log_lut = np.zeros(4096, dtype=np.int64)
    for i in range(4096):
        val = np.log(i + 1.0)
        # log_t is ap_fixed<16,5>, F=11
        v = int(np.floor(val * 2048)) & 0xFFFF
        if v >= 32768: v -= 65536
        log_lut[i] = v
        
    reinhard_lut = np.zeros(1024, dtype=np.int64)
    exposure_gain = 3.0
    target_max = 4095.0
    log_target_max = np.log(target_max + 1.0)
    for i in range(1024):
        log_I_base = i / 123.0
        L = (log_I_base / log_target_max) * exposure_gain
        L_white = exposure_gain
        L_mapped = (L * (1.0 + (L / (L_white * L_white)))) / (1.0 + L)
        log_I_mapped_base = L_mapped * log_target_max
        v = int(np.floor(log_I_mapped_base * 2048)) & 0xFFFF
        if v >= 32768: v -= 65536
        reinhard_lut[i] = v
        
    exp_lut = np.zeros(1024, dtype=np.int64)
    for i in range(1024):
        log_I_final = i / 123.0
        I_final = np.exp(log_I_final) - 1.0
        if I_final > 4095.0: I_final = 4095.0
        if I_final < 0.0: I_final = 0.0
        v = int(np.floor(I_final * 64)) & 0x3FFFF
        exp_lut[i] = v
        
    # Convert input to int64
    r_val = img_in[:, :, 0].astype(np.int64)
    g_val = img_in[:, :, 1].astype(np.int64)
    b_val = img_in[:, :, 2].astype(np.int64)
    
    # Luminance
    # ap_ufixed<16, 0> constants
    c_r = 19595  # floor(0.299 * 65536)
    c_g = 38469  # floor(0.587 * 65536)
    c_b = 7471   # floor(0.114 * 65536)
    c_half = 32  # 0.5f in ap_ufixed<18, 12> (F=6)
    
    r_f = (r_val * c_r) >> 10
    g_f = (g_val * c_g) >> 10
    b_f = (b_val * c_b) >> 10
    
    y_sum = r_f + g_f + b_f + c_half
    y_int = y_sum >> 6
    y_int = np.clip(y_int, 0, 4095)
    
    log_y = log_lut[y_int]  # F=11
    
    # Conv1
    pixel_in_var = log_y >> 1  # cast to var_t F=10
    pixel_sq_in = (pixel_in_var * pixel_in_var) >> 10 # cast product to var_t F=10
    pixel_sq_in = pixel_sq_in & 0x3FFFF
    pixel_sq_in = np.where(pixel_sq_in >= 0x20000, pixel_sq_in - 0x40000, pixel_sq_in)
    
    sum_I = box_filter_sum_exact(log_y)       # F=11
    sum_II = box_filter_sum_exact(pixel_sq_in) # F=10
    
    # 0.020408f as ap_ufixed<16, 0> = 1337
    mean_I = (sum_I * 1337) >> 16   # F=11
    mean_II = (sum_II * 1337) >> 16 # F=10
    
    mean_I_sq = (mean_I * mean_I) >> 12
    mean_I_sq = mean_I_sq & 0x3FFFF
    mean_I_sq = np.where(mean_I_sq >= 0x20000, mean_I_sq - 0x40000, mean_I_sq)
    
    var_I = mean_II - mean_I_sq
    var_I = np.maximum(var_I, 0)

    eps1 = 1      # 0.001 * 1024 in F=10
    eps_base = 512 # 0.5 * 1024 in F=10
    
    eps_map_raw = cpp_div(eps_base << 20, var_I + eps1) >> 10 # F=10
    eps_map = eps_map_raw & 0xFFFFF
    eps_map = np.where(eps_map >= 0x80000, eps_map - 0x100000, eps_map)
    
    a_ext_raw = cpp_div(var_I << 20, var_I + eps_map) >> 10 # F=10
    a_ext = a_ext_raw & 0xFFFFF
    a_ext = np.where(a_ext >= 0x80000, a_ext - 0x100000, a_ext)
    a_ext = np.minimum(a_ext, 614) # 0.6 * 1024 = 614
    
    out_a_val = np.clip(a_ext << 1, 0, 4095) # cast F=10 to F=11

    
    out_b_val_sub = (out_a_val * mean_I) >> 12 # cast to F=10
    out_b_val = (mean_I - (out_b_val_sub << 1)) >> 1 # F=10
    out_b_val = out_b_val & 0x7FFF
    out_b_val = np.where(out_b_val >= 0x4000, out_b_val - 0x8000, out_b_val)
    
    # Conv2
    sum_a = box_filter_sum_exact(out_a_val) # F=11
    sum_b = box_filter_sum_exact(out_b_val) # F=10
    
    mean_a = (sum_a * 1337 + 32768) >> 16 # F=11, AP_RND
    mean_a = np.clip(mean_a, 0, 4095) # AP_SAT
    mean_b = (sum_b * 1337) >> 16 # F=10
    
    # Recombine
    log_I_base_prod = ((mean_a >> 1) * (log_y >> 1)) >> 10 # cast F=20 to F=10
    log_I_base = log_I_base_prod + mean_b # F=10
    
    log_I_detail = log_y - (log_I_base << 1) # cast log_I_base to F=11
    log_I_detail = log_I_detail & 0xFFFF
    log_I_detail = np.where(log_I_detail >= 0x8000, log_I_detail - 0x10000, log_I_detail)
    
    idx_float = ((log_I_base >> 2) * 31488) >> 8
    lut_idx = (idx_float + 128) >> 8
    lut_idx = np.clip(lut_idx, 0, 1023)
    
    log_I_mapped_base = reinhard_lut[lut_idx] # F=11
    
    w_prod = log_I_mapped_base * 64
    w = ((3194 << 11) - w_prod) >> 11 # F=11
    w = w & 0x3FFF
    w = np.where(w >= 0x2000, w - 0x4000, w)
    
    log_I_final_prod = w * log_I_detail
    log_I_final = ((log_I_mapped_base << 11) + log_I_final_prod) >> 11 # F=11
    log_I_final = log_I_final & 0xFFFF
    log_I_final = np.where(log_I_final >= 0x8000, log_I_final - 0x10000, log_I_final)
    
    exp_idx_float = ((log_I_final << 1) * 503808) >> 12
    exp_lut_idx = (exp_idx_float + 2048) >> 12
    exp_lut_idx = np.clip(exp_lut_idx, 0, 1023)
    
    I_final = exp_lut[exp_lut_idx] # F=6
    
    y_divisor_int = np.where(y_int == 0, 1, y_int)
    gain = (I_final // y_divisor_int) << 6 # F=12
    
    r_out_raw = r_val * gain # F=12
    g_out_raw = g_val * gain # F=12
    b_out_raw = b_val * gain # F=12
    
    # AP_RND, AP_SAT
    r_out = (r_out_raw + 2048) >> 12
    g_out = (g_out_raw + 2048) >> 12
    b_out = (b_out_raw + 2048) >> 12
    
    r_out = np.clip(r_out, 0, 4095).astype(np.uint16)
    g_out = np.clip(g_out, 0, 4095).astype(np.uint16)
    b_out = np.clip(b_out, 0, 4095).astype(np.uint16)
    
    out_img = np.stack([r_out, g_out, b_out], axis=-1)
    return out_img

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input_dir", type=str, default="../dataset/input_csim/Sequence_small/")
    parser.add_argument("--output", type=str, default="../dataset/output_csim_golden/LTM/Sequence_small/golden_ltm_output.ppm")
    args = parser.parse_args()
    
    bin_files = glob.glob(os.path.join(args.input_dir, "*.bin"))
    bin_files.sort(key=lambda x: (len(x), x))
    
    if not bin_files:
        print("No .bin files found.")
        exit(1)
        
    all_out_imgs = []
    
    for f in bin_files:
        print(f"Processing {f}...")
        with open(f, 'rb') as file:
            header = file.read(8)
            W, H = struct.unpack('ii', header)
            data = file.read()
            img = np.frombuffer(data, dtype=np.uint16).reshape((H, W, 3))
            
            out_img = process_frame(img)
            all_out_imgs.append(out_img)
            
    final_img = np.concatenate(all_out_imgs, axis=0)
    
    print(f"Writing to {args.output}...")
    with open(args.output, 'w') as f:
        f.write(f"P3\n{W} {final_img.shape[0]}\n4095\n")
        # Flatten and space separated
        # For performance, use np.savetxt or similar
        # But a simple loop or string join is fine
        flat = final_img.flatten()
        
        # Fast write
        # Chunking to avoid massive memory string
        chunk_size = W * 3
        for r in range(final_img.shape[0]):
            row_data = final_img[r].flatten()
            f.write(" ".join(map(str, row_data)) + "\n")
            
    print("Generating ltm_lut.h...")
    with open("ltm_lut.h", "w") as f:
        f.write("#ifndef LTM_LUT_H\n#define LTM_LUT_H\n\n")
        f.write("#include \"../ltm_gamma.hpp\"\n\n")
        
        # Generate LOG LUT
        f.write("const log_t LOG_LUT[4096] = {\n")
        log_lut = []
        for i in range(4096):
            val = np.log(i + 1.0)
            v = int(np.floor(val * 2048)) & 0xFFFF
            if v >= 32768: v -= 65536
            log_lut.append(str(v / 2048.0))
        f.write(",\n".join(log_lut))
        f.write("\n};\n\n")

        # Generate REINHARD LUT
        f.write("const log_t REINHARD_LUT[1024] = {\n")
        reinhard_lut = []
        exposure_gain = 3.0
        target_max = 4095.0
        log_target_max = np.log(target_max + 1.0)
        for i in range(1024):
            log_I_base = i / 123.0
            L = (log_I_base / log_target_max) * exposure_gain
            L_white = exposure_gain
            L_mapped = (L * (1.0 + (L / (L_white * L_white)))) / (1.0 + L)
            log_I_mapped_base = L_mapped * log_target_max
            v = int(np.floor(log_I_mapped_base * 2048)) & 0xFFFF
            if v >= 32768: v -= 65536
            reinhard_lut.append(str(v / 2048.0))
        f.write(",\n".join(reinhard_lut))
        f.write("\n};\n\n")

        # Generate EXP LUT
        f.write("const exp_out_t EXP_LUT[1024] = {\n")
        exp_lut = []
        for i in range(1024):
            log_I_final = i / 123.0
            I_final = np.exp(log_I_final) - 1.0
            if I_final > 4095.0: I_final = 4095.0
            if I_final < 0.0: I_final = 0.0
            v = int(np.floor(I_final * 64)) & 0x3FFFF
            exp_lut.append(str(v / 64.0))
        f.write(",\n".join(exp_lut))
        f.write("\n};\n\n#endif\n")
            
    print("Done!")
