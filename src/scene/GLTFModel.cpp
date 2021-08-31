#include <common/Print.h>
#include <common/Assert.h>
#include <common/vkinit.h>

#include <scene/GLTFModel.h>

#include <glm/gtc/type_ptr.hpp>

bool GLTFModel::load(const std::string& path) {
    tinygltf::TinyGLTF loader;

    std::string err, warn;

    // parse the gltf file
    if (!loader.LoadASCIIFromFile(&_model, &err, &warn, path)) {
        print(err.c_str());
        print("Could not parse .gltf file: %s\n", path.c_str());
        return onCpu;
    }

    if (!warn.empty()) {
        print(warn.c_str());
    }

    Vertex* vertex;
    UI32* index;
    // extract vertices (only draw triangle list primitives for now)
    for (UI32 m = 0; m < _model.meshes.size(); m++) {
        auto& mesh = _model.meshes[m];
        for (UI32 p = 0; p < mesh.primitives.size(); p++) {
            auto& primitive = mesh.primitives[p];
            if (primitive.mode == TRIANGLES) {
                // resize vertices array using number of vertex positions
                auto& accessor = _model.accessors[primitive.attributes["POSITION"]];
                _vertices.resize(_vertices.size() + accessor.count);
                // get iterator to first new element in vertex array
                vertex = (_vertices.end() - accessor.count)._Ptr;

                // get the buffer views for each attribute
                std::vector<tinygltf::BufferView*> bufferViews = {
                    &_model.bufferViews[primitive.attributes["POSITION"]],
                    &_model.bufferViews[primitive.attributes["NORMAL"]],                  
                    &_model.bufferViews[primitive.attributes["TANGENT"]],
                    &_model.bufferViews[primitive.attributes["TEXCOORD_0"]]
                };

                UC* pData;
                // extract vertices
                for (UI64 v = 0; v < accessor.count; v++) {
                    // copy in position (vec3 = 12)
                    pData = _model.buffers[bufferViews[0]->buffer].data.data() + bufferViews[0]->byteOffset;
                    memcpy(glm::value_ptr(vertex->positionU), (F32*)(pData  + v * 12), 12);
                    // copy in normal (vec3 = 12)
                    pData = _model.buffers[bufferViews[1]->buffer].data.data() + bufferViews[1]->byteOffset;
                    memcpy(glm::value_ptr(vertex->normalV), (F32*)(pData + v * 12), 12);
                    // copy in tangent (vec4 = 16)
                    pData = _model.buffers[bufferViews[2]->buffer].data.data() + bufferViews[2]->byteOffset;
                    memcpy(glm::value_ptr(vertex->tangent), (F32*)(pData+ v * 16), 16);
                    // copy in texture coordinates (vec2 = 8)
                    pData = _model.buffers[bufferViews[3]->buffer].data.data() + bufferViews[3]->byteOffset;
                    vertex->positionU.w = *(F32*)(pData + v * 8);
                    vertex->normalV.w   = *(F32*)(pData + v * 8 + 4);
                    // increment vertex iterator
                    vertex++;
                }

                // accessor to index data
                accessor = _model.accessors[primitive.indices];
                _indices.resize(_indices.size() + accessor.count);
                // pointer to the first new element in index array
                index = (_indices.end() - accessor.count)._Ptr;

                // buffer view for index data
                auto& indexBufferView = _model.bufferViews[accessor.bufferView];
                pData = _model.buffers[indexBufferView.buffer].data.data() + indexBufferView.byteOffset;
                // extract indices (can't memcpy because indices may need to cast from short)
                for (UI64 i = 0; i < accessor.count; i++) {
                    index[i] = static_cast<UI32>(*(UI16*)(pData + i * 2));
                }
            }
        }
    }

    onCpu = true;
    return onCpu;
}

// TODO: move material texture loading to material class (disable tinygltf load image and manage on own)
bool GLTFModel::uploadToGpu(Renderer& renderer) {
    m_assert(onCpu, "model not loaded on CPU, cannot upload data to GPU!");

    if (onGpu) {
        return onGpu;
    }

    // model buffers
    {
        // create vertex buffer
        _vertexBuffer = Buffer::createDeviceLocalBuffer(&renderer._context, renderer._commandPools[RENDER_CMD_POOL],
            BufferData{ (UC*)_vertices.data(), _vertices.size() * sizeof(Vertex) }, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        // create index buffer
        _indexBuffer = Buffer::createDeviceLocalBuffer(&renderer._context, renderer._commandPools[RENDER_CMD_POOL],
            BufferData{ (UC*)_indices.data(), _indices.size() * sizeof(UI32) }, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        // create uniform buffer
        _uniformBuffer = Buffer::createBuffer(renderer._context,
            renderer._swapChain.imageCount() * sizeof(CompositionUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    _materials.resize(_model.materials.size());
    // generate materials (pipelines, descriptors)
    for (UI32 i = 0; i < _materials.size(); i++) {
        auto& material = _model.materials[i];
        // material type determines the descriptor set layout to use
        kDescriptorSetLayout type = OFFSCREEN_DEFAULT_DESCRIPTOR_LAYOUT;
        kTextureMask textures = NO_TEXTURE_BIT;
        if (material.pbrMetallicRoughness.baseColorTexture.index != -1 &&
            material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
            type = OFFSCREEN_PBR_DESCRIPTOR_LAYOUT;
            textures |= ALBEDO_TEXTURE_BIT | OCCLUSION_METALLIC_ROUGNESS_TEXTURE_BIT;
            if (material.normalTexture.index != -1) {
                type = OFFSCREEN_PBR_NORMAL_DESCRIPTOR_LAYOUT;
                textures |= NORMAL_TEXTURE_BIT;
            }
        }
        // TODO: check if pipeline for material type exists already (create pipeline cache)
        _materials[i].createPipeline(renderer, type);

        // create the descriptors and descriptors sets
        {
            // check set bits in mask for number of textures
            // bit magic from: https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
            textures = textures - ((textures >> 1) & 0x55555555);
            textures = (textures & 0x33333333) + ((textures >> 2) & 0x33333333);
            UI32 size = ((textures + (textures >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;

            _materials[i]._textures.resize(size);
            // create necessary textures based on descriptor set layout (ie type of material)
            // TODO: material and texture cache
            // !! -- Assumption that textures are always RGBA format -- !!
            switch (type) {
            case OFFSCREEN_DEFAULT_DESCRIPTOR_LAYOUT: {
                // 0: offscreen uniform
                VkDescriptorBufferInfo offScreenUboInf{};
                offScreenUboInf.buffer = _uniformBuffer._vkBuffer;
                offScreenUboInf.range = sizeof(OffscreenUBO);

                // create descriptor set
                VkWriteDescriptorSet writeDescriptorSet = vkinit::writeDescriptorSet(_materials[i]._descriptorSet, 
                    0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &offScreenUboInf);

                vkUpdateDescriptorSets(renderer._context.device, 1, &writeDescriptorSet, 0, nullptr);
            }
            case OFFSCREEN_PBR_DESCRIPTOR_LAYOUT: {
                // create textures
                auto& texture = _model.images[material.pbrMetallicRoughness.baseColorTexture.index];
                _materials[i]._textures[0].uploadToGpu(renderer, { 
                    { static_cast<UI32>(texture.width), static_cast<UI32>(texture.height), 1 },
                    VK_FORMAT_R8G8B8A8_SRGB, { texture.image.data(), texture.image.size() } });

                texture = _model.images[material.pbrMetallicRoughness.baseColorTexture.index];
                _materials[i]._textures[1].uploadToGpu(renderer, { 
                    { static_cast<UI32>(texture.width), static_cast<UI32>(texture.height), 1},
                    VK_FORMAT_R8G8B8A8_UNORM, { texture.image.data(), texture.image.size() } });

                // allocate descriptor set
                VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocInfo(renderer._descriptorPool,
                    1, &renderer._descriptorSetLayouts[OFFSCREEN_PBR_DESCRIPTOR_LAYOUT]);

                if (vkAllocateDescriptorSets(renderer._context.device, &allocInfo, &_materials[i]._descriptorSet) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate descriptor sets!");
                }

                // 0: offscreen uniform buffer
                VkDescriptorBufferInfo offScreenUboInf{};
                offScreenUboInf.buffer = _uniformBuffer._vkBuffer;
                offScreenUboInf.range = sizeof(OffscreenUBO);

                // 1: albedo sampler
                VkDescriptorImageInfo albedoImageInfo{};
                albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                albedoImageInfo.imageView = _materials[i]._textures[0]._imageView;
                albedoImageInfo.sampler   = _materials[i]._textures[0]._sampler;

                // 2: ambient occlusion metallic roughness
                VkDescriptorImageInfo aoMetallicRoughnessImageInfo{};
                aoMetallicRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                aoMetallicRoughnessImageInfo.imageView = _materials[i]._textures[1]._imageView;
                aoMetallicRoughnessImageInfo.sampler   = _materials[i]._textures[1]._sampler;

                // create descriptor set
                VkWriteDescriptorSet writeDescriptorSets[3] = {
                    vkinit::writeDescriptorSet(_materials[i]._descriptorSet, 0, 
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &offScreenUboInf),
                    vkinit::writeDescriptorSet(_materials[i]._descriptorSet, 1, 
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &albedoImageInfo),
                    vkinit::writeDescriptorSet(_materials[i]._descriptorSet, 2, 
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &aoMetallicRoughnessImageInfo)
                };

                vkUpdateDescriptorSets(renderer._context.device, 3, writeDescriptorSets, 0, nullptr);
                break;
            }
            case OFFSCREEN_PBR_NORMAL_DESCRIPTOR_LAYOUT: {
                // upload textures to the gpu
                auto& texture = _model.images[material.pbrMetallicRoughness.baseColorTexture.index];
                _materials[i]._textures[0].uploadToGpu(renderer, { 
                    { static_cast<UI32>(texture.width), static_cast<UI32>(texture.height), 1 },
                    VK_FORMAT_R8G8B8A8_SRGB, { texture.image.data(), texture.image.size() } });

                texture = _model.images[material.pbrMetallicRoughness.baseColorTexture.index];
                _materials[i]._textures[1].uploadToGpu(renderer, { 
                    { static_cast<UI32>(texture.width), static_cast<UI32>(texture.height), 1 },
                    VK_FORMAT_R8G8B8A8_UNORM, { texture.image.data(), texture.image.size() } });

                texture = _model.images[material.normalTexture.index];
                _materials[i]._textures[2].uploadToGpu(renderer, { 
                    { static_cast<UI32>(texture.width), static_cast<UI32>(texture.height), 1 },
                    VK_FORMAT_R8G8B8A8_UNORM, { texture.image.data(), texture.image.size() } });

                // allocate descriptor set
                VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocInfo(renderer._descriptorPool,
                    1, &renderer._descriptorSetLayouts[OFFSCREEN_PBR_NORMAL_DESCRIPTOR_LAYOUT]);

                if (vkAllocateDescriptorSets(renderer._context.device, &allocInfo, &_materials[i]._descriptorSet) != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate descriptor sets!");
                }

                // 0: offscreen uniform buffer
                VkDescriptorBufferInfo offScreenUboInf{};
                offScreenUboInf.buffer = _uniformBuffer._vkBuffer;
                offScreenUboInf.range = sizeof(OffscreenUBO);

                // 1: albedo sampler
                VkDescriptorImageInfo albedoImageInfo{};
                albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                albedoImageInfo.imageView   = _materials[i]._textures[0]._imageView;
                albedoImageInfo.sampler     = _materials[i]._textures[0]._sampler;

                // 2: ambient occlusion metallic roughness
                VkDescriptorImageInfo aoMetallicRoughnessImageInfo{};
                aoMetallicRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                aoMetallicRoughnessImageInfo.imageView   = _materials[i]._textures[1]._imageView;
                aoMetallicRoughnessImageInfo.sampler     = _materials[i]._textures[1]._sampler;

                // 3: normal map
                VkDescriptorImageInfo normalMapImageInfo{};
                normalMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                normalMapImageInfo.imageView   = _materials[i]._textures[2]._imageView;
                normalMapImageInfo.sampler     = _materials[i]._textures[2]._sampler;

                // create descriptor set
                VkWriteDescriptorSet writeDescriptorSets[4] = {
                    vkinit::writeDescriptorSet(_materials[i]._descriptorSet, 0, 
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &offScreenUboInf),
                    vkinit::writeDescriptorSet(_materials[i]._descriptorSet, 1, 
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &albedoImageInfo),
                    vkinit::writeDescriptorSet(_materials[i]._descriptorSet, 2, 
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &aoMetallicRoughnessImageInfo),
                    vkinit::writeDescriptorSet(_materials[i]._descriptorSet, 3, 
                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normalMapImageInfo)
                };

                vkUpdateDescriptorSets(renderer._context.device, 4, writeDescriptorSets, 0, nullptr);
                break;
            }
            default:
                print("unhandled material type %i!", type);
                break;
            }
        }
    }
    onGpu = true;
    return onGpu;
}

bool GLTFModel::cleanup(Renderer& renderer) {
    if (onGpu) {
        // destroy geometry 
        _indexBuffer.cleanupBufferData(renderer._context.device);
        _vertexBuffer.cleanupBufferData(renderer._context.device);

        // destroy uniforms
        _uniformBuffer.cleanupBufferData(renderer._context.device);

        // destroy material (takes care of texture descriptors, sets, pipelines)
        for (auto& material : _materials) {
            material.cleanup(renderer._context.device);
        }
        onGpu = false;
    }
    return onGpu;    
}


void GLTFModel::draw(VkCommandBuffer commandBuffer) {
    // take care of drawing all the meshes in the model
    // TODO: batch primitives according to material
    m_assert(_materials.size() == 1, "Only support a single material!");

    // bind pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _materials.begin()->_pipeline);

    // bind descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _materials.begin()->_pipelineLayout,
        0, 1, &_materials.begin()->_descriptorSet, 0, nullptr);

    // bind vertex buffer
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._vkBuffer, &offset);

    // bind index buffer
    vkCmdBindIndexBuffer(commandBuffer, _indexBuffer._vkBuffer, 0, VK_INDEX_TYPE_UINT32);

    // draw
    vkCmdDrawIndexed(commandBuffer, static_cast<UI32>(_indices.size()), 1, 0, 0, 0);
}