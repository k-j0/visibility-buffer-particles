#include "Particles.h"

#include "StaticSettings.h"


ParticleSystemSettings ParticleSystem::settings = ParticleSystemSettings();




bool ParticleSystem::setParticlesComplexity(int complexity, bool noRecompile) {

	if (complexity == ParticleSystem::settings.complexity) return false; // nothing to change :)

	if (complexity < 0 || complexity > 3) throw std::runtime_error("Out of bounds: particle complexity should be 0, 1, 2 or 3.");

	ParticleSystem::settings.complexity = complexity;

	// Change __.defines to mirror the new complexity.
	std::string definesContents = U::readFileStr("__.defines");
	std::vector<std::string> splitDefinesContents = U::splitStr("PARTICLE_COMPLEXITY_", definesContents);
	if (splitDefinesContents.size() != 2) throw std::runtime_error("Could not modify __.defines to recompile shaders for particles complexity.");
	std::string newDefinesContents = splitDefinesContents[0] + "PARTICLE_COMPLEXITY_" + std::to_string(complexity) + splitDefinesContents[1].substr(1);
	U::writeFile("__.defines", newDefinesContents);
	printf(("Wrote to __.defines: #define PARTICLE_COMPLEXITY_"+std::to_string(complexity)+".\n").c_str());

	// if complexity != 2, cutout should be false always
	if (complexity != 2)
		setParticlesCutout(false, !noRecompile); // no need to recompile from setParticlesCutout as we're already going to recompile below.

	// Recompile shaders
	if (!noRecompile) {
		CompileShader("Shaders/comp_particles_fwd.frag");
		CompileShader("Shaders/comp_particles_g3.frag");
		CompileShader("Shaders/comp_particles_g6.frag");
		CompileShader("Shaders/comp_particles_v.frag");
		CompileShader("Shaders/particles_fwd.frag");
		CompileShader("Shaders/particles_g3.frag");
		CompileShader("Shaders/particles_g6.frag");
		CompileShader("Shaders/particles_v.frag");
		CompileShader("Shaders/pp_lighting_v.frag");
	}

	// Force rebuilding pipelines & swapchain (using newly compiled shaders)
	return true;
}

bool ParticleSystem::setParticlesCutout(bool cutout, bool noRecompile) {

	if (cutout == ParticleSystem::settings.cutout) return false;// nothing to change!

	ParticleSystem::settings.cutout = cutout;

	// Change __.defines to mirror the new cutout style
	std::string definesContents = U::readFileStr("__.defines");
	std::vector<std::string> splitDefinesContents = U::splitStr("PARTICLE_CUTOUT_MODE_", definesContents);
	if (splitDefinesContents.size() != 2) throw std::runtime_error("Could not modify __.defines to recompile shaders for particles complexity.");
	std::string cutoutDef = (cutout ? "1" : "0");
	std::string newDefinesContents = splitDefinesContents[0] + "PARTICLE_CUTOUT_MODE_" + cutoutDef + splitDefinesContents[1].substr(1);
	U::writeFile("__.defines", newDefinesContents);
	printf(("Wrote to __.defines: #define PARTICLE_CUTOUT_MODE_" + cutoutDef + ".\n").c_str());

	// if we're setting cutout to true, we need to make sure complexity is set to 2 (textured)
	if (cutout) {
		setParticlesComplexity(2, !noRecompile);
	}

	// Recompile shaders
	if (!noRecompile) {
		CompileShader("Shaders/comp_particles_fwd.frag");
		CompileShader("Shaders/comp_particles_g3.frag");
		CompileShader("Shaders/comp_particles_g6.frag");
		CompileShader("Shaders/comp_particles_v.frag");
		CompileShader("Shaders/particles_fwd.frag");
		CompileShader("Shaders/particles_g3.frag");
		CompileShader("Shaders/particles_g6.frag");
		CompileShader("Shaders/particles_v.frag");
		CompileShader("Shaders/pp_lighting_v.frag");
	}

	// Force rebuilding pipelines & swapchain (using newly compiled shaders)
	return true;
}

ParticleSystem::ParticleSystem(ParticlesConstructorParams& args) : params(args) {

	// lazy init pattern:
	static bool firstTime = true;
	if (firstTime) {
		firstTime = false;
		if (RC_SETTINGS) {
			settings.complexity = RC_SETTINGS->pComplexity;
			settings.density = RC_SETTINGS->pSpread;
			settings.genMode = RC_SETTINGS->pMode == RuntimeConstantSettings::ParticleMode::Co ? ParticleGenerationMode::ComputeGenExp :
							   RC_SETTINGS->pMode == RuntimeConstantSettings::ParticleMode::Ge ? ParticleGenerationMode::GeometryGenExp :
							   RC_SETTINGS->pMode == RuntimeConstantSettings::ParticleMode::Ve ? ParticleGenerationMode::VertexGenExp :
							   ParticleGenerationMode::VertexGenGeometryExp;
			settings.halfSize = RC_SETTINGS->pHalfSize;
			settings.particleCount = RC_SETTINGS->pCount;
		}
	}// only executes first time around.

	renMode = args.rMode;
	particlesUBO.particleCount = settings.particleCount;
	particlesUBO.density = settings.density;
	particlesUBO.gravity = settings.gravity;
	particlesUBO.halfSize = settings.halfSize;
	particlesUBO.initialUpwardsForce = settings.initialUpwardsForce;
	this->devices = args.devices;

	/// Ensure static Complexity field is the same as what we expect from the __.defines file.
	if (settings.complexity == UNDEFINED_PARTICLE_COMPLEXITY) {
		std::string definesContents = U::readFileStr("__.defines");
		std::vector<std::string> splitDefinesContents = U::splitStr("PARTICLE_COMPLEXITY_", definesContents);
		if (splitDefinesContents.size() != 2 || splitDefinesContents[1].length() < 1) throw std::runtime_error("Could not access __.defines to set up particle system.");
		std::string c = splitDefinesContents[1].substr(0, 1);// first char after PARTICLE_COMPLEXITY
		settings.complexity = std::stoi("" + c);
		printf(("Read complexity from __.defines as: " + std::to_string(settings.complexity) + "\n").c_str());
	}

	// Select different options based on rendering mode
	int outputAttachmentCount =	renMode == ParticleRenderingMode::DeferredG3Ren ?	3 :
								renMode == ParticleRenderingMode::DeferredG6Ren ?	6 :
																					1;
	// determine which fragment shader to use to render the particles in the first subpass, depending on modes.
	std::string frag =	renMode == ParticleRenderingMode::DeferredG3Ren ?	"particles_g3" :
						renMode == ParticleRenderingMode::DeferredG6Ren ?	"particles_g6" :
						renMode == ParticleRenderingMode::DeferredVRen ?	"particles_v" :
																			"particles_fwd";
	if (settings.genMode == ParticleGenerationMode::ComputeGenExp)
		frag = "comp_" + frag; // fragment shader will need slight changes as textures aren't bound in the same locations.

	std::vector<Descriptor::ImageInfoDescriptor> imageDescriptors = {};
	bool uploadTexture = settings.complexity == 2 && (settings.cutout || renMode != ParticleRenderingMode::DeferredVRen); // in V-Buffer rendering, no need to upload the texture in the particle pass - texturing is done in the lighting pass (which already has the texture)
	if (uploadTexture) {
		particlesTexture = new Texture(settings.cutout ? "Textures/leaf.png" : "Textures/shrimp.png", devices(), devices->getPhysicalDevice(), args.commandPool, devices->getGraphicsQueue());
		imageDescriptors.push_back(Descriptor::ImageInfoDescriptor(particlesTexture, args.sampler));
	}



	// setup differently based on mode:

	if (settings.genMode == ParticleGenerationMode::ComputeGenExp) {

		printf("Creating ComputeGenExp particles.\n");

		// we'll need the compute fields.
		computeFields = new ComputeFields;

		// Compute pipeline
		uboBuffer = new UniformBuffer<ParticlesUBO>(1, devices(), devices->getPhysicalDevice());
		computeFields->ssboBuffer = new UniformBuffer<ComputeSSBO>(1, devices(), devices->getPhysicalDevice(),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,	// usage as an SSBO for compute, and as a VBO for the vertex shader that uses that data
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,	// on the GPU
			settings.particleCount * 6		// amount of vertices that will need to be passed from Compute to Vertex shader.
			);// SSBO setup
		DESCRIPTOR_BINDING_ARRAY computeBindings = { DESCRIPTOR_BINDING_UBO_COMPUTE, DESCRIPTOR_BINDING_STORAGE_BUFFER_COMPUTE };
		computeFields->descriptor = new Descriptor(computeBindings, devices(), VK_PIPELINE_BIND_POINT_COMPUTE);
		computeFields->descriptor->createPipelineLayout();
		computeFields->descriptor->createDescriptorSets(1, *args.descriptorPool, {
							Descriptor::UBODescriptor(uboBuffer->getBuffers(), sizeof(ParticlesUBO)),						// Uniform buffer
							Descriptor::UBODescriptor(computeFields->ssboBuffer->getBuffers(), sizeof(ComputeSSBO) * settings.particleCount * 6)		// Storage buffer
			}, {/* no samplers */ });
		computeFields->pipeline = new ComputePipeline("particles", computeFields->descriptor->getPipelineLayout(), devices());

		// Graphics pipeline
		DESCRIPTOR_BINDING_ARRAY particlesBindings = {};
		if (uploadTexture) particlesBindings.push_back(DESCRIPTOR_BINDING_SAMPLER_FRAGMENT);
		graphicsDescriptor = new Descriptor(particlesBindings, devices());
		graphicsDescriptor->createPipelineLayout();
		graphicsDescriptor->createDescriptorSets(args.swapchainSize, *args.descriptorPool, {}, imageDescriptors);
		graphicsPipeline = new GraphicsPipeline("particles_fwd", frag, NULL, args.swapchainExtent, graphicsDescriptor->getPipelineLayout(), args.renderPass, 0, true, outputAttachmentCount, devices());

	} else if (settings.genMode == ParticleGenerationMode::VertexGenExp) {

		printf("Creating VertexGenExp particles.\n");

		// Graphics pipeline setup
		DESCRIPTOR_BINDING_ARRAY particlesBindings = { DESCRIPTOR_BINDING_UBO_VERTEX };
		if(uploadTexture) particlesBindings.push_back(DESCRIPTOR_BINDING_SAMPLER_FRAGMENT);
		graphicsDescriptor = new Descriptor(particlesBindings, devices());
		graphicsDescriptor->createPipelineLayout();
		uboBuffer = new UniformBuffer<ParticlesUBO>(args.swapchainSize, devices(), devices->getPhysicalDevice());
		graphicsDescriptor->createDescriptorSets(args.swapchainSize, *args.descriptorPool, { Descriptor::UBODescriptor(uboBuffer->getBuffers(), sizeof(ParticlesUBO)) }, imageDescriptors);
		graphicsPipeline = new NulTriangleGraphicsPipeline("vert_particles_fwd", frag, NULL, args.swapchainExtent, graphicsDescriptor->getPipelineLayout(), args.renderPass, 0, true, outputAttachmentCount, devices());
		vertexBufferMesh = new Mesh_Base<NulVertex>({ NulVertex() }, { 0 }, devices(), devices->getPhysicalDevice(), args.commandPool, devices->getGraphicsQueue());
		vertexBufferMesh->bindOnlyVertexBuffer = true;

	} else if (settings.genMode == ParticleGenerationMode::GeometryGenExp) {

		printf("Creating GeometryGenExp particles.\n");

		// Graphics pipeline setup
		DESCRIPTOR_BINDING_ARRAY particlesBindings = { DESCRIPTOR_BINDING_UBO_GEOMETRY };
		if (uploadTexture) particlesBindings.push_back(DESCRIPTOR_BINDING_SAMPLER_FRAGMENT);
		graphicsDescriptor = new Descriptor(particlesBindings, devices());
		graphicsDescriptor->createPipelineLayout();
		std::vector<Descriptor::UBODescriptor> particlesUBODescriptors = {};
		uboBuffer = new UniformBuffer<ParticlesUBO>(args.swapchainSize, devices(), devices->getPhysicalDevice());
		particlesUBODescriptors.push_back(Descriptor::UBODescriptor(uboBuffer->getBuffers(), sizeof(ParticlesUBO)));
		graphicsDescriptor->createDescriptorSets(args.swapchainSize, *args.descriptorPool, particlesUBODescriptors, imageDescriptors);
		std::string gsParts = "particles";
		graphicsPipeline = new NulPointGraphicsPipeline("geom_particles_fwd", frag, &gsParts, args.swapchainExtent, graphicsDescriptor->getPipelineLayout(), args.renderPass, 0, true, outputAttachmentCount, devices());
		vertexBufferMesh = new Mesh_Base<NulVertex>({ NulVertex() }, { 0 }, devices(), devices->getPhysicalDevice(), args.commandPool, devices->getGraphicsQueue());
		vertexBufferMesh->bindOnlyVertexBuffer = true;

	} else if (settings.genMode == ParticleGenerationMode::VertexGenGeometryExp) {

		printf("Creating VertexGenGeometryExp particles.\n");

		// Graphics pipeline setup
		DESCRIPTOR_BINDING_ARRAY particlesBindings = { DESCRIPTOR_BINDING_UBO_VERTEX };
		if (uploadTexture) particlesBindings.push_back(DESCRIPTOR_BINDING_SAMPLER_FRAGMENT);
		graphicsDescriptor = new Descriptor(particlesBindings, devices());
		graphicsDescriptor->createPipelineLayout();
		std::vector<Descriptor::UBODescriptor> particlesUBODescriptors = {};
		uboBuffer = new UniformBuffer<ParticlesUBO>(args.swapchainSize, devices(), devices->getPhysicalDevice());
		particlesUBODescriptors.push_back(Descriptor::UBODescriptor(uboBuffer->getBuffers(), sizeof(ParticlesUBO)));
		graphicsDescriptor->createDescriptorSets(args.swapchainSize, *args.descriptorPool, particlesUBODescriptors, imageDescriptors);
		std::string gsParts = "quadexpand";
		graphicsPipeline = new NulPointGraphicsPipeline("vertgeom_particles_fwd", frag, &gsParts, args.swapchainExtent, graphicsDescriptor->getPipelineLayout(), args.renderPass, 0, true, outputAttachmentCount, devices());
		vertexBufferMesh = new Mesh_Base<NulVertex>({ NulVertex() }, { 0 }, devices(), devices->getPhysicalDevice(), args.commandPool, devices->getGraphicsQueue());
		vertexBufferMesh->bindOnlyVertexBuffer = true;

	} else {
		throw std::runtime_error("Cannot use particles gen mode: unimplemented mode.");
	}

}

ParticleSystem::~ParticleSystem() {

	/// Cleanup of all resources.

	if (computeFields) {
		DELETE(computeFields->pipeline);
		DELETE(computeFields->descriptor);
		DELETE(computeFields->ssboBuffer);
		DELETE(computeFields);
	}

	DELETE(graphicsPipeline);
	DELETE(uboBuffer);
	DELETE(graphicsDescriptor);
	DELETE(vertexBufferMesh);
	DELETE(particlesTexture);

}

void ParticleSystem::Update(uint32_t imageIndex, float dt, float time, const glm::mat4& view, const glm::mat4& proj) {

	/// Check whether the UBO should be sent.
	if (particlesUBO.time == time && particlesUBO.view == view && particlesUBO.proj == proj) ++uboNoUpdateCount;
	else uboNoUpdateCount = 0;
	if (uboNoUpdateCount > uboBuffer->getBuffers().size()) return;// nothing to update.

	/// Update UBO.
	particlesUBO.time = time;
	particlesUBO.view = view;
	particlesUBO.proj = proj;

	/// Send to required shader(s).
	if (settings.genMode == ParticleGenerationMode::ComputeGenExp) {
		uboBuffer->copyBuffer(0, particlesUBO); // image index doesn't matter in Compute; there is only one UBO.
	} else {
		uboBuffer->copyBuffer(imageIndex, particlesUBO);
	}

}

void ParticleSystem::cmdBind(const VkCommandBuffer& cmdBuffer, int index) {

	graphicsDescriptor->cmdBind(cmdBuffer, index);
	graphicsPipeline->cmdBind(cmdBuffer, index);

	if (settings.genMode == ParticleGenerationMode::ComputeGenExp) {
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &computeFields->ssboBuffer->getBuffers()[0], offsets);
		vkCmdDraw(cmdBuffer, settings.particleCount * 6, 1, 0, 0);// 6 vertices / particle quad.
	} else if (settings.genMode == ParticleGenerationMode::VertexGenExp) {
		vertexBufferMesh->cmdBind(cmdBuffer, index);
		vkCmdDraw(cmdBuffer, settings.particleCount * 6, 1, 0, 0);// one call/vertex -> inconvenience of generating the same particle 6 times instead of once.
	} else if (settings.genMode == ParticleGenerationMode::GeometryGenExp) {
		int invocations = settings.particleCount / GEOMETRY_OUTPUT_PARTICLES_PER_VERTEX;
		if ((float)invocations != (float)settings.particleCount / GEOMETRY_OUTPUT_PARTICLES_PER_VERTEX) ++invocations;// need one more invocation to cover all particles
		vertexBufferMesh->cmdBind(cmdBuffer, index);
		vkCmdDraw(cmdBuffer, invocations, 1, 0, 0);
	} else if (settings.genMode == ParticleGenerationMode::VertexGenGeometryExp) {
		vertexBufferMesh->cmdBind(cmdBuffer, index);
		vkCmdDraw(cmdBuffer, settings.particleCount, 1, 0, 0);
	} else {
		throw std::runtime_error("Cannot cmd bind with unimplemented particles gen mode.");
	}

}

void ParticleSystem::cmdBindCompute(const VkCommandBuffer& cmdBuffer) {
	if (settings.genMode == ParticleGenerationMode::ComputeGenExp) {

		/// Add memory barrier for the graphics shaders to fetch attribs before writing to compute buffer
		VkBufferMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.pNext = NULL;
		barrier.buffer = computeFields->ssboBuffer->getBuffers()[0];
		barrier.size = sizeof(ComputeSSBO);
		barrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.srcQueueFamilyIndex = devices->getGraphicsQueueFamily();
		barrier.dstQueueFamilyIndex = devices->getComputeQueueFamily();
		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 1, &barrier, 0, NULL);

		/// Dispatch command buffer
		computeFields->descriptor->cmdBind(cmdBuffer, 0);
		computeFields->pipeline->cmdBind(cmdBuffer, 0);
		int invocations = settings.particleCount / 256;
		if ((float)invocations != (float)settings.particleCount / 256.f) ++invocations;// need one more invocation to cover all particles
		vkCmdDispatch(cmdBuffer, invocations, 1, 1);

		// Ensure compute shader has finished writing to the buffer
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		barrier.srcQueueFamilyIndex = devices->getComputeQueueFamily();
		barrier.dstQueueFamilyIndex = devices->getGraphicsQueueFamily();
		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 1, &barrier, 0, NULL);

	}// in other generation modes, nothing to query the Compute pipeline.
}

ParticleSystem* ParticleSystem::UI(ParticleSystem* particles, bool& rebuild) {

	rebuild = false;

	static bool showParticlesUI = true;
	ImGui::Checkbox("Show Particles Settings", &showParticlesUI);
	if (!showParticlesUI) return particles;


	/// Complexity selection
	int complexity = ParticleSystem::settings.complexity;
	ImGui::SliderInt("Particle Complexity", &complexity, 0, 3);
	if (setParticlesComplexity(complexity)) {
		rebuild = true;// force a swapchain rebuild to use newly compiled shaders
	}

	/// Cutout or not
	bool cutout = ParticleSystem::settings.cutout;
	ImGui::Checkbox("Use Cutout Particles", &cutout);
	if (setParticlesCutout(cutout)) {
		rebuild = true;// force a swapchain rebuild to use newly compiled shaders
	}
	
	/// Drop-down list for gen mode
	static const char* genModes[] = { "VertexGenExp", "ComputeGenExp", "GeometryGenExp", "VertexGenGeometryExp" };
	int currentId = (int)particles->getGenMode();
	if (ImGui::BeginCombo("GenMode##pgenmode", genModes[currentId])) {
		for (int i = 0; i < IM_ARRAYSIZE(genModes); ++i) {
			bool isSelected = currentId == i;
			if (ImGui::Selectable(genModes[i], isSelected)) {
				if (!isSelected) {
					// select this new gen mode.
					ParticlesConstructorParams args = particles->getConstructorParams();
					vkDeviceWaitIdle(*particles->getDevices()());
					delete particles;
					settings.genMode = (ParticleGenerationMode)i;
					particles = new ParticleSystem(args);
				}
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}// Particles gen mode dropdown.

	/// Particle count editor
	int pCount = particles->getParticleCount();
	ImGui::SliderInt("Count##particlecount", &pCount, 16, 1024*1024*4);
	if (pCount != particles->getParticleCount()) {
		// select this new particle count.
		ParticlesConstructorParams args = particles->getConstructorParams();
		vkDeviceWaitIdle(*particles->getDevices()());
		delete particles;
		settings.particleCount = pCount;
		particles = new ParticleSystem(args);
	}// particle count edit

	if(ImGui::SliderFloat("Particle Half Size", &settings.halfSize, 0.005f, 0.5f))
		particles->particlesUBO.halfSize = settings.halfSize;

	if(ImGui::SliderFloat("Particle Spread", &settings.density, 0.005f, 1.0f))
		particles->particlesUBO.density = settings.density;

	if (ImGui::SliderFloat("Gravity", &settings.gravity, 0, 2))
		particles->particlesUBO.gravity = settings.gravity;

	if (ImGui::SliderFloat("Initial Upwards Force", &settings.initialUpwardsForce, 0, 2))
		particles->particlesUBO.initialUpwardsForce = settings.initialUpwardsForce;

	return particles;// may be a new pointer now, or unchanged (most likely).

}
