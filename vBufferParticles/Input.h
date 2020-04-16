#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>

// helper macro to obtain the input instance from code that has access to the GLFW window handle.
#define INPUT Input::getInstance(window)

/// Singleton class. Used to get user input data.
class Input {

	static Input* instance;

	GLFWwindow* window;

	/// Creates the Input singleton object
	inline Input(GLFWwindow* window) : window(window) {
		glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
		if (glfwRawMouseMotionSupported())
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		else throw std::runtime_error("GLFW raw mouse motion is not supported on this platform");
	}

public:

	/// Returns the singleton instance.
	static inline Input* const getInstance([[maybe_unused]] GLFWwindow* window) {
		if (!instance)
			instance = new Input(window);
		return instance;
	}

	/// Returns the current GLFW window handle used.
	inline GLFWwindow* getWindow() { return window; }

	/// Returns whether a key is currently pressed.
	bool isKeyDown(int key);

};// class Input

