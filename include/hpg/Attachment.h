///////////////////////////////////////////////////////
// Attachment class declaration
///////////////////////////////////////////////////////

#ifndef ATTACHMENT_H
#define ATTACHMENT_H

#include <vulkan/vulkan_core.h>

class Attachment {
public:
	Attachment() : _vkImage(nullptr), _format(VK_FORMAT_UNDEFINED), _view(nullptr), _memory(nullptr) {}

	VkImage getImage();
	VkImageView getImageView();


private:
	VkImage _vkImage;
	VkFormat _format;

	VkImageView _view;

	VkDeviceMemory _memory;

};

#endif // !ATTACHMENT_H

