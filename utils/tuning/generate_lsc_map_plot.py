import yaml
import numpy as np
import matplotlib.pyplot as plt

# 1. Load the data
try:
    with open('tuning2700.yaml', 'r') as f:
        data = yaml.safe_load(f)
except FileNotFoundError:
    print("Error: 'tuning.yaml' not found. Please ensure the file exists.")
    exit()

# 2. Find LensShadingCorrection safely
lsc_data = None
for algo in data['algorithms']:
    if 'LensShadingCorrection' in algo:
        lsc_data = algo['LensShadingCorrection']
        break

if not lsc_data:
    print("Error: LensShadingCorrection block not found in YAML.")
    exit()

# 3. Extract the set for disirable Kelvin
kelvin = 2700
sets = lsc_data['sets']
target_set = next((item for item in sets if item['ct'] == kelvin), None)

if not target_set:
    print("Error: CT 6500 not found in sets.")
    exit()

# 4. Get lists and normalize (1024 = 1.0 gain)
r_list = np.array(target_set['r'])
gr_list = np.array(target_set['gr'])
gb_list = np.array(target_set['gb'])
b_list = np.array(target_set['b'])

r_norm = r_list / 1024.0
b_norm = b_list / 1024.0
# Average the two greens for the shader
g_norm = (gr_list + gb_list) / 2.0 / 1024.0

# 5. Reshape into 17x17 Grids
grid_size = (17, 17)
r_grid = r_norm.reshape(grid_size)
g_grid = g_norm.reshape(grid_size)
b_grid = b_norm.reshape(grid_size)

# 6. Visualization
# We create 3 separate plots to see the data distribution correctly
fig, axs = plt.subplots(1, 3, figsize=(15, 5))

# Plot Red
im1 = axs[0].imshow(r_grid, cmap='viridis')
axs[0].set_title('Red Gain Map')
fig.colorbar(im1, ax=axs[0])

# Plot Green
im2 = axs[1].imshow(g_grid, cmap='viridis')
axs[1].set_title('Green Gain Map')
fig.colorbar(im2, ax=axs[1])

# Plot Blue
im3 = axs[2].imshow(b_grid, cmap='viridis')
axs[2].set_title('Blue Gain Map')
fig.colorbar(im3, ax=axs[2])

plt.suptitle(f"LSC Gain Maps (Center ~1.0, Corners > 1.0) for collor temprature {kelvin}")
plt.show()

# 7. Prepare Texture for Export (Optional)
# Stack them for your shader: (17, 17, 3)
lsc_texture = np.dstack((r_grid, g_grid, b_grid))
print(f"Final Texture Shape: {lsc_texture.shape}")
print(f"Sample Blue Value at Corner: {b_grid[0,0]}")
