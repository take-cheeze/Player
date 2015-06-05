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
#include <map>
#include <vector>
#include <ciso646>

#include <boost/next_prior.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <boost/type_traits/remove_pointer.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_MODULE_H

#include "reader_util.h"
#include "shinonome.hxx"

#include "filefinder.h"
#include "output.h"
#include "font.h"
#include "bitmap.h"
#include "utils.h"
#include "cache.h"
#include "rect.h"

bool operator<(ShinonomeGlyph const& lhs, uint32_t const code) {
	return lhs.code < code;
}

// Static variables.
namespace {
	typedef std::map<std::string, EASYRPG_WEAK_PTR<boost::remove_pointer<FT_Face>::type> > face_cache_type;
	face_cache_type face_cache;
	ShinonomeGlyph const* find_glyph(ShinonomeGlyph const* data, size_t size, uint32_t code) {
		ShinonomeGlyph const* ret = std::lower_bound(data, data + size, code);
		if(ret != (data + size) and ret->code == code) {
			return ret;
		} else {
			static ShinonomeGlyph const empty_glyph = { 0, true, {0} };
			Output::Debug("glyph not found: 0x%04x", code);
			return &empty_glyph;
		}
	}

	ShinonomeGlyph const* find_gothic_glyph(uint32_t code) {
		return find_glyph(SHINONOME_GOTHIC,
						  sizeof(SHINONOME_GOTHIC) / sizeof(ShinonomeGlyph), code);
	}

	ShinonomeGlyph const* find_mincho_glyph(uint32_t code) {
		ShinonomeGlyph const* const mincho =
			find_glyph(SHINONOME_MINCHO,
					   sizeof(SHINONOME_MINCHO) / sizeof(ShinonomeGlyph), code);
		return mincho == NULL? find_gothic_glyph(code) : mincho;
	}

	struct ShinonomeFont : public Font {
		enum { HEIGHT = 12, FULL_WIDTH = HEIGHT, HALF_WIDTH = FULL_WIDTH / 2 };

		typedef ShinonomeGlyph const*(*function_type)(uint32_t);

		ShinonomeFont(function_type func);

		Rect GetSize(std::string const& txt) const;

		void RenderGlyph(unsigned code, pixel_getter const&, pixel_setter const&);

	private:
		function_type const func_;
	}; // class ShinonomeFont


	void delete_face(FT_Face f) {
		if(FT_Done_Face(f) != FT_Err_Ok) {
			Output::Warning("FT_Face deleting error.");
		}
	}

	void delete_library(FT_Library f) {
		if(FT_Done_Library(f) != FT_Err_Ok) {
			Output::Warning("FT_Library deleting error.");
		}
	}

	struct FTFont : public Font  {
		FTFont(const std::string& name, int size, bool bold, bool italic);

		Rect GetSize(std::string const& txt) const;

		void RenderGlyph(unsigned code, pixel_getter const&, pixel_setter const&);

	private:
		static EASYRPG_WEAK_PTR<boost::remove_pointer<FT_Library>::type> library_checker_;
		EASYRPG_SHARED_PTR<boost::remove_pointer<FT_Library>::type> library_;
		EASYRPG_SHARED_PTR<boost::remove_pointer<FT_Face>::type> face_;
		std::string face_name_;
		unsigned current_size_;

		bool check_face();
	}; // class FTFont

	struct ExFont : public Font {
		ExFont();
		Rect GetSize(std::string const& txt) const;
		void RenderGlyph(unsigned code, pixel_getter const&, pixel_setter const&);
	};

	struct GameSystemPixelAccessor {
		Bitmap const& src_bmp;
		Bitmap& dst_bmp;
		int src_x, src_y, dst_x, dst_y;
		Color operator()(int x, int y) const {
			return src_bmp.GetPixel(src_x + x, src_y + y);
		}
		void operator()(int x, int y, Color const& c) const {
			dst_bmp.SetPixel(dst_x + x, dst_y + y, c);
		}
	};

	struct FixedColorGetterPixelAccessor {
		Bitmap& bmp;
		Color color;
		int base_x, base_y;
		Color operator()(int, int) const {
			return color;
		}
		void operator()(int x, int y, Color const& c) const {
			bmp.SetPixel(base_x + x, base_y + y, c);
		}
	};
} // anonymous namespace

ShinonomeFont::ShinonomeFont(ShinonomeFont::function_type func)
	: Font("Shinonome", HEIGHT, false, false), func_(func) {}

Rect ShinonomeFont::GetSize(std::string const& txt) const {
	typedef boost::u8_to_u32_iterator<std::string::const_iterator> iterator;
	size_t units = 0;
	iterator i(txt.begin(), txt.begin(), txt.end());
	iterator const end(txt.end(), txt.begin(), txt.end());
	for(; i != end; ++i) {
		ShinonomeGlyph const* const glyph = func_(*i);
		assert(glyph);
		units += glyph->is_full? 2 : 1;
	}
	return Rect(0, 0, units * HALF_WIDTH, HEIGHT);
}

void ShinonomeFont::RenderGlyph(unsigned code, pixel_getter const& g, pixel_setter const& s) {
	ShinonomeGlyph const* const glyph = func_(code);
	assert(glyph);
	size_t const width = glyph->is_full? FULL_WIDTH : HALF_WIDTH;

	for (size_t y = 0; y < HEIGHT; ++y)
		for(size_t x = 0; x < width; ++x)
			if (glyph->data[y] & (0x1 << x)) s(x, y, g(x, y));
}

EASYRPG_WEAK_PTR<boost::remove_pointer<FT_Library>::type> FTFont::library_checker_;

FTFont::FTFont(const std::string& name, int size, bool bold, bool italic)
	: Font(name, size, bold, italic), current_size_(0) {}

Rect FTFont::GetSize(std::string const& txt) const {
	Utils::wstring tmp = Utils::ToWideString(txt);
	int const s = Font::Default()->GetSize(txt).width;

	if (s == -1) {
		Output::Warning("Text contains invalid chars.\n"\
			"Is the encoding correct?");

		return Rect(0, 0, pixel_size() * txt.size() / 2, pixel_size());
	} else {
		return Rect(0, 0, s, pixel_size());
	}
}

void FTFont::RenderGlyph(unsigned glyph, pixel_getter const& g, pixel_setter const& s) {
	if(!check_face()) {
		return Font::Default()->RenderGlyph(glyph, g, s);
	}

	if (FT_Load_Char(face_.get(), glyph, FT_LOAD_NO_BITMAP) != FT_Err_Ok) {
		Output::Error("Couldn't load FreeType character %d", glyph);
	}

	if (FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_MONO) != FT_Err_Ok) {
		Output::Error("Couldn't render FreeType character %d", glyph);
	}

	FT_Bitmap const& ft_bitmap = face_->glyph->bitmap;
	assert(face_->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO);

	for(size_t y = 0; y < ft_bitmap.rows; ++y) {
		for(size_t x = 0; x < ft_bitmap.width; ++x) {
			unsigned c = ft_bitmap.buffer[std::abs(ft_bitmap.pitch) * y + (x/8)];
			unsigned bit = 7 - (x%8);
			if (c & (0x01 << bit)) s(x, y, g(x, y));
		}
	}
}

FontRef Font::default_font_ = Font::Gothic();

FontRef Font::Mincho() {
	return EASYRPG_MAKE_SHARED<ShinonomeFont>(&find_mincho_glyph);
}

FontRef Font::Gothic() {
	return EASYRPG_MAKE_SHARED<ShinonomeFont>(&find_gothic_glyph);
}

FontRef Font::Default() { return default_font_; }
void Font::SetDefault(FontRef const& r) { default_font_ = r; }

FontRef Font::Create(const std::string& name, int size, bool bold, bool italic) {
	return EASYRPG_MAKE_SHARED<FTFont>(name, size, bold, italic);
}

void Font::Dispose() {
	for(face_cache_type::const_iterator i = face_cache.begin(); i != face_cache.end(); ++i) {
		if(i->second.expired()) { continue; }
		Output::Debug("possible leak in cached font face %s", i->first.c_str());
	}
	face_cache.clear();
}

// Constructor.
Font::Font(const std::string& name, int size, bool bold, bool italic)
	: name(name)
	, size(size)
	, bold(bold)
	, italic(italic)
{
}

bool FTFont::check_face() {
	if(!library_) {
		if(library_checker_.expired()) {
			FT_Library lib;
			if(FT_Init_FreeType(&lib) != FT_Err_Ok) {
				Output::Error("Couldn't initialize FreeType");
				return false;
			}
			library_.reset(lib, delete_library);
			library_checker_ = library_;
		} else {
			library_ = library_checker_.lock();
		}
	}

	if(!face_ || face_name_ != name) {
	    face_cache_type::const_iterator it = face_cache.find(name);
		if(it == face_cache.end() || it->second.expired()) {
			std::string const face_path = FileFinder::FindFont(name);
			FT_Face face;
			if(FT_New_Face(library_.get(), face_path.c_str(), 0, &face) != FT_Err_Ok) {
				Output::Error("Couldn't initialize FreeType face: %s(%s)",
							  name.c_str(), face_path.c_str());
				return false;
			}

			for (int i = 0; i < face_->num_fixed_sizes; i++) {
				FT_Bitmap_Size* size = &face_->available_sizes[i];
				Output::Debug("Font Size %d: %d %d %f %f %f", i,
							  size->width, size->height, size->size / 64.0,
							  size->x_ppem / 64.0, size->y_ppem / 64.0);
			}

			face_.reset(face, delete_face);
			face_cache[name] = face_;
		} else {
			face_ = it->second.lock();
		}
		face_name_ = name;
	}

	face_->style_flags =
		(bold? FT_STYLE_FLAG_BOLD : 0) |
		(italic? FT_STYLE_FLAG_ITALIC : 0);

	if(current_size_ != size) {
		int sz, dpi;
		if (face_->num_fixed_sizes == 1) {
			sz = face_->available_sizes[0].size;
			dpi = 96;
		} else {
			sz = size * 64;
			dpi = 72;
		}

		if (FT_Set_Char_Size(face_.get(), sz, sz, dpi, dpi) != FT_Err_Ok) {
			Output::Error("Couldn't set FreeType face size");
			return false;
		}
		current_size_ = size;
	}

	return true;
}

void Font::Render(Bitmap& bmp, int const x, int const y, Bitmap const& sys, int color, unsigned code) {
	// Shadow first
	GameSystemPixelAccessor func = { sys, bmp, 16, 32, x + 1, y + 1 };
	RenderGlyph(code, func, func);

	func.src_x = color % 10 * 16 + 2;
	func.src_y = color / 10 * 16 + 48 + 16 - ShinonomeFont::HEIGHT;
	func.dst_x -= 1; func.dst_y -= 1;
	RenderGlyph(code, func, func);
}

void Font::Render(Bitmap& bmp, int x, int y, Color const& color, unsigned code) {
	FixedColorGetterPixelAccessor const func = { bmp, color, x, y };
	RenderGlyph(code, func, func);
}

ExFont::ExFont() : Font("exfont", 12, false, false) {
}

FontRef const Font::exfont = EASYRPG_MAKE_SHARED<ExFont>();

void ExFont::RenderGlyph(unsigned code, pixel_getter const& g, pixel_setter const& s) {
	BitmapRef exfont = Cache::Exfont();
	int base_x = (code % 13) * 12, base_y = (code / 13) * 12;
	for (int y = 0; y < 12; ++y)
		for (int x = 0; x < 12; ++x)
			if (exfont->GetPixel(base_x + x, base_y + y).alpha != 0) { s(x, y, g(x, y)); }
}

Rect ExFont::GetSize(std::string const& /* txt */) const {
	return Rect(0, 0, 12, 12);
}
