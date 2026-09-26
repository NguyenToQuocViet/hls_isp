import argparse
import glob
import os

import cv2
import numpy as np


def apply_gamma_float(image_path, gamma=0.45, max_val=4095.0):
    """
    Applies floating point gamma correction.
    Input: 12-bit HDR image (max 4095)
    Output: 8-bit SDR image (max 255)
    """
    img_in = cv2.imread(image_path, cv2.IMREAD_UNCHANGED)
    if img_in is None:
        raise FileNotFoundError(f"Could not load image at {image_path}")

    img_rgb = cv2.cvtColor(img_in, cv2.COLOR_BGR2RGB).astype(np.float32)

    # Normalize to [0, 1]
    img_norm = img_rgb / max_val
    img_norm = np.clip(img_norm, 0.0, 1.0)

    # Apply standard Gamma curve
    img_gamma = np.power(img_norm, gamma)

    # Scale to 8-bit (0-255)
    img_out = np.round(img_gamma * 255.0).astype(np.uint8)

    return img_out


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Floating Point Gamma Correction Model"
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
        "--gamma",
        type=float,
        default=0.45,
        help="Gamma correction factor",
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

    out_dir_gamma = os.path.join(
        base_dir, "..", "dataset", "output", "gamma_float", dataset_name
    )
    os.makedirs(out_dir_gamma, exist_ok=True)

    # --- SINGLE IMAGE PROCESSING ---
    if args.input_image:
        filename = os.path.basename(input_path)
        print(f"Processing single image: {filename}...")

        try:
            tone_mapped_8bit = apply_gamma_float(
                input_path,
                gamma=args.gamma,
            )
        except Exception as e:
            print(f"Error processing {filename}: {e}")
            exit(1)

        mapped_bgr = cv2.cvtColor(tone_mapped_8bit, cv2.COLOR_RGB2BGR)

        out_path = os.path.join(out_dir_gamma, filename)
        cv2.imwrite(out_path, mapped_bgr)
        print(f"Saved gamma corrected image to {out_path}")

    # --- BATCH DIRECTORY PROCESSING ---
    else:
        input_dir = os.path.join(base_dir, args.input_dir)
        image_files = glob.glob(os.path.join(input_dir, "*.[pP][nN][gG]"))

        for input_file in image_files:
            filename = os.path.basename(input_file)
            print(f"Processing {filename}...")

            try:
                tone_mapped_8bit = apply_gamma_float(
                    input_file,
                    gamma=args.gamma,
                )
            except Exception as e:
                print(f"Skipping {filename} due to error: {e}")
                continue

            mapped_bgr = cv2.cvtColor(tone_mapped_8bit, cv2.COLOR_RGB2BGR)

            out_path = os.path.join(out_dir_gamma, filename)
            cv2.imwrite(out_path, mapped_bgr)

        print(f"Done. Processed {len(image_files)} images.")
