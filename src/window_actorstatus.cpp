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
#include <iomanip>
#include <sstream>
#include "window_actorstatus.h"
#include "game_actors.h"
#include "game_party.h"
#include "bitmap.h"
#include "text.h"

Window_ActorStatus::Window_ActorStatus(int ix, int iy, int iwidth, int iheight, int actor_id) :
	Window_Base(ix, iy, iwidth, iheight),
	actor_id(actor_id) {

	SetContents(Bitmap::Create(width - 16, height - 16));

	Refresh();
}

void Window_ActorStatus::Refresh() {
	contents->Clear();

	DrawStatus();
}

void Window_ActorStatus::DrawStatus(){

	Game_Actor* actor = Game_Actors::GetActor(actor_id);

	// Draw Hp
	Text::Draw(*contents, 1, 3, 1, Data::terms.hp_short);
	DrawMinMax(100,3,actor->GetHp(), actor->GetMaxHp());

	// Draw Sp
	Text::Draw(*contents, 1, 18, 1, Data::terms.sp_short);
	DrawMinMax(100,18,actor->GetSp(), actor->GetMaxSp());

	// Draw Exp
	Text::Draw(*contents, 1, 33, 1, Data::terms.exp_short);
	DrawMinMax(100,33, -1, -1);
}

void Window_ActorStatus::DrawMinMax(int cx, int cy, int min, int max){
	std::stringstream ss;
	if (max >= 0)
		ss << min;
	else
		ss << Game_Actors::GetActor(actor_id)->GetExpString();
	Text::Draw(*contents, Rect(0, cy, cx, 12), Text::ColorDefault, ss.str(), Text::AlignRight);
	Text::Draw(*contents, cx, cy, Text::ColorDefault, "/");
	ss.str("");
	if (max >= 0)
		ss << max;
	else
		ss << Game_Actors::GetActor(actor_id)->GetNextExpString();
	Text::Draw(*contents, Rect(0, cy, cx + 48, 12), Text::ColorDefault, ss.str(), Text::AlignRight);
}
