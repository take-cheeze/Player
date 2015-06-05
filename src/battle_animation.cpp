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

#include "bitmap.h"
#include "async_handler.h"
#include "rpg_animation.h"
#include "output.h"
#include "graphics.h"
#include "filefinder.h"
#include "cache.h"
#include "battle_animation.h"

#include <boost/bind.hpp>

BattleAnimation::BattleAnimation(int x, int y, const RPG::Animation* animation) :
	Drawable(TypeDefault, 1500),
	x(x), y(y), animation(animation), frame(0)
{
	const std::string& name = animation->animation_name;
	BitmapRef graphic;

	large = false;

	if (name.empty()) return;

	// Emscripten handled special here because of the FileFinder checks
	// Filefinder is not reliable for Emscripten because the files must be
	// downloaded first.
	// And we can't rely on "success" state of FileRequest because it's always
	// true on desktop.
#ifdef EMSCRIPTEN
	FileRequestAsync* request = AsyncHandler::RequestFile("Battle", animation->animation_name);
	request->Bind(&BattleAnimation::OnBattleSpriteReady, this);
	request->Start();
#else
	if (!FileFinder::FindImage("Battle", name).empty()) {
		FileRequestAsync* request = AsyncHandler::RequestFile("Battle", animation->animation_name);
		request->Bind(boost::bind(&BattleAnimation::OnBattleSpriteReady, this, _1));
		request->Start();
	}
	else if (!FileFinder::FindImage("Battle2", name).empty()) {
		FileRequestAsync* request = AsyncHandler::RequestFile("Battle2", animation->animation_name);
		request->Bind(boost::bind(&BattleAnimation::OnBattle2SpriteReady, this, _1));
		request->Start();
	}
	else {
		Output::Warning("Couldn't find animation: %s", name.c_str());
	}
#endif
}

void BattleAnimation::Update() {
	static bool update = true;
	if (update) {
		frame++;
	}
	update = !update;
}

void BattleAnimation::SetFrame(int _frame) {
	frame = _frame;
}

int BattleAnimation::GetFrame() const {
	return frame;
}

int BattleAnimation::GetFrames() const {
	return animation->frames.size();
}

bool BattleAnimation::IsDone() const {
	return GetFrame() >= GetFrames();
}

void BattleAnimation::OnBattleSpriteReady(FileRequestResult* result) {
	if (result->success) {
		screen = Cache::Battle(result->file);
	}
	else {
		// Try battle2
		FileRequestAsync* request = AsyncHandler::RequestFile("Battle2", result->file);
		request->Bind(boost::bind(&BattleAnimation::OnBattle2SpriteReady, this, _1));
		request->Start();
	}
}

void BattleAnimation::OnBattle2SpriteReady(FileRequestResult* result) {
	if (result->success) {
		screen = Cache::Battle2(result->file);
	}
	else {
		Output::Warning("Couldn't find animation: %s", result->file.c_str());
	}
}
