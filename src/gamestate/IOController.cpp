/*
 * IOController.cpp:
 *  Handles dispatch of events bound to keyboard keys
 */

#include <lcommon/geometry.h>
#include <ldraw/display.h>

#include "IOController.h"

IOState::IOState() {
	clear();

//	for (int i = 0; i < SDL_NumJoysticks(); i++) {
//
//	}
    //int MaxJoysticks = SDL_NumJoysticks();
    //int ControllerIndex = 0;
    //for(int JoystickIndex=0; JoystickIndex < MaxJoysticks; ++JoystickIndex)
    //{
    //    if (!SDL_IsGameController(JoystickIndex))
    //    {
    //        continue;
    //    }
    //    if (ControllerIndex >= MAX_CONTROLLERS)
    //    {
    //        break;
    //    }
    //    ControllerHandles[ControllerIndex] = SDL_GameControllerOpen(JoystickIndex);
    //    ControllerIndex++;
    //}
}

void IOState::clear() {
	clear_for_step();
        for (std::pair<const SDL_Keycode, bool>& entry : key_down_states) {
            entry.second = false; // No longer down.
        }

	keymod = KMOD_NONE;

	mouse_leftdown = false;
	mouse_rightdown = false;
	mouse_middledown = false;
}

void IOState::clear_for_step(bool resetprev) {
        for (std::pair<const SDL_Keycode, bool>& entry : key_press_states) {
            entry.second = false; // No longer pressed.
        }

	mouse_leftclick = false;
	mouse_rightclick = false;
	mouse_middleclick = false;

	mouse_leftrelease = false;
	mouse_rightrelease = false;

	mouse_didupwheel = false;
	mouse_diddownwheel = false;

	sdl_events.clear();
}

IOController::IOController() {
    reinit_controllers();
}

void IOController::reinit_controllers() {
    controllers.clear();
	/* Initialize controllers */
	int num_joysticks = SDL_NumJoysticks();
	iostate.gamepad_states.clear();
	for(int joystick_idx=0; joystick_idx < num_joysticks; ++joystick_idx)
	{
		printf("Initial Joystick Detected ID: %i\n",  joystick_idx);
		if (!SDL_IsGameController(joystick_idx))
		{
			continue;
		}

		SDL_Joystick* joystick = SDL_JoystickOpen(joystick_idx);
		SDL_JoystickID instance_id = SDL_JoystickInstanceID(joystick);
		printf("Initial Controller Detected ID: %i, Instance ID: %i\n",  joystick_idx, instance_id);

		SDL_GameController* controller = SDL_GameControllerOpen(joystick_idx);//SDL_GameControllerFromInstanceID(instance_id);

		controllers.push_back(controller);
	}
}

/* Mouse click states */
bool IOController::mouse_right_click() {
	return iostate.mouse_rightclick;
}

bool IOController::mouse_left_down() {
	return iostate.mouse_leftdown;
}

bool IOController::mouse_right_down() {
	return iostate.mouse_rightdown;
}

bool IOController::mouse_left_release() {
	return iostate.mouse_leftrelease;
}

bool IOController::mouse_right_release() {
	return iostate.mouse_rightrelease;
}

bool IOController::mouse_upwheel() {
	return iostate.mouse_didupwheel;
}
bool IOController::mouse_downwheel() {
	return iostate.mouse_diddownwheel;
}
bool IOController::key_down_state(int keyval) {
	return iostate.key_down_states[keyval];
}

bool IOController::key_press_state(int keyval) {
	return iostate.key_press_states[keyval];
}

bool IOController::mouse_left_click() {
	return iostate.mouse_leftclick;
}

int IOController::mouse_x() {
	return iostate.mousex;
}

int IOController::mouse_y() {
	return iostate.mousey;
}

std::vector<IOGamepadState>& IOController::gamepad_states() {
    return iostate.gamepad_states;
}

int IOController::handle_event(SDL_Event* event) {
	int done = 0;
	iostate.sdl_events.push_back(*event);
	SDL_Keycode key = event->key.keysym.sym;

	switch (event->type) {
	case SDL_KEYDOWN: {
	    iostate.keymod = SDL_Keymod(event->key.keysym.mod);
		if (shift_held() && key == SDLK_ESCAPE) {
			done = 1;
		}
		iostate.key_down_states[key] = true;
		iostate.key_press_states[key] = true;
		break;
	}
	case SDL_KEYUP: {
	    iostate.keymod = SDL_Keymod(event->key.keysym.mod);
		iostate.key_down_states[key] = false;
		break;
	}
	case SDL_MOUSEBUTTONDOWN: {
	    iostate.keymod = SDL_Keymod(event->key.keysym.mod);
		if (event->button.button == SDL_BUTTON_LEFT) {
			iostate.mouse_leftdown = true;
			iostate.mouse_leftclick = true;
		} else if (event->button.button == SDL_BUTTON_RIGHT) {
			iostate.mouse_rightdown = true;
			iostate.mouse_rightclick = true;
		} else if (event->button.button == SDL_BUTTON_MIDDLE) {
			iostate.mouse_middledown = true;
			iostate.mouse_middleclick = true;
		}
		break;
	}
        case SDL_MOUSEWHEEL: {
		if (event->wheel.y < 0) {
			iostate.mouse_diddownwheel = true;
                } else {
			iostate.mouse_didupwheel = true;
                }
                break;
        }
	case SDL_MOUSEBUTTONUP: {
	    iostate.keymod = SDL_Keymod(event->key.keysym.mod);
		if (event->button.button == SDL_BUTTON_LEFT) {
			iostate.mouse_leftdown = false;
			iostate.mouse_leftrelease = true;
		} else if (event->button.button == SDL_BUTTON_RIGHT) {
			iostate.mouse_rightdown = false;
			iostate.mouse_rightrelease = true;
		} else if (event->button.button == SDL_BUTTON_MIDDLE) {
			iostate.mouse_middledown = false;
			iostate.mouse_middleclick = false;
		}
		break;
	}
	case SDL_CONTROLLERDEVICEADDED: {
                printf("IOController::handle_event CONTROLLER ADDED\n");
        reinit_controllers();
		break;
	}
	case SDL_CONTROLLERDEVICEREMOVED: {
                printf("IOController::handle_event CONTROLLER REMOVED\n");
        reinit_controllers();
		break;
	}
	case SDL_QUIT:
		done = 2;
		break;
	}

	return (done);
}

void IOController::update_iostate(bool resetprev) {
	iostate.clear_for_step(resetprev);
	SDL_GetMouseState(&iostate.mousex, &iostate.mousey);
	Size display_size = ldraw::display_size();
	Size window_size = ldraw::window_size();

    //scale the mouse position for when display_size != window_size
	iostate.mousex = display_size.w * (iostate.mousex / (float) window_size.w);
	iostate.mousey = display_size.h * (iostate.mousey / (float) window_size.h);
	
	iostate.gamepad_states.clear();

	for ( auto controller : controllers ) {
        int instance_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
        IOGamepadState state = create_gamepad_state(controller,  instance_id);

        iostate.gamepad_states.push_back(state);
    }
}

IOGamepadState IOController::create_gamepad_state(SDL_GameController* controller, int id) {
    IOGamepadState state;
    state.gamepad_id = id;
    state.gamepad_axis_left_trigger = SDL_GameControllerGetAxis(controller,  SDL_CONTROLLER_AXIS_TRIGGERLEFT) / (float)(INT16_MAX);
    state.gamepad_axis_right_trigger = SDL_GameControllerGetAxis(controller,  SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / (float)(INT16_MAX);
    state.gamepad_axis_left_x = SDL_GameControllerGetAxis(controller,  SDL_CONTROLLER_AXIS_LEFTX) / (float)(INT16_MAX);
    state.gamepad_axis_left_y = SDL_GameControllerGetAxis(controller,  SDL_CONTROLLER_AXIS_LEFTY) / (float)(INT16_MAX);
    state.gamepad_axis_right_x = SDL_GameControllerGetAxis(controller,  SDL_CONTROLLER_AXIS_RIGHTX) / (float)(INT16_MAX);
    state.gamepad_axis_right_y = SDL_GameControllerGetAxis(controller,  SDL_CONTROLLER_AXIS_RIGHTY) / (float)(INT16_MAX);
    state.gamepad_button_a = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
    state.gamepad_button_b = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B);
    state.gamepad_button_x = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
    state.gamepad_button_y = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y);
    state.gamepad_button_back = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK);
    state.gamepad_button_guide = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_GUIDE);
    state.gamepad_button_start = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START);
    state.gamepad_button_left_stick = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    state.gamepad_button_right_stick = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    state.gamepad_button_left_shoulder = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    state.gamepad_button_right_shoulder = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    state.gamepad_button_up_dpad = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
    state.gamepad_button_down_dpad = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    state.gamepad_button_left_dpad = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    state.gamepad_button_right_dpad = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    
    return state;
}


std::vector<SDL_Event>& IOController::get_events() {
	return iostate.sdl_events;
}

bool IOController::user_has_exit() const {
	for (int i = 0; i < iostate.sdl_events.size(); i++) {
		if (iostate.sdl_events[i].type == SDL_QUIT) {
			return true;
		}
	}
	return false;
}

bool IOController::user_has_requested_exit() {
	return shift_held() && key_down_state(SDLK_ESCAPE);
}

void IOController::set_key_down_state(int keyval) {
    iostate.key_down_states[keyval] = true;
}

void IOController::set_key_press_state(int keyval) {
    iostate.key_press_states[keyval] = true;
}

bool IOController::ctrl_held() {
    return key_down_state(SDLK_LCTRL) || key_down_state(SDLK_RCTRL);
//    return (iostate.keymod & KMOD_LCTRL) || (iostate.keymod & KMOD_RCTRL);
}

bool IOController::shift_held() {
    return key_down_state(SDLK_LSHIFT) || key_down_state(SDLK_RSHIFT);
//    return (iostate.keymod & KMOD_LSHIFT) || (iostate.keymod & KMOD_RSHIFT);
}

void IOController::clear() {
    iostate.clear();
}
