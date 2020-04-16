#pragma once

#include "Utils.h"
#include "Input.h"
#include "ReferenceRotation.h"

/// Constant parameters for the camera.
#define CAMERA_SPEED 5.f
#define CAMERA_ANGULAR_SPEED 45.f * 0.01f


/// A 3D free camera implementation.
/// Use WASD to move, Space to toggle FPS mode (mouse movements to rotate)
class Camera : ReferenceRotation {

public:
	Camera();

	/// Called each frame to update the position and rotation of the camera based on user input
	void Update(Input* input, float dt);

	/// Returns the camera's current view matrix for 3D rendering.
	glm::mat4 getViewMatrix() const;

protected:
	/// The current position
	glm::vec3 position;

	/// Flags for FPS (mouse movement = rotation) mode
	bool fpsMode = false;
	bool switchingFpsMode = false;

	/// The previous known mouse position, used for FPS mode
	glm::vec2 previousMousePos;

	/// The current euler angles of the camera; the actual rotation is controlled by the base class.
	/// Note that roll is assumed to always be 0 (otherwise -> nausea)
	float pitch = 0, yaw = 0;

}; // class Camera
