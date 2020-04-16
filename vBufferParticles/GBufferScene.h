#pragma once

#include "Scene.h"
#include "Particles.h"


#define SEND_DEBUG_BUFFER_G3 // comment out to prevent sending debug data to lighting shader. Shader must reflect this.



#ifdef SEND_DEBUG_BUFFER_G3
#include "DebugBuffer.h"
#endif

/// A simple scene used to demonstrate standard deferred rendering with a set of G-Buffers
class GBufferScene : public Scene {

	/// Single render pass
	RenderPass* renderPass;

	/// One descriptor set for each subpass
	Descriptor* firstSubpassDescriptor;
	Descriptor* secondSubpassDescriptor;

	/// Graphics pipelines for the initial subpass (textured objects)
	GraphicsPipeline* shrimpPipeline;
	GraphicsPipeline* raymarchPipeline;
	GraphicsPipeline* raccoonPipeline;

	/// Meshes (see descriptions in ForwardRendererScene.h)
	Mesh* quad;
	Mesh* cube;
	Mesh* cube2;
	Mesh* cube3;
	Mesh* ground;
	Mesh* raymarchCube;

	/// Textures loaded from files
	Texture* shrimpTex;
	Texture* raccoonTex;

	/// Graphics pipeline for the second subpass (lighting using G-Buffer data)
	GraphicsPipeline* ppPipeline;

	/// Components of the G-Buffers
	Texture* colorAttachment;// albedo
	Texture* positionAttachment;// ws position
	Texture* normalAttachment;// ws normals

	/// Uniform buffers sent to shaders
	LightBuffer* lightBuffer;
	MatrixBuffer* matrixBuffer;

	/// Particle system.
	ParticleSystem* particles;

	/// Whether to hide everything other than particles
	bool particlesOnly = true;

	/// Debug data
#ifdef SEND_DEBUG_BUFFER_G3
	DebugBuffer* debugBuffer;
	uint8_t debugView = 0;// shaded or raw view of albedo, position, etc.
#endif

public:

	/// Returns (only) render pass
	inline RenderPass* getRenderPass() override { return renderPass; }

	/// Initializer
	GBufferScene(VulkanAppBase* vulkanApp);
	
	/// Cleanup
	~GBufferScene() override;

	/// Updates the scene; called each frame
	void Update(uint32_t imageIndex, float dt, float time) override;

	/// Scene settings
	bool UI() override;

	/// Used to update a command buffer with the scene data
	RenderPass* cmdBind(const VkCommandBuffer& cmdBuffer, int index) override;

	/// Used to update the compute command buffer with current scene data
	void cmdBindCompute(const VkCommandBuffer& cmdBuffer) override;

};// class GBufferScene
