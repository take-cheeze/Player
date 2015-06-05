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

#ifndef _EASYRPG_TEXT_H_
#define _EASYRPG_TEXT_H_

#include "system.h"
#include <string>

class Color;
class Rect;

namespace Text {
	/** TextDraw alignment options. */
	enum Alignment {
		AlignLeft,
		AlignCenter,
		AlignRight
	};

	enum SystemColor {
		ColorShadow = -1,
		ColorDefault = 0,
		ColorDisabled = 3,
		ColorCritical = 4,
		ColorKnockout = 5
	};

	void Draw(Bitmap& dest, int x, int y, int color, std::string const& text, Text::Alignment align = Text::AlignLeft);
	void Draw(Bitmap& dest, Rect const& dst_rect, int color, std::string const& text, Text::Alignment align = Text::AlignLeft);

	/**
	 * Draws text using the specified color on dest
	 */
	void Draw(Bitmap& dest, int x, int y, Color const& color, std::string const& text);
}
#endif
