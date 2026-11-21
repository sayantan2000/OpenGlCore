#ifndef INPUT_H
#define INPUT_H

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

struct GLFWwindow;

namespace ogl {

// A thin input abstraction over GLFW covering keyboard, mouse and gamepad:
// edge-detected key/button queries, mouse position/delta, gamepad buttons/sticks/triggers with a
// configurable deadzone, and a small configurable "action" layer that maps names (e.g. "Jump")
// to any mix of keys, mouse buttons and gamepad buttons. `Update` is called once per frame by
// the Application after polling events; scripts read it in Play mode.
//
// Keys are GLFW key codes (GLFW_KEY_*), gamepad buttons/axes are GLFW_GAMEPAD_BUTTON_* /
// GLFW_GAMEPAD_AXIS_*. The class keeps the previous frame's state so the *Down/*Up queries report
// the transition this frame. Up to kMaxPads controllers are tracked.
class Input
{
public:
	static constexpr int kMaxPads = 4;

	void Init(GLFWwindow* window);
	void Update();   // snapshot current state, compute deltas (call once per frame)

	// ---- Keyboard ----
	bool GetKey(int key) const;      // held
	bool GetKeyDown(int key) const;  // pressed this frame
	bool GetKeyUp(int key) const;    // released this frame

	// ---- Mouse ----
	bool GetMouseButton(int button) const;
	glm::vec2 MousePosition() const { return mousePos; }
	glm::vec2 MouseDelta() const { return mouseDelta; }

	// ---- Gamepad ----
	bool IsGamepadConnected(int pad = 0) const;
	const char* GamepadName(int pad = 0) const;
	int  ConnectedGamepadCount() const;
	bool GetGamepadButton(int button, int pad = 0) const;     // held
	bool GetGamepadButtonDown(int button, int pad = 0) const; // pressed this frame
	bool GetGamepadButtonUp(int button, int pad = 0) const;   // released this frame
	// Raw axis in [-1,1] (triggers in [0,1] after GLFW's [-1,1]->remap), deadzone applied.
	float GetGamepadAxis(int axis, int pad = 0) const;
	glm::vec2 LeftStick(int pad = 0) const;   // (x, y) with y up-positive, deadzone applied
	glm::vec2 RightStick(int pad = 0) const;
	float LeftTrigger(int pad = 0) const;     // [0,1]
	float RightTrigger(int pad = 0) const;    // [0,1]
	void  SetDeadzone(float dz) { deadzone = glm::clamp(dz, 0.0f, 0.9f); }
	float Deadzone() const { return deadzone; }

	// ---- Named axes ----
	// "Horizontal" (A/D, Left/Right + left-stick X), "Vertical" (W/S, Down/Up + left-stick Y).
	float GetAxis(const char* axis) const;

	// ---- Configurable actions ----
	// An action fires if ANY bound key / mouse button / gamepad button is active. Bindings are
	// data, so a future settings UI (or a config file) can rebind them without code changes.
	struct ActionBinding
	{
		std::vector<int> keys;          // GLFW_KEY_*
		std::vector<int> mouseButtons;  // GLFW_MOUSE_BUTTON_*
		std::vector<int> padButtons;    // GLFW_GAMEPAD_BUTTON_*
	};
	void BindAction(const std::string& name, ActionBinding binding);
	bool GetAction(const std::string& name, int pad = 0) const;      // held
	bool GetActionDown(const std::string& name, int pad = 0) const;  // pressed this frame

private:
	bool KeyNow(int key) const;
	void RegisterDefaultActions();

	GLFWwindow* window = nullptr;
	static constexpr int kMaxKeys = 512;
	bool cur[kMaxKeys] = {};
	bool prev[kMaxKeys] = {};
	glm::vec2 mousePos = glm::vec2(0.0f);
	glm::vec2 mouseDelta = glm::vec2(0.0f);
	bool firstUpdate = true;

	// Gamepad state (GLFWgamepadstate is 15 buttons + 6 axes; stored as plain arrays to keep
	// the GLFW type out of this header).
	struct PadState
	{
		bool connected = false;
		unsigned char buttons[15] = {};
		float axes[6] = {};
		std::string name;
	};
	PadState pads[kMaxPads];
	PadState padsPrev[kMaxPads];
	float deadzone = 0.15f;

	std::unordered_map<std::string, ActionBinding> actions;
};


} // namespace ogl
#endif
