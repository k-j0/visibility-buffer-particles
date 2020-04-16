#include "VBufferScene.h"

VBufferScene::VBufferScene(VulkanAppBase* vulkanApp) : Scene(vulkanApp) {
	/// Create objects that do not rely on a specific swapchain layout

	//descriptor set & pipeline layouts
	DESCRIPTOR_BINDING_ARRAY firstSubpassBindings = { DESCRIPTOR_BINDING_UBO_VERTEX };
	firstSubpassDescriptor = new Descriptor(firstSubpassBindings, devices());
	DESCRIPTOR_BINDING_ARRAY secondSubpassBindings = { DESCRIPTOR_BINDING_INPUT_ATTACHMENT_FRAGMENT, DESCRIPTOR_BINDING_UBO_FRAGMENT, DESCRIPTOR_BINDING_UBO_FRAGMENT, DESCRIPTOR_BINDING_UBO_FRAGMENT, DESCRIPTOR_BINDING_SAMPLER_FRAGMENT, DESCRIPTOR_BINDING_SAMPLER_FRAGMENT, DESCRIPTOR_BINDING_SAMPLER_FRAGMENT };
#ifdef SEND_DEBUG_BUFFER_V
	secondSubpassBindings.push_back(DESCRIPTOR_BINDING_UBO_FRAGMENT);
#endif
	secondSubpassDescriptor = new Descriptor(secondSubpassBindings, devices());

	// create visibility meshes; each vertex keeps track of its primitive id
	uint16_t triId = 0;
	vQuad = MeshFactory::createVisibilityQuadMesh(glm::vec3(-0.5f, 0, 0), glm::vec2(0.5f, 0.5f), triId, SHRIMP_MAT, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	vCube = MeshFactory::createVisibilityCubeMesh(glm::vec3(0.5f, 0, -3.0f), glm::vec3(2.5f, 2.5f, 2.5f), triId, SHRIMP_MAT, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	vCube2 = MeshFactory::createVisibilityCubeMesh(glm::vec3(2.0f, 0.3f, 2.0f), glm::vec3(1.0f, 1.5f, 1.0f), triId, SHRIMP_MAT, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	vCube3 = MeshFactory::createVisibilityCubeMesh(glm::vec3(-2.0f, 0.3f, 2.0f), glm::vec3(1.5f, 1.5f, 1.5f), triId, RACCOON_MAT, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	vGround = MeshFactory::createVisibilityCubeMesh(glm::vec3(0, -2.5f, 0), glm::vec3(20, 0.2f, 20), triId, SHRIMP_MAT, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	vRaymarchCube = MeshFactory::createVisibilityCubeMesh(glm::vec3(3.0f, 0, -2.0f), glm::vec3(2, 2, 2), triId, RAYMARCH_MAT, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	// check that we haven't gone over the allowed limit for the vertex & index uniform buffers
	assert(triId < VBUFFER_MAX_TRIANGLES);
	assert(vQuad->getIndexCount() + vCube->getIndexCount() + vCube2->getIndexCount() + vCube3->getIndexCount() + vGround->getIndexCount() + vRaymarchCube->getIndexCount() < VBUFFER_MAX_INDICES);
	assert(vQuad->getVertexCount() + vCube->getVertexCount() + vCube2->getVertexCount() + vCube3->getVertexCount() + vGround->getVertexCount() + vRaymarchCube->getVertexCount() < VBUFFER_MAX_VERTICES);

	//Create vertex buffer that will be used in lighting pass
	{
		std::vector<uint32_t> indices(VBUFFER_MAX_INDICES);
		std::vector<VBufferVertexInput> vertices(VBUFFER_MAX_VERTICES);

		uint32_t indicesOffset = 0;
		uint32_t verticesOffset = 0;

		/// Push all mesh data to the uber vertex buffer
#define PUSHDATA(mesh) mesh->pushDataToUberVertexBuffer(indices, vertices, indicesOffset, verticesOffset)
		PUSHDATA(vQuad);
		PUSHDATA(vCube);
		PUSHDATA(vCube2);
		PUSHDATA(vCube3);
		PUSHDATA(vGround);
		PUSHDATA(vRaymarchCube);
#undef PUSHDATA

		/// Create uniform buffer from the data
		vertexBuffer = new VBufferVertexBuffer(indices, vertices, vulkanApp->getSwapchain()->getSize(), devices(), devices->getPhysicalDevice());

		// Update vertex UBO immediately
		for (int i = 0; i < vertexBuffer->getBuffers().size(); ++i)
			vertexBuffer->updateBuffer(i);

	}

	// load textures from image files
	shrimpTex = new Texture("Textures/shrimp.png", devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	raccoonTex = new Texture("Textures/raccoon.png", devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	leafTex = new Texture("Textures/leaf.png", devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());


	/// Create objects and layouts dependant on swapchain size
	// Create render pass & attachments
	std::vector<RenderPass::RenderPassAttachmentDesc> attachments = { RENDERPASS_ATTACHMENT_DESC_PRESENT(vulkanApp->getSwapchain()->getFormat()), RENDERPASS_ATTACHMENT_DESC_VEC4, RENDERPASS_ATTACHMENT_DESC_DEPTH };
	renderPass = new RenderPass(devices(), attachments, 2, vulkanApp->getSwapchain()->getExtent());

	// Create pipeline layouts
	firstSubpassDescriptor->createPipelineLayout();
	secondSubpassDescriptor->createPipelineLayout();

	// Create pipelines
	visibilityPipeline = new VisibilityGraphicsPipeline("default_v", "default_v", NULL, vulkanApp->getSwapchain()->getExtent(), firstSubpassDescriptor->getPipelineLayout(), renderPass, 0, true, 1, devices());
	ppPipeline = new GraphicsPipeline("pp", "pp_lighting_v", NULL, vulkanApp->getSwapchain()->getExtent(), secondSubpassDescriptor->getPipelineLayout(), renderPass, 1, false, 1, devices());

	//Create attachments
	visibilityAttachment = new Texture(VK_FORMAT_R16G16B16A16_SFLOAT, vulkanApp->getSwapchain()->getExtent(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());

	// Create uniform buffers
	lightBuffer = new LightBuffer(glm::vec3(2, 2, 2), 20, glm::vec3(1, 1, 0), glm::vec3(0.1f, 0.1f, 0.5f), vulkanApp->getSwapchain()->getSize(), devices(), devices->getPhysicalDevice());
	matrixBuffer = new MatrixBuffer(vulkanApp->getSwapchain()->getSize(), devices(), devices->getPhysicalDevice());
#ifdef SEND_DEBUG_BUFFER_V
	debugBuffer = new DebugBuffer(vulkanApp->getSwapchain()->getSize(), devices(), devices->getPhysicalDevice());
#endif

	// Create framebuffer attachments / note: attachment images will be prepended with present image
	std::vector<VkImageView> attachmentImages = { visibilityAttachment->getImageView(), vulkanApp->getDepthBuffer()->getImageView() };
	vulkanApp->getSwapchain()->createFramebuffers(attachmentImages, renderPass->getRenderPass());

	// Create subpass descriptor sets
	std::vector<Descriptor::UBODescriptor> uboDescriptors1 = { Descriptor::UBODescriptor(matrixBuffer->getBuffers(), sizeof(MatrixBufferObject)) };
	std::vector<Descriptor::ImageInfoDescriptor> imgDescriptors1 = { };
	firstSubpassDescriptor->createDescriptorSets(vulkanApp->getSwapchain()->getSize(), *descriptorPool, uboDescriptors1, imgDescriptors1);
	std::vector<Descriptor::UBODescriptor> uboDescriptors2 = { Descriptor::UBODescriptor(lightBuffer->getBuffers(), (int)sizeof(LightBufferObject)), Descriptor::UBODescriptor(matrixBuffer->getBuffers(), sizeof(MatrixBufferObject)), Descriptor::UBODescriptor(vertexBuffer->getBuffers(), sizeof(VBufferVertexBufferObject)) };
#ifdef SEND_DEBUG_BUFFER_V
	uboDescriptors2.push_back(Descriptor::UBODescriptor(debugBuffer->getBuffers(), (int)sizeof(DebugBufferObject)));
#endif
	std::vector<Descriptor::ImageInfoDescriptor> imgDescriptors2 = { DESCRIPTOR_IMG_ATTACHMENT_INFO(visibilityAttachment), Descriptor::ImageInfoDescriptor(shrimpTex, vulkanApp->getSampler()), Descriptor::ImageInfoDescriptor(raccoonTex, vulkanApp->getSampler()), Descriptor::ImageInfoDescriptor(leafTex, vulkanApp->getSampler()) };
	secondSubpassDescriptor->createDescriptorSets(vulkanApp->getSwapchain()->getSize(), *descriptorPool, uboDescriptors2, imgDescriptors2);

	/// Setup particles
	ParticleSystem::ParticlesConstructorParams args(ParticleRenderingMode::DeferredVRen, devices, descriptorPool, vulkanApp->getSwapchain()->getSize(),
		vulkanApp->getSwapchain()->getExtent(), renderPass, *commandPool, vulkanApp->getSampler());
	particles = new ParticleSystem(args);

}

VBufferScene::~VBufferScene() {

	DELETE(particles);

	/// Objects dependant on swapchain
	DELETE(visibilityAttachment);

	DELETE(lightBuffer);
	DELETE(matrixBuffer);
	DELETE(vertexBuffer);
#ifdef SEND_DEBUG_BUFFER_V
	DELETE(debugBuffer);
#endif

	DELETE(visibilityPipeline);
	DELETE(ppPipeline);
	DELETE(renderPass);

	/// Objects independant from swapchain
	delete vQuad;
	delete vCube;
	delete vCube2;
	delete vCube3;
	delete shrimpTex;
	delete raccoonTex;
	delete vRaymarchCube;
	delete vGround;

	delete firstSubpassDescriptor;
	delete secondSubpassDescriptor;

}

void VBufferScene::Update(uint32_t imageIndex, float dt, float time) {

	/// Setup matrices
	const glm::mat4& view = vulkanApp->getCamera().getViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), vulkanApp->getSwapchain()->getExtent().width / (float)vulkanApp->getSwapchain()->getExtent().height, NEAR, FAR);
	projection[1][1] *= -1;//fix ogl upside-down y coordinate scaling

	/// Update uniform buffers
	if (!particlesOnly)
		matrixBuffer->updateBuffer(imageIndex, time, glm::mat4(1), view, projection);

	lightBuffer->updateBuffer(imageIndex, dt, time);

	/// Update particles
	particles->Update(imageIndex, dt, time, view, projection);

#ifdef SEND_DEBUG_BUFFER_V
	debugBuffer->updateBuffer(imageIndex, { (float)debugView });//send debug data to shaders
#endif

}

bool VBufferScene::UI() {

	ImGui::Text("V-Buffer Settings");

	/// Drop-down list for debug view displayed
	static const char* debugViews[] = { "Shaded", "Visibility UV", "Primitive ID", "Material ID" };
	if (ImGui::BeginCombo("##vbufferDebugViews", debugViews[debugView])) {
		for (int i = 0; i < IM_ARRAYSIZE(debugViews); ++i) {
			bool isSelected = debugView == i;
			if (ImGui::Selectable(debugViews[i], isSelected)) {
				/// Potentially switch debug view
				debugView = i;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}// Debug views drop down

	ImGui::Checkbox("Particles Only", &particlesOnly);

	bool rebuild;
	particles = ParticleSystem::UI(particles, rebuild);
	if (rebuild) return true;

	return false;
}

RenderPass* VBufferScene::cmdBind(const VkCommandBuffer& cmdBuffer, int index) {
	renderPass->begin(cmdBuffer, vulkanApp->getSwapchain()->getFramebuffer(index)); {

		//Geometry subpass:
		{	//vkCmdFirstSubpass

			if (!particlesOnly) {
				firstSubpassDescriptor->cmdBind(cmdBuffer, index);

				visibilityPipeline->cmdBind(cmdBuffer, index);

				/// All meshes are rasterized using the same shaders.
				vQuad->getVMesh().cmdBind(cmdBuffer, index);
				vCube->getVMesh().cmdBind(cmdBuffer, index);
				vCube2->getVMesh().cmdBind(cmdBuffer, index);
				vCube3->getVMesh().cmdBind(cmdBuffer, index);
				vGround->getVMesh().cmdBind(cmdBuffer, index);
				vRaymarchCube->getVMesh().cmdBind(cmdBuffer, index);
			}

			// Particles are done separately with their own shader sets
			particles->cmdBind(cmdBuffer, index);

		}

		// Post-processing subpass:
		{	vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

		secondSubpassDescriptor->cmdBind(cmdBuffer, index);

		/// Lighting, full screen pass.
		ppPipeline->cmdBind(cmdBuffer, index);
		vkCmdDraw(cmdBuffer, 3, 1, 0, 0);// full-screen quad
		}

	}
	return renderPass;
}

void VBufferScene::cmdBindCompute(const VkCommandBuffer& cmdBuffer) {
	particles->cmdBindCompute(cmdBuffer);
}
