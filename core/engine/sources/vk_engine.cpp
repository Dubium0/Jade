#include "vk_engine.hpp"

#include <thread>
#include <algorithm>

#include <SDL.h>
#include <SDL_vulkan.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <imgui.h>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>


#include "vk_types.hpp"
#include "vk_images.hpp"
#include "vk_initializers.hpp"
#include "vk_pipelines.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>


constexpr bool bUseValidationLayers = false;

VulkanEngine* VulkanEngine::instance_{nullptr};
std::mutex VulkanEngine::mutex_;


VulkanEngine *VulkanEngine::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ == nullptr)
    {
        instance_ = new VulkanEngine();
    }
    return instance_;
}

bool VulkanEngine::init(jade::EngineCreateInfo createInfo)
{
    windowTitle = createInfo.windowTitle;
    windowExtent.width = createInfo.windowWidth;
    windowExtent.height =createInfo.windowHeight;
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

	int monitorCount = SDL_GetNumVideoDisplays();
	if (monitorCount < 1) {
		fmt::println("No monitors found!");
		SDL_Quit();
		return false;
	}
	for (int i = 0; i < monitorCount; i++) {
		SDL_DisplayMode displayMode;
		if (SDL_GetDesktopDisplayMode(i, &displayMode) == 0) {
			fmt::println("Monitor {} maximum resolution: {} , {}", i, displayMode.w, displayMode.h);
			availableDisplayModes.push_back(displayMode);
				
		}
		else {
			fmt::println("Failed to get display mode for monitor {} ", i);

		}
	}
	


    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    window = SDL_CreateWindow(
        windowTitle.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        windowExtent.width,
        windowExtent.height,
        window_flags);
	


    initVulkan();
    initSwapchain();
    initCommands();
    initSyncStructures();
	initDescriptors();	
	initPipelines();
	initImgui();
	initDefaultData();
    // everything went fine
	std::string assetRootPath{};
	#ifdef ASSETS_PATH
		assetRootPath = ASSETS_PATH;
	#endif
	auto cubePath = assetRootPath + "/cube.glb";

    auto cubeFile = loadGltf(this,cubePath);

	auto x = 10;
	fmt::print("x is: {}\n", x);

    assert(cubeFile.has_value());

    loadedScenes["cube"] = *cubeFile;
	auto structurePath = assetRootPath + "/structure.glb";

    auto structureFile = loadGltf(this,structurePath);

    assert(structureFile.has_value());

    loadedScenes["structure"] = *structureFile;

    isInitialized = true;
	return isInitialized;
}

void VulkanEngine::cleanup()
{
    if (isInitialized) {
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(device);
		
		loadedScenes.clear();

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			vkDestroyCommandPool(device, frames[i].commandPool, nullptr);

			//destroy sync objects
			vkDestroyFence(device, frames[i].renderFence, nullptr);
			vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
			vkDestroySemaphore(device ,frames[i].swapchainSemaphore, nullptr);
			frames[i].deletionQueue.flush();
		}
		metalRoughMaterial.clearResources(device);
		mainDeletionQueue.flush();

        destroySwapchain();
        vkDestroySurfaceKHR(vkInstance, surface, nullptr);
        
		vkDestroyDevice(device, nullptr);
		
		vkb::destroy_debug_utils_messenger(vkInstance, debugMessenger);
		vkDestroyInstance(vkInstance, nullptr);
        SDL_DestroyWindow(window);
    }
}

void VulkanEngine::draw()
{
	updateScene();
    // wait until the gpu has finished rendering the last frame. Timeout of 1
	// second
	VK_CHECK(vkWaitForFences(device, 1, &getCurrentFrame().renderFence, true, 1000000000));

	getCurrentFrame().deletionQueue.flush();
	getCurrentFrame().frameDescriptors.clearPools(device);

	VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().renderFence));

    //request image from the swapchain
	uint32_t swapchainImageIndex;
	{
		VkResult result = vkAcquireNextImageKHR(device, swapchain, 1000000000, getCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
		if( result == VK_ERROR_OUT_OF_DATE_KHR){
			resizeRequested = true;
			return;
		}
	}


    //naming it cmd for shorter writing
	VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;

	// now that we are sure that the commands finished executing, we can safely
	// reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = vkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	drawExtent.height = std::min(swapchainExtent.height, drawImage.imageExtent.height) * renderScale;
	drawExtent.width = std::min(swapchainExtent.width, drawImage.imageExtent.width) * renderScale;

	//start the command buffer recording
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    //make the swapchain image into writeable mode before rendering
	vkUtil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);


	drawBackground(cmd);
	vkUtil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vkUtil::transition_image(cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	{

		//begin a render pass  connected to our draw image
		VkRenderingAttachmentInfo colorAttachment = vkInit::attachmentInfo(drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttachment = vkInit::depthAttachmentInfo(depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);


		VkRenderingInfo renderInfo = vkInit::renderingInfo(drawExtent, &colorAttachment, &depthAttachment);
		vkCmdBeginRendering(cmd, &renderInfo);
		drawGeometry(cmd);
		vkCmdEndRendering(cmd);
	}
	//transition the draw image and the swapchain image into their correct transfer layouts
	vkUtil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkUtil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	vkUtil::copy_image_to_image(cmd, drawImage.image, swapchainImages[swapchainImageIndex], drawExtent, swapchainExtent);

	// set swapchain image layout to Present so we can show it on the screen
	vkUtil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	
	drawImgui(cmd,  swapchainImageViews[swapchainImageIndex]);

	vkUtil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	//prepare the submission to the queue. 
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo cmdinfo = vkInit::command_buffer_submit_info(cmd);	
	
	VkSemaphoreSubmitInfo waitInfo = vkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,getCurrentFrame().swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);	
	
	VkSubmitInfo2 submit = vkInit::submit_info(&cmdinfo,&signalInfo,&waitInfo);	

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, getCurrentFrame().renderFence));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	{

		VkResult result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
		if(result ==VK_ERROR_OUT_OF_DATE_KHR){
			resizeRequested =true;
		}
	
	}

	//increase the number of frames drawn
	frameNumber++;



}

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
		//begin clock
		auto start = std::chrono::system_clock::now();
		//run logic
		{

			// Handle events on queue
			while (SDL_PollEvent(&e) != 0) {
				// close the window when user alt-f4s or clicks the X button
				if (e.type == SDL_QUIT)
                bQuit = true;
				
				if (e.type == SDL_WINDOWEVENT) {
					if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
						stopRendering = true;
					}
					if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
						stopRendering = false;
					}
				}
				camera.processSDLEvent(e);
				//send SDL event to imgui for handling
				ImGui_ImplSDL2_ProcessEvent(&e);
			}
			
			// do not draw if we are minimized
			if (stopRendering) {
				// throttle the speed to avoid the endless spinning
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			if (resizeRequested) {
				resizeSwapchain();
			}
			// imgui new frame
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			
			if (ImGui::Begin("background")) {
				
				ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];
				ImGui::SliderFloat("Render Scale",&renderScale, 0.3f, 1.f);
				ImGui::Text("Selected effect: ", selected.name);
				
				ImGui::SliderInt("Effect Index", &currentBackgroundEffect,0, backgroundEffects.size() - 1);
				
				ImGui::InputFloat4("data1",(float*)& selected.data.data1);
				ImGui::InputFloat4("data2",(float*)& selected.data.data2);
				ImGui::InputFloat4("data3",(float*)& selected.data.data3);
				ImGui::InputFloat4("data4",(float*)& selected.data.data4);
			}
			ImGui::End();
			if(ImGui::Begin("Camera")){
				ImGui::Text("Transformation");
				ImGui::InputFloat3("position",(float*)& camera.getTransformationRW().getPositionRW());
				ImGui::InputFloat4("rotation",(float*)& camera.getTransformationRW().getRotationRW());
				ImGui::InputFloat3("scale",(float*)& camera.getTransformationRW().getScaleRW());
				
				ImGui::Text("Properties");
				ImGui::SliderFloat("speed",(float*)& camera.getCameraSpeedRW(),0.0f,10.0f);
				ImGui::SliderFloat("sensitivity",(float*)& camera.getCameraSensitivityRW(),0.0f,10.0f);
			}
			ImGui::End();

			if(ImGui::Begin("Stats")){
				ImGui::Text("frametime %f ms", stats.frametime);
				ImGui::Text("draw time %f ms", stats.mesh_draw_time);
				ImGui::Text("update time %f ms", stats.scene_update_time);
				ImGui::Text("triangles %i", stats.triangle_count);
				ImGui::Text("draws %i", stats.drawcall_count);
			}
			ImGui::End();
			//make imgui calculate internal draw structures
			ImGui::Render();
			
		
        	draw();
		}

		//get clock again, compare with start clock
		auto end = std::chrono::system_clock::now();
     
		//convert to microseconds (integer), and then come back to miliseconds
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		stats.frametime = elapsed.count() / 1000.f;
    }
}

void VulkanEngine::initVulkan()
{
	vkb::InstanceBuilder builder;

	//make the vulkan instance, with basic debug features
	auto inst_ret = builder.set_app_name("Jade Engine")
		.request_validation_layers(bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//grab the instance 
	vkInstance = vkb_inst.instance;
	debugMessenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(window, vkInstance, &surface);

	//vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{  };
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{  };
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;


	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_surface(surface)
		.select()
		.value();


	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	device = vkbDevice.device;
	chosenGPU = physicalDevice.physical_device;

    // use vkbootstrap to get a Graphics queue
	graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = chosenGPU;
    allocatorInfo.device = device;
    allocatorInfo.instance = vkInstance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    mainDeletionQueue.pushFunction([&]() {
        vmaDestroyAllocator(allocator);
    });


}

void VulkanEngine::initSwapchain()
{
    createSwapchain(windowExtent.width,windowExtent.height);

	//draw image size will match the window
	VkExtent3D drawImageExtent = {
		windowExtent.width,
		windowExtent.height,
		1
	};

	//hardcoding the draw format to 32 bit float
	drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = vkInit::image_create_info(drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(allocator, &rimg_info, &rimg_allocinfo, &drawImage.image, &drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkInit::imageview_create_info(drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(device, &rview_info, nullptr, &drawImage.imageView));


	depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
	depthImage.imageExtent = drawImageExtent;
	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo dimg_info = vkInit::image_create_info(depthImage.imageFormat, depthImageUsages, drawImageExtent);

	//allocate and create the image
	vmaCreateImage(allocator, &dimg_info, &rimg_allocinfo, &depthImage.image, &depthImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo dview_info = vkInit::imageview_create_info(depthImage.imageFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(device, &dview_info, nullptr, &depthImage.imageView));


	//add to deletion queues
	mainDeletionQueue.pushFunction([=,this]() {
		vkDestroyImageView(device, drawImage.imageView, nullptr);
		vmaDestroyImage(allocator, drawImage.image, drawImage.allocation);
		
		vkDestroyImageView(device, depthImage.imageView, nullptr);
		vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
	});

}

void VulkanEngine::initCommands()
{
    //create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolCreateInfo = vkInit::command_pool_create_info(graphicsQueueFamily,VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) {

		VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &frames[i].commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::command_buffer_allocate_info(frames[i].commandPool,1);

		VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].mainCommandBuffer));
	}

	VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &immCommandPool));

	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::command_buffer_allocate_info(immCommandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &immCommandBuffer));

	mainDeletionQueue.pushFunction([=,this]() { 
	vkDestroyCommandPool(device, immCommandPool, nullptr);
	});
}

void VulkanEngine::initSyncStructures()
{
    //create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo fenceCreateInfo = vkInit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkInit::semaphore_create_info(0);

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));

		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
	}

	VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immFence));
	mainDeletionQueue.pushFunction([=,this]() { vkDestroyFence(device, immFence, nullptr); });
}

void VulkanEngine::createSwapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder{ chosenGPU,device,surface };

	swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	swapchainExtent = vkbSwapchain.extent;
	//store swapchain and its related images
	swapchain = vkbSwapchain.swapchain;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::destroySwapchain()
{
    vkDestroySwapchainKHR(device, swapchain, nullptr);

	// destroy swapchain resources
	for (auto i = 0; i < swapchainImageViews.size(); i++) {

		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
	}
}

void VulkanEngine::drawBackground(VkCommandBuffer cmd)
{

	//make a clear-color from frame number. This will flash with a 120 frame period.
	//VkClearColorValue clearValue;
	//float flash = std::abs(std::sin(frameNumber / 120.f));
	//clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

	//VkImageSubresourceRange clearRange = vkInit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	// old way
	//vkCmdClearColorImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];

	// bind the background compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.layout, 0, 1, &drawImageDescriptors, 0, nullptr);

	vkCmdPushConstants(cmd, effect.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
	
	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(drawImage.imageExtent.width/ 16.0), std::ceil(drawImage.imageExtent.height/ 16.0), 1);
}

void VulkanEngine::initDescriptors()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	globalDescriptorAllocator.init(device, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		drawImageDescriptorLayout = builder.build(device, VK_SHADER_STAGE_COMPUTE_BIT);
	}
	{
		DescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		singleImageDescriptorLayout = builder.build(device, VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	drawImageDescriptors = globalDescriptorAllocator.allocate(device,drawImageDescriptorLayout);	

	DescriptorWriter writer{};
	writer.writeImage(0,drawImage.imageView,VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	writer.updateSet(device,drawImageDescriptors);
	//make sure both the descriptor allocator and the new layout get cleaned up properly

	{
		DescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		gpuSceneDataDescriptorLayout = builder.build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	mainDeletionQueue.pushFunction([&]() {
		globalDescriptorAllocator.destroyPools(device);

		vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, gpuSceneDataDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, singleImageDescriptorLayout, nullptr);
	});

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		// create a descriptor pool
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frameSizes = { 
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		frames[i].frameDescriptors = DescriptorAllocatorGrowable{};
		frames[i].frameDescriptors.init(device, 1000, frameSizes);
	
		mainDeletionQueue.pushFunction([&, i]() {
			frames[i].frameDescriptors.destroyPools(device);
		});
	}

}

void VulkanEngine::initPipelines()
{
	initBackgroundPipelines();
	initMeshPipeline();

	metalRoughMaterial.buildPipelines(this);
}

void VulkanEngine::initBackgroundPipelines()
{
	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;
	
	
	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputePushConstants) ;
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computeLayout.pPushConstantRanges = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(device, &computeLayout, nullptr, &gradientPipelineLayout));

	std::string shadersRootPath{ "../ShaderCompiler" };

	#ifdef SHADERS_PATH
		shadersRootPath = SHADERS_PATH;
	#endif
		auto gradientComp = shadersRootPath + "/gradient.comp.spv";
	VkShaderModule gradientShader;
	if (!vkUtil::loadShaderModule(gradientComp, device, gradientShader))
	{
		fmt::print("Error when building the compute shader \n");
	}
	auto skyComp = shadersRootPath + "/sky.comp.spv";
	VkShaderModule skyShader;
	if (!vkUtil::loadShaderModule(skyComp, device, skyShader)) {
		fmt::print("Error when building the compute shader \n");
	}

	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = gradientShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;
	
	ComputeEffect gradient;
	gradient.layout = gradientPipelineLayout;
	gradient.name = "gradient";
	gradient.data = {};

	//default colors
	gradient.data.data1 = glm::vec4(1, 0, 0, 1);
	gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	VK_CHECK(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &gradient.pipeline));
	//change the shader module only to create the sky shader
	computePipelineCreateInfo.stage.module = skyShader;

	ComputeEffect sky;
	sky.layout = gradientPipelineLayout;
	sky.name = "gradient";
	sky.data = {};

	//default colors
	sky.data.data1 = glm::vec4(0.1, 0.2, 0.4 ,0.97);

	VK_CHECK(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &sky.pipeline));

	backgroundEffects.push_back(gradient);
	backgroundEffects.push_back(sky);

	vkDestroyShaderModule(device, gradientShader, nullptr);
	vkDestroyShaderModule(device, skyShader, nullptr);

	mainDeletionQueue.pushFunction([=,this]() {
		vkDestroyPipelineLayout(device, gradientPipelineLayout, nullptr);
		vkDestroyPipeline(device, sky.pipeline, nullptr);
		vkDestroyPipeline(device, gradient.pipeline, nullptr);
		});
}

void VulkanEngine::initImgui()
{
// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize poolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000;
	poolInfo.poolSizeCount = (uint32_t)std::size(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = vkInstance;
	initInfo.PhysicalDevice = chosenGPU;
	initInfo.Device = device;
	initInfo.Queue = graphicsQueue;
	initInfo.DescriptorPool = imguiPool;
	initInfo.MinImageCount = 3;
	initInfo.ImageCount = 3;
	initInfo.UseDynamicRendering = true;

	//dynamic rendering parameters for imgui to use
	initInfo.PipelineRenderingCreateInfo = {};
	initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat;

	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&initInfo);

	ImGui_ImplVulkan_CreateFontsTexture();

	// add the destroy the imgui created structures
	mainDeletionQueue.pushFunction([=,this]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(device, imguiPool, nullptr);
	});
}

void VulkanEngine::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(device, 1, &immFence));
	VK_CHECK(vkResetCommandBuffer(immCommandBuffer, 0));

	VkCommandBuffer cmd = immCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = vkInit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkInit::submit_info(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, immFence));

	VK_CHECK(vkWaitForFences(device, 1, &immFence, true, 9999999999));

}

void VulkanEngine::drawImgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
	VkRenderingAttachmentInfo colorAttachment = vkInit::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = vkInit::renderingInfo(swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
	
}

void VulkanEngine::drawGeometry(VkCommandBuffer cmd)
{
	//reset counters
	stats.drawcall_count = 0;
	stats.triangle_count = 0;
	//begin clock
	auto start = std::chrono::system_clock::now();
	
	// draw geometry logic
	{

		std::vector<uint32_t> opaque_draws{};
		opaque_draws.reserve(mainDrawContext.OpaqueSurfaces.size());

		for (uint32_t i = 0; i < mainDrawContext.OpaqueSurfaces.size(); i++) {
			opaque_draws.push_back(i);
		}

		// sort the opaque surfaces by material and mesh
		std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
			const RenderObject& A = mainDrawContext.OpaqueSurfaces[iA];
			const RenderObject& B = mainDrawContext.OpaqueSurfaces[iB];
			if (A.material == B.material) {
				return A.indexBuffer < B.indexBuffer;
			}
			else {
				return A.material < B.material;
			}
			});

		//allocate a new uniform buffer for the scene data
		AllocatedBuffer gpuSceneDataBuffer = createBuffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		//add it to the deletion queue of this frame so it gets deleted once its been used
		getCurrentFrame().deletionQueue.pushFunction([=, this]() {
			destroyBuffer(gpuSceneDataBuffer);
			});

		//write the buffer
		GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
		*sceneUniformData = sceneData;
		//create a descriptor set that binds that buffer and update it
		VkDescriptorSet globalDescriptor = getCurrentFrame().frameDescriptors.allocate(device, gpuSceneDataDescriptorLayout);

		DescriptorWriter writer;
		writer.writeBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.updateSet(device, globalDescriptor);

		MaterialPipeline* lastPipeline = nullptr;
		MaterialInstance* lastMaterial = nullptr;
		VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

		auto drawLambda = [&](const RenderObject& draw){
			
			if (draw.material != lastMaterial) {
				lastMaterial = draw.material;

				if (draw.material->pipeline != lastPipeline) {
					lastPipeline = draw.material->pipeline;
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 0, 1,
						&globalDescriptor, 0, nullptr);

					VkViewport viewport = {};
					viewport.x = 0;
					viewport.y = 0;
					viewport.width = (float)windowExtent.width;
					viewport.height = (float)windowExtent.height;
					viewport.minDepth = 0.f;
					viewport.maxDepth = 1.f;

					vkCmdSetViewport(cmd, 0, 1, &viewport);

					VkRect2D scissor = {};
					scissor.offset.x = 0;
					scissor.offset.y = 0;
					scissor.extent.width = windowExtent.width;
					scissor.extent.height = windowExtent.height;

					vkCmdSetScissor(cmd, 0, 1, &scissor);
				}

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 1, 1,
					&draw.material->materialSet, 0, nullptr);
			}
			if (draw.indexBuffer != lastIndexBuffer) {
				lastIndexBuffer = draw.indexBuffer;
				vkCmdBindIndexBuffer(cmd, draw.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			}
			

			GPUDrawPushConstants pushConstants;
			pushConstants.vertexBuffer = draw.vertexBufferAddress;
			pushConstants.worldMatrix = draw.transform;
			vkCmdPushConstants(cmd,draw.material->pipeline->layout ,VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(GPUDrawPushConstants), &pushConstants);

			vkCmdDrawIndexed(cmd,draw.indexCount,1,draw.firstIndex,0,0);

			stats.drawcall_count++;
        	stats.triangle_count += draw.indexCount / 3; 
		};

		
		for (auto& r : opaque_draws) {
			drawLambda(mainDrawContext.OpaqueSurfaces[r]);
		}
	
		for (const RenderObject& draw : mainDrawContext.TransparentSurfaces) {
			drawLambda(draw);
		}
		mainDrawContext.OpaqueSurfaces.clear();
		mainDrawContext.TransparentSurfaces.clear();

	
	}

	auto end = std::chrono::system_clock::now();

    //convert to microseconds (integer), and then come back to miliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.mesh_draw_time = elapsed.count() / 1000.f;
}

AllocatedBuffer VulkanEngine::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	// allocate buffer
	VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;

	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = memoryUsage;
	vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation,
		&newBuffer.info));

	return newBuffer;
}

void VulkanEngine::destroyBuffer(const AllocatedBuffer& buffer)
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

GPUMeshBuffers VulkanEngine::uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers newSurface;

	//create vertex buffer
	newSurface.vertexBuffer = createBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	//find the adress of the vertex buffer
	VkBufferDeviceAddressInfo deviceAdressInfo{  };
	deviceAdressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	deviceAdressInfo.buffer = newSurface.vertexBuffer.buffer;
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAdressInfo);

	//create index buffer
	newSurface.indexBuffer = createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	AllocatedBuffer staging = createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data = staging.allocation->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	immediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{ };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
	});

	destroyBuffer(staging);

	return newSurface;

}

void VulkanEngine::initMeshPipeline()
{
	std::string shadersRootPath{ "../ShaderCompiler" };
	#ifdef SHADERS_PATH
		shadersRootPath = SHADERS_PATH;
	#endif
	VkShaderModule triangleFragShader;

	auto texImageFrag = shadersRootPath + "/tex_image.frag.spv";
	if (!vkUtil::loadShaderModule(texImageFrag, device, triangleFragShader)) {
		fmt::print("Error when building the triangle fragment shader module");
	}
	else {
		fmt::print("Triangle fragment shader succesfully loaded");
	}

	VkShaderModule triangleVertexShader;
	auto texImageVert = shadersRootPath + "/colored_triangle_mesh.vert.spv";
	if (!vkUtil::loadShaderModule(texImageVert, device, triangleVertexShader)) {
		fmt::print("Error when building the triangle vertex shader module");
	}
	else {
		fmt::print("Triangle vertex shader succesfully loaded");
	}

	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkInit::pipelineLayoutCreateInfo();
	pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pSetLayouts = &singleImageDescriptorLayout;
	pipelineLayoutInfo.setLayoutCount = 1;
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &meshPipelineLayout));

	PipelineBuilder pipelineBuilder;
	std::string entryName = "main";
	//use the triangle layout we created
	pipelineBuilder.pipelineLayout = meshPipelineLayout;
	//connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.setShaders(triangleVertexShader, entryName,triangleFragShader,entryName);
	//it will draw triangles
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.setMultisamplingNone();
	//no blending
	//pipelineBuilder.disableBlending();
	pipelineBuilder.enableBlendingAdditive();
	//pipelineBuilder.disableDepthTest();
	pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
	//connect the image format we will draw into, from draw image
	pipelineBuilder.setColorAttachmentFormat(drawImage.imageFormat);
	pipelineBuilder.setDepthFormat(depthImage.imageFormat);

	//finally build the pipeline
	meshPipeline = pipelineBuilder.buildPipeline(device);

	//clean structures
	vkDestroyShaderModule(device, triangleFragShader, nullptr);
	vkDestroyShaderModule(device, triangleVertexShader, nullptr);

	mainDeletionQueue.pushFunction([&]() {
		vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
		vkDestroyPipeline(device, meshPipeline, nullptr);
	});
}

void VulkanEngine::initDefaultData() 
{	
	std::string assetRootPath{ };
	#ifdef ASSETS_PATH
	assetRootPath = ASSETS_PATH;
	#endif
	auto basicMeshPath =  assetRootPath  + "/basicmesh.glb";
	testMeshes = loadMesh(this, basicMeshPath).value();

	for(auto& meshes : testMeshes ){
		mainDeletionQueue.pushFunction([&](){
			destroyBuffer(meshes->meshBuffers.indexBuffer);
			destroyBuffer(meshes->meshBuffers.vertexBuffer);
			

		});	
	}

	//3 default textures, white, grey, black. 1 pixel each
	uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	whiteImage = createImage((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	greyImage = createImage((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	blackImage = createImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	//checkerboard image
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 *16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	errorCheckerboardImage = createImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampl = {};
	sampl.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;

	vkCreateSampler(device, &sampl, nullptr, &defaultSamplerNearest);

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(device, &sampl, nullptr, &defaultSamplerLinear);

	mainDeletionQueue.pushFunction([&](){
		vkDestroySampler(device,defaultSamplerNearest,nullptr);
		vkDestroySampler(device,defaultSamplerLinear,nullptr);

		destroyImage(whiteImage);
		destroyImage(greyImage);
		destroyImage(blackImage);
		destroyImage(errorCheckerboardImage);
	});

	{
		
	GLTFMetallic_Roughness::MaterialResources materialResources;
	//default the material textures
	materialResources.colorImage = whiteImage;
	materialResources.colorSampler = defaultSamplerLinear;
	materialResources.metalRoughImage = whiteImage;
	materialResources.metalRoughSampler = defaultSamplerLinear;

	//set the uniform buffer for the material data
	AllocatedBuffer materialConstants = createBuffer(sizeof(GLTFMetallic_Roughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//write the buffer
	GLTFMetallic_Roughness::MaterialConstants* sceneUniformData = (GLTFMetallic_Roughness::MaterialConstants*)materialConstants.allocation->GetMappedData();
	sceneUniformData->colorFactors = glm::vec4{1,1,1,1};
	sceneUniformData->metalRoughFactors = glm::vec4{1,0.5,0,0};

	mainDeletionQueue.pushFunction([=, this]() {
		destroyBuffer(materialConstants);
	});

	materialResources.dataBuffer = materialConstants.buffer;
	materialResources.dataBufferOffset = 0;

	defaultData = metalRoughMaterial.writeMaterial(device,MaterialPass::MainColor,materialResources, globalDescriptorAllocator);

	for (auto& m : testMeshes) {
		std::shared_ptr<MeshNode> newNode = std::make_shared<MeshNode>();
		newNode->mesh = m;

		newNode->localTransform = glm::mat4{ 1.f };
		newNode->worldTransform = glm::mat4{ 1.f };

		for (auto& s : newNode->mesh->surfaces) {
			s.material = std::make_shared<GLTFMaterial>(defaultData);
		}

		loadedNodes[m->name] = std::move(newNode);
	}
	}
}

void VulkanEngine::resizeSwapchain()
{
	vkDeviceWaitIdle(device);

	destroySwapchain();

	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	windowExtent.width = w;
	windowExtent.height = h;

	createSwapchain(windowExtent.width, windowExtent.height);

	resizeRequested = false;
}

AllocatedImage VulkanEngine::createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage newImage;
	newImage.imageFormat = format;
	newImage.imageExtent = size;

	VkImageCreateInfo imgInfo = vkInit::image_create_info(format, usage, size);
	if (mipmapped) {
		imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VK_CHECK(vmaCreateImage(allocator, &imgInfo, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo viewInfo = vkInit::imageview_create_info(format, newImage.image, aspectFlag);
	viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;

	VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &newImage.imageView));

	return newImage;
}

AllocatedImage VulkanEngine::createImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	size_t data_size = size.depth * size.width * size.height * 4;
	AllocatedBuffer uploadbuffer = createBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(uploadbuffer.info.pMappedData, data, data_size);

	AllocatedImage newImage = createImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	immediateSubmit([&](VkCommandBuffer cmd) {
		vkUtil::transition_image(cmd, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = size;

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		vkUtil::transition_image(cmd, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

	destroyBuffer(uploadbuffer);

	return newImage;
}

void VulkanEngine::destroyImage(const AllocatedImage& img)
{
    vkDestroyImageView(device, img.imageView, nullptr);
    vmaDestroyImage(allocator, img.image, img.allocation);
}



void GLTFMetallic_Roughness::buildPipelines(VulkanEngine* engine)
{

	std::string shadersRootPath{"../ShaderCompiler"};

	
#ifdef SHADERS_PATH
	shadersRootPath = SHADERS_PATH;
#endif
	auto meshFrag = shadersRootPath + "/mesh.frag.spv";
	VkShaderModule meshFragShader;
	if (!vkUtil::loadShaderModule(meshFrag, engine->device, meshFragShader)) {
		fmt::println("Error when building the triangle fragment shader module");
	}
	auto meshVert = shadersRootPath + "/mesh.vert.spv";
	VkShaderModule meshVertexShader;
	if (!vkUtil::loadShaderModule(meshVert, engine->device, meshVertexShader)) {
		fmt::println("Error when building the triangle vertex shader module");
	}

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.addBinding(0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    materialLayout = layoutBuilder.build(engine->device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layouts[] = { engine->gpuSceneDataDescriptorLayout,
        materialLayout };

	VkPipelineLayoutCreateInfo meshLayoutInfo = vkInit::pipelineLayoutCreateInfo();
	meshLayoutInfo.setLayoutCount = 2;
	meshLayoutInfo.pSetLayouts = layouts;
	meshLayoutInfo.pPushConstantRanges = &matrixRange;
	meshLayoutInfo.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->device, &meshLayoutInfo, nullptr, &newLayout));

    opaquePipeline.layout = newLayout;
    transparentPipeline.layout = newLayout;

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	std::string entryPoint = "main";
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.setShaders(meshVertexShader,entryPoint, meshFragShader,entryPoint);
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.setMultisamplingNone();
	pipelineBuilder.disableBlending();
	pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//render format
	pipelineBuilder.setColorAttachmentFormat(engine->drawImage.imageFormat);
	pipelineBuilder.setDepthFormat(engine->depthImage.imageFormat);

	// use the triangle layout we created
	pipelineBuilder.pipelineLayout = newLayout;

	// finally build the pipeline
    opaquePipeline.pipeline = pipelineBuilder.buildPipeline(engine->device);

	// create the transparent variant
	pipelineBuilder.enableBlendingAdditive();

	pipelineBuilder.enableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	transparentPipeline.pipeline = pipelineBuilder.buildPipeline(engine->device);
	
	vkDestroyShaderModule(engine->device, meshFragShader, nullptr);
	vkDestroyShaderModule(engine->device, meshVertexShader, nullptr);
	
}

MaterialInstance GLTFMetallic_Roughness::writeMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
{
	MaterialInstance matData;
	matData.passType = pass;
	if (pass == MaterialPass::Transparent) {
		matData.pipeline = &transparentPipeline;
	}
	else {
		matData.pipeline = &opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.allocate(device, materialLayout);


	writer.clear();
	writer.writeBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.writeImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.writeImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.updateSet(device, matData.materialSet);

	return matData;
}
void GLTFMetallic_Roughness::clearResources(VkDevice device){
	vkDestroyDescriptorSetLayout(device,materialLayout,nullptr);
	vkDestroyPipelineLayout(device,transparentPipeline.layout,nullptr);

	vkDestroyPipeline(device, transparentPipeline.pipeline, nullptr);
	vkDestroyPipeline(device, opaquePipeline.pipeline, nullptr);
}

void VulkanEngine::updateScene()
{

	auto start = std::chrono::system_clock::now();
	//scene update logic
	{
		mainDrawContext.OpaqueSurfaces.clear();
		mainDrawContext.TransparentSurfaces.clear();
		//camera.getTransformationRW().getPositionRW() = glm::vec3(0.0f,0.0f,5.0f);
		camera.setPerspectiveProjection(glm::radians(60.f),(float)drawExtent.width / (float)drawExtent.height,1000.0f,0.01f);
		loadedNodes["Suzanne"]->Draw(glm::mat4{1.f}, mainDrawContext);	

		sceneData.view = camera.getViewMatrix();
		// camera projection
		sceneData.proj = camera.getProjectionMatrix();

		sceneData.viewproj = sceneData.proj * sceneData.view;

		//some default lighting parameters
		sceneData.ambientColor = glm::vec4(.1f);
		sceneData.sunlightColor = glm::vec4(1.f);
		sceneData.sunlightDirection = glm::vec4(0,1,0.5,1.f);

		for (int x = -3; x < 3; x++) {

			glm::mat4 scale = glm::scale(glm::vec3{0.2});
			glm::mat4 translation =  glm::translate(glm::vec3{x, 1, 0});

			loadedNodes["Cube"]->Draw(translation * scale, mainDrawContext);
		}
		loadedScenes["cube"]->Draw(glm::mat4{ 1.f }, mainDrawContext);
		loadedScenes["structure"]->Draw(glm::mat4{ 1.f }, mainDrawContext);
	}
	
	auto end = std::chrono::system_clock::now();

    //convert to microseconds (integer), and then come back to miliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.scene_update_time = elapsed.count() / 1000.f;
}


void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
	glm::mat4 nodeMatrix = topMatrix * worldTransform;

	for (auto& s : mesh->surfaces) {
		RenderObject def;
		def.indexCount = s.count;
		def.firstIndex = s.startIndex;
		def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
		def.material = &s.material->data;

		def.transform = nodeMatrix;
		def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;
		
		ctx.OpaqueSurfaces.push_back(def);
	}

	

	// recurse down
	Node::Draw(topMatrix, ctx);
}


