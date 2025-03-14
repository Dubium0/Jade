#pragma once 

#include <mutex>
#include <queue>
#include <functional>

#include "jade_structs.hpp"
#include "vk_types.hpp"
#include "vk_descriptors.hpp"
#include "vk_loader.hpp"
#include "vk_camera.hpp"


struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void pushFunction(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call functors
		}

		deletors.clear();
	}
};

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

struct FrameData {
	VkSemaphore swapchainSemaphore{};
	VkSemaphore renderSemaphore{};
	VkFence renderFence{};

	VkCommandPool commandPool{};
	VkCommandBuffer mainCommandBuffer{};
	DescriptorAllocatorGrowable frameDescriptors{};
	DeletionQueue deletionQueue{};
};

struct RenderObject {
	uint32_t indexCount = 0;
	uint32_t firstIndex = 0;
	VkBuffer indexBuffer{};

	MaterialInstance* material = nullptr;

	glm::mat4 transform{};
	VkDeviceAddress vertexBufferAddress{};
};

struct DrawContext {
	std::vector<RenderObject> OpaqueSurfaces;
	std::vector<RenderObject> TransparentSurfaces;
};
struct MeshNode : public Node {

	std::shared_ptr<MeshAsset> mesh;

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};

struct GLTFMetallic_Roughness {
	MaterialPipeline opaquePipeline{};
	MaterialPipeline transparentPipeline{};

	VkDescriptorSetLayout materialLayout{};

	struct MaterialConstants {
		glm::vec4 colorFactors{};
		glm::vec4 metalRoughFactors{};
		//padding, we need it anyway for uniform buffers
		glm::vec4 extra[14];
	};

	struct MaterialResources {
		AllocatedImage colorImage{};
		VkSampler colorSampler{};
		AllocatedImage metalRoughImage{};
		VkSampler metalRoughSampler{};
		VkBuffer dataBuffer{};
		uint32_t dataBufferOffset=0;
	};

	DescriptorWriter writer{};

	void buildPipelines(VulkanEngine* engine);
	void clearResources(VkDevice device);

	MaterialInstance writeMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};

constexpr int FRAME_OVERLAP = 2;

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
    const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
};
struct EngineStats {
    float frametime;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};
class VulkanEngine {
	private:
	static VulkanEngine * instance_;
    static std::mutex mutex_;

	protected:
	VulkanEngine( )
    {
    }
    ~VulkanEngine() {}
public:
	std::string windowTitle{};
	VkExtent2D windowExtent{1700,700};

	VkInstance vkInstance{};// Vulkan library handle
	VkDebugUtilsMessengerEXT debugMessenger{nullptr};// Vulkan debug output handle
	VkPhysicalDevice chosenGPU{};// GPU chosen as the default device
	VkDevice device{}; // Vulkan device for commands
	VkSurfaceKHR surface{};// Vulkan window surface
	VkSwapchainKHR swapchain{};
	VkFormat swapchainImageFormat{};

	std::vector<VkImage> swapchainImages{};
	std::vector<VkImageView> swapchainImageViews{};
	VkExtent2D swapchainExtent{};
	
	bool isInitialized{ false };
	int frameNumber{0};
	bool stopRendering{ false };

	struct SDL_Window* window{ nullptr };
	
	FrameData frames[FRAME_OVERLAP];

	inline FrameData& getCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; };

	VkQueue graphicsQueue{};
	uint32_t graphicsQueueFamily{};

	DeletionQueue mainDeletionQueue{};

	VmaAllocator allocator{};

	AllocatedImage drawImage{};
	AllocatedImage depthImage{};
	VkExtent2D drawExtent{};

	DescriptorAllocatorGrowable globalDescriptorAllocator{};

	VkDescriptorSet drawImageDescriptors{};
	VkDescriptorSetLayout drawImageDescriptorLayout{};

	VkPipeline gradientPipeline{};
	VkPipelineLayout gradientPipelineLayout{};

	// immediate submit structures
	VkFence immFence{};
	VkCommandBuffer immCommandBuffer{};
	VkCommandPool immCommandPool{};
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	std::vector<ComputeEffect> backgroundEffects{};
	int currentBackgroundEffect{0};

	VkPipelineLayout meshPipelineLayout{};
	VkPipeline meshPipeline{};
	
	GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

	std::vector<std::shared_ptr<MeshAsset>> testMeshes{};

	Camera camera{};

	bool resizeRequested{true};
    float renderScale = 1.f;

	GPUSceneData sceneData{};
	VkDescriptorSetLayout gpuSceneDataDescriptorLayout{};

	AllocatedImage whiteImage{};
	AllocatedImage blackImage{};
	AllocatedImage greyImage{};
	AllocatedImage errorCheckerboardImage{};

    VkSampler defaultSamplerLinear{};
	VkSampler defaultSamplerNearest{};

	VkDescriptorSetLayout singleImageDescriptorLayout{};

	MaterialInstance defaultData{};
    GLTFMetallic_Roughness metalRoughMaterial{};


	DrawContext mainDrawContext{};
    std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes{};

	std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes{};

	EngineStats stats{};

	std::vector<SDL_DisplayMode> availableDisplayModes{};
public:
	//singleton logic
	VulkanEngine(VulkanEngine &other) = delete;
    void operator=(const VulkanEngine &) = delete;
	static VulkanEngine *getInstance();

	//initializes everything in the engine
	bool init(jade::EngineCreateInfo createInfo);

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();
	

	//run main loop
	void run();

	AllocatedImage createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage createImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void destroyImage(const AllocatedImage& img);
	void updateScene();
	AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroyBuffer(const AllocatedBuffer& buffer);

private:
	void initVulkan();
	void initSwapchain();
	void initCommands();
	void initSyncStructures();
	void createSwapchain(uint32_t width, uint32_t height);
	void destroySwapchain();
	void drawBackground(VkCommandBuffer cmd);
	void initDescriptors();
	void initPipelines();
	void initBackgroundPipelines();
	void initImgui();
	void drawImgui(VkCommandBuffer cmd, VkImageView targetImageView);
	void drawGeometry(VkCommandBuffer cmd);


	void initMeshPipeline();
	void initDefaultData();
	void resizeSwapchain();

};