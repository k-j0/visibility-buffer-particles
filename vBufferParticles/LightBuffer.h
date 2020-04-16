#pragma once

#include "UniformBuffer.h"

//#define ANIMATE_LIGHT //define to make light move in ellipses

/// The uniform buffer object available to shaders.
struct LightBufferObject {
	alignas(16) glm::vec4 position; // xyz = position / w = radius
	alignas(16) glm::vec4 colour; // xyz = diffuse color / w = unused
	alignas(16) glm::vec4 ambient; // xyz = ambient color / w = unused
};// struct LightBufferObject

/// A UniformBuffer used to send data for a single light to shaders.
struct LightBuffer : public UniformBuffer<LightBufferObject> {

	LightBufferObject ubo;// the uniform buffer object.
	bool noUpdatesCount = 0;

	/// Creates the light uniform buffer from a position, radius, diffuse and ambient colours for the light
	inline LightBuffer(const glm::vec3& position, const float& radius, const glm::vec3& color, const glm::vec3& ambient, UNIFORM_BUFFER_CONSTRUCTOR) {
		ubo.position = glm::vec4(position.x, position.y, position.z, radius);
		ubo.colour = glm::vec4(color.x, color.y, color.z, 1);// w unused
		ubo.ambient = glm::vec4(ambient.x, ambient.y, ambient.z, 1);//w unused

#ifndef ANIMATE_LIGHT
		// no need to update UBO ever again unless we're animating the light, just copy once here.
		for (int i = 0; i < getBuffers().size(); ++i)
			copyBuffer(i, ubo);
#endif
	}

	/// Updates and uploads the uniform buffer data to the GPU.
	inline void updateBuffer(uint32_t currentImage, float dt, float time) {

#ifdef ANIMATE_LIGHT
		LightBufferObject ubo;
		ubo.ambient = this->ubo.ambient;
		ubo.colour = this->ubo.colour;
		ubo.position = this->ubo.position;
		ubo.position += glm::vec4(sinf(time * 1.37f), sinf(time * 2.43f), 0, 0);

		copyBuffer(currentImage, ubo);// send to GPU.
#endif
	}

};// struct LightBuffer
