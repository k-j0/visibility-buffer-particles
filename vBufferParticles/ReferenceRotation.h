#pragma once

// A reference rotation has a forward, right and up vectors, which can be updated by changing its pitch, yaw and roll

#include <glm/glm.hpp>
#include "Utils.h"

class ReferenceRotation {

private:
	glm::vec3 fwd, right, up;// forward, right and up direction vectors.
	glm::vec3 euler;//degrees

public:
	//Set euler angles
	inline void setEuler(float pitch, float yaw, float roll) {
		euler = glm::vec3(pitch, yaw, roll);

		float cosP, cosY, cosR;
		float sinP, sinY, sinR;

		// cache cosines
		cosP = U::cos(euler.x);
		cosY = U::cos(euler.y);
		cosR = U::cos(euler.z);

		// cache sines
		sinP = U::sin(euler.x);
		sinY = U::sin(euler.y);
		sinR = U::sin(euler.z);

		//Update directions:
		fwd = glm::normalize(glm::vec3(sinY * cosP, sinP, cosP * -cosY));
		up = glm::normalize(glm::vec3(-cosY * sinR - sinY * sinP * cosR, cosP * cosR, -sinY * sinR - sinP * cosR * -cosY));
		right = glm::normalize(glm::cross(fwd, up));

	}

	//Getters
	inline const glm::vec3& getFwd() const { return fwd; }
	inline const glm::vec3& getUp() const { return up; }
	inline const glm::vec3& getRight() const { return right; }
	inline const glm::vec3& getEuler() const { return euler; }

};// class ReferenceRotation
