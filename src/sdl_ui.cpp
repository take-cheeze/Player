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

#include "system.h"
#ifdef USE_SDL

// Headers
#include "sdl_ui.h"
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif
	#include <windows.h>
	#include "SDL_syswm.h"
#elif GEKKO
	#include <gccore.h>
	#include <wiiuse/wpad.h>
#elif __ANDROID__
	#include <jni.h>
	#include <SDL_system.h>
#elif EMSCRIPTEN
	#include <emscripten.h>
#endif
#include "color.h"
#include "graphics.h"
#include "keys.h"
#include "output.h"
#include "player.h"
#include "bitmap.h"
#include "audio.h"

#ifdef HAVE_SDL_MIXER
#  include "sdl_audio.h"
#endif
#ifdef HAVE_OPENAL
#  include "platform/al_audio.h"
#endif

AudioInterface& SdlUi::GetAudio() {
	return *audio_;
}

static int FilterUntilFocus(const SDL_Event* evnt);

#if defined(USE_KEYBOARD) && defined(SUPPORT_KEYBOARD)
	static Input::Keys::InputKey SdlKey2InputKey(SDL_Keycode sdlkey);
#endif

#if defined(USE_JOYSTICK) && defined(SUPPORT_JOYSTICK)
	static Input::Keys::InputKey SdlJKey2InputKey(int button_index);
#endif

#ifdef GEKKO
	static void GekkoResetCallback();
#endif

SdlUi::SdlUi(long width, long height, const std::string& title, bool fs_flag) :
	BaseUi(),
	zoom_available(true),
	toggle_fs_available(false),
	mode_changing(false) {

#ifdef GEKKO
	WPAD_Init();

	SYS_SetResetCallback(GekkoResetCallback);
#endif

	uint32_t flags = SDL_INIT_VIDEO;
	
#ifndef EMSCRIPTEN
	flags |= SDL_INIT_TIMER;
#endif

#if (!defined(NDEBUG) || defined(_WIN32))
	flags |= SDL_INIT_NOPARACHUTE;
#endif

	// Set some SDL env. variables before starting
	// These are platform dependent, so every port
	// needs to set them manually

	// Set window position to the middle of the
	// screen
#ifndef GEKKO
	putenv(const_cast<char *>("SDL_VIDEO_WINDOW_POS=center"));
#endif
#if defined(PSP)
	putenv(const_cast<char *>("SDL_ASPECT_RATIO=4:3"));
#endif

	if (SDL_Init(flags) < 0) {
		Output::Error("Couldn't initialize SDL.\n%s\n", SDL_GetError());
	}

	sdl_window = NULL;

	BeginDisplayModeChange();
		if (!RequestVideoMode(width, height, fs_flag)) {
			Output::Error("No suitable video resolution found. Aborting.");
		}
	EndDisplayModeChange();

	SetTitle(title);

#if (defined(USE_JOYSTICK) && defined(SUPPORT_JOYSTICK)) || (defined(USE_JOYSTICK_AXIS) && defined(SUPPORT_JOYSTICK_AXIS)) || (defined(USE_JOYSTICK_HAT) && defined(SUPPORT_JOYSTICK_HAT))
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
		Output::Warning("Couldn't initialize joystick.\n%s", SDL_GetError());
	}

	SDL_JoystickEventState(1);
	SDL_JoystickOpen(0);
#endif

#if defined(USE_MOUSE) && defined(SUPPORT_MOUSE)
	ShowCursor(true);
#else
	ShowCursor(false);
#endif

#if defined(HAVE_SDL_MIXER)
	audio_.reset(new SdlAudio());
#elif defined(HAVE_OPENAL)
	audio_.reset(new ALAudio());
#else
	audio_.reset(new EmptyAudio());
#endif
}

SdlUi::~SdlUi() {
#if defined(GPH)
	chdir("/usr/gp2x");
	execl("./gp2xmenu", "./gp2xmenu", NULL);
#else
	SDL_Quit();
#endif
}

uint32_t SdlUi::GetTicks() const {
	return SDL_GetTicks();
}

void SdlUi::Sleep(uint32_t time) {
#ifndef EMSCRIPTEN
	SDL_Delay(time);
#endif
}

bool SdlUi::RequestVideoMode(int width, int height, bool fullscreen) {
	// SDL2 documentation says that resolution dependent code should not be used
	// anymore. The library takes care of it now.
	current_display_mode.width = width;
	current_display_mode.height = height;
	current_display_mode.bpp = 32;
	if (fullscreen) {
		current_display_mode.flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	toggle_fs_available = true;
	
	current_display_mode.zoom = true;
#ifdef SUPPORT_ZOOM
	zoom_available = true;
#else
	zoom_available = false;
#endif

	return true;
}

void SdlUi::BeginDisplayModeChange() {
	last_display_mode = current_display_mode;
	current_display_mode.effective = false;
	mode_changing = true;
}

void SdlUi::EndDisplayModeChange() {
	// Check if the new display mode is different from last one
	if (mode_changing && (
		current_display_mode.flags != last_display_mode.flags ||
		current_display_mode.zoom != last_display_mode.zoom ||
		current_display_mode.width != last_display_mode.width ||
		current_display_mode.height != last_display_mode.height)) {

			if (!RefreshDisplayMode()) {
				// Mode change failed, check if last one was effective
				if (last_display_mode.effective) {
					current_display_mode = last_display_mode;

					// Try a rollback to last mode
					if (!RefreshDisplayMode()) {
						Output::Error("Couldn't rollback to last display mode.\n%s", SDL_GetError());
					}
				} else {
					Output::Error("Couldn't set display mode.\n%s", SDL_GetError());
				}
			}

			current_display_mode.effective = true;

			mode_changing = false;
	}
}

bool SdlUi::RefreshDisplayMode() {
	uint32_t flags = current_display_mode.flags;
	int display_width = current_display_mode.width;
	int display_height = current_display_mode.height;

	// Display on screen fps while fullscreen or no window available
	bool is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP;
	Graphics::fps_on_screen = is_fullscreen || !toggle_fs_available;

	if (zoom_available && current_display_mode.zoom) {
		display_width *= 2;
		display_height *= 2;
	}

	if (!sdl_window) {
		#ifdef __ANDROID__
		// Workaround SDL bug: https://bugzilla.libsdl.org/show_bug.cgi?id=2291
		// Set back buffer format to 565
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		#else
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		#endif
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);

		// Create our window
		sdl_window = SDL_CreateWindow("EasyRPG Player",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			display_width, display_height,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | flags);

		if (!sdl_window)
			return false;

		sdl_gl_context = SDL_GL_CreateContext(sdl_window);
		if (not sdl_gl_context) return false;

		SetAppIcon();
	} else {
		// Browser handles fast resizing for emscripten
#ifndef EMSCRIPTEN
		if (is_fullscreen) {
			SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		} else {
			SDL_SetWindowFullscreen(sdl_window, 0);
			if ((last_display_mode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
					== SDL_WINDOW_FULLSCREEN_DESKTOP) {
				// Restore to pre-fullscreen size
				SDL_SetWindowSize(sdl_window, 0, 0);
			}
			else {
				SDL_SetWindowSize(sdl_window, display_width, display_height);
			}
			SetAppIcon();
		}
#endif
	}

	return true;
}

#ifdef SUPPORT_FULL_SCALING
void SdlUi::Resize(long width, long height) {
	if (mode_changing) {
		current_display_mode.width = width;
		current_display_mode.height = height;
	}
}
#else
void SdlUi::Resize(long /*width*/, long /*height*/) {
}
#endif

void SdlUi::ToggleFullscreen() {
	if (toggle_fs_available && mode_changing) {
		if ((current_display_mode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP)
			current_display_mode.flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
		else
			current_display_mode.flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
}

void SdlUi::ToggleZoom() {
	if (zoom_available && mode_changing) {
		current_display_mode.zoom = !current_display_mode.zoom;
	}
}

void SdlUi::ProcessEvents() {
	SDL_Event evnt;

	// Poll SDL events and process them
	while (SDL_PollEvent(&evnt)) {
		ProcessEvent(evnt);

		if (Player::exit_flag)
			break;
	}
}

void SdlUi::UpdateDisplay() {
	SDL_GL_SwapWindow(sdl_window);
}

void SdlUi::SetTitle(const std::string &title) {
	SDL_SetWindowTitle(sdl_window, title.c_str());
}

bool SdlUi::ShowCursor(bool flag) {
	bool temp_flag = cursor_visible;
	cursor_visible = flag;
	SDL_ShowCursor(flag ? SDL_ENABLE : SDL_DISABLE);
	return temp_flag;
}

void SdlUi::ProcessEvent(SDL_Event &evnt) {
	switch (evnt.type) {
		case SDL_WINDOWEVENT:
			ProcessActiveEvent(evnt);
			return;

		case SDL_QUIT:
			Player::exit_flag = true;
			return;

		case SDL_KEYDOWN:
			ProcessKeyDownEvent(evnt);
			return;

		case SDL_KEYUP:
			ProcessKeyUpEvent(evnt);
			return;

		case SDL_MOUSEMOTION:
			ProcessMouseMotionEvent(evnt);
			return;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			ProcessMouseButtonEvent(evnt);
			return;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			ProcessJoystickButtonEvent(evnt);
			return;

		case SDL_JOYHATMOTION:
			ProcessJoystickHatEvent(evnt);
			return;

		case SDL_JOYAXISMOTION:
			ProcessJoystickAxisEvent(evnt);
			return;
	}
}

void SdlUi::ProcessActiveEvent(SDL_Event &evnt) {
#ifdef PAUSE_GAME_WHEN_FOCUS_LOST
	int state;
	state = evnt.window.event;

	if (state == SDL_WINDOWEVENT_FOCUS_LOST) {
#ifdef _WIN32
		// Prevent the player from hanging when it receives a
		// focus changed event but actually has focus.
		// This happens when a MsgBox appears.
		if (GetActiveWindow() != NULL) {
			return;
		}
#endif

		Player::Pause();

		bool last = ShowCursor(true);

		// Filter SDL events with FilterUntilFocus until focus is
		// regained

#ifndef EMSCRIPTEN
		SDL_Event wait_event;

		while (SDL_WaitEvent(&wait_event)) {
			if (FilterUntilFocus(&wait_event)) {
				break;
			}
		}
#endif
		
		ShowCursor(last);

		Player::Resume();
		ResetKeys();

		return;
	}
#endif
#if defined(USE_MOUSE) && defined(SUPPORT_MOUSE)
	if (state == SDL_WINDOWEVENT_ENTER) {
		mouse_focus = true;
	} else if (state == SDL_WINDOWEVENT_LEAVE) {
		mouse_focus = false;
	}
#endif
}

void SdlUi::ProcessKeyDownEvent(SDL_Event &evnt) {
#if defined(USE_KEYBOARD) && defined(SUPPORT_KEYBOARD)
	switch (evnt.key.keysym.sym) {
	case SDLK_F4:
		// Close program on LeftAlt+F4
		if (evnt.key.keysym.mod & KMOD_LALT) {
			Player::exit_flag = true;
			return;
		}

		// Toggle fullscreen on F4 and no alt is pressed
		if (!(evnt.key.keysym.mod & KMOD_RALT) && !(evnt.key.keysym.mod & KMOD_LALT)) {
			BeginDisplayModeChange();
				ToggleFullscreen();
			EndDisplayModeChange();
		}
		return;

	case SDLK_F5:
		// Toggle zoom on F5
		BeginDisplayModeChange();
			ToggleZoom();
		EndDisplayModeChange();
		return;

	case SDLK_F12:
		// Reset the game engine on F12
		Player::reset_flag = true;
		return;

	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		// Toggle fullscreen on Alt+Enter
		if (evnt.key.keysym.mod & KMOD_LALT || (evnt.key.keysym.mod & KMOD_RALT)) {
			BeginDisplayModeChange();
				ToggleFullscreen();
			EndDisplayModeChange();
			return;
		}

		// Continue if return/enter not handled by fullscreen hotkey
	default:
		// Update key state
		keys[SdlKey2InputKey(evnt.key.keysym.scancode)] = true;
		return;
	}
#endif
}

void SdlUi::ProcessKeyUpEvent(SDL_Event &evnt) {
#if defined(USE_KEYBOARD) && defined(SUPPORT_KEYBOARD)
	keys[SdlKey2InputKey(evnt.key.keysym.scancode)] = false;
#endif
}

void SdlUi::ProcessMouseMotionEvent(SDL_Event& /* evnt */) {
#if defined(USE_MOUSE) && defined(SUPPORT_MOUSE)
	mouse_focus = true;
	mouse_x = evnt.motion.x;
	mouse_y = evnt.motion.y;
#endif
}

void SdlUi::ProcessMouseButtonEvent(SDL_Event& /* evnt */) {
#if defined(USE_MOUSE) && defined(SUPPORT_MOUSE)
	switch (evnt.button.button) {
	case SDL_BUTTON_LEFT:
		keys[Input::Keys::MOUSE_LEFT] = evnt.button.state == SDL_PRESSED;
		break;
	case SDL_BUTTON_MIDDLE:
		keys[Input::Keys::MOUSE_MIDDLE] = evnt.button.state == SDL_PRESSED;
		break;
	case SDL_BUTTON_RIGHT:
		keys[Input::Keys::MOUSE_RIGHT] = evnt.button.state == SDL_PRESSED;
		break;
	}
#endif
}

void SdlUi::ProcessJoystickButtonEvent(SDL_Event &evnt) {
#if defined(USE_JOYSTICK) && defined(SUPPORT_JOYSTICK)
	keys[SdlJKey2InputKey(evnt.jbutton.button)] = evnt.jbutton.state == SDL_PRESSED;
#endif
}

void SdlUi::ProcessJoystickHatEvent(SDL_Event &evnt) {
#if defined(USE_JOYSTICK_HAT)  && defined(SUPPORT_JOYSTICK_HAT)
	// Set all states to false
	keys[Input::Keys::JOY_HAT_LOWER_LEFT] = false;
	keys[Input::Keys::JOY_HAT_DOWN] = false;
	keys[Input::Keys::JOY_HAT_LOWER_RIGHT] = false;
	keys[Input::Keys::JOY_HAT_LEFT] = false;
	keys[Input::Keys::JOY_HAT_RIGHT] = false;
	keys[Input::Keys::JOY_HAT_UPPER_LEFT] = false;
	keys[Input::Keys::JOY_HAT_UP] = false;
	keys[Input::Keys::JOY_HAT_UPPER_RIGHT] = false;

	// Check hat states
	if ((evnt.jhat.value & SDL_HAT_RIGHTUP) == SDL_HAT_RIGHTUP)
		keys[Input::Keys::JOY_HAT_UPPER_RIGHT] = true;

	else if ((evnt.jhat.value & SDL_HAT_RIGHTDOWN)  == SDL_HAT_RIGHTDOWN)
		keys[Input::Keys::JOY_HAT_LOWER_RIGHT] = true;

	else if ((evnt.jhat.value & SDL_HAT_LEFTUP)  == SDL_HAT_LEFTUP)
		keys[Input::Keys::JOY_HAT_UPPER_LEFT] = true;

	else if ((evnt.jhat.value & SDL_HAT_LEFTDOWN)  == SDL_HAT_LEFTDOWN)
		keys[Input::Keys::JOY_HAT_LOWER_LEFT] = true;

	else if (evnt.jhat.value & SDL_HAT_UP)
		keys[Input::Keys::JOY_HAT_UP] = true;

	else if (evnt.jhat.value & SDL_HAT_RIGHT)
		keys[Input::Keys::JOY_HAT_RIGHT] = true;

	else if (evnt.jhat.value & SDL_HAT_DOWN)
		keys[Input::Keys::JOY_HAT_DOWN] = true;

	else if (evnt.jhat.value & SDL_HAT_LEFT)
		keys[Input::Keys::JOY_HAT_LEFT] = true;
#endif
}

void SdlUi::ProcessJoystickAxisEvent(SDL_Event &evnt) {
#if defined(USE_JOYSTICK_AXIS)  && defined(SUPPORT_JOYSTICK_AXIS)
	// Horizontal axis
	if (evnt.jaxis.axis == 0) {
		if (evnt.jaxis.value < -JOYSTICK_AXIS_SENSIBILITY) {
			keys[Input::Keys::JOY_AXIS_X_LEFT] = true;
			keys[Input::Keys::JOY_AXIS_X_RIGHT] = false;
		} else if (evnt.jaxis.value > JOYSTICK_AXIS_SENSIBILITY) {
			keys[Input::Keys::JOY_AXIS_X_LEFT] = false;
			keys[Input::Keys::JOY_AXIS_X_RIGHT] = true;
		} else {
			keys[Input::Keys::JOY_AXIS_X_LEFT] = false;
			keys[Input::Keys::JOY_AXIS_X_RIGHT] = false;
		}

	// Vertical Axis
	} else if (evnt.jaxis.axis == 1) {
		if (evnt.jaxis.value < -JOYSTICK_AXIS_SENSIBILITY) {
			keys[Input::Keys::JOY_AXIS_Y_UP] = true;
			keys[Input::Keys::JOY_AXIS_Y_DOWN] = false;
		} else if (evnt.jaxis.value > JOYSTICK_AXIS_SENSIBILITY) {
			keys[Input::Keys::JOY_AXIS_Y_UP] = false;
			keys[Input::Keys::JOY_AXIS_Y_DOWN] = true;
		} else {
			keys[Input::Keys::JOY_AXIS_Y_UP] = false;
			keys[Input::Keys::JOY_AXIS_Y_DOWN] = false;
		}
	}
#endif
}

void SdlUi::SetAppIcon() {
#ifdef _WIN32
	static bool icon_set = false;
	if (icon_set)
		return;

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)

	SDL_bool success = SDL_GetWindowWMInfo(sdl_window, &wminfo);

	if (success < 0)
		Output::Error("Wrong SDL version");

	HINSTANCE handle = GetModuleHandle(NULL);
	HICON icon = LoadIcon(handle, MAKEINTRESOURCE(23456));

	if (icon == NULL)
		Output::Error("Couldn't load icon.");

	HWND window;
	window = wminfo.info.win.window;
	SetClassLongPtr(window, GCLP_HICON, (LONG_PTR) icon);

	icon_set = true;
#endif
}

void SdlUi::ResetKeys() {
	for (size_t i = 0; i < keys.size(); i++) {
		keys[i] = false;
	}
}

bool SdlUi::IsFullscreen() {
	return (current_display_mode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP;
}

#if defined(USE_KEYBOARD) && defined(SUPPORT_KEYBOARD)
Input::Keys::InputKey SdlKey2InputKey(SDL_Keycode sdlkey) {
	switch (sdlkey) {
		case SDL_SCANCODE_BACKSPACE		: return Input::Keys::BACKSPACE;
		case SDL_SCANCODE_TAB			: return Input::Keys::TAB;
		case SDL_SCANCODE_CLEAR			: return Input::Keys::CLEAR;
		case SDL_SCANCODE_RETURN		: return Input::Keys::RETURN;
		case SDL_SCANCODE_PAUSE			: return Input::Keys::PAUSE;
		case SDL_SCANCODE_ESCAPE		: return Input::Keys::ESCAPE;
		case SDL_SCANCODE_SPACE			: return Input::Keys::SPACE;
		case SDL_SCANCODE_PAGEUP		: return Input::Keys::PGUP;
		case SDL_SCANCODE_PAGEDOWN		: return Input::Keys::PGDN;
		case SDL_SCANCODE_END			: return Input::Keys::ENDS;
		case SDL_SCANCODE_HOME			: return Input::Keys::HOME;
		case SDL_SCANCODE_LEFT			: return Input::Keys::LEFT;
		case SDL_SCANCODE_UP			: return Input::Keys::UP;
		case SDL_SCANCODE_RIGHT			: return Input::Keys::RIGHT;
		case SDL_SCANCODE_DOWN			: return Input::Keys::DOWN;
		case SDL_SCANCODE_PRINTSCREEN	: return Input::Keys::SNAPSHOT;
		case SDL_SCANCODE_INSERT		: return Input::Keys::INSERT;
		case SDL_SCANCODE_DELETE		: return Input::Keys::DEL;
		case SDL_SCANCODE_LSHIFT		: return Input::Keys::LSHIFT;
		case SDL_SCANCODE_RSHIFT		: return Input::Keys::RSHIFT;
		case SDL_SCANCODE_LCTRL			: return Input::Keys::LCTRL;
		case SDL_SCANCODE_RCTRL			: return Input::Keys::RCTRL;
		case SDL_SCANCODE_LALT			: return Input::Keys::LALT;
		case SDL_SCANCODE_RALT			: return Input::Keys::RALT;
		case SDL_SCANCODE_0				: return Input::Keys::N0;
		case SDL_SCANCODE_1				: return Input::Keys::N1;
		case SDL_SCANCODE_2				: return Input::Keys::N2;
		case SDL_SCANCODE_3				: return Input::Keys::N3;
		case SDL_SCANCODE_4				: return Input::Keys::N4;
		case SDL_SCANCODE_5				: return Input::Keys::N5;
		case SDL_SCANCODE_6				: return Input::Keys::N6;
		case SDL_SCANCODE_7				: return Input::Keys::N7;
		case SDL_SCANCODE_8				: return Input::Keys::N8;
		case SDL_SCANCODE_9				: return Input::Keys::N9;
		case SDL_SCANCODE_A				: return Input::Keys::A;
		case SDL_SCANCODE_B				: return Input::Keys::B;
		case SDL_SCANCODE_C				: return Input::Keys::C;
		case SDL_SCANCODE_D				: return Input::Keys::D;
		case SDL_SCANCODE_E				: return Input::Keys::E;
		case SDL_SCANCODE_F				: return Input::Keys::F;
		case SDL_SCANCODE_G				: return Input::Keys::G;
		case SDL_SCANCODE_H				: return Input::Keys::H;
		case SDL_SCANCODE_I				: return Input::Keys::I;
		case SDL_SCANCODE_J				: return Input::Keys::J;
		case SDL_SCANCODE_K				: return Input::Keys::K;
		case SDL_SCANCODE_L				: return Input::Keys::L;
		case SDL_SCANCODE_M				: return Input::Keys::M;
		case SDL_SCANCODE_N				: return Input::Keys::N;
		case SDL_SCANCODE_O				: return Input::Keys::O;
		case SDL_SCANCODE_P				: return Input::Keys::P;
		case SDL_SCANCODE_Q				: return Input::Keys::Q;
		case SDL_SCANCODE_R				: return Input::Keys::R;
		case SDL_SCANCODE_S				: return Input::Keys::S;
		case SDL_SCANCODE_T				: return Input::Keys::T;
		case SDL_SCANCODE_U				: return Input::Keys::U;
		case SDL_SCANCODE_V				: return Input::Keys::V;
		case SDL_SCANCODE_W				: return Input::Keys::W;
		case SDL_SCANCODE_X				: return Input::Keys::X;
		case SDL_SCANCODE_Y				: return Input::Keys::Y;
		case SDL_SCANCODE_Z				: return Input::Keys::Z;
		case SDL_SCANCODE_MENU			: return Input::Keys::MENU;
		case SDL_SCANCODE_KP_0			: return Input::Keys::KP0;
		case SDL_SCANCODE_KP_1			: return Input::Keys::KP1;
		case SDL_SCANCODE_KP_2			: return Input::Keys::KP2;
		case SDL_SCANCODE_KP_3			: return Input::Keys::KP3;
		case SDL_SCANCODE_KP_4			: return Input::Keys::KP4;
		case SDL_SCANCODE_KP_5			: return Input::Keys::KP5;
		case SDL_SCANCODE_KP_6			: return Input::Keys::KP6;
		case SDL_SCANCODE_KP_7			: return Input::Keys::KP7;
		case SDL_SCANCODE_KP_8			: return Input::Keys::KP8;
		case SDL_SCANCODE_KP_9			: return Input::Keys::KP9;
		case SDL_SCANCODE_KP_MULTIPLY	: return Input::Keys::MULTIPLY;
		case SDL_SCANCODE_KP_PLUS		: return Input::Keys::ADD;
		case SDL_SCANCODE_KP_ENTER		: return Input::Keys::RETURN;
		case SDL_SCANCODE_KP_MINUS		: return Input::Keys::SUBTRACT;
		case SDL_SCANCODE_KP_PERIOD		: return Input::Keys::PERIOD;
		case SDL_SCANCODE_KP_DIVIDE		: return Input::Keys::DIVIDE;
		case SDL_SCANCODE_F1			: return Input::Keys::F1;
		case SDL_SCANCODE_F2			: return Input::Keys::F2;
		case SDL_SCANCODE_F3			: return Input::Keys::F3;
		case SDL_SCANCODE_F4			: return Input::Keys::F4;
		case SDL_SCANCODE_F5			: return Input::Keys::F5;
		case SDL_SCANCODE_F6			: return Input::Keys::F6;
		case SDL_SCANCODE_F7			: return Input::Keys::F7;
		case SDL_SCANCODE_F8			: return Input::Keys::F8;
		case SDL_SCANCODE_F9			: return Input::Keys::F9;
		case SDL_SCANCODE_F10			: return Input::Keys::F10;
		case SDL_SCANCODE_F11			: return Input::Keys::F11;
		case SDL_SCANCODE_F12			: return Input::Keys::F12;
		case SDL_SCANCODE_CAPSLOCK		: return Input::Keys::CAPS_LOCK;
		case SDL_SCANCODE_NUMLOCKCLEAR	: return Input::Keys::NUM_LOCK;
		case SDL_SCANCODE_SCROLLLOCK	: return Input::Keys::SCROLL_LOCK;
		case SDL_SCANCODE_AC_BACK		: return Input::Keys::AC_BACK;
		case SDL_SCANCODE_SELECT		: return Input::Keys::SELECT;

		default					: return Input::Keys::NONE;
	}
}
#endif

#if defined(USE_JOYSTICK) && defined(SUPPORT_JOYSTICK)
Input::Keys::InputKey SdlJKey2InputKey(int button_index) {
	switch (button_index) {
		case 0	: return Input::Keys::JOY_0;
		case 1	: return Input::Keys::JOY_1;
		case 2	: return Input::Keys::JOY_2;
		case 3	: return Input::Keys::JOY_3;
		case 4	: return Input::Keys::JOY_4;
		case 5	: return Input::Keys::JOY_5;
		case 6	: return Input::Keys::JOY_6;
		case 7	: return Input::Keys::JOY_7;
		case 8	: return Input::Keys::JOY_8;
		case 9	: return Input::Keys::JOY_9;
		case 10	: return Input::Keys::JOY_10;
		case 11	: return Input::Keys::JOY_11;
		case 12	: return Input::Keys::JOY_12;
		case 13	: return Input::Keys::JOY_13;
		case 14	: return Input::Keys::JOY_14;
		case 15	: return Input::Keys::JOY_15;
		case 16	: return Input::Keys::JOY_16;
		case 17	: return Input::Keys::JOY_17;
		case 18	: return Input::Keys::JOY_18;
		case 19	: return Input::Keys::JOY_19;
		case 20	: return Input::Keys::JOY_20;
		case 21	: return Input::Keys::JOY_21;
		case 22	: return Input::Keys::JOY_22;
		case 23	: return Input::Keys::JOY_23;
		case 24	: return Input::Keys::JOY_24;
		case 25	: return Input::Keys::JOY_25;
		case 26	: return Input::Keys::JOY_23;
		case 27	: return Input::Keys::JOY_27;
		case 28	: return Input::Keys::JOY_28;
		case 29	: return Input::Keys::JOY_29;
		case 30	: return Input::Keys::JOY_30;
		case 31	: return Input::Keys::JOY_31;
		default : return Input::Keys::NONE;
	}
}
#endif

int FilterUntilFocus(const SDL_Event* evnt) {
	// Prevent throwing events away received after focus gained but filter
	// not detached.
	
	bool filtering_done = false;

	switch (evnt->type) {
	case SDL_QUIT:
		Player::exit_flag = true;
		return 1;

	case SDL_WINDOWEVENT:
		filtering_done = evnt->window.event == SDL_WINDOWEVENT_FOCUS_GAINED;

		return !!filtering_done;

	default:
		return 0;
	}
}

#ifdef GEKKO
void GekkoResetCallback() {
	Player::reset_flag = true;
}
#endif

#endif

void SdlUi::MakeGLContextCurrent() {
	SDL_GL_MakeCurrent(sdl_window, sdl_gl_context);
}
