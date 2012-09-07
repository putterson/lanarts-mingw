/*
 * TextField.h:
 *  Interface component for entering text.
 *  This includes stuff like entering name before play, etc.
 */

#ifndef TEXTFIELD_H_
#define TEXTFIELD_H_

#include <string>
#include <SDL.h>

class GameState;

class TextField {
public:
	TextField(int max_length, const std::string& default_text = std::string());

    const std::string& text() const {
        return _text;
    }

    bool empty() const {
    	return _text.empty();
    }
    void step();
    void clear();
	bool handle_event(SDL_Event *event);
private:
	SDLKey _current_key;
	SDLMod _current_mod;

	std::string _text;
	int _max_length;
	int _repeat_cooldown;
};

#endif /* TEXTFIELD_H_ */
