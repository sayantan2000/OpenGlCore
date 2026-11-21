#include "Input.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstring>

namespace ogl {

void Input::Init(GLFWwindow* w)
{
	window = w;
	std::memset(cur, 0, sizeof(cur));
	std::memset(prev, 0, sizeof(prev));
	firstUpdate = true;
	for (int i = 0; i < kMaxPads; ++i) { pads[i] = PadState{}; padsPrev[i] = PadState{}; }
	RegisterDefaultActions();
}

void Input::RegisterDefaultActions()
{
	// Sensible defaults; a settings UI or config file can override these later.
	BindAction("Jump",   { { GLFW_KEY_SPACE }, {}, { GLFW_GAMEPAD_BUTTON_A } });
	BindAction("Fire",   { { GLFW_KEY_LEFT_CONTROL }, { GLFW_MOUSE_BUTTON_LEFT }, { GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER } });
	BindAction("Sprint", { { GLFW_KEY_LEFT_SHIFT }, {}, { GLFW_GAMEPAD_BUTTON_LEFT_THUMB } });
	BindAction("Cancel", { { GLFW_KEY_ESCAPE }, {}, { GLFW_GAMEPAD_BUTTON_B } });
	BindAction("Submit", { { GLFW_KEY_ENTER }, {}, { GLFW_GAMEPAD_BUTTON_A } });
}

bool Input::KeyNow(int key) const
{
	if (!window || key < 0 || key >= kMaxKeys) return false;
	return glfwGetKey(window, key) == GLFW_PRESS;
}

void Input::Update()
{
	if (!window) return;

	std::memcpy(prev, cur, sizeof(cur));
	for (int k = 0; k < kMaxKeys; ++k)
		cur[k] = glfwGetKey(window, k) == GLFW_PRESS;

	double mx, my;
	glfwGetCursorPos(window, &mx, &my);
	glm::vec2 now((float)mx, (float)my);
	mouseDelta = firstUpdate ? glm::vec2(0.0f) : (now - mousePos);
	mousePos = now;
	firstUpdate = false;

	// Poll each gamepad slot. A slot is "connected" only if GLFW recognizes it as a gamepad
	// (has an SDL mapping); otherwise its axes/buttons stay zeroed.
	for (int i = 0; i < kMaxPads; ++i)
	{
		padsPrev[i] = pads[i];
		PadState& p = pads[i];
		GLFWgamepadstate gs;
		int jid = GLFW_JOYSTICK_1 + i;
		if (glfwJoystickIsGamepad(jid) && glfwGetGamepadState(jid, &gs))
		{
			p.connected = true;
			std::memcpy(p.buttons, gs.buttons, sizeof(p.buttons));
			std::memcpy(p.axes, gs.axes, sizeof(p.axes));
			const char* nm = glfwGetGamepadName(jid);
			p.name = nm ? nm : "Gamepad";
		}
		else
		{
			p.connected = false;
			std::memset(p.buttons, 0, sizeof(p.buttons));
			std::memset(p.axes, 0, sizeof(p.axes));
			p.name.clear();
		}
	}
}

bool Input::GetKey(int key) const { return (key >= 0 && key < kMaxKeys) && cur[key]; }
bool Input::GetKeyDown(int key) const { return (key >= 0 && key < kMaxKeys) && cur[key] && !prev[key]; }
bool Input::GetKeyUp(int key) const { return (key >= 0 && key < kMaxKeys) && !cur[key] && prev[key]; }

bool Input::GetMouseButton(int button) const
{
	return window && glfwGetMouseButton(window, button) == GLFW_PRESS;
}

// ---- Gamepad ----
static bool ValidPad(int pad) { return pad >= 0 && pad < Input::kMaxPads; }

bool Input::IsGamepadConnected(int pad) const { return ValidPad(pad) && pads[pad].connected; }
const char* Input::GamepadName(int pad) const { return ValidPad(pad) ? pads[pad].name.c_str() : ""; }
int Input::ConnectedGamepadCount() const
{
	int n = 0; for (int i = 0; i < kMaxPads; ++i) if (pads[i].connected) ++n; return n;
}

bool Input::GetGamepadButton(int button, int pad) const
{
	if (!ValidPad(pad) || !pads[pad].connected || button < 0 || button > 14) return false;
	return pads[pad].buttons[button] == GLFW_PRESS;
}
bool Input::GetGamepadButtonDown(int button, int pad) const
{
	if (!ValidPad(pad) || button < 0 || button > 14) return false;
	return pads[pad].buttons[button] == GLFW_PRESS && padsPrev[pad].buttons[button] != GLFW_PRESS;
}
bool Input::GetGamepadButtonUp(int button, int pad) const
{
	if (!ValidPad(pad) || button < 0 || button > 14) return false;
	return pads[pad].buttons[button] != GLFW_PRESS && padsPrev[pad].buttons[button] == GLFW_PRESS;
}

float Input::GetGamepadAxis(int axis, int pad) const
{
	if (!ValidPad(pad) || !pads[pad].connected || axis < 0 || axis > 5) return 0.0f;
	float v = pads[pad].axes[axis];
	// Triggers rest at -1 and travel to +1; remap to [0,1] (no radial deadzone).
	if (axis == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER || axis == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER)
		return (v + 1.0f) * 0.5f;
	return (v > deadzone || v < -deadzone) ? v : 0.0f;
}

glm::vec2 Input::LeftStick(int pad) const
{
	// GLFW stick Y is down-positive; flip so +Y is up (forward).
	return glm::vec2(GetGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X, pad), -GetGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_Y, pad));
}
glm::vec2 Input::RightStick(int pad) const
{
	return glm::vec2(GetGamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_X, pad), -GetGamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_Y, pad));
}
float Input::LeftTrigger(int pad) const { return GetGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, pad); }
float Input::RightTrigger(int pad) const { return GetGamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, pad); }

float Input::GetAxis(const char* axis) const
{
	if (std::strcmp(axis, "Horizontal") == 0)
	{
		float v = 0.0f;
		if (GetKey(GLFW_KEY_D) || GetKey(GLFW_KEY_RIGHT)) v += 1.0f;
		if (GetKey(GLFW_KEY_A) || GetKey(GLFW_KEY_LEFT))  v -= 1.0f;
		v += LeftStick(0).x;
		return glm::clamp(v, -1.0f, 1.0f);
	}
	if (std::strcmp(axis, "Vertical") == 0)
	{
		float v = 0.0f;
		if (GetKey(GLFW_KEY_W) || GetKey(GLFW_KEY_UP))    v += 1.0f;
		if (GetKey(GLFW_KEY_S) || GetKey(GLFW_KEY_DOWN))  v -= 1.0f;
		v += LeftStick(0).y;
		return glm::clamp(v, -1.0f, 1.0f);
	}
	return 0.0f;
}

// ---- Actions ----
void Input::BindAction(const std::string& name, ActionBinding binding)
{
	actions[name] = std::move(binding);
}

bool Input::GetAction(const std::string& name, int pad) const
{
	auto it = actions.find(name);
	if (it == actions.end()) return false;
	const ActionBinding& b = it->second;
	for (int k : b.keys)         if (GetKey(k)) return true;
	for (int m : b.mouseButtons) if (GetMouseButton(m)) return true;
	for (int g : b.padButtons)   if (GetGamepadButton(g, pad)) return true;
	return false;
}

bool Input::GetActionDown(const std::string& name, int pad) const
{
	auto it = actions.find(name);
	if (it == actions.end()) return false;
	const ActionBinding& b = it->second;
	for (int k : b.keys)         if (GetKeyDown(k)) return true;
	for (int g : b.padButtons)   if (GetGamepadButtonDown(g, pad)) return true;
	// Mouse "down" isn't edge-tracked here; held is close enough for action purposes.
	for (int m : b.mouseButtons) if (GetMouseButton(m)) return true;
	return false;
}

} // namespace ogl
