/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

// Headers
#include <cmath>
#include "tilemap_layer.h"
#include "graphics.h"
#include "output.h"
#include "player.h"
#include "map_data.h"
#include "bitmap.h"
#include "rect.h"

// Blocks subtiles IDs
// Mess with this code and you will die in 3 days...
// [tile-id][idx]
static const uint16_t BlockA_Subtiles_IDS[47] = {
#define pack_to_uint16(a, b, c, d) ((a << (4 * 0)) | (b << (4 * 1)) | (c << (4 * 2)) | (d << (4 * 3)))
#define N 0xf
	pack_to_uint16(N, N, N, N),
	pack_to_uint16(3, N, N, N),
	pack_to_uint16(N, 3, N, N),
	pack_to_uint16(3, 3, N, N),
	pack_to_uint16(N, N, N, 3),
	pack_to_uint16(3, N, N, 3),
	pack_to_uint16(N, 3, N, 3),
	pack_to_uint16(3, 3, N, 3),
	pack_to_uint16(N, N, 3, N),
	pack_to_uint16(3, N, 3, N),
	pack_to_uint16(N, 3, 3, N),
	pack_to_uint16(3, 3, 3, N),
	pack_to_uint16(N, N, 3, 3),
	pack_to_uint16(3, N, 3, 3),
	pack_to_uint16(N, 3, 3, 3),
	pack_to_uint16(3, 3, 3, 3),
	pack_to_uint16(1, N, 1, N),
	pack_to_uint16(1, 3, 1, N),
	pack_to_uint16(1, N, 1, 3),
	pack_to_uint16(1, 3, 1, 3),
	pack_to_uint16(2, 2, N, N),
	pack_to_uint16(2, 2, N, 3),
	pack_to_uint16(2, 2, 3, N),
	pack_to_uint16(2, 2, 3, 3),
	pack_to_uint16(N, 1, N, 1),
	pack_to_uint16(N, 1, 3, 1),
	pack_to_uint16(3, 1, N, 1),
	pack_to_uint16(3, 1, 3, 1),
	pack_to_uint16(N, N, 2, 2),
	pack_to_uint16(3, N, 2, 2),
	pack_to_uint16(N, 3, 2, 2),
	pack_to_uint16(3, 3, 2, 2),
	pack_to_uint16(1, 1, 1, 1),
	pack_to_uint16(2, 2, 2, 2),
	pack_to_uint16(0, 2, 1, N),
	pack_to_uint16(0, 2, 1, 3),
	pack_to_uint16(2, 0, N, 1),
	pack_to_uint16(2, 0, 3, 1),
	pack_to_uint16(N, 1, 2, 0),
	pack_to_uint16(3, 1, 2, 0),
	pack_to_uint16(1, N, 0, 2),
	pack_to_uint16(1, 3, 0, 2),
	pack_to_uint16(0, 0, 1, 1),
	pack_to_uint16(0, 2, 0, 2),
	pack_to_uint16(1, 1, 0, 0),
	pack_to_uint16(2, 0, 2, 0),
	pack_to_uint16(0, 0, 0, 0),
#undef N
#undef pack_to_uint16
};


uint8_t const TilemapLayer::subtile_base[4][2] = {
#define H (TILE_SIZE / 2)
	{0, 0}, {H, 0},
	{0, H}, {H, H},
#undef H
};

void  TilemapLayer::sea_pattern(unsigned id, unsigned anime) {
	unsigned const x = id / 1000, y = (id % 1000) / 50, z = (id % 1000) % 50;

	unsigned const sea_on_y = x == 2? 2 : 1, sea_off_y = x == 2? 3 : 0;
	// from block B
	for (int i = 0; i < 4; ++i) {
		sea_subtiles[i][0] = subtile_base[i][0] + TILE_SIZE * anime;
		sea_subtiles[i][1] = subtile_base[i][1] + TILE_SIZE * (4 + ((y & (0x1 << i))? sea_on_y : sea_off_y));
	}

	// from block A1, A2
	unsigned const seaside_x = x == 1? 3 : 0;
	for (int i = 0; i < 4; ++i) {
		unsigned const seaside_y = (BlockA_Subtiles_IDS[z] >> (4 * i)) & 0xf;
		if (seaside_y == 0xf) {
			seaside_subtiles[i][0] = SKIP_SUBTILE;
			continue;
		}
		seaside_subtiles[i][0] = subtile_base[i][0] + TILE_SIZE * (seaside_x + anime);
		seaside_subtiles[i][1] = subtile_base[i][1] + TILE_SIZE * (seaside_y);
	}
}

// [tile-id][idx][x/y]
static const uint16_t BlockD_Subtiles_IDS[50] = {
#define pack_to_uint16(a, b, c, d, e, f, g, h) \
	((a << (2 * 0)) | (b << (2 * 1)) | (c << (2 * 2)) | (d << (2 * 3)) | \
	 (e << (2 * 4)) | (f << (2 * 5)) | (g << (2 * 6)) | (h << (2 * 7)))
//                 T-L   T-R   B-L   B-R
    pack_to_uint16(1, 2, 1, 2, 1, 2, 1, 2),
    pack_to_uint16(2, 0, 1, 2, 1, 2, 1, 2),
    pack_to_uint16(1, 2, 2, 0, 1, 2, 1, 2),
    pack_to_uint16(2, 0, 2, 0, 1, 2, 1, 2),
    pack_to_uint16(1, 2, 1, 2, 1, 2, 2, 0),
    pack_to_uint16(2, 0, 1, 2, 1, 2, 2, 0),
    pack_to_uint16(1, 2, 2, 0, 1, 2, 2, 0),
    pack_to_uint16(2, 0, 2, 0, 1, 2, 2, 0),
    pack_to_uint16(1, 2, 1, 2, 2, 0, 1, 2),
    pack_to_uint16(2, 0, 1, 2, 2, 0, 1, 2),
    pack_to_uint16(1, 2, 2, 0, 2, 0, 1, 2),
    pack_to_uint16(2, 0, 2, 0, 2, 0, 1, 2),
    pack_to_uint16(1, 2, 1, 2, 2, 0, 2, 0),
    pack_to_uint16(2, 0, 1, 2, 2, 0, 2, 0),
    pack_to_uint16(1, 2, 2, 0, 2, 0, 2, 0),
    pack_to_uint16(2, 0, 2, 0, 2, 0, 2, 0),
    pack_to_uint16(0, 2, 0, 2, 0, 2, 0, 2),
    pack_to_uint16(0, 2, 2, 0, 0, 2, 0, 2),
    pack_to_uint16(0, 2, 0, 2, 0, 2, 2, 0),
    pack_to_uint16(0, 2, 2, 0, 0, 2, 2, 0),
    pack_to_uint16(1, 1, 1, 1, 1, 1, 1, 1),
    pack_to_uint16(1, 1, 1, 1, 1, 1, 2, 0),
    pack_to_uint16(1, 1, 1, 1, 2, 0, 1, 1),
    pack_to_uint16(1, 1, 1, 1, 2, 0, 2, 0),
    pack_to_uint16(2, 2, 2, 2, 2, 2, 2, 2),
    pack_to_uint16(2, 2, 2, 2, 2, 0, 2, 2),
    pack_to_uint16(2, 0, 2, 2, 2, 2, 2, 2),
    pack_to_uint16(2, 0, 2, 2, 2, 0, 2, 2),
    pack_to_uint16(1, 3, 1, 3, 1, 3, 1, 3),
    pack_to_uint16(2, 0, 1, 3, 1, 3, 1, 3),
    pack_to_uint16(1, 3, 2, 0, 1, 3, 1, 3),
    pack_to_uint16(2, 0, 2, 0, 1, 3, 1, 3),
    pack_to_uint16(0, 2, 2, 2, 0, 2, 2, 2),
    pack_to_uint16(1, 1, 1, 1, 1, 3, 1, 3),
    pack_to_uint16(0, 1, 0, 1, 0, 1, 0, 1),
    pack_to_uint16(0, 1, 0, 1, 0, 1, 2, 0),
    pack_to_uint16(2, 1, 2, 1, 2, 1, 2, 1),
    pack_to_uint16(2, 1, 2, 1, 2, 0, 2, 1),
    pack_to_uint16(2, 3, 2, 3, 2, 3, 2, 3),
    pack_to_uint16(2, 0, 2, 3, 2, 3, 2, 3),
    pack_to_uint16(0, 3, 0, 3, 0, 3, 0, 3),
    pack_to_uint16(0, 3, 2, 0, 0, 3, 0, 3),
    pack_to_uint16(0, 1, 2, 1, 0, 1, 2, 1),
    pack_to_uint16(0, 1, 0, 1, 0, 3, 0, 3),
    pack_to_uint16(0, 3, 2, 3, 0, 3, 2, 3),
    pack_to_uint16(2, 1, 2, 1, 2, 3, 2, 3),
    pack_to_uint16(0, 1, 2, 1, 0, 3, 2, 3),
    pack_to_uint16(1, 2, 1, 2, 1, 2, 1, 2),
    pack_to_uint16(1, 2, 1, 2, 1, 2, 1, 2),
    pack_to_uint16(0, 0, 0, 0, 0, 0, 0, 0),
};

void TilemapLayer::terrain_pattern(unsigned id) {
	unsigned const x = (id - 4000) / 50, y = (id - 4000) % 50;
	unsigned const block_x = (x % 2 + (x < 4? 0 : 2)) * 3, block_y = ((x / 2 + 2) % 4) * 4;

	for (int i = 0; i < 4; ++i) {
		unsigned const coord = (BlockD_Subtiles_IDS[y] >> (4 * i)) & 0xf;
		terrain_subtiles[i][0] = subtile_base[i][0] + TILE_SIZE * (block_x + (coord & 0x3));
		terrain_subtiles[i][1] = subtile_base[i][1] + TILE_SIZE * (block_y + ((coord >> 2) & 0x3));
	}
}

TilemapLayer::TilemapLayer(int ilayer) :
	visible(true),
	ox(0),
	oy(0),
	width(0),
	height(0),
	animation_frame(0),
	animation_step_ab(0),
	animation_step_c(0),
	animation_speed(24),
	animation_type(0),
	layer(ilayer) {
	int tiles_y = ceil(SCREEN_TARGET_HEIGHT / (float)TILE_SIZE) + 1;
	for (int i = 0; i < tiles_y + 2; i++) {
		tilemap_tiles.push_back(EASYRPG_MAKE_SHARED<TilemapTile>(this, TILE_SIZE * i));
	}
}

void TilemapLayer::Draw(int z_order) {
	if (!visible) return;

	prepare_draw();

	// Get the number of tiles that can be displayed on window
	int tiles_x = ceil(SCREEN_TARGET_WIDTH / (float)TILE_SIZE);
	int tiles_y = ceil(SCREEN_TARGET_HEIGHT / (float)TILE_SIZE);

	// If ox or oy are not equal to the tile size draw the next tile too
	// to prevent black (empty) tiles at the borders
	if (ox % TILE_SIZE != 0) { ++tiles_x; }
	if (oy % TILE_SIZE != 0) { ++tiles_y; }

	for (int x = 0; x < tiles_x; x++) {
		for (int y = 0; y < tiles_y; y++) {

			// Get the real maps tile coordinates
			int map_x = (ox / TILE_SIZE + x + width) % width;
			int map_y = (oy / TILE_SIZE + y + height) % height;

			if (width <= map_x || height <= map_y) continue;

			int map_draw_x = x * TILE_SIZE - ox % TILE_SIZE;
			int map_draw_y = y * TILE_SIZE - oy % TILE_SIZE;

			// Get the tile data
			TileData const& tile = data_cache[map_x][map_y];

			int map_draw_z = tile.z;
			if (map_draw_z > 0 and map_draw_z < 9999) {
				map_draw_z += y * TILE_SIZE;
				if (y == 0) map_draw_z += TILE_SIZE;
			}

			// Draw the tile if its z is being draw now
			if (z_order != map_draw_z) { continue; }

			if (layer == 0) {
				// If lower layer

				if (tile.ID >= BLOCK_E && tile.ID < BLOCK_E + BLOCK_E_TILES) {
					int id = substitutions[tile.ID - BLOCK_E];
					// If Block E

					int row, col;

					// Get the tile coordinates from chipset
					if (id < 96) {
						// If from first column of the block
						col = 12 + id % 6;
						row = id / 6;
					} else {
						// If from second column of the block
						col = 18 + (id - 96) % 6;
						row = (id - 96) / 6;
					}

					DrawTile(map_draw_x, map_draw_y, row, col);
				} else if (tile.ID >= BLOCK_C && tile.ID < BLOCK_D) {
					// If Block C

					// Get the tile coordinates from chipset
					int col = 3 + (tile.ID - BLOCK_C) / 50;
					int row = 4 + animation_step_c;

					// Draw the tile
					DrawTile(map_draw_x, map_draw_y, row, col);
				} else if (tile.ID < BLOCK_C) {
					// If Blocks A1, A2, B
					sea_pattern(tile.ID, animation_step_ab);
					DrawTile(map_draw_x, map_draw_y, sea_subtiles);
					DrawTile(map_draw_x, map_draw_y, seaside_subtiles);
				} else {
					// If blocks D1-D12
					terrain_pattern(tile.ID);
					DrawTile(map_draw_x, map_draw_y, terrain_subtiles);
				}
			} else {
				// If upper layer

				// Check that block F is being drawn
				if (tile.ID >= BLOCK_F && tile.ID < BLOCK_F + BLOCK_F_TILES) {
					int id = substitutions[tile.ID - BLOCK_F];
					if (id == 0) { continue; } // skip transparent 10000 tile
					int row, col;

					// Get the tile coordinates from chipset
					if (id < 48) {
						// If from first column of the block
						col = 18 + id % 6;
						row = 8 + id / 6;
					} else {
						// If from second column of the block
						col = 24 + (id - 48) % 6;
						row = (id - 48) / 6;
					}

					// Draw the tile
					DrawTile(map_draw_x, map_draw_y, row, col);
				}
			}
		}
	}
}

void TilemapLayer::CreateTileCache(const std::vector<int16_t>& nmap_data) {
	data_cache.resize(width);
	for (int x = 0; x < width; x++) {
		data_cache[x].resize(height);
		for (int y = 0; y < height; y++) {
			TileData tile;

			// Get the tile ID
			tile.ID = nmap_data[x + y * width];

			tile.z = 0;

			// Calculate the tile Z
			if (!passable.empty()) {
				if (tile.ID >= BLOCK_F) {
					if ((passable[substitutions[tile.ID - BLOCK_F]] & Passable::Above) != 0)
						tile.z = 32;

				}
				else if (tile.ID >= BLOCK_E) {
					if ((passable[substitutions[tile.ID - BLOCK_E + 18]] & Passable::Above) != 0)
						tile.z = 32;

				}
				else if (tile.ID >= BLOCK_D) {
					if ((passable[(tile.ID - BLOCK_D) / 50 + 6] & (Passable::Wall | Passable::Above)) != 0)
						tile.z = 32;

				}
				else if (tile.ID >= BLOCK_C) {
					if ((passable[(tile.ID - BLOCK_C) / 50 + 3] & Passable::Above) != 0)
						tile.z = 32;

				}
				else {
					if ((passable[tile.ID / 1000] & Passable::Above) != 0)
						tile.z = 32;
				}
			}
			data_cache[x][y] = tile;
		}
	}
}

void TilemapLayer::Update() {
	animation_frame += 1;

	// Step to the next animation frame
	if (animation_frame % 6 == 0) {
		animation_step_c = (animation_step_c + 1) % 4;
	}
	if (animation_frame == animation_speed) {
		animation_step_ab = 1;
	} else if (animation_frame == animation_speed * 2) {
		animation_step_ab = 2;
	} else if (animation_frame == animation_speed * 3) {
		if (animation_type == 0) {
			// If animation type is 1-2-3-2
			animation_step_ab = 1;
		} else {
			// If animation type is 1-2-3
			animation_step_ab = 0;
			animation_frame = 0;
		}
	} else if (animation_frame >= animation_speed * 4) {
		animation_step_ab = 0;
		animation_frame = 0;
	}
}

BitmapRef const& TilemapLayer::GetChipset() const {
	return chipset;
}

void TilemapLayer::SetChipset(BitmapRef const& nchipset) {
	chipset = nchipset;
}

std::vector<int16_t> const& TilemapLayer::GetMapData() const {
	return map_data;
}

void TilemapLayer::SetMapData(const std::vector<int16_t>& nmap_data) {
	// Create the tiles data cache
	CreateTileCache(nmap_data);
	map_data = nmap_data;
}

std::vector<uint8_t> const& TilemapLayer::GetPassable() const {
	return passable;
}

void TilemapLayer::SetPassable(const std::vector<uint8_t>& npassable) {
	passable = npassable;

	if (substitutions.size() < passable.size())
	{
		substitutions.resize(passable.size());
		for (uint8_t i = 0; i < substitutions.size(); i++)
			substitutions[i] = i;
	}

	// Recalculate z values of all tiles
	CreateTileCache(map_data);
}

bool TilemapLayer::GetVisible() const {
	return visible;
}

void TilemapLayer::SetVisible(bool nvisible) {
	visible = nvisible;
}

int TilemapLayer::GetOx() const {
	return ox;
}

void TilemapLayer::SetOx(int nox) {
	ox = nox;
}

int TilemapLayer::GetOy() const {
	return oy;
}

void TilemapLayer::SetOy(int noy) {
	oy = noy;
}

int TilemapLayer::GetWidth() const {
	return width;
}

void TilemapLayer::SetWidth(int nwidth) {
	width = nwidth;
}

int TilemapLayer::GetHeight() const {
	return height;
}

void TilemapLayer::SetHeight(int nheight) {
	height = nheight;
}

int TilemapLayer::GetAnimationSpeed() const {
	return animation_speed;
}

void TilemapLayer::SetAnimationSpeed(int speed) {
	animation_speed = speed;
}

int TilemapLayer::GetAnimationType() const {
	return animation_type;
}

void TilemapLayer::SetAnimationType(int type) {
	animation_type = type;
}

void TilemapLayer::Substitute(int old_id, int new_id) {
	int subst_count = 0;

	for (size_t i = 0; i < substitutions.size(); ++i) {
		if (substitutions[i] == old_id) {
			++subst_count;
			substitutions[i] = (uint8_t) new_id;
		}
	}

	if (subst_count > 0) {
		// Recalculate z values of all tiles
		CreateTileCache(map_data);
	}
}

TilemapTile::TilemapTile(TilemapLayer* tilemap, int z) :
	Drawable(TypeTilemap, z),
	tilemap(tilemap)
{
}

void TilemapTile::Draw() {
	if (!tilemap->GetChipset()) {
		return;
	}

	tilemap->Draw(GetZ());
}
