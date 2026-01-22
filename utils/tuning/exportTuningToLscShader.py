import yaml
import numpy as np
import argparse
import sys

# --- Configuration ---
GRID_W = 16
GRID_H = 16

# Formula constants
BLACK_LEVEL = 1024.0
SCALE_FACTOR = 3071  # Divisor to map the range to 0-255

def load_and_process_yaml(input_filename, target_ct):
    # 1. Load the YAML file
    try:
        with open(input_filename, 'r') as f:
            data = yaml.safe_load(f)
    except FileNotFoundError:
        print(f"Error: Input file '{input_filename}' not found.")
        return None, None, None

    # 2. Find the LensShadingCorrection block
    lsc_data = None
    for algo in data['algorithms']:
        if 'LensShadingCorrection' in algo:
            lsc_data = algo['LensShadingCorrection']
            break

    if not lsc_data:
        print("Error: LensShadingCorrection block not found.")
        return None, None, None

    # 3. Extract the set for the specific Color Temperature (CT) provided in arguments
    sets = lsc_data['sets']
    target_set = next((item for item in sets if item['ct'] == target_ct), None)

    if not target_set:
        print(f"Error: Set for Color Temperature {target_ct} not found in '{input_filename}'.")
        return None, None, None

    print(f"Found data for CT {target_ct}. Applying formula: (x - {int(BLACK_LEVEL)}) / {SCALE_FACTOR} ...")

    # 4. Get Raw Data
    r_raw = np.array(target_set['r'])
    b_raw = np.array(target_set['b'])
    gr_raw = np.array(target_set['gr'])
    gb_raw = np.array(target_set['gb'])

    # Calculate Green Channel (Average of GR and GB)
    g_raw = (gr_raw + gb_raw) / 2.0

    # 5. Define the calculation logic
    def apply_formula(data_array):
        """
        Applies the specific user formula:
        1. Subtract Black Level (1024)
        2. Divide by the Scale Factor (3071)
        3. Multiply by 255 and convert to integer
        """
        result = ((data_array - BLACK_LEVEL) / SCALE_FACTOR) * 255
        return result.astype(int)

    # 6. Apply calculation to all channels
    r_final = apply_formula(r_raw)
    g_final = apply_formula(g_raw)
    b_final = apply_formula(b_raw)

    return r_final, g_final, b_final

def save_custom_grid_yaml(output_filename, r, g, b, target_ct):

    # Helper function to format the array as a visual grid string
    def format_array_as_grid_string(arr):
        lines = []
        # Loop through the array in chunks of 16 (GRID_W)
        for i in range(0, len(arr), GRID_W):
            row = arr[i:i+GRID_W]
            # Join numbers with commas
            row_str = ", ".join(map(str, row))
            lines.append(f"    {row_str}")
        # Wrap in brackets to form a valid YAML list, but visually formatted
        return "[\n" + ",\n".join(lines) + "\n  ]"

    # Write the file manually to ensure specific formatting
    with open(output_filename, 'w') as f:
        f.write(f"description: 'LSC Fixed Formula ((x-{int(BLACK_LEVEL)})/{SCALE_FACTOR})'\n")
        f.write(f"source_ct: {target_ct}\n")
        f.write(f"grid_size: [{GRID_W}, {GRID_H}]\n")
        f.write(f"formula_used: '(RawValue - {int(BLACK_LEVEL)}) / {SCALE_FACTOR} -> [0..255]'\n")
        f.write(f"channels:\n")

        f.write("  red: " + format_array_as_grid_string(r) + "\n")
        f.write("  green: " + format_array_as_grid_string(g) + "\n")
        f.write("  blue: " + format_array_as_grid_string(b) + "\n")

    print(f"Success! Saved formatted grid to '{output_filename}'")

# --- Main Execution ---
if __name__ == "__main__":
    # 1. Setup Argument Parser
    parser = argparse.ArgumentParser(description="Convert LSC YAML data to shader grid format.")
    parser.add_argument("ct", type=int, help="The Color Temperature to process (e.g. 2700, 5000, 6500)")

    # 2. Parse arguments
    args = parser.parse_args()
    ct_val = args.ct

    # 3. Construct filenames based on the CT value
    # Assumes input file is named 'tuning_XXXX.yaml'
    input_file = f'tuning{ct_val}.yaml'
    output_file = f'lsc_shader_16x16_{ct_val}_fixed.yaml'

    print(f"--- Processing for Color Temp: {ct_val} ---")

    # 4. Run Process
    r, g, b = load_and_process_yaml(input_file, ct_val)

    if r is not None:
        save_custom_grid_yaml(output_file, r, g, b, ct_val)
