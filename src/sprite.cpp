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
#include <string>
#include "sprite.h"
#include "player.h"
#include "graphics.h"
#include "util_macro.h"
#include "bitmap.h"

// Constructor
Sprite::Sprite() :
	Drawable(TypeSprite, 0),
	x(0),
	y(0),
	ox(0),
	oy(0),
	flash_duration(0),
	flash_frame(0),

	opacity_top_effect(255),
	opacity_bottom_effect(128),
	bush_effect(0),
	tone_effect(Tone()),
	flipx_effect(false),
	flipy_effect(false),
	zoom_x_effect(1.0),
	zoom_y_effect(1.0),
	angle_effect(0.0),
	waver_effect_depth(0),
	waver_effect_phase(0.0),
	flash_effect(Color(0,0,0,0))
{
}

int Sprite::GetWidth() const {
	return src_rect.width;
}

int Sprite::GetHeight() const {
	return src_rect.height;
}

void Sprite::Update() {
	if (flash_duration != 0) {
		flash_frame += 1;
		if (flash_duration == flash_frame) {
			flash_duration = 0;
			SetFlashEffect(Color());
		} else {
			Color flash_effect = flash_color;
			flash_effect.alpha = flash_duration == 0 || flash_frame >= flash_duration
				? 0
				: flash_effect.alpha * (flash_duration - flash_frame) / flash_duration;
			SetFlashEffect(flash_effect);
		}
	}
}

void Sprite::Flash(int duration){
	SetFlashEffect(flash_color);
	flash_duration = duration;
	flash_frame = 0;
}
void Sprite::Flash(Color color, int duration){
	flash_color = color;
	flash_duration = duration;
	flash_frame = 0;
	SetFlashEffect(color);
}

void Sprite::SetFlashEffect(const Color &color) {
	flash_effect = color;
}

BitmapRef const& Sprite::GetBitmap() const {
	return bitmap;
}

void Sprite::SetBitmap(BitmapRef const& nbitmap) {
	bitmap = nbitmap;
	if (!bitmap) {
		src_rect = Rect();
	} else {
		src_rect = bitmap->GetRect();
	}
}

Rect const& Sprite::GetSrcRect() const {
	return src_rect;
}

void Sprite::SetSrcRect(Rect const& nsrc_rect) {
	src_rect = nsrc_rect;
}

int Sprite::GetX() const {
	return x;
}
void Sprite::SetX(int nx) {
	x = nx;
}

int Sprite::GetY() const {
	return y;
}
void Sprite::SetY(int ny) {
	y = ny;
}

int Sprite::GetOx() const {
	return ox;
}
void Sprite::SetOx(int nox) {
	ox = nox;
}

int Sprite::GetOy() const {
	return oy;
}
void Sprite::SetOy(int noy) {
	oy = noy;
}

double Sprite::GetZoomX() const {
	return zoom_x_effect;
}
void Sprite::SetZoomX(double zoom_x) {
	zoom_x_effect = zoom_x;
}

double Sprite::GetZoomY() const {
	return zoom_y_effect;
}
void Sprite::SetZoomY(double zoom_y) {
	zoom_y_effect = zoom_y;
}

double Sprite::GetAngle() const {
	return angle_effect;
}

void Sprite::SetAngle(double angle) {
	angle_effect = angle;
}

bool Sprite::GetFlipX() const {
	return flipx_effect;
}

void Sprite::SetFlipX(bool flipx) {
	flipx_effect = flipx;
}

bool Sprite::GetFlipY() const {
	return flipy_effect;
}

void Sprite::SetFlipY(bool flipy) {
	flipy_effect = flipy;
}

int Sprite::GetBushDepth() const {
	return bush_effect;
}

void Sprite::SetBushDepth(int bush_depth) {
	bush_effect = bush_depth;
}

int Sprite::GetOpacity(int which) const {
	return which > 0 ? opacity_bottom_effect : opacity_top_effect;
}

void Sprite::SetOpacity(int opacity_top, int opacity_bottom) {
	opacity_top_effect = opacity_top;
	if (opacity_bottom == -1)
		opacity_bottom = (opacity_top + 1) / 2;
	opacity_bottom_effect = opacity_bottom;
}

int Sprite::GetBlendType() const {
	return blend_type_effect;
}

void Sprite::SetBlendType(int blend_type) {
	blend_type_effect = blend_type;
}

Color Sprite::GetBlendColor() const {
	return blend_color_effect;
}

void Sprite::SetBlendColor(Color blend_color) {
	blend_color_effect = blend_color;
}

Tone Sprite::GetTone() const {
	return tone_effect;
}

void Sprite::SetTone(Tone tone) {
	tone_effect = tone;
}

int Sprite::GetWaverDepth() const {
	return waver_effect_depth;
}

void Sprite::SetWaverDepth(int depth) {
	waver_effect_depth = depth;
}

double Sprite::GetWaverPhase() const {
	return waver_effect_phase;
}

void Sprite::SetWaverPhase(double phase) {
	waver_effect_phase = phase;
}
