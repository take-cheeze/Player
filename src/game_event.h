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

#ifndef _GAME_EVENT_H_
#define _GAME_EVENT_H_

// Headers
#include <vector>
#include "game_character.h"
#include "rpg_event.h"
#include "game_interpreter.h"
#include <boost/scoped_ptr.hpp>

/**
 * Game_Event class.
 */
class Game_Event : public Game_Character {
public:
	/**
	 * Constructor.
	 */
	Game_Event(int map_id, const RPG::Event& event);

	/**
	 * Constructor.
	 * Create event from save data.
	 */
	Game_Event(int map_id, const RPG::Event& event, const RPG::SaveMapEvent& data);

	/**
	 * Implementation of abstract methods
	 */
	/** @{ */
	int GetX() const;
	void SetX(int new_x);
	int GetY() const;
	void SetY(int new_y);
	int GetMapId() const;
	void SetMapId(int new_map_id);
	int GetDirection() const;
	void SetDirection(int new_direction);
	int GetSpriteDirection() const;
	void SetSpriteDirection(int new_direction);
	bool IsFacingLocked() const;
	void SetFacingLocked(bool locked);
	int GetLayer() const;
	void SetLayer(int new_layer);
	int GetMoveSpeed() const;
	void SetMoveSpeed(int speed);
	int GetMoveFrequency() const;
	void SetMoveFrequency(int frequency);
	const RPG::MoveRoute& GetMoveRoute() const;
	void SetMoveRoute(const RPG::MoveRoute& move_route);
	int GetOriginalMoveRouteIndex() const;
	void SetOriginalMoveRouteIndex(int new_index);
	int GetMoveRouteIndex() const;
	void SetMoveRouteIndex(int new_index);
	bool IsMoveRouteOverwritten() const;
	void SetMoveRouteOverwritten(bool force);
	bool IsMoveRouteRepeated() const;
	void SetMoveRouteRepeated(bool force);
	const std::string& GetSpriteName() const;
	void SetSpriteName(const std::string& sprite_name);
	int GetSpriteIndex() const;
	void SetSpriteIndex(int index);
	Color GetFlashColor() const;
	void SetFlashColor(const Color& flash_color);
	double GetFlashLevel() const;
	void SetFlashLevel(double flash_level);
	int GetFlashTimeLeft() const;
	void SetFlashTimeLeft(int time_left);
	bool IsMessageBlocking() const;
	/** @} */

	/**
	 * Clears starting flag.
	 */
	void ClearStarting();

	/**
	 * Does refresh.
	 */
	void Refresh();

	void Setup(RPG::EventPage* new_page);
	void SetupFromSave(RPG::EventPage* new_page);

	/**
	 * Gets event ID.
	 *
	 * @return event ID.
	 */
	int GetId() const;

	/**
	 * Gets starting flag.
	 *
	 * @return starting flag.
	 */
	bool GetStarting() const;

	/**
	 * Gets trigger condition.
	 *
	 * @return trigger condition.
	 */
	int GetTrigger() const;

	/**
	 * Gets through state.
	 *
	 * @return Event has through state or has a null-page.
	 */
	bool GetThrough() const;

	/**
	 * Gets event commands list.
	 *
	 * @return event commands list.
	 */
	std::vector<RPG::EventCommand>& GetList();

	/**
	 * Event's sprite looks towards the hero but its original direction is remembered.
	 */
	void StartTalkToHero();

	/**
	 * Event returns to its original direction before talking to the hero.
	 */
	void StopTalkToHero();

	void CheckEventTriggerAuto();
	bool CheckEventTriggerTouch(int x, int y);
	void Start();
	void Update();
	bool AreConditionsMet(const RPG::EventPage& page);

	/**
	 * Activates or deactivates the event.
	 *
	 * @param dis_flag enables or disables the event.
	 */
	void SetActive(bool active);

	/**
	 * Gets if the event is active.
	 *
	 * @return if the event is active (or inactive via EraseEvent-EventCommand).
	 */
	bool GetActive() const;

	RPG::Event& GetEvent();

	const RPG::SaveMapEvent& GetSaveData();
private:
	// Not a reference on purpose.
	// Events change during map change and old are destroyed, breaking the
	// reference.
	RPG::SaveMapEvent data;

	unsigned int ID;
	bool starting;
	bool ready1, ready2;
	int trigger;
	RPG::Event event;
	RPG::EventPage* page;
	std::vector<RPG::EventCommand> list;
	EASYRPG_SHARED_PTR<Game_Interpreter> interpreter;
	bool from_save;
};

#endif
