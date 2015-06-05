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

#ifndef _BITMAP_H_
#define _BITMAP_H_

// Headers
#include <string>
#include "memory_management.h"

class Rect;
class Color;

/**
 * Base Bitmap class.
 */
class Bitmap {
public:
	/**
	 * Creates bitmap with empty surface.
	 *
	 * @param width surface width.
	 * @param height surface height.
	 * @param color color for filling.
	 */
	static BitmapRef Create(int width, int height, const Color& color);

	/**
	 * Loads a bitmap from an image file.
	 *
	 * @param filename image file to load.
	 * @param transparent allow transparency on bitmap.
	 * @param flags bitmap flags.
	 */
	static BitmapRef Create(const std::string& filename, bool transparent);

	/*
	 * Loads a bitmap from memory.
	 *
	 * @param data image data.
	 * @param bytes size of data.
	 * @param transparent allow transparency on bitmap.
	 * @param flags bitmap flags.
	 */
	static BitmapRef Create(const uint8_t* data, unsigned bytes, bool transparent);

	/**
	 * Creates a bitmap from another.
	 *
	 * @param source source bitmap.
	 * @param src_rect rect to copy from source bitmap.
	 * @param transparent allow transparency on bitmap.
	 */
	static BitmapRef Create(Bitmap const& source, Rect const& src_rect);

	/**
	 * Creates a surface.
	 *
	 * @param width surface width.
	 * @param height surface height.
	 * @param bpp surface bpp.
	 * @param transparent allow transparency on surface.
	 */
	static BitmapRef Create(int width, int height);

	/**
	 * Destructor.
	 */
	~Bitmap();

	/**
	 * Gets the bitmap width.
	 *
	 * @return the bitmap width.
	 */
	int GetWidth() const;

	/**
	 * Gets the bitmap height.
	 *
	 * @return the bitmap height.
	 */
	int GetHeight() const;

	/**
	 * Gets bitmap bounds rect.
	 *
	 * @return bitmap bounds rect.
	 */
	Rect GetRect() const;

	/**
	 * Gets pixel from bitmap.
	 *
	 * @param x x coordinate
	 * @param y y coordinate
	 * @return pixel value of (x, y)
	 */
	Color GetPixel(int x, int y) const;

	/**
	 * Sets pixel of bitmap.
	 *
	 * @param x x coordinate
	 * @param y y coordinate
	 * @param c color to set
	 */
	void SetPixel(int x, int y, Color const& c);

public:

	/**
	 * Creates a surface wrapper around existing pixel data.
	 *
	 * @param pixels pointer to pixel data.
	 * @param width surface width.
	 * @param height surface height.
	 * @param pitch surface pitch.
	 * @param format pixel format.
	*/
	static BitmapRef Create(void *pixels, int width, int height, int pitch);

	/**
	 * Blits source bitmap to this one.
	 *
	 * @param x x position.
	 * @param y y position.
	 * @param src source bitmap.
	 * @param src_rect source bitmap rect.
	 * @param opacity opacity for blending with bitmap.
	 */
	void Blit(int x, int y, Bitmap const& src, Rect const& src_rect, uint8_t opacity = 255);

	/**
	 * Blits source bitmap stretched to this one.
	 *
	 * @param src source bitmap.
	 * @param src_rect source bitmap rect.
	 * @param opacity opacity for blending with bitmap.
	 */
	void StretchBlit(Bitmap const& src, Rect const& src_rect, uint8_t opacity = 255);

	/**
	 * Blits source bitmap stretched to this one.
	 *
	 * @param dst_rect destination rect.
	 * @param src source bitmap.
	 * @param src_rect source bitmap rect.
	 * @param opacity opacity for blending with bitmap.
	 */
	void StretchBlit(Rect const& dst_rect, Bitmap const& src, Rect const& src_rect, uint8_t opacity = 255);

	/**
	 * Fills entire bitmap with color.
	 *
	 * @param color color for filling.
	 */
	void Fill(const Color &color);

	/**
	 * Fills bitmap rect with color.
	 *
	 * @param dst_rect destination rect.
	 * @param color color for filling.
	 */
	void FillRect(Rect const& dst_rect, const Color &color);

	/**
	 * Clears the bitmap with transparent pixels.
	 */
	void Clear();

	/**
	 * Clears the bitmap rect with transparent pixels.
	 *
	 * @param dst_rect destination rect.
	 */
	void ClearRect(Rect const& dst_rect);

	/**
	 * Rotates bitmap hue.
	 *
	 * @param x x position.
	 * @param y y position.
	 * @param src source bitmap.
	 * @param src_rect source bitmap rect.
	 * @param hue hue change, degrees.
	 */
	void HueChange(double hue);

	/**
	 * Gets text drawing font.
	 *
	 * @return text drawing font.
	 */
	FontRef const& GetFont() const;

	/**
	 * Sets text drawing font.
	 *
	 * @param font drawing font.
	 */
	void SetFont(FontRef const& font);

	Bitmap(int width, int height);
	Bitmap(Bitmap const& source, Rect const& src_rect);
	Bitmap(const std::string& filename, bool transparent);
	Bitmap(const uint8_t* data, unsigned bytes, bool transparent);
	Bitmap(void *pixels, int width, int height, int pitch);

	void const* GetData() const;
	bool GetDirty();

private:
	/** Font for text drawing. */
	FontRef font;
	/** Flag to check dirty. */
	bool dirty;
	/** Bitmap data. */
	union pixman_image *bitmap;

	void Init(int width, int height, void* data, int pitch = 0, bool destroy = true);
	void InitState();
};

#endif
