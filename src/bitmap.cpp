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
#include <pixman.h>

#include <vector>

#include "bitmap.h"
#include "bitmap_hslrgb.h"
#include "font.h"
#include "rect.h"
#include "text.h"
#include "color.h"
#include "output.h"
#include "filefinder.h"

#include "image_bmp.h"
#include "image_png.h"
#include "image_xyz.h"

BitmapRef Bitmap::Create(int width, int height, const Color& color) {
	BitmapRef surface = Bitmap::Create(width, height);
	surface->Fill(color);
	return surface;
}

BitmapRef Bitmap::Create(const std::string& filename, bool transparent) {
	return EASYRPG_MAKE_SHARED<Bitmap>(filename, transparent);
}

BitmapRef Bitmap::Create(const uint8_t* data, unsigned bytes, bool transparent) {
	return EASYRPG_MAKE_SHARED<Bitmap>(data, bytes, transparent);
}

BitmapRef Bitmap::Create(Bitmap const& source, Rect const& src_rect) {
	return EASYRPG_MAKE_SHARED<Bitmap>(source, src_rect);
}

Bitmap::~Bitmap() {
	pixman_image_unref(bitmap);
}

int Bitmap::GetWidth() const {
	return pixman_image_get_width(bitmap);
}

int Bitmap::GetHeight() const {
	return pixman_image_get_height(bitmap);
}

Rect Bitmap::GetRect() const {
	return Rect(0, 0, GetWidth(), GetHeight());
}

BitmapRef Bitmap::Create(int width, int height) {
	return EASYRPG_MAKE_SHARED<Bitmap>(width, height);
}

BitmapRef Bitmap::Create(void *pixels, int width, int height, int pitch) {
	return EASYRPG_MAKE_SHARED<Bitmap>(pixels, width, height, pitch);
}

void Bitmap::HueChange(double hue_) {
	int hue  = (int) (hue_ / 60.0 * 0x100);
	if (hue < 0)
		hue += ((-hue + 0x5FF) / 0x600) * 0x600;
	else if (hue > 0x600)
		hue -= (hue / 0x600) * 0x600;

	uint32_t *data = pixman_image_get_data(bitmap);
	int const stride = pixman_image_get_stride(bitmap);

	for (int y = 0; y < GetHeight(); ++y) {
		for (int x = 0; x < GetWidth(); ++x) {
			uint8_t* c = reinterpret_cast<uint8_t*>(data + stride * y + x);
			uint8_t r = c[0], g = c[1], b = c[2], a = c[3];
			RGB_adjust_HSL(r, g, b, hue);
			c[0] = r; c[1] = g; c[2] = b; c[3] = a;
		}
	}
	dirty = true;
}

FontRef const& Bitmap::GetFont() const {
	return font;
}

void Bitmap::SetFont(FontRef const& new_font) {
	font = new_font;
}

static void destroy_func(pixman_image_t * /* image */, void *data) {
	free(data);
}

void Bitmap::InitState() {
	dirty = true;
	font = Font::Default();
}

void Bitmap::Init(int width, int height, void* data, int pitch, bool destroy) {
	if (!pitch)
		pitch = width * sizeof(uint32_t);

	bitmap = pixman_image_create_bits(
		PIXMAN_r8g8b8a8, width, height, reinterpret_cast<uint32_t*>(data), pitch);

	if (not bitmap) {
		Output::Error("Couldn't create %dx%d image.", width, height);
	}

	if (data != NULL && destroy)
		pixman_image_set_destroy_function(bitmap, destroy_func, data);
}

Bitmap::Bitmap(int width, int height) {
	InitState();
	Init(width, height, (void *) NULL);
}

Bitmap::Bitmap(void *pixels, int width, int height, int pitch) {
	InitState();
	Init(width, height, pixels, pitch, false);
}

Bitmap::Bitmap(const std::string& filename, bool transparent) {
	InitState();

	FILE* stream = FileFinder::fopenUTF8(filename, "rb");
	if (!stream) {
		Output::Error("Couldn't open image file %s", filename.c_str());
		return;
	}

	int w = 0;
	int h = 0;
	void* pixels;

	char data[4];
	size_t bytes = fread(&data, 1, 4, stream);
	fseek(stream, 0, SEEK_SET);

	if (bytes >= 4 && strncmp((char*)data, "XYZ1", 4) == 0)
		ImageXYZ::ReadXYZ(stream, transparent, w, h, pixels);
	else if (bytes > 2 && strncmp((char*)data, "BM", 2) == 0)
		ImageBMP::ReadBMP(stream, transparent, w, h, pixels);
	else if (bytes >= 4 && strncmp((char*)(data + 1), "PNG", 3) == 0)
		ImagePNG::ReadPNG(stream, (void*)NULL, transparent, w, h, pixels);
	else {
		Output::Error("Unsupported image file %s", filename.c_str());
		return;
	}

	fclose(stream);

	Init(w, h, pixels);
}

Bitmap::Bitmap(const uint8_t* data, unsigned bytes, bool transparent) {
	InitState();

	int w, h;
	void* pixels;

	if (bytes > 4 && strncmp((char*) data, "XYZ1", 4) == 0)
		ImageXYZ::ReadXYZ(data, bytes, transparent, w, h, pixels);
	else if (bytes > 2 && strncmp((char*) data, "BM", 2) == 0)
		ImageBMP::ReadBMP(data, bytes, transparent, w, h, pixels);
	else if (bytes > 4 && strncmp((char*)(data + 1), "PNG", 3) == 0)
		ImagePNG::ReadPNG((FILE*) NULL, (const void*) data, transparent, w, h, pixels);
	else {
		Output::Error("Unsupported image");
		return;
	}

	Init(w, h, pixels);
}

Bitmap::Bitmap(Bitmap const& source, Rect const& src_rect) {
	InitState();
	Init(src_rect.width, src_rect.height, (void *) NULL);
	Blit(0, 0, source, src_rect);
}

static EASYRPG_SHARED_PTR<pixman_image_t> CreateMask(uint8_t opacity) {
	pixman_color_t tcolor = {0, 0, 0, static_cast<uint16_t>(opacity << 8)};
	return EASYRPG_SHARED_PTR<pixman_image_t>(
	    pixman_image_create_solid_fill(&tcolor), pixman_image_unref);
}

void Bitmap::Blit(int x, int y, Bitmap const& src, Rect const& src_rect, uint8_t opacity) {
	EASYRPG_SHARED_PTR<pixman_image_t> mask = CreateMask(opacity);

	pixman_image_composite32(PIXMAN_OP_OVER,
				 src.bitmap,
				 mask.get(), bitmap,
				 src_rect.x, src_rect.y,
				 0, 0,
				 x, y,
				 src_rect.width, src_rect.height);
	dirty = true;
}

void Bitmap::StretchBlit(Rect const& dst_rect, Bitmap const& src, Rect const& src_rect, uint8_t opacity) {
	double zoom_x = (double)src_rect.width  / dst_rect.width;
	double zoom_y = (double)src_rect.height / dst_rect.height;

	pixman_transform_t xform;

	pixman_transform_init_scale(&xform,
								pixman_double_to_fixed(zoom_x),
								pixman_double_to_fixed(zoom_y));

	pixman_transform_translate((pixman_transform_t *) NULL, &xform, src_rect.x, src_rect.y);

	pixman_image_set_transform(src.bitmap, &xform);

	EASYRPG_SHARED_PTR<pixman_image_t> mask = CreateMask(opacity);

	pixman_image_composite32(PIXMAN_OP_OVER,
				 src.bitmap, mask.get(), bitmap,
				 src_rect.x / zoom_x, src_rect.y / zoom_y,
				 0, 0,
				 dst_rect.x, dst_rect.y,
				 dst_rect.width, dst_rect.height);

	pixman_transform_init_identity(&xform);
	pixman_image_set_transform(src.bitmap, &xform);
	dirty = true;
}

static pixman_color_t PixmanColor(const Color &color) {
	pixman_color_t pcolor;
	pcolor.red = color.red * color.alpha;
	pcolor.green = color.green * color.alpha;
	pcolor.blue = color.blue * color.alpha;
	pcolor.alpha = color.alpha << 8;
	return pcolor;
}

static pixman_rectangle16_t PixmanRect(Rect const& r) {
	pixman_rectangle16_t ret = {
		static_cast<int16_t>(r.x), static_cast<int16_t>(r.y),
		static_cast<uint16_t>(r.width), static_cast<uint16_t>(r.height), };
	return ret;
}

void Bitmap::Fill(const Color &color) {
	pixman_color_t pcolor = PixmanColor(color);
	pixman_rectangle16_t rect = PixmanRect(GetRect());
	pixman_image_fill_rectangles(PIXMAN_OP_OVER, bitmap, &pcolor, 1, &rect);
	dirty = true;
}

void Bitmap::FillRect(Rect const& dst_rect, const Color &color) {
	pixman_color_t pcolor = PixmanColor(color);
	pixman_rectangle16_t rect = PixmanRect(dst_rect);
	pixman_image_fill_rectangles(PIXMAN_OP_OVER, bitmap, &pcolor, 1, &rect);
	dirty = true;
}

void Bitmap::Clear() {
	pixman_color_t pcolor = {0, 0, 0, 0};
	pixman_rectangle16_t rect = PixmanRect(GetRect());
	pixman_image_fill_rectangles(PIXMAN_OP_CLEAR, bitmap, &pcolor, 1, &rect);
	dirty = true;
}

void Bitmap::ClearRect(Rect const& dst_rect) {
	pixman_color_t pcolor = {0, 0, 0, 0};
	pixman_rectangle16_t rect = PixmanRect(dst_rect);
	pixman_image_fill_rectangles(PIXMAN_OP_CLEAR, bitmap, &pcolor, 1, &rect);
	dirty = true;
}

void Bitmap::SetPixel(int x, int y, Color const& src) {
	if (x < 0 or GetWidth() <= x or
	    y < 0 or GetHeight() <= y) { return; }

	dirty = true;
	uint8_t* c = reinterpret_cast<uint8_t*>(pixman_image_get_data(bitmap))
		+ pixman_image_get_stride(bitmap) * y + sizeof(uint32_t) * x;
	c[0] = src.red; c[1] = src.green; c[2] = src.blue; c[3] = src.alpha;
}

Color Bitmap::GetPixel(int x, int y) const {
	if (x < 0 or GetWidth() <= x or
	    y < 0 or GetHeight() <= y) { return Color(); }

	uint8_t const* c = reinterpret_cast<uint8_t*>(pixman_image_get_data(bitmap))
		+ pixman_image_get_stride(bitmap) * y + sizeof(uint32_t) * x;
	return Color(c[0], c[1], c[2], c[3]);
}

void const* Bitmap::GetData() const {
	return pixman_image_get_data(bitmap);
}

bool Bitmap::GetDirty() {
	using std::swap;
	bool ret = false;
	swap(ret, dirty);
	return ret;
}
