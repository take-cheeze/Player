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
#include "window_actorinfo.h"
#include "game_actors.h"
#include "game_party.h"
#include "bitmap.h"
#include "text.h"

Window_ActorInfo::Window_ActorInfo(int ix, int iy, int iwidth, int iheight, int actor_id) :
	Window_Base(ix, iy, iwidth, iheight),
	actor_id(actor_id) {

	SetContents(Bitmap::Create(width - 16, height - 16));

	Refresh();
}

void Window_ActorInfo::Refresh() {
	contents->Clear();

	DrawInfo();
}

void Window_ActorInfo::DrawInfo() {

	// Draw Row formation.
	std::string battle_row = Game_Actors::GetActor(actor_id)->GetBattleRow() == 1 ? "Back" : "Front";
	Text::Draw(*contents, Rect(0, 5, contents->GetWidth(), 12),
		   Text::ColorDefault, battle_row, Text::AlignRight);

	// Draw Face
	DrawActorFace(Game_Actors::GetActor(actor_id), 0, 0);

	// Draw Name
	Text::Draw(*contents, 3, 50, 1, "Name");
	DrawActorName(Game_Actors::GetActor(actor_id), 36, 65);

	// Draw Profession
	Text::Draw(*contents, 3, 80, 1, "Profession");
	DrawActorClass(Game_Actors::GetActor(actor_id), 36, 95);

	// Draw Rank
	Text::Draw(*contents, 3, 110, 1, "Title");
	DrawActorTitle(Game_Actors::GetActor(actor_id), 36, 125);

	// Draw Status
	Text::Draw(*contents, 3, 140, 1, "Status");
	DrawActorState(Game_Actors::GetActor(actor_id), 36, 155);

	//Draw Level
	Text::Draw(*contents, 3, 170, 1, Data::terms.lvl_short);
	std::stringstream ss;
	ss << Game_Actors::GetActor(actor_id)->GetLevel();
	Text::Draw(*contents, Rect(0, 170, 79, 12), Text::ColorDefault, ss.str(), Text::AlignRight);

}
