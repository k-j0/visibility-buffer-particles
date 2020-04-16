#include "ForwardRendererScene.h"

ForwardRendererScene::ForwardRendererScene(VulkanAppBase* vulkanApp) : Scene(vulkanApp) {

	/// Create objects that do not rely on a specific swapchain layout

	//descriptor set & pipeline layouts
	DESCRIPTOR_BINDING_ARRAY firstSubpassBindings = { DESCRIPTOR_BINDING_UBO_VERTEX, DESCRIPTOR_BINDING_SAMPLER_FRAGMENT, DESCRIPTOR_BINDING_UBO_FRAGMENT, DESCRIPTOR_BINDING_SAMPLER_FRAGMENT };
	firstSubpassDescriptor = new Descriptor(firstSubpassBindings, devices());

	// create meshes
	quad = MeshFactory::createQuadMesh(glm::vec3(-0.5f, 0, 0), glm::vec2(0.5f, 0.5f), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	cube = MeshFactory::createCubeMesh(glm::vec3(0.5f, 0, -3.0f), glm::vec3(2.5f, 2.5f, 2.5f), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	cube2 = MeshFactory::createCubeMesh(glm::vec3(2.0f, 0.3f, 2.0f), glm::vec3(1.0f, 1.5f, 1.0f), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	cube3 = MeshFactory::createCubeMesh(glm::vec3(-2.0f, 0.3f, 2.0f), glm::vec3(1.5f, 1.5f, 1.5f), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	ground = MeshFactory::createCubeMesh(glm::vec3(0, -2.5f, 0), glm::vec3(20, 0.2f, 20), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	raymarchCube = MeshFactory::createCubeMesh(glm::vec3(3.0f, 0, -2.0f), glm::vec3(2, 2, 2), devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());

	// load textures
	shrimpTex = new Texture("Textures/shrimp.png", devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());
	raccoonTex = new Texture("Textures/raccoon.png", devices(), devices->getPhysicalDevice(), *commandPool, devices->getGraphicsQueue());


	/// Create objects and layouts dependant on swapchain size
	// Create render pass & attachments
	std::vector<RenderPass::RenderPassAttachmentDesc> attachments = { RENDERPASS_ATTACHMENT_DESC_PRESENT(vulkanApp->getSwapchain()->getFormat()), RENDERPASS_ATTACHMENT_DESC_DEPTH };
	renderPass = new RenderPass(devices(), attachments, 1, vulkanApp->getSwapchain()->getExtent());

	// Create pipeline layouts
	firstSubpassDescriptor->createPipelineLayout();

	// Create pipelines
	shrimpPipeline = new GraphicsPipeline("default", "shrimp_fwd", NULL, vulkanApp->getSwapchain()->getExtent(), firstSubpassDescriptor->getPipelineLayout(), renderPass, 0, true, 1, devices());
	raymarchPipeline = new GraphicsPipeline("default", "raymarch_fwd", NULL, vulkanApp->getSwapchain()->getExtent(), firstSubpassDescriptor->getPipelineLayout(), renderPass, 0, true, 1, devices());
	raccoonPipeline = new GraphicsPipeline("default", "raccoon_fwd", NULL, vulkanApp->getSwapchain()->getExtent(), firstSubpassDescriptor->getPipelineLayout(), renderPass, 0, true, 1, devices());

	// Create uniform buffers
	lightBuffer = new LightBuffer(glm::vec3(2, 2, 2), 20, glm::vec3(1, 1, 0), glm::vec3(0.1f, 0.1f, 0.5f), vulkanApp->getSwapchain()->getSize(), devices(), devices->getPhysicalDevice());
	matrixBuffer = new MatrixBuffer(vulkanApp->getSwapchain()->getSize(), devices(), devices->getPhysicalDevice());

	// Create framebuffer attachments / note: attachment images will be prepended with present image
	std::vector<VkImageView> attachmentImages = { vulkanApp->getDepthBuffer()->getImageView() };
	vulkanApp->getSwapchain()->createFramebuffers(attachmentImages, renderPass->getRenderPass());

	// Create subpass descriptor sets
	std::vector<Descriptor::UBODescriptor> uboDescriptors1 = { Descriptor::UBODescriptor(matrixBuffer->getBuffers(), sizeof(MatrixBufferObject)), Descriptor::UBODescriptor(lightBuffer->getBuffers(), (int)sizeof(LightBufferObject)) };
	std::vector<Descriptor::ImageInfoDescriptor> imgDescriptors1 = { Descriptor::ImageInfoDescriptor(shrimpTex, vulkanApp->getSampler()), Descriptor::ImageInfoDescriptor(raccoonTex, vulkanApp->getSampler()) };
	firstSubpassDescriptor->createDescriptorSets(vulkanApp->getSwapchain()->getSize(), *descriptorPool, uboDescriptors1, imgDescriptors1);

	// Create particles
	ParticleSystem::ParticlesConstructorParams args(ParticleRenderingMode::ForwardRen, devices, descriptorPool, vulkanApp->getSwapchain()->getSize(),
		vulkanApp->getSwapchain()->getExtent(), renderPass, *commandPool, vulkanApp->getSampler());
	particles = new ParticleSystem(args);
}

ForwardRendererScene::~ForwardRendererScene() {

	DELETE(particles);

	/// Objects dependant on swapchain
	DELETE(lightBuffer);
	DELETE(matrixBuffer);

	DELETE(shrimpPipeline);
	DELETE(raymarchPipeline);
	DELETE(raccoonPipeline);

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
}

void ForwardRendererScene::Update(uint32_t imageIndex, float dt, float time) {

	/// Setup matrices
	const glm::mat4& view = vulkanApp->getCamera().getViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), vulkanApp->getSwapchain()->getExtent().width / (float)vulkanApp->getSwapchain()->getExtent().height, NEAR, FAR);
	projection[1][1] *= -1;//fix ogl upside-down y coordinate scaling

	if (!particlesOnly) {
		/// Update uniform buffers
		matrixBuffer->updateBuffer(imageIndex, time, glm::mat4(1), view, projection);
		lightBuffer->updateBuffer(imageIndex, dt, time);
	}

	/// Update particles ubos
	particles->Update(imageIndex, dt, time, view, projection);

}

bool ForwardRendererScene::UI() {

	ImGui::Text("Forward Settings");

	ImGui::Checkbox("Particles Only", &particlesOnly);

	bool rebuild;
	particles = ParticleSystem::UI(particles, rebuild);
	if (rebuild) return true;

	return false;

}

RenderPass* ForwardRendererScene::cmdBind(const VkCommandBuffer& cmdBuffer, int index) {
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

	}
	return renderPass;
}

void ForwardRendererScene::cmdBindCompute(const VkCommandBuffer& cmdBuffer) {

	particles->cmdBindCompute(cmdBuffer);

}
