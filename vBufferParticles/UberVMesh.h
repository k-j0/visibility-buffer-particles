#pragma once

#include "Mesh.h"
#include "VBufferVertexBuffer.h"

/// A mesh containing data that can be used in both subpasses of the V-Buffer render pass
struct UberVMesh {

	/// From UberVVertices, creates both a mesh with VisibilityVertex layout and uniform buffers for sending the data to the lighting pass
	inline UberVMesh(const std::vector<UberVVertex>& vertices, const std::vector<uint16_t>& indices, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
		
		this->indices = indices;

		std::vector<VisibilityVertex> visVertices;
		for (int i = 0; i < vertices.size(); ++i) {
			this->vertices.push_back(VBufferVertex(vertices[i].vector3s[0], vertices[i].vector3s[1], vertices[i].vector2s[0]));
			visVertices.push_back(VisibilityVertex(vertices[i].vector3s[0], vertices[i].vector2s[0], vertices[i].vector2s[1].x, vertices[i].vector2s[1].y));
		}

		vMesh = new VisibilityMesh(visVertices, indices, logicalDevice, physicalDevice, commandPool, graphicsQueue);

	}

	// cleanup vk resources for the underlying mesh
	inline ~UberVMesh() {
		delete vMesh;
	}

	/// Getters
	const inline VisibilityMesh& getVMesh() { return *vMesh; }
	inline uint32_t getIndexCount() { return (uint32_t)indices.size(); }
	inline uint32_t getVertexCount() { return (uint32_t)vertices.size(); }

	/// Write the data that describes this mesh into the buffers that will be pushed as uniform buffers to the lighting pass' fragment shader.
	inline void pushDataToUberVertexBuffer(std::vector<uint32_t>& vbIndices, std::vector<VBufferVertexInput>& vbVertices, uint32_t& indexOffset, uint32_t& vertexOffset) {

		// Push indices
		for (int i = 0; i < indices.size(); ++i) {
			assert(i + indexOffset < VBUFFER_MAX_INDICES);
			vbIndices[i + indexOffset] = indices[i] + vertexOffset;
		}
		indexOffset += (uint32_t)indices.size();

		// Push vertices
		for (int i = 0; i < vertices.size(); ++i) {
			assert(i + vertexOffset < VBUFFER_MAX_VERTICES);
			glm::vec3 pos = vertices[i].vector3s[0];
			glm::vec3 normal = vertices[i].vector3s[1];
			glm::vec2 uv = vertices[i].vector2s[0];
			VBufferVertexInput vertexInput(pos, normal, uv);
			vbVertices[i + vertexOffset] = vertexInput;
		}
		vertexOffset += (uint32_t)vertices.size();

	}

private:

	VisibilityMesh* vMesh;// mesh that can be bound for rendering directly
	// buffers that can be added to the uniform buffers for shading in lighting pass.
	std::vector<uint16_t> indices;
	std::vector<VBufferVertex> vertices;

};// struct UberVMesh
