#include <hpg/Material.h>
#include <hpg/Shader.h>

#include <common/Vertex.h>
#include <common/vkinit.h>
#include <common/Print.h>

void Material::createPipeline(Renderer& renderer, kDescriptorSetLayout type) {
	// create pipeline layout
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(1, 
            &renderer._descriptorSetLayouts[type]);

        if (vkCreatePipelineLayout(renderer._context.device, &pipelineLayoutCreateInfo, nullptr, &_pipelineLayout) 
            != VK_SUCCESS) {
            throw std::runtime_error("Could not create deferred pipeline layout!");
        }
    }

    // create pipeline
    {
        VkColorComponentFlags colBlendAttachFlag =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachmentStates = {
            vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE),
            vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE),
            vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE),
            vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE)
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
            vkinit::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

        VkPipelineRasterizationStateCreateInfo rasterizerStateCreateInfo =
            vkinit::pipelineRasterStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, 
                VK_FRONT_FACE_COUNTER_CLOCKWISE);

        VkPipelineColorBlendStateCreateInfo    colorBlendingStateCreateInfo =
            vkinit::pipelineColorBlendStateCreateInfo(static_cast<UI32>(colorBlendAttachmentStates.size()), 
                colorBlendAttachmentStates.data());

        VkPipelineDepthStencilStateCreateInfo  depthStencilStateCreateInfo =
            vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

        VkPipelineViewportStateCreateInfo      viewportStateCreateInfo =
            vkinit::pipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);

        VkPipelineMultisampleStateCreateInfo   multisamplingStateCreateInfo =
            vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

        // dynamic view port for resizing
        VkDynamicState dynamicStateEnables[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = 
            vkinit::pipelineDynamicStateCreateInfo(dynamicStateEnables, 2);

        auto bindingDescription = Vertex::getBindingDescriptions(0);
        auto attributeDescriptions = Vertex::getAttributeDescriptions(0);

        VkPipelineVertexInputStateCreateInfo   vertexInputStateCreateInfo =
            vkinit::pipelineVertexInputStateCreateInfo(1, &bindingDescription,
                static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

        VkShaderModule vertShaderModule, fragShaderModule;
        vertShaderModule = Shader::createShaderModule(&renderer._context, Shader::readFile(kShaders[type].first));
        fragShaderModule = Shader::createShaderModule(&renderer._context, Shader::readFile(kShaders[type].second));
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{
            vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main"),
            vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main")
        };

        VkGraphicsPipelineCreateInfo pipelineCreateInfo =
            vkinit::graphicsPipelineCreateInfo(_pipelineLayout, renderer._renderPass, OFFSCREEN_SUBPASS);

        pipelineCreateInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
        pipelineCreateInfo.pStages             = shaderStages.data();
        pipelineCreateInfo.pVertexInputState   = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pDynamicState       = &dynamicStateCreateInfo;
        pipelineCreateInfo.pMultisampleState   = &multisamplingStateCreateInfo;
        pipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState  = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState    = &colorBlendingStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizerStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;

        if (vkCreateGraphicsPipelines(renderer._context.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, 
            &_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Could not create graphics pipeline!");
        }

        vkDestroyShaderModule(renderer._context.device, vertShaderModule, nullptr);
        vkDestroyShaderModule(renderer._context.device, fragShaderModule, nullptr);
    }
}