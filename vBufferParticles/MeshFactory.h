#pragma once

#include "Mesh.h"
#include "UberVMesh.h"

/// Static helper class with factory functions for creation of meshes.
class MeshFactory {

private:

	/// Adds a vertex to the index and vertex buffer, by comparing it to all current vertices all only adding it to the vertex buffer if it doesn't exist yet.
	template<typename VertexType>
	static inline void pushVert(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, const VertexType& vertex) {
		for (int i = 0; i < vertices.size(); ++i) {
			if (vertices[i] == vertex) {// see overload of Vertex_Base::operator==()
				indices.push_back(i);// these two vertices are the same, so only update the index buffer
				return;
			}
		}

		// no other twin vertex found, add the vertex to both buffers
		vertices.push_back(vertex);
		indices.push_back((uint16_t)(vertices.size() - 1));
	}

	/// Helper macros for adding the two different vertex types to vertex and index buffers (assuming their names)
#define VERT(v) pushVert<Vertex>(vertices, indices, v)
#define VISVERT(v) pushVert<UberVVertex>(vertices, indices, v)


public:

	/// Quad mesh facing down Z axis, with specified center position and size.
	static inline Mesh* createQuadMesh(glm::vec3 origin, glm::vec2 size, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
		const std::vector<Vertex> vertices = {
			Vertex(origin+glm::vec3(-0.5f*size.x, -0.5f*size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)),
			Vertex(origin+glm::vec3(0.5f*size.x, -0.5f*size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
			Vertex(origin+glm::vec3(0.5f*size.x, 0.5f*size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)),
			Vertex(origin+glm::vec3(-0.5f*size.x, 0.5f*size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f))
		};
		const std::vector<uint16_t> indices = {
			0, 1, 2, 2, 3, 0
		};

		return new Mesh(vertices, indices, logicalDevice, physicalDevice, commandPool, graphicsQueue);
	}

	/// Quad mesh for visibility vertices. triId will be increased by 2. See UberVMesh class.
	static inline UberVMesh* createVisibilityQuadMesh(glm::vec3 origin, glm::vec2 size, uint16_t& triId, uint16_t matId, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
		const std::vector<UberVVertex> vertices = {
			UberVVertex(origin + glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), triId, matId),
			UberVVertex(origin + glm::vec3(0.5f * size.x, -0.5f * size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f), triId, matId),
			UberVVertex(origin + glm::vec3(0.5f * size.x, 0.5f * size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), triId, matId),
			UberVVertex(origin + glm::vec3(0.5f * size.x, 0.5f * size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), triId, matId),
			UberVVertex(origin + glm::vec3(-0.5f * size.x, 0.5f * size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), triId, matId),
			UberVVertex(origin + glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), triId, matId)
		};
		const std::vector<uint16_t> indices = {
			0, 1, 2, 3, 4, 5
		};

		++triId;
		++triId;

		return new UberVMesh(vertices, indices, logicalDevice, physicalDevice, commandPool, graphicsQueue);
	}

	/// Cube mesh with center position and scale.
	static inline Mesh* createCubeMesh(glm::vec3 origin, glm::vec3 size, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;

		// Cube as follows:
		// FRONT:	A--B	BACK:	E--F   (hidden connections: A-E, B-F, C-G, D-H)
		//			|  |			|  |
		//			D--C			H--G
		glm::vec3 a(-1, 1, 1), b(1, 1, 1), c(1, -1, 1), d(-1, -1, 1), e(-1, 1, -1), f(1, 1, -1), g(1, -1, -1), h(-1, -1, -1); // vertex multipliers
#define LOCAL(v) v = v * size * 0.5f + origin;// macro for transforming vertices by the size and origin position of the cube.
		LOCAL(a)LOCAL(b)LOCAL(c)LOCAL(d)LOCAL(e)LOCAL(f)LOCAL(g)LOCAL(h)
#undef LOCAL
		glm::vec3 normal;

		// macros for adding a vertex and a quad face to the index and vertex buffers, assuming a vec3 called "normal".
#define QUAD_VERT(pos, u, v) VERT(Vertex(pos, normal, glm::vec2(u, v)))
#define QUAD_FACE(one, two, three, four) do{ QUAD_VERT(one, 0, 0); QUAD_VERT(four, 0, 1); QUAD_VERT(two, 1, 0); QUAD_VERT(two, 1, 0); QUAD_VERT(four, 0, 1); QUAD_VERT(three, 1, 1); }while(false)

		//Front Face
		normal = glm::vec3(0, 0, 1);
		QUAD_FACE(a, b, c, d);
		//Back Face
		normal = glm::vec3(0, 0, -1);
		QUAD_FACE(f, e, h, g);
		//Left Face
		normal = glm::vec3(-1, 0, 0);
		QUAD_FACE(e, a, d, h);
		//Right Face
		normal = glm::vec3(1, 0, 0);
		QUAD_FACE(b, f, g, c);
		//Top Face
		normal = glm::vec3(0, 1, 0);
		QUAD_FACE(a, e, f, b);
		//Bottom Face
		normal = glm::vec3(0, -1, 0);
		QUAD_FACE(d, c, g, h);

#undef QUAD_FACE
#undef QUAD_VERT

		return new Mesh(vertices, indices, logicalDevice, physicalDevice, commandPool, graphicsQueue);
	}

	/// Same as normal cube mesh, used to create specifically visibility mesh.
	static inline UberVMesh* createVisibilityCubeMesh(glm::vec3 origin, glm::vec3 size, uint16_t& triId, uint16_t matId, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
		std::vector<UberVVertex> vertices;
		std::vector<uint16_t> indices;

		// Cube as follows:
		// FRONT:	A--B	BACK:	E--F   (hidden connections: A-E, B-F, C-G, D-H)
		//			|  |			|  |
		//			D--C			H--G
		glm::vec3 a(-1, 1, 1), b(1, 1, 1), c(1, -1, 1), d(-1, -1, 1), e(-1, 1, -1), f(1, 1, -1), g(1, -1, -1), h(-1, -1, -1);
#define LOCAL(v) v = v * size * 0.5f + origin;
		LOCAL(a)LOCAL(b)LOCAL(c)LOCAL(d)LOCAL(e)LOCAL(f)LOCAL(g)LOCAL(h)
#undef LOCAL
		glm::vec3 normal;

		// see above function.
#define QUAD_VERT(pos, u, v) VISVERT(UberVVertex(pos, normal, glm::vec2(u, v), (float)triId, (float)matId))
#define QUAD_FACE(one, two, three, four) do{ QUAD_VERT(one, 0, 0); QUAD_VERT(four, 0, 1); QUAD_VERT(two, 1, 0); QUAD_VERT(two, 1, 0); QUAD_VERT(four, 0, 1); QUAD_VERT(three, 1, 1); }while(false)

		//Front Face
		normal = glm::vec3(0, 0, 1);
		QUAD_FACE(a, b, c, d);
		//Back Face
		normal = glm::vec3(0, 0, -1);
		QUAD_FACE(f, e, h, g);
		//Left Face
		normal = glm::vec3(-1, 0, 0);
		QUAD_FACE(e, a, d, h);
		//Right Face
		normal = glm::vec3(1, 0, 0);
		QUAD_FACE(b, f, g, c);
		//Top Face
		normal = glm::vec3(0, 1, 0);
		QUAD_FACE(a, e, f, b);
		//Bottom Face
		normal = glm::vec3(0, -1, 0);
		QUAD_FACE(d, c, g, h);

#undef QUAD_FACE
#undef QUAD_VERT

		triId += 6 * 2;// 2 triangles per face

		return new UberVMesh(vertices, indices, logicalDevice, physicalDevice, commandPool, graphicsQueue);
	}


	// undefine macros
#undef VERT
#undef VISVERT

};// class MeshFactory
