#include "Camera.h"

/// Default constructor; camera starts at (0,0,2) with a null rotation.
Camera::Camera() {
	position = glm::vec3(0, 0, 2);
	setEuler(0, 0, 0);
}

void Camera::Update(Input* input, float dt) {

	/// Movement based on WASD key presses
	if (input->isKeyDown(GLFW_KEY_W)) {// forward
		position += getFwd() * dt * CAMERA_SPEED;
	}
	if (input->isKeyDown(GLFW_KEY_S)) {// backwards
		position -= getFwd() * dt * CAMERA_SPEED;
	}
	if (input->isKeyDown(GLFW_KEY_A)) {// leftwards
		position -= getRight() * dt * CAMERA_SPEED;
	}
	if (input->isKeyDown(GLFW_KEY_D)) {// rightwards
		position += getRight() * dt * CAMERA_SPEED;
	}
	if (input->isKeyDown(GLFW_KEY_Q)) {// down
		position -= getUp() * dt * CAMERA_SPEED;
	}
	if (input->isKeyDown(GLFW_KEY_E)) {// up
		position += getUp() * dt * CAMERA_SPEED;
	}

	/// Toggle FPS mode
	if (input->isKeyDown(GLFW_KEY_SPACE)) {
		if (!switchingFpsMode) {// only do once per key press
			switchingFpsMode = true;
			fpsMode = !fpsMode;
			if (fpsMode) {
				int width, height;
				// place the cursor at the center of the screen (only used for going back to normal mode after)
				glfwGetWindowSize(input->getWindow(), &width, &height);
				glfwSetCursorPos(input->getWindow(), width*0.5f, height*0.5f);
				previousMousePos = glm::vec2(width*0.5f, height*0.5f);
				// disable cursor visibility and make it immovable
				glfwSetInputMode(input->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			} else {
				// reenable normal behaviour for cursor
				glfwSetInputMode(input->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
	} else switchingFpsMode = false;

	/// Mouse movements to rotate camera in FPS mode
	if (fpsMode) {
		double x, y;
		glfwGetCursorPos(input->getWindow(), &x, &y);

		glm::vec2 mousePos(x, y);
		glm::vec2 mouseDelta = mousePos - previousMousePos;
		previousMousePos = mousePos;

		// add mouse movement to pitch and roll
		pitch -= mouseDelta.y * CAMERA_ANGULAR_SPEED;
		yaw += mouseDelta.x * CAMERA_ANGULAR_SPEED;

		// let base class recompute angles
		setEuler(fmodf(pitch, 360), fmodf(yaw, 360), 0);

	}

}

/// Returns the current view matrix.
glm::mat4 Camera::getViewMatrix() const {
	return glm::lookAt(position, position + getFwd(), getUp());
}
