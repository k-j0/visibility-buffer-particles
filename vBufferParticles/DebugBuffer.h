#pragma once

#include "UniformBuffer.h"

/// The uniform buffer object available to shaders.
struct DebugBufferObject {
	alignas(4) float value;
};// struct DebugBufferObject

// A buffer used to send debug values to shaders.
struct DebugBuffer : public UniformBuffer<DebugBufferObject> {

	inline DebugBuffer(UNIFORM_BUFFER_CONSTRUCTOR) {}
	int noUpdatesCount = 0;
	float previousValue = -10001;

	/// Updates and uploads the debug data to the GPU.
	inline void updateBuffer(uint32_t currentImage, DebugBufferObject data) {

		/// Should we bother updating UBO?
		if (previousValue == data.value) ++noUpdatesCount;
		else noUpdatesCount = 0;
		if (noUpdatesCount > getBuffers().size()) return;// nothing to update GPU-side

		previousValue = data.value;

		copyBuffer(currentImage, data);// send to GPU
	}

};// struct DebugBuffer
