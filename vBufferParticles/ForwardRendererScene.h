#pragma once

#include "Scene.h"
#include "Particles.h"



/// Simple scene used for demonstrating forward rendering.
class ForwardRendererScene : public Scene {

	/// Single render pass
	RenderPass* renderPass;

	/// Single descriptor for the only subpass
	Descriptor* firstSubpassDescriptor;

	/// Graphics pipelines (shader sets)
	GraphicsPipeline* shrimpPipeline;// shrimp textured objects
	GraphicsPipeline* raymarchPipeline;// raymarch animated object
	GraphicsPipeline* raccoonPipeline;// raccoon textured object

	/// Meshes in the scene (all standard meshes with pos/norm/uv)
	Mesh* quad;// simple quad facing the camera
	Mesh* cube;// big cube behind the quad
	Mesh* cube2;// cube to the right
	Mesh* cube3;// cube to the left
	Mesh* ground;// big cube below the scene
	Mesh* raymarchCube;// cube to the right of the initial cube

	/// Textures used in the scene
	Texture* shrimpTex;
	Texture* raccoonTex;

	/// Uniform buffers sent to shaders
	LightBuffer* lightBuffer;
	MatrixBuffer* matrixBuffer;

	/// Particle system.
	ParticleSystem* particles;

	/// Whether to hide everything other than particles
	bool particlesOnly = true;

public:

	/// Returns (only) render pass
	inline RenderPass* getRenderPass() override { return renderPass; }

	/// Initializer
	ForwardRendererScene(VulkanAppBase* vulkanApp);

	/// Cleanup
	~ForwardRendererScene() override;

	/// Updates the scene; called each frame
	void Update(uint32_t imageIndex, float dt, float time) override;

	/// Scene settings
	bool UI() override;

	/// Used to update a command buffer with the scene data
	RenderPass* cmdBind(const VkCommandBuffer& cmdBuffer, int index) override;

	/// Used to update the compute command buffer with current scene data
	void cmdBindCompute(const VkCommandBuffer& cmdBuffer) override;

};//class ForwardRendererScene

