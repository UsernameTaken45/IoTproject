import yaml
import numpy as np

# Instellingen
GRID_W = 16
GRID_H = 16

def load_and_process_yaml(input_filename):
    try:
        with open(input_filename, 'r') as f:
            data = yaml.safe_load(f)
    except FileNotFoundError:
        print(f"Error: '{input_filename}' not found.")
        return None, None, None, None, None

    # LSC Block zoeken
    lsc_data = None
    for algo in data['algorithms']:
        if 'LensShadingCorrection' in algo:
            lsc_data = algo['LensShadingCorrection']
            break

    if not lsc_data:
        print("Error: LensShadingCorrection block not found.")
        return None, None, None, None, None

    # Set ophalen
    sets = lsc_data['sets']
    target_set = next((item for item in sets if item['ct'] == 6500), None)
    if not target_set:
        print("Error: Target CT set not found.")
        return None, None, None, None, None

    print("Found data. Calculating Min-Max normalization...")

    # Data laden
    r_raw = np.array(target_set['r'])
    b_raw = np.array(target_set['b'])
    gr_raw = np.array(target_set['gr'])
    gb_raw = np.array(target_set['gb'])
    g_raw = (gr_raw + gb_raw) / 2.0

    # 1. Bepaal het GLOBALE minimum en maximum over alle kanalen
    # Dit is cruciaal: als we per kanaal apart normaliseren, klopt de witbalans niet meer.
    all_values = np.concatenate([r_raw, g_raw, b_raw])

    global_min_val = np.min(all_values)
    global_max_val = np.max(all_values)
    value_range = global_max_val - global_min_val

    # Omrekenen naar gain factors voor in de header (handig voor shader wiskunde)
    min_gain_ref = global_min_val / 1024.0
    max_gain_ref = global_max_val / 1024.0

    print(f"Range found: Raw {global_min_val} to {global_max_val}")
    print(f"Gain Range: {min_gain_ref:.4f}x to {max_gain_ref:.4f}x")

    # 2. Normalisatie functie (Strekken van 0 tot 255)
    def normalize_stretch(data_array):
        # Formule: (Waarde - Min) / (Max - Min) * 255
        normalized = (data_array - global_min_val) / value_range * 255.0
        return np.clip(normalized, 0, 255).astype(int)

    # Converteren
    r_final = normalize_stretch(r_raw)
    g_final = normalize_stretch(g_raw)
    b_final = normalize_stretch(b_raw)

    return r_final, g_final, b_final, min_gain_ref, max_gain_ref

def save_custom_grid_yaml(output_filename, r, g, b, min_gain, max_gain):
    # We schrijven het bestand handmatig om exacte controle te hebben over de opmaak
    # YAML parsers lezen dit nog steeds correct als een lijst.

    def format_array_as_grid_string(arr):
        # Zet array om naar tekstblok van 16 breed
        lines = []
        for i in range(0, len(arr), GRID_W):
            # Pak 16 items
            row = arr[i:i+GRID_W]
            # Maak er strings van: "1, 2, 3..."
            row_str = ", ".join(map(str, row))
            # Voeg inspringing toe voor netheid
            lines.append(f"    {row_str}")

        # Voeg blokhaken toe en newlines
        return "[\n" + ",\n".join(lines) + "\n  ]"

    with open(output_filename, 'w') as f:
        f.write(f"description: 'LSC Stretched 0-255 (0=MinGain, 255=MaxGain)'\n")
        f.write(f"source_ct: 6500\n")
        f.write(f"grid_size: [{GRID_W}, {GRID_H}]\n")
        f.write(f"gain_range:\n")
        f.write(f"  min: {min_gain:.5f}\n")
        f.write(f"  max: {max_gain:.5f}\n")
        f.write(f"channels:\n")

        f.write("  red: " + format_array_as_grid_string(r) + "\n")
        f.write("  green: " + format_array_as_grid_string(g) + "\n")
        f.write("  blue: " + format_array_as_grid_string(b) + "\n")

    print(f"Success! Saved formatted grid to '{output_filename}'")

# --- Main Execution ---
if __name__ == "__main__":
    r, g, b, min_gain, max_gain = load_and_process_yaml('output_16x16_6500.yaml')

    if r is not None:
        save_custom_grid_yaml('lsc_shader_16x16_6500.yaml', r, g, b, min_gain, max_gain)
