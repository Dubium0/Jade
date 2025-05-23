#pragma once
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <memory>

#include <fastgltf/core.hpp>

#include "vk_types.hpp"
#include "vk_transform.hpp"
struct GLTFMaterial {
	MaterialInstance data;
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset {
    std::string name;
    Transformation transformation{};
    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};
//forward declaration
class VulkanEngine;

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadMesh(VulkanEngine* engine, std::filesystem::path path);

struct LoadedGLTF : public IRenderable {

    // storage for all the data on a given glTF file
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    // nodes that dont have a parent, for iterating through the file in tree order
    std::vector<std::shared_ptr<Node>> topNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    AllocatedBuffer materialDataBuffer;

    VulkanEngine* creator;

    ~LoadedGLTF() { clearAll(); };

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

private:

    void clearAll();
};

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine,std::string_view filePath);
VkFilter extractFilter(fastgltf::Filter filter);
VkSamplerMipmapMode extractMipmapMode(fastgltf::Filter filter);
std::optional<AllocatedImage> loadImage(VulkanEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image);