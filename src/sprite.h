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

#ifndef _SPRITE_H_
#define _SPRITE_H_

// Headers
#include "color.h"
#include "drawable.h"
#include "rect.h"
#include "tone.h"

/**
 * Sprite class.
 */
class Sprite : public Drawable {
public:
	Sprite();

	void Draw();

	void Flash(int duration);
	void Flash(Color color, int duration);
	void Update();

	int GetWidth() const;
	int GetHeight() const;

	BitmapRef const& GetBitmap() const;
	void SetBitmap(BitmapRef const& bitmap);
	Rect const& GetSrcRect() const;
	void SetSrcRect(Rect const& src_rect);
	int GetX() const;
	void SetX(int x);
	int GetY() const;
	void SetY(int y);
	int GetOx() const;
	void SetOx(int ox);
	int GetOy() const;
	void SetOy(int oy);
	double GetZoomX() const;
	void SetZoomX(double zoom_x);
	double GetZoomY() const;
	void SetZoomY(double zoom_y);
	double GetAngle() const;
	void SetAngle(double angle);
	bool GetFlipX() const;
	void SetFlipX(bool flipx);
	bool GetFlipY() const;
	void SetFlipY(bool flipy);
	int GetBushDepth() const;
	void SetBushDepth(int bush_depth);
	int GetOpacity(int which = 0) const;
	void SetOpacity(int top_opacity, int bottom_opacity = -1);
	int GetBlendType() const;
	void SetBlendType(int blend_type);
	Color GetBlendColor() const;
	void SetBlendColor(Color color);
	Tone GetTone() const;
	void SetTone(Tone tone);
	int GetWaverDepth() const;
	void SetWaverDepth(int depth);
	double GetWaverPhase() const;
	void SetWaverPhase(double phase);

private:
	BitmapRef bitmap;

	Rect src_rect;
	int x;
	int y;
	int ox;
	int oy;

	Color flash_color;
	int flash_duration;
	int flash_frame;

	int opacity_top_effect;
	int opacity_bottom_effect;
	int bush_effect;
	Tone tone_effect;
	bool flipx_effect;
	bool flipy_effect;
	double zoom_x_effect;
	double zoom_y_effect;
	double angle_effect;
	int blend_type_effect;
	Color blend_color_effect;
	int waver_effect_depth;
	double waver_effect_phase;
	Color flash_effect;

	void SetFlashEffect(const Color &color);
};

#endif
