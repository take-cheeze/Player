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
#include <cmath>
#include "system.h"
#include "graphics.h"
#include "player.h"
#include "rect.h"
#include "util_macro.h"
#include "window.h"

Window::Window():
	Drawable(TypeWindow, 0),
	stretch(true),
	active(true),
	pause(false),
	closing(false),
	up_arrow(false),
	down_arrow(false),
	x(0),
	y(0),
	width(0),
	height(0),
	ox(0),
	oy(0),
	border_x(8),
	border_y(8),
	opacity(255),
	back_opacity(255),
	contents_opacity(255),
	cursor_frame(0),
	pause_frame(0),
	animation_frames(0),
	animation_count(0.0),
	animation_increment(0.0) {
}

void Window::SetOpenAnimation(int frames) {
	animation_frames = frames;
	animation_count = 0.0;
	if (frames > 0) {
		animation_increment = (height / 2.0) / frames;
	}
	else {
		animation_increment = 0.0;
	}
}

void Window::SetCloseAnimation(int frames) {
	if (frames > 0) {
		closing = true;
		animation_frames = frames;
		animation_count = (height / 2.0);
		animation_increment = - animation_count / frames;
	} else {
		SetVisible(false);
	}
}

void Window::Update() {
	if (active) {
		cursor_frame += 1;
		if (cursor_frame > 20) cursor_frame = 0;
		if (pause) {
			pause_frame += 1;
			if (pause_frame == 40) pause_frame = 0;
		}
	}
}

BitmapRef const& Window::GetWindowskin() const {
	return windowskin;
}
void Window::SetWindowskin(BitmapRef const& nwindowskin) {
	windowskin = nwindowskin;
}

BitmapRef Window::GetContents() const {
	return contents;
}
void Window::SetContents(BitmapRef const& ncontents) {
	contents = ncontents;
}

bool Window::GetStretch() const {
	return stretch;
}
void Window::SetStretch(bool nstretch) {
	stretch = nstretch;
}

Rect const& Window::GetCursorRect() const {
	return cursor_rect;
}
void Window::SetCursorRect(Rect const& ncursor_rect) {
	cursor_rect = ncursor_rect;
}

bool Window::GetActive() const {
	return active;
}
void Window::SetActive(bool nactive) {
	active = nactive;
}

bool Window::GetPause() const {
	return pause;
}
void Window::SetPause(bool npause) {
	pause = npause;
}

bool Window::GetUpArrow() const {
	return up_arrow;
}
void Window::SetUpArrow(bool nup_arrow) {
	up_arrow = nup_arrow;
}

bool Window::GetDownArrow() const {
	return down_arrow;
}
void Window::SetDownArrow(bool ndown_arrow) {
	down_arrow = ndown_arrow;
}

int Window::GetX() const {
	return x;
}
void Window::SetX(int nx) {
	x = nx;
}

int Window::GetY() const {
	return y;
}
void Window::SetY(int ny) {
	y = ny;
}

int Window::GetWidth() const {
	return width;
}
void Window::SetWidth(int nwidth) {
	width = nwidth;
}

int Window::GetHeight() const {
	return height;
}
void Window::SetHeight(int nheight) {
	height = nheight;
}

int Window::GetOx() const {
	return ox;
}
void Window::SetOx(int nox) {
	ox = nox;
}

int Window::GetOy() const {
	return oy;
}
void Window::SetOy(int noy) {
	oy = noy;
}

int Window::GetBorderX() const {
	return border_x;
}
void Window::SetBorderX(int x) {
	border_x = x;
}

int Window::GetBorderY() const {
	return border_y;
}
void Window::SetBorderY(int y) {
	border_y = y;
}

int Window::GetOpacity() const {
	return opacity;
}
void Window::SetOpacity(int nopacity) {
	opacity = nopacity;
}

int Window::GetBackOpacity() const {
	return back_opacity;
}
void Window::SetBackOpacity(int nback_opacity) {
	back_opacity = nback_opacity;
}

int Window::GetContentsOpacity() const {
	return contents_opacity;
}
void Window::SetContentsOpacity(int ncontents_opacity) {
	contents_opacity = ncontents_opacity;
}
