#pragma once

class Input
{
public:
	Input() = default;
	~Input() = default;
	ROF_COPY_MOVE_DELETE(Input)

public:
	// per-frame
	void flush()
	{
		keysPressed.clear();
		keysReleased.clear();
	}
	// e.g. on window exit
	void flush_all()
	{
		keysPressed.clear();
		keysDown.clear();
		keysReleased.clear();
	}

	// keyboard
	void register_keyboard_event(SDL_KeyboardEvent keyEvent)
	{
		// do not register repeat inputs
		if (keyEvent.repeat) return;

		switch (keyEvent.type) {
			case SDL_KEYDOWN:
			{
				keysPressed.insert(keyEvent.keysym.sym);
				keysDown.insert(keyEvent.keysym.sym);
				break;
			}
			case SDL_KEYUP:
			{
				keysReleased.insert(keyEvent.keysym.sym);
				keysDown.erase(keyEvent.keysym.sym);
				break;
			}
		}
	}
	// mouse
	void register_mouse_button_event(SDL_MouseButtonEvent)
	{
		// TODO
	}
	void register_mouse_motion_event(SDL_MouseMotionEvent)
	{
		// TODO
	}

public:
	std::set<SDL_Keycode> keysPressed;
	std::set<SDL_Keycode> keysDown;
	std::set<SDL_Keycode> keysReleased;
};