#pragma once

#include "UniformBuffer.h"

// the limits of the Vertex uniform buffer available to the lighting pass; should correspond to the #defines in pp_lighting_v fragment shader.
#define VBUFFER_MAX_TRIANGLES 64
#define VBUFFER_MAX_INDICES VBUFFER_MAX_TRIANGLES*3
#define VBUFFER_MAX_VERTICES VBUFFER_MAX_INDICES

/// Represents one vertex that can be sent to the gpu, packed into 128 bits.
struct VBufferVertexInput {
	glm::vec4 position_u;//xyz: vertex position; w: u
	glm::vec4 normal_v;//xyz: vertex normal; w: v

	inline VBufferVertexInput() {}

	inline VBufferVertexInput(const glm::vec3& pos, const glm::vec3& norm, const glm::vec2& uv) {
		position_u = glm::vec4(pos.x, pos.y, pos.z, uv.x);
		normal_v = glm::vec4(norm.x, norm.y, norm.z, uv.y);
	}
};// struct VBufferVertexInput

/// The UBO for all indices and vertices in the scene
struct VBufferVertexBufferObject {
	uint32_t indices[VBUFFER_MAX_INDICES];//global (scene-wide) index buffer
	VBufferVertexInput vertices[VBUFFER_MAX_VERTICES];//global (scene-wide) vertex buffer
};// struct VBufferVertexBufferObject

/// The uniform buffer used for sending data for indices and vertices in the scene to the lighting pass of the VBuffer pipeline.
struct VBufferVertexBuffer : public UniformBuffer<VBufferVertexBufferObject> {

	VBufferVertexBufferObject ubo;

	/// copy data over for indices and vertices
	inline VBufferVertexBuffer(std::vector<uint32_t>& indices, std::vector<VBufferVertexInput>& vertices, UNIFORM_BUFFER_CONSTRUCTOR) {
		memcpy(ubo.indices, indices.data(), VBUFFER_MAX_INDICES*sizeof(uint32_t));
		memcpy(ubo.vertices, vertices.data(), VBUFFER_MAX_VERTICES*sizeof(VBufferVertexInput));
	}

	/// update and upload buffer to GPU
	inline void updateBuffer(uint32_t currentImage) {

		copyBuffer(currentImage, ubo);

	}

};// struct VBufferVertexBuffer
