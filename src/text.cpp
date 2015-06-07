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
#include "data.h"
#include "cache.h"
#include "output.h"
#include "utils.h"
#include "bitmap.h"
#include "font.h"
#include "text.h"
#include "game_system.h"
#include "rect.h"

#include <cctype>

#include <boost/next_prior.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>

void Text::Draw(Bitmap& dest, int x, int y, int color, std::string const& text) {
	Draw(dest, Rect(x, y, dest.GetWidth() - x, dest.GetHeight() - y), color, text);
}

void Text::Draw(Bitmap& dest, Rect const& dst_rect_, int color, std::string const& text, Text::Alignment align) {
	FontRef font = dest.GetFont();
	Rect dst_rect = dst_rect_;
	Rect const text_size = font->GetSize(text);

	switch (align) {
	case Text::AlignCenter:
		dst_rect.x += std::max(0, (dst_rect.width - text_size.width) / 2); break;
	case Text::AlignRight:
		dst_rect.x += std::max(0, dst_rect.width - text_size.width); break;
	case Text::AlignLeft:
		dst_rect.x += 0; break;
	default: assert(false);
	}

	BitmapRef system = Cache::System();

	// Where to draw the next glyph (x pos)
	int next_glyph_pos = 0;

	// This loops always renders a single char, color blends it and then puts
	// it onto the text_surface (including the drop shadow)
	for (boost::u8_to_u32_iterator<std::string::const_iterator>
			 c(text.begin(), text.begin(), text.end()),
			 end(text.end(), text.begin(), text.end()); c != end; ++c) {
		boost::u8_to_u32_iterator<std::string::const_iterator> next_c_it = boost::next(c);
		uint32_t const next_c = std::distance(c, end) > 1? *next_c_it : 0;

		// ExFont-Detection: Check for A-Z or a-z behind the $
		if (*c == '$' && std::isalpha(next_c)) {
			int exfont_value = -1;
			// Calculate which exfont shall be rendered
			if (islower(next_c)) {
				exfont_value = 26 + next_c - 'a';
			} else if (isupper(next_c)) {
				exfont_value = next_c - 'A';
			} else { assert(false); }

			Font::exfont->Render(dest, dst_rect.x + next_glyph_pos, dst_rect.y,
					     *system, color, exfont_value);

			next_glyph_pos += 12;
			// Skip the next character
			++c;
		} else { // Not ExFont, draw normal text
			font->Render(dest, dst_rect.x + next_glyph_pos, dst_rect.y, *system, color, *c);
			next_glyph_pos += font->GetSize(std::string(c.base(), next_c_it.base())).width;
		}
	}
}

void Text::Draw(Bitmap& dest, int x, int y, Color const& color, std::string const& text) {
	FontRef font = dest.GetFont();

	int next_glyph_pos = 0;

	for (boost::u8_to_u32_iterator<std::string::const_iterator>
			 c(text.begin(), text.begin(), text.end()),
			 end(text.end(), text.begin(), text.end()); c != end; ++c) {
		boost::u8_to_u32_iterator<std::string::const_iterator> next_c_it = boost::next(c);

		std::string const glyph(c.base(), next_c_it.base());
		if (*c == '\n') {
			y += font->GetSize(glyph).height;
			next_glyph_pos = 0;
			continue;
		}

		font->Render(dest, x + next_glyph_pos, y, color, *c);
		next_glyph_pos += font->GetSize(glyph).width;
	}
}
