#pragma once

#include "Vertex.h"
#include "Utils.h"
#include "Bindable.h"

/// Templated abstract class for meshes with any vertex layout.
template<typename VertexType>
class Mesh_Base : public Bindable {

public:

	/// Creates the mesh, given a vertex and index buffer.
	inline Mesh_Base(const std::vector<VertexType>& vertices, const std::vector<uint16_t>& indices, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) : logicalDevice(logicalDevice) {
		indexCount = static_cast<uint32_t>(indices.size());
		vertexCount = static_cast<uint32_t>(vertices.size());
		createMeshBuffer<VertexType>(vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory, *logicalDevice, physicalDevice, commandPool, graphicsQueue);
		createMeshBuffer<uint16_t>(indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMemory, *logicalDevice, physicalDevice, commandPool, graphicsQueue);
	}
	
	/// Cleans up vulkan resources for the vertex and index buffers.
	inline virtual ~Mesh_Base() {
		vkDestroyBuffer(*logicalDevice, vertexBuffer, NULL);
		vkFreeMemory(*logicalDevice, vertexBufferMemory, NULL);
		vkDestroyBuffer(*logicalDevice, indexBuffer, NULL);
		vkFreeMemory(*logicalDevice, indexBufferMemory, NULL);
	}

	/// Binds the mesh object at command buffer record time
	inline void cmdBind(const VkCommandBuffer& commandBuffer, int index) const override {
		VkDeviceSize offsets[] = { 0 };

		// bind the vertex and index buffers
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
		if (bindOnlyVertexBuffer) return;
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		if (bindOnlyVertexAndIndexBuffers) return;

		// draw the mesh using the index buffer
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	}

	/// Getters for the number of vertices and indices.
	inline const uint32_t& getIndexCount() { return indexCount; }
	inline const uint32_t& getVertexCount() { return vertexCount; }

	inline const VkBuffer* getVertexBuffer() const { return &vertexBuffer; }

	/// Optional parameters to provide an early-exit to cmdBind(), in case we only want one or both buffers to be bound without calling vkCmdDraw.
	bool bindOnlyVertexBuffer = false;
	bool bindOnlyVertexAndIndexBuffers = false;

private:

	VkDevice* logicalDevice;

	/// The vertex and index buffers and their allocated memory
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	/// How many indices and vertices the mesh has.
	uint32_t indexCount;
	uint32_t vertexCount;

	/// Helper function to create one of the buffers used for the mesh. Templated to create a buffer of any type (in practice, one for the vertex type and one for the index type ie uint16_t)
	template<typename T>
	inline void createMeshBuffer(const std::vector<T>& data, VkBufferUsageFlagBits usage, VkBuffer& buffer, VkDeviceMemory& memory, const VkDevice& logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
		assert(data.size() > 0);

		VkDeviceSize bufferSize = sizeof(data[0]) * data.size();

		// Create staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		U::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, logicalDevice, physicalDevice);

		// Fill staging buffer
		void* dat;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &dat); {
			memcpy(dat, data.data(), (size_t)bufferSize);
		} vkUnmapMemory(logicalDevice, stagingBufferMemory);

		// Create actual buffer on GPU
		U::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory, logicalDevice, physicalDevice);
		U::copyBuffer(stagingBuffer, buffer, bufferSize, commandPool, logicalDevice, graphicsQueue);

		// Free staging buffer
		vkDestroyBuffer(logicalDevice, stagingBuffer, NULL);
		vkFreeMemory(logicalDevice, stagingBufferMemory, NULL);
	}

};// class Mesh_Base


// Mesh types used in application.
typedef Mesh_Base<Vertex> Mesh;
typedef Mesh_Base<VisibilityVertex> VisibilityMesh;
