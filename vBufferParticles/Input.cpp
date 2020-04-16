#include "Input.h"

Input* Input::instance = NULL;

/// Returns whether a keyboard key is currently pressed
bool Input::isKeyDown(int key) {
	return glfwGetKey(window, key) == GLFW_PRESS;
}
