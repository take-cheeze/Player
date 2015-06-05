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

#ifndef _DRAWABLE_H_
#define _DRAWABLE_H_

// What kind of drawable is the current one?
enum DrawableType {
	TypeWindow,
	TypeTilemap,
	TypeSprite,
	TypePlane,
	TypeBackground,
	TypeScreen,
	TypeWeather,
	TypeMessageOverlay,
	TypeDefault};

/**
 * Drawable virtual
 */
class Drawable {
public:
	Drawable(DrawableType t, int z, bool g = false);
	virtual ~Drawable();

	virtual void Draw() = 0;

	int GetZ() const;
	void SetZ(int z);

	DrawableType GetType() const;

	void SetVisible(bool v);
	bool GetVisible() const;

private:
	DrawableType type;
	int z;
	bool global;
	bool visible;
};

#endif
