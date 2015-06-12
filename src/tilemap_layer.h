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

#ifndef _TILEMAP_LAYER_H_
#define _TILEMAP_LAYER_H_

// Headers
#include <vector>
#include <map>
#include "system.h"
#include "drawable.h"
#include "rect.h"

class TilemapLayer;

class TilemapTile : public Drawable {
public:
	TilemapTile(TilemapLayer* tilemap, int z);

	void Draw();

private:
	TilemapLayer* tilemap;
};

/**
 * TilemapLayer class.
 */
class TilemapLayer {
public:
	TilemapLayer(int ilayer);

	typedef EASYRPG_ARRAY<Rect, 4> subtile_coords;

	void DrawTile(int x, int y, int row, int col);
	void DrawTile(int x, int y, subtile_coords const& coords);
	void Draw(int z_order);

	void Update();

	BitmapRef const& GetChipset() const;
	void SetChipset(BitmapRef const& nchipset);
	std::vector<int16_t> const& GetMapData() const;
	void SetMapData(const std::vector<int16_t>& nmap_data);
	std::vector<uint8_t> const& GetPassable() const;
	void SetPassable(const std::vector<uint8_t>& npassable);
	int GetOx() const;
	void SetOx(int nox);
	int GetOy() const;
	void SetOy(int noy);
	int GetWidth() const;
	void SetWidth(int nwidth);
	int GetHeight() const;
	void SetHeight(int nheight);
	int GetAnimationSpeed() const;
	void SetAnimationSpeed(int speed);
	int GetAnimationType() const;
	void SetAnimationType(int type);
	bool GetVisible() const;
	void SetVisible(bool v);

	void Substitute(int old_id, int new_id);

private:
	BitmapRef chipset;
	std::vector<int16_t> map_data;
	std::vector<uint8_t> passable;
	std::vector<uint8_t> substitutions;
	bool visible;
	int ox;
	int oy;
	int width;
	int height;
	char animation_frame;
	char animation_step_ab;
	char animation_step_c;
	int animation_speed;
	int animation_type;
	int layer;

	void CreateTileCache(const std::vector<int16_t>& nmap_data);

	static uint8_t const subtile_base[4][2];
	enum { SKIP_SUBTILE = -1 };
	subtile_coords sea_subtiles, seaside_subtiles, terrain_subtiles;
	void sea_pattern(unsigned id, unsigned anime);
	void terrain_pattern(unsigned id);

	int autotiles_ab_next;
	int autotiles_d_next;

	struct TileData {
		short ID;
		int z;
	};
	std::vector<std::vector<TileData> > data_cache;
	std::vector<EASYRPG_SHARED_PTR<TilemapTile> > tilemap_tiles;
};

#endif
