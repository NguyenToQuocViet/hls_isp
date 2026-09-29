import argparse
import glob
import os

import cv2
import numpy as np


def proposed_ltm(image_path, kernel=7, exposure_gain=1.2, a_max=0.6, detail_boost=1.3):
    """
    Proposed Local Tone Mapping Operator.
    Input: 12-bit HDR image (max 4095)
    Output: 12-bit Tone Mapped image (max 4095)
    """
    img_in = cv2.imread(image_path, cv2.IMREAD_UNCHANGED)
    if img_in is None:
        raise FileNotFoundError(f"Could not load image at {image_path}")

    img_rgb = cv2.cvtColor(img_in, cv2.COLOR_BGR2RGB).astype(np.float32)

    # Extract Luminance
    I_original = (
        0.299 * img_rgb[:, :, 0] + 0.587 * img_rgb[:, :, 1] + 0.114 * img_rgb[:, :, 2]
    )

    # Phase 0: Log-Domain (Convert luminance to log domain immediately)
    # HW TIP: Adding 1.0 ensures the log domain is strictly positive (log(1) = 0).
    # This completely eliminates the need for signed arithmetic in your log representations!
    log_I = np.log(I_original + 1.0)

    # Phase 1: Shift-Optimized Adaptive Guided Filter
    mean_I = cv2.boxFilter(log_I, -1, (kernel, kernel))
    mean_II = cv2.boxFilter(log_I * log_I, -1, (kernel, kernel))
    var_I = mean_II - mean_I * mean_I
    var_I = np.maximum(var_I, 0.0)

    # Adaptive epsilon: legacy epsilon base
    eps_base = 0.5
    eps_map = eps_base / (var_I + 1e-5)

    a = var_I / (var_I + eps_map)
    a = np.clip(a, 0.0, a_max)
    b = mean_I - a * mean_I

    mean_a = cv2.boxFilter(a, -1, (kernel, kernel))
    mean_b = cv2.boxFilter(b, -1, (kernel, kernel))

    log_I_base = mean_a * log_I + mean_b

    # Phase 2: Log-Domain Subtraction (Extract detail)
    log_I_detail = log_I - log_I_base

    # Phase 3: The Global Curve (Applied directly in Log Domain)
    # Applying the Extended Reinhard curve directly to the log-domain values
    # avoids expensive exp/log operations in hardware.

    target_max = 4095.0
    log_target_max = np.log(target_max + 1.0)
    
    # 1. Normalize log base to [0, 1] and apply exposure gain
    # Multiplying in the log domain increases contrast/brightness smoothly from black (0)
    L = (log_I_base / log_target_max) * exposure_gain
    
    # 2. Extended Reinhard Compression (anchors the max white point)
    L_white = exposure_gain  # Max possible value of L
    L_mapped = (L * (1.0 + (L / (L_white ** 2)))) / (1.0 + L)
    
    # 3. Scale back to log scale for Phase 4 Recombination
    log_I_mapped_base = L_mapped * log_target_max

    # Phase 4: Dynamic Detail Recombination
    # Derive w from the base layer itself: boost shadows, preserve highlights
    w_min = 1.0
    w_max = 1.2
    w = w_max - (log_I_mapped_base / log_target_max) * (w_max - w_min)

    log_I_final = log_I_mapped_base + w * detail_boost * log_I_detail

    # Exponentiation to pull I_final back to linear space
    I_final = np.exp(log_I_final) - 1.0
    I_final = np.clip(I_final, 0, target_max)

    # RGB Restoration using Gain
    # Adding a small epsilon here to prevent division by zero in pure black pixels
    gain = I_final / (I_original + 1e-6)
    RGB_mapped = img_rgb * gain[:, :, np.newaxis]
    RGB_mapped = np.clip(RGB_mapped, 0, target_max)

    return np.round(RGB_mapped).astype(np.uint16)


def to_8bit_srgb(img_data, max_val=4095.0, apply_gamma=True):
    """Converts linear data to viewable 8-bit sRGB."""
    # Normalize to 0.0 - 1.0
    img_norm = img_data / max_val
    img_norm = np.clip(img_norm, 0, 1)
    if apply_gamma:
        # Apply standard 2.2 Gamma curve
        img_gamma = np.power(img_norm, 1.0 / 2.2)
    else:
        img_gamma = img_norm
    # Scale to 8-bit (0-255)
    return np.uint8(img_gamma * 255.0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Improved Guided Filter Hybrid Local-Global Tone Mapping (Proposed Method)"
    )
    # Argument for single image processing
    parser.add_argument(
        "--input_image",
        type=str,
        default=None,
        help="Path to a single input HDR image (overrides input_dir)",
    )
    parser.add_argument(
        "--input_dir",
        type=str,
        default="dataset/TMIQD/Content/Natural/HDR_Images",
        help="Directory of input HDR images (used if input_image is not provided)",
    )
    parser.add_argument(
        "--kernel",
        type=int,
        default=7,
        help="Kernel size for guided filter",
    )
    parser.add_argument(
        "--exposure_gain",
        type=float,
        default=1.2,
        help="Linear digital gain to boost overall brightness",
    )
    parser.add_argument(
        "--a_max",
        type=float,
        default=0.6,
        help="Maximum value for guided filter 'a' coefficient (detail preservation limit)",
    )
    parser.add_argument(
        "--detail_boost",
        type=float,
        default=1.3,
        help="Multiplier for the detail layer in dynamic recombination",
    )
    args = parser.parse_args()

    base_dir = os.path.dirname(os.path.abspath(__file__))

    # Determine dataset name to avoid image name collision across datasets
    if args.input_image:
        input_path = args.input_image
        if not os.path.isabs(input_path):
            input_path = os.path.join(base_dir, args.input_image)
        dataset_name = os.path.basename(os.path.dirname(os.path.abspath(input_path)))
    else:
        input_dir = os.path.join(base_dir, args.input_dir)
        dataset_name = os.path.basename(os.path.normpath(input_dir))

    # Define standard output directories for both single and batch processes
    out_dir_proposed = os.path.join(
        base_dir, "..", "dataset", "output", "proposed", dataset_name
    )
    out_dir_proposed_12bit = os.path.join(
        base_dir, "..", "dataset", "output", "proposed_12bit", dataset_name
    )
    out_dir_comparison = os.path.join(
        base_dir, "..", "dataset", "output", "comparison_proposed", dataset_name
    )

    os.makedirs(out_dir_proposed, exist_ok=True)
    os.makedirs(out_dir_proposed_12bit, exist_ok=True)
    os.makedirs(out_dir_comparison, exist_ok=True)

    # --- SINGLE IMAGE PROCESSING ---
    if args.input_image:
        filename = os.path.basename(input_path)
        print(f"Processing single image: {filename}...")

        try:
            tone_mapped_12bit = proposed_ltm(
                input_path,
                kernel=args.kernel,
                exposure_gain=args.exposure_gain,
                a_max=args.a_max,
                detail_boost=args.detail_boost,
            )
        except Exception as e:
            print(f"Error processing {filename}: {e}")
            exit(1)

        # Load Original for comparison
        img_in = cv2.imread(input_path, cv2.IMREAD_UNCHANGED)
        img_rgb_original = cv2.cvtColor(img_in, cv2.COLOR_BGR2RGB).astype(np.float32)

        # Convert to viewable 8-bit
        original_8bit = to_8bit_srgb(img_rgb_original, max_val=4095.0, apply_gamma=True)
        mapped_8bit = to_8bit_srgb(tone_mapped_12bit, max_val=4095.0, apply_gamma=True)

        # Save to the correct "proposed" output directories
        original_bgr = cv2.cvtColor(original_8bit, cv2.COLOR_RGB2BGR)
        mapped_bgr = cv2.cvtColor(mapped_8bit, cv2.COLOR_RGB2BGR)

        # Save independent tone-mapped image (8-bit)
        out_path = os.path.join(out_dir_proposed, filename)
        cv2.imwrite(out_path, mapped_bgr)
        print(f"Saved independent 8-bit tone-mapped image to {out_path}")

        # Save 12-bit tone-mapped image
        out_path_12bit = os.path.join(out_dir_proposed_12bit, filename)
        mapped_12bit_bgr = cv2.cvtColor(tone_mapped_12bit, cv2.COLOR_RGB2BGR)
        cv2.imwrite(out_path_12bit, mapped_12bit_bgr)
        print(f"Saved 12-bit tone-mapped image to {out_path_12bit}")

        # Save side-by-side comparison
        comparison_img = cv2.hconcat([original_bgr, mapped_bgr])
        comp_path = os.path.join(out_dir_comparison, filename)
        cv2.imwrite(comp_path, comparison_img)
        print(f"Saved side-by-side comparison to {comp_path}")

    # --- BATCH DIRECTORY PROCESSING ---
    else:
        input_dir = os.path.join(base_dir, args.input_dir)
        image_files = glob.glob(os.path.join(input_dir, "*.[pP][nN][gG]"))

        for input_file in image_files:
            filename = os.path.basename(input_file)
            print(f"Processing {filename}...")

            try:
                tone_mapped_12bit = proposed_ltm(
                    input_file,
                    kernel=args.kernel,
                    exposure_gain=args.exposure_gain,
                    a_max=args.a_max,
                    detail_boost=args.detail_boost,
                )
            except Exception as e:
                print(f"Skipping {filename} due to error: {e}")
                continue

            # Load Original for comparison
            img_in = cv2.imread(input_file, cv2.IMREAD_UNCHANGED)
            img_rgb_original = cv2.cvtColor(img_in, cv2.COLOR_BGR2RGB).astype(
                np.float32
            )

            original_8bit = to_8bit_srgb(
                img_rgb_original, max_val=4095.0, apply_gamma=True
            )
            mapped_8bit = to_8bit_srgb(
                tone_mapped_12bit, max_val=4095.0, apply_gamma=True
            )

            original_bgr = cv2.cvtColor(original_8bit, cv2.COLOR_RGB2BGR)
            mapped_bgr = cv2.cvtColor(mapped_8bit, cv2.COLOR_RGB2BGR)

            # Save independent tone-mapped image (8-bit)
            out_path = os.path.join(out_dir_proposed, filename)
            cv2.imwrite(out_path, mapped_bgr)

            # Save 12-bit tone-mapped image
            out_path_12bit = os.path.join(out_dir_proposed_12bit, filename)
            mapped_12bit_bgr = cv2.cvtColor(tone_mapped_12bit, cv2.COLOR_RGB2BGR)
            cv2.imwrite(out_path_12bit, mapped_12bit_bgr)

            # Save side-by-side comparison
            comparison_img = cv2.hconcat([original_bgr, mapped_bgr])
            comp_path = os.path.join(out_dir_comparison, filename)
            cv2.imwrite(comp_path, comparison_img)

        print(f"Done. Processed {len(image_files)} images.")
