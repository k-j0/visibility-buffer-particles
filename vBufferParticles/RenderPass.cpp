#include "RenderPass.h"


/// Creates the descriptor for a single render pass attachment image.
RenderPass::RenderPassAttachmentDesc::RenderPassAttachmentDesc(VkFormat colorFormat, VkImageLayout layout, VkImageLayout finalLayout, VkAttachmentStoreOp storeOp) {

	_layout = layout;

	_desc = {};
	_desc.format = colorFormat;
	_desc.samples = VK_SAMPLE_COUNT_1_BIT;
	_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	_desc.storeOp = storeOp;
	_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	_desc.finalLayout = finalLayout;

}



/// Creates a render pass, given the attachments that will be accessible to it and the number of subpasses that should be created.
RenderPass::RenderPass(VkDevice* logicalDevice, std::vector<RenderPassAttachmentDesc>& attachmentDescs, int subpassCount, VkExtent2D extent) : logicalDevice(logicalDevice), extent(extent), subpassCount(subpassCount) {

	assert(attachmentDescs.size() >= 2);

	attachmentCount = (int)attachmentDescs.size();

	// Read attachments + references from descriptions

	std::vector<VkAttachmentDescription> attachments = {};
	std::vector<VkAttachmentReference> attachmentRefs = {};
	for (int i = 0; i < attachmentDescs.size(); ++i) {
		attachments.push_back(attachmentDescs[i]());
		VkAttachmentReference ref = {};
		ref.attachment = i;
		ref.layout = attachmentDescs[i].layout();
		attachmentRefs.push_back(ref);
	}

	// the initial one is the present attachment, and the last one the depth attachment
	VkAttachmentDescription presentDesc = attachments[0];
	VkAttachmentReference presentRef = attachmentRefs[0];
	VkAttachmentDescription depthDesc = attachments[attachments.size() - 1];
	VkAttachmentReference depthRef = attachmentRefs[attachmentRefs.size() - 1];
	std::vector<VkAttachmentReference> colourRefs = {};
	for (int i = 1; i < attachmentRefs.size() - 1; ++i) colourRefs.push_back(attachmentRefs[i]);//copy over the attachment references for the rest of the attachments
	//input refs for last subpass will be all attachments, except their layout will be transitioned to SHADER_READ_ONLY_OPTIMAL for reading as input attachments
	std::vector<VkAttachmentReference> inputRefs = {};
	for (int i = 1; i < attachmentRefs.size() - 1; ++i) {
		VkAttachmentReference r = attachmentRefs[i];
		r.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		inputRefs.push_back(r);
	}


	// Create subpasses

	std::vector<VkSubpassDescription> subpassDescs = {};
	std::vector<VkSubpassDependency> subpassDependencies = {};

	for (int i = 0; i < subpassCount; ++i) {

		//Subpass description
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		if(i != subpassCount-1) {
			subpass.colorAttachmentCount = (uint32_t)colourRefs.size();
			subpass.pColorAttachments = colourRefs.data();
			subpass.pDepthStencilAttachment = &depthRef;
		} else {// last subpass
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &presentRef;
			if (subpassCount == 1) {// if it's the only subpass, it needs depth
				subpass.pDepthStencilAttachment = &depthRef;
			} else {// if it's the last of several subpasses, it needs the input attachments and no depth
				subpass.inputAttachmentCount = (uint32_t)inputRefs.size();
				subpass.pInputAttachments = inputRefs.data();
			}
		}
		subpassDescs.push_back(subpass);

		if (subpassCount > 1) {
			// Subpass dependency for transitioning to the next subpass
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = i == 0 ? VK_SUBPASS_EXTERNAL : i - 1;
			dependency.dstSubpass = i;
			dependency.srcStageMask = i == 0 ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = i != subpassCount - 1 ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependency.srcAccessMask = i == 0 ? VK_ACCESS_MEMORY_READ_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency.dstAccessMask = i != subpassCount - 1 ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			subpassDependencies.push_back(dependency);
		}

	}

	if (subpassCount > 1) {
		//The final dependency for transitioning out
		VkSubpassDependency finalDependency = {};
		finalDependency.srcSubpass = 0;
		finalDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		finalDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		finalDependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		finalDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		finalDependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		finalDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		subpassDependencies.push_back(finalDependency);
	} else {//only one subpass dependency if there is only one subpass
		VkSubpassDependency onlyDependency = {};
		onlyDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		onlyDependency.dstSubpass = 0;
		onlyDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		onlyDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		onlyDependency.srcAccessMask = 0;
		onlyDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}


	// Create render pass vk res.

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = (uint32_t)attachments.size();
	info.pAttachments = attachments.data();
	info.subpassCount = (uint32_t)subpassDescs.size();
	info.pSubpasses = subpassDescs.data();
	info.dependencyCount = (uint32_t)subpassDependencies.size();
	info.pDependencies = subpassDependencies.data();

	if (vkCreateRenderPass(*logicalDevice, &info, NULL, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Cannot create render pass!");
	}

}

// cleanup
RenderPass::~RenderPass(){
	vkDestroyRenderPass(*logicalDevice, renderPass, NULL);
}

/// Binds the render pass to the command buffer
void RenderPass::begin(const VkCommandBuffer& cmdBuffer, const VkFramebuffer& framebuffer) {

	// Clear framebuffers
	std::vector<VkClearValue> clearValues = {};
	clearValues.resize(attachmentCount);
	for (int i = 0; i < clearValues.size() - 1; ++i) clearValues[i].color = { 0.0f, 0.0f, 0.0f, 0.0f }; //clear colour is always full black, 0 alpha regardless of the attachment.
	clearValues[attachmentCount-1].depthStencil = { 1.0f, 0 };// clear depth buffer to maximum depth.


	// Begin render pass
	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = renderPass;
	info.framebuffer = framebuffer;
	info.renderArea.offset = { 0, 0 };
	info.renderArea.extent = extent;
	info.clearValueCount = (uint32_t)clearValues.size();
	info.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(cmdBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

}

/// Stop recording commands specific to this render pass
void RenderPass::end(const VkCommandBuffer& cmdBuffer) {
	vkCmdEndRenderPass(cmdBuffer);
}
