#ifndef INPUT_H
#define INPUT_H

#include <glm/glm.hpp>

struct GLFWwindow;

namespace ogl {

// A thin input abstraction over GLFW: edge-detected key/button queries, mouse position/delta,
// scroll, and simple named axes (e.g. "Horizontal" = A/D, "Vertical" = W/S). `Update` is
// called once per frame by the Application after polling events; scripts read it in Play mode.
//
// Keys are GLFW key codes (GLFW_KEY_*). The class keeps the previous frame's state so
// GetKeyDown / GetKeyUp report the transition this frame.
class Input
{
public:
	void Init(GLFWwindow* window);
	void Update();   // snapshot current state, compute deltas (call once per frame)

	bool GetKey(int key) const;      // held
	bool GetKeyDown(int key) const;  // pressed this frame
	bool GetKeyUp(int key) const;    // released this frame

	bool GetMouseButton(int button) const;
	glm::vec2 MousePosition() const { return mousePos; }
	glm::vec2 MouseDelta() const { return mouseDelta; }

	// Axes in [-1, 1]: "Horizontal" (A/D, Left/Right), "Vertical" (W/S, Down/Up).
	float GetAxis(const char* axis) const;

private:
	bool KeyNow(int key) const;

	GLFWwindow* window = nullptr;
	static constexpr int kMaxKeys = 512;
	bool cur[kMaxKeys] = {};
	bool prev[kMaxKeys] = {};
	glm::vec2 mousePos = glm::vec2(0.0f);
	glm::vec2 mouseDelta = glm::vec2(0.0f);
	bool firstUpdate = true;
};


} // namespace ogl
#endif
