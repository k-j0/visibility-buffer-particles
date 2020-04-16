#pragma once

#include "Scene.h"
#include "VBufferVertexBuffer.h"
#include "Particles.h"


#define SEND_DEBUG_BUFFER_V // comment out to prevent sending debug data to lighting shader. Shader must reflect this.



#ifdef SEND_DEBUG_BUFFER_V
#include "DebugBuffer.h"
#endif


/// A simple scene to demonstrate the Visibility Buffer
class VBufferScene : public Scene {

	// the different materials used in the scene
#define SHRIMP_MAT 1
#define RAYMARCH_MAT 2
#define RACCOON_MAT 3
#define PARTICLES_MAT 4

	/// a single render pass
	RenderPass* renderPass;

	/// first subpass for visibility, second for lighting/shading/texturing work
	Descriptor* firstSubpassDescriptor;
	Descriptor* secondSubpassDescriptor;

	/// Graphics pipeline for first subpass (only one as all objects are written using the same shaders)
	VisibilityGraphicsPipeline* visibilityPipeline;

	/// Visibility meshes used in the scene; same data as GBufferScene, with a different layout specific to Visibility buffer passes.
	UberVMesh* vQuad;
	UberVMesh* vCube;
	UberVMesh* vCube2;
	UberVMesh* vCube3;
	UberVMesh* vGround;
	UberVMesh* vRaymarchCube;

	/// Textures loaded from image files
	Texture* shrimpTex;
	Texture* raccoonTex;
	Texture* leafTex;

	/// Lighting pass pipeline
	GraphicsPipeline* ppPipeline;

	/// Visibility buffer
	Texture* visibilityAttachment;

	/// Uniform buffers for light, matrices, vertices (for lighting pass)
	LightBuffer* lightBuffer;
	MatrixBuffer* matrixBuffer;
	VBufferVertexBuffer* vertexBuffer;

	/// Particles
	ParticleSystem* particles;

	/// Whether to hide everything other than particles
	bool particlesOnly = true;

	/// Debug data
#ifdef SEND_DEBUG_BUFFER_V
	DebugBuffer* debugBuffer;
	uint8_t debugView = 0;// shaded or raw view of albedo, position, etc.
#endif


public:

	/// Returns (only) render pass
	inline RenderPass* getRenderPass() override { return renderPass; }

	/// Initializer
	VBufferScene(VulkanAppBase* vulkanApp);

	/// Cleanup
	~VBufferScene() override;

	/// Updates the scene; called each frame
	void Update(uint32_t imageIndex, float dt, float time) override;

	/// Scene settings
	bool UI() override;

	/// Used to update a command buffer with the scene data
	RenderPass* cmdBind(const VkCommandBuffer& cmdBuffer, int index) override;

	/// Used to update the compute command buffer with current scene data
	void cmdBindCompute(const VkCommandBuffer& cmdBuffer) override;

};// class VBufferScene

