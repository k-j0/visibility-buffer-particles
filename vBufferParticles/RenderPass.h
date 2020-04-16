#pragma once

#include "Utils.h"
#include <vulkan/vulkan.hpp>
#include <vector>


/// Represents a single render pass.
class RenderPass {

	VkRenderPass renderPass;
	VkExtent2D extent;
	VkDevice* logicalDevice;
	int subpassCount;
	int attachmentCount;

public:

	/// The attachments that this render pass writes to. Use macros defined at bottom of this header file to create.
	struct RenderPassAttachmentDesc {
	private:
		VkAttachmentDescription _desc;
		VkImageLayout _layout;
	public:
		/// Creates the description of the render pass; keeps the image layout around for later reference
		RenderPassAttachmentDesc(VkFormat colorFormat, VkImageLayout layout, VkImageLayout finalLayout, VkAttachmentStoreOp storeOp);

		/// Shorthand for accessing the attachment descriptor
		inline VkAttachmentDescription operator()() { return _desc; }

		/// Image layout getter
		inline VkImageLayout layout() { return _layout; }
	};// struct RenderPassAttachmentDesc

	/// Creates a render pass. it is assumed that the first attachment desc is the present attachment, and the last is the depth attachment.
	RenderPass(VkDevice* logicalDevice, std::vector<RenderPassAttachmentDesc>& attachmentDescs, int subpassCount, VkExtent2D extent);
	virtual ~RenderPass();// cleanup resources.

	/// Returns the vulkan resource handle
	inline const VkRenderPass& getRenderPass() const { return renderPass; }
	/// Returns the number of subpasses used in this pass
	inline int getSubpassCount() const { return subpassCount; }

	/// Binds and unbinds the render pass at command buffer recording time
	void begin(const VkCommandBuffer& cmdBuffer, const VkFramebuffer& framebuffer);
	void end(const VkCommandBuffer& cmdBuffer);

};// class RenderPass

/// Useful default renderpass attachment descriptors
///		Final attachment to render the final colour on screen
#define RENDERPASS_ATTACHMENT_DESC_PRESENT(swapchainFormat) RenderPass::RenderPassAttachmentDesc(swapchainFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_STORE_OP_STORE)
///		Colour attachment for use in between subpasses
#define RENDERPASS_ATTACHMENT_DESC_COLOUR					RenderPass::RenderPassAttachmentDesc(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE)
///		Vector4 attachment for use in between subpasses
#define RENDERPASS_ATTACHMENT_DESC_VEC4						RenderPass::RenderPassAttachmentDesc(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE)
///		Depth attachment
#define RENDERPASS_ATTACHMENT_DESC_DEPTH					RenderPass::RenderPassAttachmentDesc(VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE)
