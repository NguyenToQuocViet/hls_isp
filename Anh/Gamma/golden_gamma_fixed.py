import argparse
import glob
import os
import struct
import numpy as np

def generate_gamma_lut(gamma=0.45):
    """
    Generate Gamma LUT matching C++ implementation.
    """
    lut = np.zeros(4096, dtype=np.uint8)
    for i in range(4096):
        # Normalize 12-bit index to [0.0, 1.0]
        normalized = i / 4095.0
        # Apply power function and scale to 8-bit [0.0, 255.0]
        val = np.power(normalized, gamma) * 255.0
        # Round to nearest integer and cast to uint8
        lut[i] = int(np.round(val))
    return lut

def process_frame(img_in, lut):
    """
    img_in is (H, W, 3) uint16
    """
    # Clip input to valid 12-bit range
    img_in = np.clip(img_in, 0, 4095).astype(np.int64)
    
    r_val = img_in[:, :, 0]
    g_val = img_in[:, :, 1]
    b_val = img_in[:, :, 2]
    
    # Apply Gamma LUT
    r_out = lut[r_val]
    g_out = lut[g_val]
    b_out = lut[b_val]
    
    out_img = np.stack([r_out, g_out, b_out], axis=-1)
    return out_img

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input_dir", type=str, default="../dataset/input_csim/Sequence_small/")
    parser.add_argument("--output", type=str, default="../dataset/output_csim_golden/Gamma/Sequence_small/golden_gamma_output.ppm")
    parser.add_argument("--gamma", type=float, default=0.45)
    args = parser.parse_args()
    
    bin_files = glob.glob(os.path.join(args.input_dir, "*.bin"))
    bin_files.sort(key=lambda x: (len(x), x))
    
    if not bin_files:
        print(f"No .bin files found in {args.input_dir}.")
        exit(1)
        
    all_out_imgs = []
    
    # 1. Calculate Gamma LUT
    lut = generate_gamma_lut(args.gamma)
    
    # 2. Process frames
    for f in bin_files:
        print(f"Processing {f}...")
        with open(f, 'rb') as file:
            header = file.read(8)
            W, H = struct.unpack('ii', header)
            data = file.read()
            img = np.frombuffer(data, dtype=np.uint16).reshape((H, W, 3))
            
            out_img = process_frame(img, lut)
            all_out_imgs.append(out_img)
            
    final_img = np.concatenate(all_out_imgs, axis=0)
    
    print(f"Writing to {args.output}...")
    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    with open(args.output, 'w') as f:
        f.write(f"P3\n{W} {final_img.shape[0]}\n255\n")
        # Flatten and space separated write
        for r in range(final_img.shape[0]):
            row_data = final_img[r].flatten()
            f.write(" ".join(map(str, row_data)) + "\n")
            
    print("Done!")
