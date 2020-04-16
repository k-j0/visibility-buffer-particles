#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <array>
#include <vulkan/vulkan.hpp>


//used as the base class for Vertex types; used to statically assert whether a class inherits from Vertex_Template
struct ___vtx_base___ {
#define INVALID_VERTEX throw std::invalid_argument("Used ___vtx_base___ as a vertex type; this class exists for static assertions only. Use Vertex_Template specialization instead."); 
	static inline VkVertexInputBindingDescription getBindingDescription() { INVALID_VERTEX }
	static inline std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() { INVALID_VERTEX }
	inline bool operator==(const ___vtx_base___& o){ INVALID_VERTEX }
#undef INVALID_VERTEX
};
//use to assert whether a type inherits from Vertex_Template (will succeed upon ___vtx_base___ class used, but would throw verbose invalid argument exceptions if attempted to use after)
#define ASSERT_IS_VERTEX_TYPE(t) static_assert(std::is_base_of<___vtx_base___, t>::value);


/// Base class for a Vertex type, with any compile-time static number of float3 and float2
template<int Vec3s, int Vec2s>
struct Vertex_Template : private ___vtx_base___ {
	std::array<glm::vec3, Vec3s> vector3s;
	std::array<glm::vec2, Vec2s> vector2s;

	/// Constructs the vertex; use macro definitions below for each type of vertex instead of accessing this constructor directly.
	Vertex_Template(std::array<glm::vec3, Vec3s> vec3s, std::array<glm::vec2, Vec2s> vec2s) {
		vector3s = vec3s;
		vector2s = vec2s;
	}

	/// Returns the binding description used to build the vertex buffer for the vertex type
	static inline VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex_Template);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	/// Returns the attribute descriptions used to build the vertex buffer for the vertex type
	static inline std::array<VkVertexInputAttributeDescription, Vec3s + Vec2s> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, Vec3s + Vec2s> attributeDescriptions = {};

		int offset = 0;
		for(int i = 0; i < Vec3s + Vec2s; ++i) {
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = i;
			attributeDescriptions[i].format = (i < Vec3s) ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[i].offset = offset;
			offset += (i < Vec3s) ? sizeof(glm::vec3) : sizeof(glm::vec2);
		}

		return attributeDescriptions;
	}

	/// Compares two vertices' parameters and determines whether they should be considered identical
	inline bool operator==(const Vertex_Template<Vec3s, Vec2s>& other) {
		for (int i = 0; i < Vec3s; ++i) {
			if (other.vector3s[i] != vector3s[i]) return false;
		}
		for (int i = 0; i < Vec2s; ++i) {
			if (other.vector2s[i] != vector2s[i]) return false;
		}
		return true;
	}
};




/// Pseudo class macros & typedefs

/// Usual Vertex type, with a model-space position, model-space normal, and texture coordinates.
typedef Vertex_Template<2, 1> Vertex;// Position, Normal, Uv
#define Vertex(pos, normal, uv) Vertex(std::array<glm::vec3, 2>{ pos, normal }, std::array<glm::vec2, 1>{ uv }) //pseudo-constructor for Vertex

/// Vertex type used in the visibility pass for the V-Buffer renderer, with model-space position, texture coordinates, and a triangle and material id.
typedef Vertex_Template<1, 2> VisibilityVertex;// Position, Uv, Ids
#define VisibilityVertex(pos, uv, triId, matId) VisibilityVertex(std::array<glm::vec3, 1>{ pos }, std::array<glm::vec2, 2>{ uv, glm::vec2(triId, matId) }) //pseudo-constructor for VisibilityVertex

/// Vertex type used in the lighting pass for the V-Buffer renderer, with model-space position and model-space normal. This type of vertex will be sent via a uniform buffer to the fragment shader.
typedef Vertex_Template<2, 1> VBufferVertex;// Position, Normal, UV
#define VBufferVertex(pos, normal, uv) VBufferVertex(std::array<glm::vec3, 2>{ pos, normal }, std::array<glm::vec2, 1> { uv }) //pseudo-constructor for VBufferVertex

/// Overarching vertex type used in the V-Buffer renderer, with all data needed to create VisibilityVertices and VBufferVertices.
typedef Vertex_Template<2, 2> UberVVertex;// Position, Normal, Uv, Ids
#define UberVVertex(pos, normal, uv, triId, matId) UberVVertex(std::array<glm::vec3, 2>{pos, normal}, std::array<glm::vec2, 2>{uv, glm::vec2(triId, matId) }) //pseudo-constructor for UberVVertex

/// Vertex type with only position
typedef Vertex_Template<1, 0> PointVertex;// Position
#define PointVertex(pos) PointVertex(std::array<glm::vec3, 1>{pos}, std::array<glm::vec2, 0>{})// pseudo-constructor for PointVertex

/// Vertex type with nothing
typedef Vertex_Template<0, 0> NulVertex;
#define NulVertex() NulVertex(std::array<glm::vec3, 0>{}, std::array<glm::vec2, 0>{})// pseudo-constructor for NulVertex
