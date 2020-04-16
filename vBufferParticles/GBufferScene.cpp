#include "GBufferScene.h"

GBufferScene::GBufferScene(VulkanAppBase* vulkanApp) : Scene(vulkanApp) {
	/// Create objects that do not rely on a specific swapchain layout

	//descriptor set & pipeline layouts
	DESCRIPTOR_BINDING_ARRAY firstSubpassBindings = { DESCRIPTOR_BINDING_UBO_VERTEX, DESCRIPTOR_BINDING_SAMPLER_FRAGMENT, DESCRIPTOR_BINDING_SAMPLER_FRAGMENT };
	firstSubpassDescriptor = new Descriptor(firstSubpassBindings, devices());
	DESCRIPTOR_BINDING_ARRAY secondSubpassBindings = { DESCRIPTOR_BINDING_INPUT_ATTACHMENT_FRAGMENT, DESCRIPTOR_BINDING_INPUT_ATTACHMENT_FRAGMENT, DESCRIPTOR_BINDING_INPUT_ATTACHMENT_FRAGMENT, DESCRIPTOR_BINDING_UBO_FRAGMENT };
#ifdef SEND_DEBUG_BUFFER_G3
	secondSubpassBindings.push_back(DESCRIPTOR_BINDING_UBO_FRAGMENT);
#endif
	secondSubpassDescriptor = new Descriptor(secondSubpassBindings, devices());

	// create meshes
	quad = MeshFactory::createQuadMesh(glm::vec3(-0.5f, 0, 0), glm::vec2(0.5f, 0.5f), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	cube = MeshFactory::createCubeMesh(glm::vec3(0.5f, 0, -3.0f), glm::vec3(2.5f, 2.5f, 2.5f), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	cube2 = MeshFactory::createCubeMesh(glm::vec3(2.0f, 0.3f, 2.0f), glm::vec3(1.0f, 1.5f, 1.0f), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	cube3 = MeshFactory::createCubeMesh(glm::vec3(-2.0f, 0.3f, 2.0f), glm::vec3(1.5f, 1.5f, 1.5f), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	ground = MeshFactory::createCubeMesh(glm::vec3(0, -2.5f, 0), glm::vec3(20, 0.2f, 20), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	raymarchCube = MeshFactory::createCubeMesh(glm::vec3(3.0f, 0, -2.0f), glm::vec3(2, 2, 2), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());

	// load in textures
	shrimpTex = new Texture("Textures/shrimp.png", devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	raccoonTex = new Texture("Textures/raccoon.png", devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());

	/// Create objects and layouts dependant on swapchain size
	// Create render pass & attachments
	std::vector<RenderPass::RenderPassAttachmentDesc> attachments = { RENDERPASS_ATTACHMENT_DESC_PRESENT(vulkanApp->getSwapchain()->getFormat()), RENDERPASS_ATTACHMENT_DESC_COLOUR, RENDERPASS_ATTACHMENT_DESC_VEC4, RENDERPASS_ATTACHMENT_DESC_VEC4, RENDERPASS_ATTACHMENT_DESC_DEPTH };
	renderPass = new RenderPass(devices(), attachments, 2, vulkanApp->getSwapchain()->getExtent());

	// Create pipeline layouts
	firstSubpassDescriptor->createPipelineLayout();
	secondSubpassDescriptor->createPipelineLayout();

	// Create pipelines
	shrimpPipeline = new GraphicsPipeline("default", "shrimp_g", NULL, vulkanApp->getSwapchain()->getExtent(), firstSubpassDescriptor->getPipelineLayout(), renderPass, 0, true, 3, devices());
	raymarchPipeline = new GraphicsPipeline("default", "raymarch_g", NULL, vulkanApp->getSwapchain()->getExtent(), firstSubpassDescriptor->getPipelineLayout(), renderPass, 0, true, 3, devices());
	raccoonPipeline = new GraphicsPipeline("default", "raccoon_g", NULL, vulkanApp->getSwapchain()->getExtent(), firstSubpassDescriptor->getPipelineLayout(), renderPass, 0, true, 3, devices());
	ppPipeline = new GraphicsPipeline("pp", "pp_lighting_g", NULL, vulkanApp->getSwapchain()->getExtent(), secondSubpassDescriptor->getPipelineLayout(), renderPass, 1, false, 1, devices());

	//Create attachments
	colorAttachment = new Texture(VK_FORMAT_R8G8B8A8_UNORM, vulkanApp->getSwapchain()->getExtent(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	positionAttachment = new Texture(VK_FORMAT_R16G16B16A16_SFLOAT, vulkanApp->getSwapchain()->getExtent(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	normalAttachment = new Texture(VK_FORMAT_R16G16B16A16_SFLOAT, vulkanApp->getSwapchain()->getExtent(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());

	// Create uniform buffers
	lightBuffer = new LightBuffer(glm::vec3(2, 2, 2), 20, glm::vec3(1, 1, 0), glm::vec3(0.1f, 0.1f, 0.5f), vulkanApp->getSwapchain()->getSize(), devices(), devices->getPhysicalDevice());
	matrixBuffer = new MatrixBuffer(vulkanApp->getSwapchain()->getSize(), devices(), devices->getPhysicalDevice());
#ifdef SEND_DEBUG_BUFFER_G3
	debugBuffer = new DebugBuffer(vulkanApp->getSwapchain()->getSize(), devices(), devices->getPhysicalDevice());
#endif

	// Create framebuffer attachments / note: attachment images will be prepended with present image
	std::vector<VkImageView> attachmentImages = { colorAttachment->getImageView(), positionAttachment->getImageView(), normalAttachment->getImageView(), vulkanApp->getDepthBuffer()->getImageView() };
	vulkanApp->getSwapchain()->createFramebuffers(attachmentImages, renderPass->getRenderPass());

	// Create subpass descriptor sets
	std::vector<Descriptor::UBODescriptor> uboDescriptors1 = { Descriptor::UBODescriptor(matrixBuffer->getBuffers(), sizeof(MatrixBufferObject)) };
	std::vector<Descriptor::ImageInfoDescriptor> imgDescriptors1 = { Descriptor::ImageInfoDescriptor(shrimpTex, vulkanApp->getSampler()), Descriptor::ImageInfoDescriptor(raccoonTex, vulkanApp->getSampler()) };
	firstSubpassDescriptor->createDescriptorSets(vulkanApp->getSwapchain()->getSize(), *descriptorPool, uboDescriptors1, imgDescriptors1);
	std::vector<Descriptor::UBODescriptor> uboDescriptors2 = { Descriptor::UBODescriptor(lightBuffer->getBuffers(), (int)sizeof(LightBufferObject)) };
#ifdef SEND_DEBUG_BUFFER_G3
	uboDescriptors2.push_back(Descriptor::UBODescriptor(debugBuffer->getBuffers(), (int)sizeof(DebugBufferObject)));
#endif
	std::vector<Descriptor::ImageInfoDescriptor> imgDescriptors2 = { DESCRIPTOR_IMG_ATTACHMENT_INFO(colorAttachment), DESCRIPTOR_IMG_ATTACHMENT_INFO(positionAttachment), DESCRIPTOR_IMG_ATTACHMENT_INFO(normalAttachment) };
	secondSubpassDescriptor->createDescriptorSets(vulkanApp->getSwapchain()->getSize(), *descriptorPool, uboDescriptors2, imgDescriptors2);

	/// Setup particles
	ParticleSystem::ParticlesConstructorParams args(ParticleRenderingMode::DeferredG3Ren, devices, descriptorPool, vulkanApp->getSwapchain()->getSize(),
		vulkanApp->getSwapchain()->getExtent(), renderPass, *commandPool, vulkanApp->getSampler());
	particles = new ParticleSystem(args);

}

GBufferScene::~GBufferScene() {

	DELETE(particles);

	/// Objects dependant on swapchain
	DELETE(colorAttachment);
	DELETE(positionAttachment);
	DELETE(normalAttachment);

	DELETE(lightBuffer);
	DELETE(matrixBuffer);
#ifdef SEND_DEBUG_BUFFER_G3
	DELETE(debugBuffer);
#endif

	DELETE(shrimpPipeline);
	DELETE(raymarchPipeline);
	DELETE(raccoonPipeline);
	DELETE(ppPipeline);

	DELETE(renderPass);

	/// Objects independant from swapchain
	delete quad;
	delete cube;
	delete cube2;
	delete cube3;
	delete shrimpTex;
	delete raccoonTex;
	delete raymarchCube;
	delete ground;

	delete firstSubpassDescriptor;
	delete secondSubpassDescriptor;

}

void GBufferScene::Update(uint32_t imageIndex, float dt, float time) {

	/// Setup matrices
	const glm::mat4& view = vulkanApp->getCamera().getViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), vulkanApp->getSwapchain()->getExtent().width / (float)vulkanApp->getSwapchain()->getExtent().height, NEAR, FAR);
	projection[1][1] *= -1;//fix ogl upside-down y coordinate scaling

	if (!particlesOnly) {
		/// Update uniform buffers
		matrixBuffer->updateBuffer(imageIndex, time, glm::mat4(1), view, projection);
	}
	lightBuffer->updateBuffer(imageIndex, dt, time);

#ifdef SEND_DEBUG_BUFFER_G3
	debugBuffer->updateBuffer(imageIndex, { (float)debugView });//send debug data to shaders
#endif

		/// Update particle ubos
	particles->Update(imageIndex, dt, time, view, projection);

}

bool GBufferScene::UI() {

	ImGui::Text("G-Buffer Settings");

	/// Drop-down list for debug view displayed
	static const char* debugViews[] = { "Shaded", "Depth", "Albedo", "WS Position", "WS Normal" };
	if (ImGui::BeginCombo("##gbufferDebugViews", debugViews[debugView])) {
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

RenderPass* GBufferScene::cmdBind(const VkCommandBuffer& cmdBuffer, int index) {
	renderPass->begin(cmdBuffer, vulkanApp->getSwapchain()->getFramebuffer(index)); {

		//Geometry subpass:
		{	//vkCmdFirstSubpass

			if (!particlesOnly) {
				firstSubpassDescriptor->cmdBind(cmdBuffer, index);

				// shrimp-textured objects
				shrimpPipeline->cmdBind(cmdBuffer, index);
				quad->cmdBind(cmdBuffer, index);
				cube->cmdBind(cmdBuffer, index);
				cube2->cmdBind(cmdBuffer, index);
				ground->cmdBind(cmdBuffer, index);

				// raccoon-textured objects
				raccoonPipeline->cmdBind(cmdBuffer, index);
				cube3->cmdBind(cmdBuffer, index);

				// raymarch animated objects
				raymarchPipeline->cmdBind(cmdBuffer, index);
				raymarchCube->cmdBind(cmdBuffer, index);
			}

			// particles
			particles->cmdBind(cmdBuffer, index);

		}

		// Post-processing subpass:
		{	vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

		secondSubpassDescriptor->cmdBind(cmdBuffer, index);

		// clip space quad for post-processing of G-Buffer data
		ppPipeline->cmdBind(cmdBuffer, index);
		vkCmdDraw(cmdBuffer, 3, 1, 0, 0);// full-screen quad
		}

	}
	return renderPass;
}

void GBufferScene::cmdBindCompute(const VkCommandBuffer& cmdBuffer) {
	particles->cmdBindCompute(cmdBuffer);
}
