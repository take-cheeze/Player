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

#ifndef _EASYRPG_IMAGE_PNG_H_
#define _EASYRPG_IMAGE_PNG_H_

#include "system.h"
#ifdef SUPPORT_PNG

#include <cstdio>
#include <iosfwd>
#include "system.h"

class Bitmap;

namespace ImagePNG {
	void ReadPNG(FILE* stream, const void* buffer, bool transparent, int& width, int& height, void*& pixels);

	/**
	 * Writes PNG converted bitmap to output stream.
	 *
	 * @param os output stream that PNG will be output.
	 * @param bmp source bitmap to write
	 * @return true if success, otherwise false.
	 */
	bool WritePNG(std::ostream& os, Bitmap const& bmp);
}

#endif // SUPPORT_PNG

#endif
