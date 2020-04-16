#pragma once

#include "UniformBuffer.h"

/// The uniform buffer object available to shaders.
struct MatrixBufferObject {
	alignas(16) glm::mat4 model;// world matrix
	alignas(16) glm::mat4 view;// view matrix
	alignas(16) glm::mat4 proj;// projection matrix
	alignas(4) float time;// time since startup
};// struct MatrixBufferObject

// The world, projection, view matrices sent to shaders. Also includes time for ease of access in shaders.
struct MatrixBuffer : public UniformBuffer<MatrixBufferObject> {

	MatrixBufferObject ubo;// the uniform buffer object sent to shaders
	int noUpdatesCount = 0;

	inline MatrixBuffer(UNIFORM_BUFFER_CONSTRUCTOR) {}

	/// Updates and uploads the matrices to the GPU.
	inline void updateBuffer(uint32_t currentImage, float time, const glm::mat4& world, const glm::mat4& view, const glm::mat4& proj) {

		// Check whether we should send any data to the gpu
		if (ubo.model == world && ubo.view == view && ubo.proj == proj && ubo.time == time) ++noUpdatesCount;
		else noUpdatesCount = 0;
		if (noUpdatesCount > getBuffers().size()) return;// nothing to update on the GPU.

		// copy the data into the UBO.
		ubo.model = world;
		ubo.view = view;
		ubo.proj = proj;
		ubo.time = time;

		copyBuffer(currentImage, ubo);// send to GPU
	}

};// struct MatrixBuffer
