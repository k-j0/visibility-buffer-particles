#pragma once

#include <vulkan/vulkan.hpp>
#include "VulkanDevices.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"
#include "UniformBuffer.h"
#include "Descriptor.h"
#include "Mesh.h"
#include "Utils.h"
#include <imgui.h>
#include "Texture.h"


#define INITIAL_PARTICLE_COUNT 1024 * 1024 // start-up particle count (unless specified in command-line arguments)
#define INITIAL_PARTICLE_GEN_MODE ParticleGenerationMode::VertexGenExp // start-up generation mode (unless specified in command-line arguments)



#define GEOMETRY_OUTPUT_PARTICLES_PER_VERTEX 28 // In Geometry generation mode, the amount of particles created by each geometry shader call - must match the value in particles.geom



#define UNDEFINED_PARTICLE_COMPLEXITY -1024 // flag for complexity undefined yet



/// The mode with which to generate the particles
enum ParticleGenerationMode {
	VertexGenExp = 0,			// Call vertex shader 6 times the amount of particle, each call generating one vertex of a particle quad.
	ComputeGenExp = 1,			// Use compute shader to generate 6 vertices per particle, then passed to a passthrough vertex shader for rendering.
	GeometryGenExp = 2,			// Empty vertices are transparently passed through a vertex shader to call a geometry shader; each call creates up to 28 particle quads.
	VertexGenGeometryExp = 3	// Call vertex shader once per particle, which creates a vertex at its center, then expanded into a quad by the geometry shader.
};// enum ParticleSystemMode


/// The rendering mode with which the particles will be rendered
enum ParticleRenderingMode {
	ForwardRen,		// Forward Rendering
	DeferredG3Ren,	// Deferred Rendering with 3 framebuffers forming the G-Buffer
	DeferredG6Ren,	// Deferred Rendering with 6 framebuffers forming the G-Buffer
	DeferredVRen	// Deferred Rendering with Visibility Buffer
};// enum ParticleRenderingMode


/// Settings that can be modified for the particles
struct ParticleSystemSettings {
	unsigned int particleCount = INITIAL_PARTICLE_COUNT;
	int complexity = UNDEFINED_PARTICLE_COMPLEXITY;// complexity level of fragment shader used on particles (initialized depending on value in file __.defines)
	bool cutout = false;// whether to use cutout-style particles (mirrors value in __.defines file).
	ParticleGenerationMode genMode = INITIAL_PARTICLE_GEN_MODE;
	float halfSize = 0.03f;// half the size of each particle, in view space
	float density = 0.4f;// how packed together the particles are
	float gravity = 0.f;
	float initialUpwardsForce = 0.f;
};// struct ParticleSystemSettings



/// Represents a set of particles that can be rendered using several different methods and settings, in all different rendering modes.
class ParticleSystem {

	/// The uniform buffer struct that will be sent to one of the rendering shaders to setup particles at runtime.
	struct ParticlesUBO {
		glm::mat4 view; // View matrix
		glm::mat4 proj; // Projection matrix
		float time;// amount of seconds passed since creation
		float halfSize;// half the size of each particle, in view space.
		float density;
		float gravity;
		float initialUpwardsForce;
		uint32_t particleCount;// amount of particles that should be generated
	} particlesUBO;// struct ParticlesUBO
	int uboNoUpdateCount = 0;

	/// The SSBO with the position data to get as output from the compute shader in Compute generation mode
	struct ComputeSSBO {
		glm::vec4 position_nX;	// rgb: particle position in screen space; a: unused (normal.x)
		glm::vec4 nYZ_uv;		// rg: unused (normal.yz); ba: particle quad UVs
	};// struct ComputeSSBO



	/// Modes with which to generate and render the particles
	ParticleRenderingMode renMode;
	static ParticleSystemSettings settings;

	// keep track of devices
	DevicesPtr devices;

	GraphicsPipeline_Base* graphicsPipeline;// shader set used to render the particles.
	UniformBuffer<ParticlesUBO>* uboBuffer;// the uniform buffer object sent to the GPU with particle settings.
	Descriptor* graphicsDescriptor;// descriptor for the graphics pipeline.
	Mesh_Base<NulVertex>* vertexBufferMesh = NULL;// need a dummy vertex buffer bound before calling vkCmdDraw according to Vulkan spec, even if we're not using the data.
	Texture* particlesTexture = NULL;// optional texture applied to particles in certain complexity modes.

	// Fields used for Compute Generation Mode only
	struct ComputeFields {
		ComputePipeline* pipeline;// compute shader used to generate the particles.
		Descriptor* descriptor;// descriptor set for the compute pipeline
		UniformBuffer<ComputeSSBO>*	ssboBuffer;// SSBO sent to (received from) the compute shader calls.
	};// struct ComputeFields
	ComputeFields* computeFields = NULL;// will be NULL unless generation mode is set to Compute.

public:

	// Keep the constructor params that the particleSystem was generated with.
	struct ParticlesConstructorParams {
		ParticleRenderingMode rMode;
		DevicesPtr devices;
		const VkDescriptorPool* descriptorPool;
		uint32_t swapchainSize;
		VkExtent2D swapchainExtent;
		RenderPass* renderPass;
		VkCommandPool commandPool;
		VkSampler sampler;

		// shorthand for creating the params
		ParticlesConstructorParams(ParticleRenderingMode rMode, DevicesPtr devices, const VkDescriptorPool* descriptorPool, uint32_t swapchainSize,
			VkExtent2D swapchainExtent, RenderPass* renderPass, VkCommandPool commandPool, VkSampler sampler)
			:
			rMode(rMode), devices(devices), descriptorPool(descriptorPool), swapchainSize(swapchainSize),
			swapchainExtent(swapchainExtent), renderPass(renderPass), commandPool(commandPool), sampler(sampler)
		{ }

	};// struct ParticlesConstructorParams

	/// Resets the complexity of the fragment shader applied to the particles; this is static and will cause a re-compile of certain shaders automatically.
	/// Returns true if the swapchain should be rebuilt
	static bool setParticlesComplexity(int complexity, bool noRecompile = false);

	/// Resets whether the particles in complexity mode 2 will use a cutout-style shader (false -> fully opaque)
	static bool setParticlesCutout(bool cutout, bool noRecompile = false);

protected:
	ParticlesConstructorParams params;
public:

	/// Default constructor for a particle system; changing most of the settings will require re-creating a ParticleSystem instance.
	ParticleSystem(ParticlesConstructorParams& args);

	/// Clean up routine
	virtual ~ParticleSystem();

	/// Update the particle UBOs, called each frame.
	void Update(uint32_t imageIndex, float dt, float time, const glm::mat4& view, const glm::mat4& proj);

	/// Bind to a graphics command buffer to render
	void cmdBind(const VkCommandBuffer& cmdBuffer, int index);

	/// Bind to a compute command buffer to update (only in Compute mode)
	void cmdBindCompute(const VkCommandBuffer& cmdBuffer);



	/// Getters
	inline ParticleGenerationMode getGenMode() { return settings.genMode; }
	inline uint32_t getParticleCount() { return settings.particleCount; }
	inline ParticlesConstructorParams getConstructorParams() { return params; }
	inline DevicesPtr getDevices() { return devices; }



	/// Static method for selecting different settings for a ParticleSystem, reinitializing it whenever needed as selected by user.
	static ParticleSystem* UI(ParticleSystem* particles, bool& rebuild);

};// class Particles
