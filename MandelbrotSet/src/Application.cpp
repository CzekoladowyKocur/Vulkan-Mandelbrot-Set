#pragma once
#include "include\Application.h"
#include "include\Platform.h"
#include "include\Input.h"
#include "glm/glm.hpp"

namespace Utilities {
	#if APP_DEBUG
	/* Instance */
	INTERNALSCOPE const std::vector<const char*> RequiredExtensions = { "VK_KHR_win32_surface", "VK_KHR_surface", "VK_EXT_debug_utils",
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
	INTERNALSCOPE const std::vector<const char*> RequestedLayers = { "VK_LAYER_KHRONOS_validation" };
	INTERNALSCOPE VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		uint64_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData);
	/* Validation is enabled if debug mode is on */
	#else		
	INTERNALSCOPE const std::vector<const char*> RequiredExtensions = { "VK_KHR_win32_surface", "VK_KHR_surface" };
	INTERNALSCOPE const std::vector<const char*> RequestedLayers = {};
	#endif
	INTERNALSCOPE const std::string_view RetrieveVendorFromID(const uint32_t vendorID);
	
	/* Logical Device */
	INTERNALSCOPE const std::vector<const char*> RequiredDeviceExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	INTERNALSCOPE const std::vector<const char*> RequestedDeviceLayers = {
	};

	constexpr uint64_t MaxSwapchainTimeout = UINT64_MAX;
	/* These can be tweaked. In order to change the dimensions, adjust the compute shader code and recompile them. */
	constexpr uint32_t ComputeRenderWidth = 3200 * 2;
	constexpr uint32_t ComputeRenderHeight = 2400 * 2;
	constexpr std::size_t ComputeBufferSize = ComputeRenderWidth * ComputeRenderHeight * sizeof(float) * 4;
	constexpr std::size_t RenderedImageSize = ComputeRenderWidth * ComputeRenderHeight * sizeof(uint8_t) * 4;
}

VulkanApp* VulkanApp::s_ApplicationInstance = nullptr;
VulkanApp::VulkanApp(const ERenderMethod renderMethod, HINSTANCE hInstance, const bool showConsole)
	:
	m_RenderMethod(renderMethod),
	m_Running(true),
	m_Window(renderMethod == ERenderMethod::Graphics ? new Window(hInstance, { 1280, 720, showConsole, std::bind(&VulkanApp::OnEvent, this, std::placeholders::_1) }) : nullptr),
	/* Vulkan API */
	m_Instance(VK_NULL_HANDLE),
	m_Surface(VK_NULL_HANDLE),
	m_PhysicalDeviceProperties(),
	m_PhysicalDeviceFeatures(),
	m_PhysicalDeviceMemoryProperties(),
	m_PhysicalDevice(VK_NULL_HANDLE),
	m_QueueIndices(),
	m_LogicalDevice(VK_NULL_HANDLE),
	m_GraphicsQueue(VK_NULL_HANDLE),
	m_ComputeQueue(VK_NULL_HANDLE),
	m_PresentQueue(VK_NULL_HANDLE),
	m_GraphicsCommandPool(VK_NULL_HANDLE),
	m_ComputeCommandPool(VK_NULL_HANDLE),
	m_Swapchain(VK_NULL_HANDLE),
	m_SurfaceCapabilities(),
	m_SurfaceFormat(),
	m_PresentMode(),
	m_SwapchainExtent({ 1280, 720 }),
	m_Semaphores(),
	m_MaxFramesInFlight(2),
	m_ImageCount(0),
	m_MinimalImageCount(0),
	m_SwapchainImages(),
	m_SwapchainImageViews(),
	m_SwapchainFramebuffers(),
	m_SwapchainRenderPass(VK_NULL_HANDLE),
	m_VertexBuffer(),
	m_IndexBuffer(),
	m_UBOBuffer(),
	m_VertexShaderModule(VK_NULL_HANDLE),
	m_FragmentShaderModule(VK_NULL_HANDLE),
	m_GraphicsPipelineUBOBufferDescriptorSetLayout(VK_NULL_HANDLE),
	m_GraphicsPipelineColorPaletteDescriptorSetLayout(VK_NULL_HANDLE),
	m_GraphicsPipeline(VK_NULL_HANDLE),
	m_GraphicsPipelineLayout(VK_NULL_HANDLE),
	m_GraphicsPipelineDescriptorPool(VK_NULL_HANDLE),
	m_GraphicsPipelineUBOBufferDescriptorSet(VK_NULL_HANDLE),
	m_GraphicsPipelineColorPaletteDescriptorSet(VK_NULL_HANDLE),
	m_GraphicsPipelineCommandBuffers(),
	m_ComputePipelineStorageBuffer(),
	m_ComputeShaderModule(VK_NULL_HANDLE),
	m_ComputePipelineDescriptorSetLayout(VK_NULL_HANDLE),
	m_ComputePipelineDescriptorPool(VK_NULL_HANDLE),
	m_ComputePipelineStorageBufferDescriptorSet(VK_NULL_HANDLE),
	m_ComputePipeline(VK_NULL_HANDLE),
	m_ComputePipelineLayout(VK_NULL_HANDLE),
	m_ComputePipelineCommandBuffer(VK_NULL_HANDLE),
	m_ImageIndex(0),
	m_FrameIndex(0),
	m_InFlightFences(),
	m_ImagesInFlight(),
	m_ColorPaletteImage(nullptr)
#ifdef APP_DEBUG
	,m_DebugReportCallback(VK_NULL_HANDLE)
#endif
{
	assert(!s_ApplicationInstance);
	s_ApplicationInstance = this;
}

VulkanApp::~VulkanApp()
{
	m_Running = false;
	if(m_Window)
		delete m_Window;

	/* Vulkan API */
}

bool VulkanApp::Initialize()
{
	if (!CreateInstance())
	{
		printf("Failed to create vulkan instance\n");
		return false;
	}

	if (!CreateLogicalDevice())
	{
		printf("Failed to create vulkan logical device\n");
		return false;
	}

	if (m_RenderMethod == ERenderMethod::Graphics)
	{
		if (!LoadAssets())
		{
			printf("Failed to load assets\n");
			return false;
		}

		if (!CreateSurface())
		{
			printf("Failed to create vulkan surface\n");
			return false;
		}

		if (!CreateSwapchain())
		{
			printf("Failed to create vulkan swapchain\n");
			return false;
		}

		if (!CreateGraphicsBasedPipeline())
		{
			printf("Failed to create graphics based pipeline\n");
			return false;
		}

		if (!AllocateGraphicsCommandBuffers())
		{
			printf("Failed to allocate graphics command buffers\n");
			return false;
		}

		if (!RecordGraphicsCommandBuffers())
		{
			printf("Failed to create graphics command buffers\n");
			return false;
		}
	}
	else
	{
		if (!CreateComputeBasedPipeline())
		{
			printf("Failed to create compute based pipeline\n");
			return false;
		}
		
		if (!AllocateComputeCommandBuffers())
		{
			printf("Failed to allocate compute commands buffers\n");
			return false;
		}
		
		if (!RecordComputeCommandBuffers())
		{
			printf("Failed to create compute command buffers\n");
			return false;
		}
	}

	return true;
}

bool VulkanApp::Run()
{
	double timer = 0.0;
	if (m_RenderMethod == ERenderMethod::Compute)
	{
		DrawFrame();
		return true;
	}

	while (m_Running) 
	{
		/* Poll events */

		m_Window->PollEvents();
		const double deltaTime = Platform::GetAbsoluteTime() - timer;
		timer = Platform::GetAbsoluteTime();

		UpdateFrameData(deltaTime);
		DrawFrame();
	}

	return true;
}

bool VulkanApp::Shutdown()
{
	VK_CHECK(vkDeviceWaitIdle(m_LogicalDevice));
	delete m_ColorPaletteImage;
	/* Device level */
	VK_CHECK(vkDeviceWaitIdle(m_LogicalDevice));

	/* Graphics */
	/* Destroy buffers */
	if (m_UBOBuffer.Handle)
	{
		vkFreeMemory(
			m_LogicalDevice,
			m_UBOBuffer.DeviceMemory,
			nullptr);

		vkDestroyBuffer(
			m_LogicalDevice,
			m_UBOBuffer.Handle,
			nullptr);
	}

	if (m_VertexBuffer.Handle)
	{
		vkFreeMemory(
			m_LogicalDevice,
			m_VertexBuffer.DeviceMemory,
			nullptr);
	
		vkDestroyBuffer(
			m_LogicalDevice,
			m_VertexBuffer.Handle,
			nullptr);
	}
	
	if (m_IndexBuffer.Handle)
	{
		vkFreeMemory(
			m_LogicalDevice,
			m_IndexBuffer.DeviceMemory,
			nullptr);
	
		vkDestroyBuffer(
			m_LogicalDevice,
			m_IndexBuffer.Handle,
			nullptr);
	}
	
	/* Destroy Pipelines */
	if(m_GraphicsPipeline)
		vkDestroyPipeline(
			m_LogicalDevice,
			m_GraphicsPipeline,
			nullptr);

	if (m_GraphicsPipelineLayout)
		vkDestroyPipelineLayout(
			m_LogicalDevice,
			m_GraphicsPipelineLayout,
			nullptr);

	if (m_GraphicsPipelineUBOBufferDescriptorSetLayout)
		vkDestroyDescriptorSetLayout(
			m_LogicalDevice,
			m_GraphicsPipelineUBOBufferDescriptorSetLayout,
			nullptr);

	if (m_GraphicsPipelineColorPaletteDescriptorSetLayout)
		vkDestroyDescriptorSetLayout(
			m_LogicalDevice,
			m_GraphicsPipelineColorPaletteDescriptorSetLayout,
			nullptr);

	if (m_GraphicsPipelineColorPaletteDescriptorSet)
		vkFreeDescriptorSets(
			m_LogicalDevice,
			m_GraphicsPipelineDescriptorPool,
			1,
			&m_GraphicsPipelineColorPaletteDescriptorSet);
			
	if (m_GraphicsPipelineDescriptorPool)
		vkDestroyDescriptorPool(
			m_LogicalDevice,
			m_GraphicsPipelineDescriptorPool,
			nullptr);

	if(m_GraphicsCommandPool)
		vkDestroyCommandPool(
			m_LogicalDevice,
			m_GraphicsCommandPool,
			nullptr);
	/* Compute */
	if (m_ComputePipelineStorageBuffer.Handle)
		vkDestroyBuffer(
			m_LogicalDevice,
			m_ComputePipelineStorageBuffer.Handle,
			nullptr);

	if (m_ComputePipelineStorageBuffer.DeviceMemory)
		vkFreeMemory(
			m_LogicalDevice,
			m_ComputePipelineStorageBuffer.DeviceMemory,
			nullptr);

	if (m_ComputePipeline)
		vkDestroyPipeline(
			m_LogicalDevice,
			m_ComputePipeline,
			nullptr);

	if (m_ComputePipelineLayout)
		vkDestroyPipelineLayout(
			m_LogicalDevice,
			m_ComputePipelineLayout,
			nullptr);

	if (m_ComputePipelineDescriptorSetLayout)
		vkDestroyDescriptorSetLayout(
			m_LogicalDevice,
			m_ComputePipelineDescriptorSetLayout,
			nullptr);

	if(m_ComputePipelineDescriptorPool)
		vkDestroyDescriptorPool(
			m_LogicalDevice,
			m_ComputePipelineDescriptorPool,
			nullptr);

	if (m_ComputeCommandPool)
		vkDestroyCommandPool(
			m_LogicalDevice,
			m_ComputeCommandPool,
			nullptr);

	CleanupSwapchain();
	VK_CHECK(vkDeviceWaitIdle(m_LogicalDevice));
	vkDestroyDevice(
		m_LogicalDevice,
		nullptr);

	/* Instance level */
	vkDestroySurfaceKHR(
		m_Instance,
		m_Surface,
		nullptr);
#ifdef APP_DEBUG
	auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugReportCallbackEXT");
	assert(vkDestroyDebugReportCallbackEXT != nullptr);
	vkDestroyDebugReportCallbackEXT(
		m_Instance,
		m_DebugReportCallback,
		nullptr);
#endif
	vkDestroyInstance(
		m_Instance,
		nullptr);

	return true;
}

void VulkanApp::OnEvent(Event& event)
{
	switch (event.GetEventType())
	{
		case EEventType::WindowClose:
		{
			m_Running = false;
			
			break;
		}

		case EEventType::WindowResize:
		{
			WindowResizeEvent* e = (WindowResizeEvent*)&event;
			const auto [windowWidth, windowHeight] = e->GetSize();
			m_SwapchainExtent.width = windowWidth;
			m_SwapchainExtent.height = windowHeight;

			printf("Window resized: [width, height]: %d, %d\n", windowWidth, windowHeight);
			break;
		}
	}
}

bool VulkanApp::Close()
{
	s_ApplicationInstance->m_Running = false;
	return true;
}

VulkanApp* VulkanApp::GetInstance()
{
	return s_ApplicationInstance;
}

bool VulkanApp::CreateInstance()
{
	VkApplicationInfo applicationInfo;
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	applicationInfo.pApplicationName = "Mandelbrot Renderer";
	applicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	applicationInfo.pEngineName = "Good engine name";
	applicationInfo.apiVersion = VK_API_VERSION_1_2;
	applicationInfo.pNext = nullptr;

	std::vector<const char*> availableRequestedLayers;
	/* Print required instance-level extensions */
	for (const char* requiredExtension : Utilities::RequiredExtensions)
		printf("Requested layer: %s\n", requiredExtension);

	/* Verify availibility of instance extensions (required) */
	uint32_t extensionPropertiesCount;
	vkEnumerateInstanceExtensionProperties(0, &extensionPropertiesCount, nullptr);
	std::vector<VkExtensionProperties> availableInstanceExtensions(extensionPropertiesCount);
	vkEnumerateInstanceExtensionProperties(0, &extensionPropertiesCount, availableInstanceExtensions.data());

	for (uint32_t i = 0; i < static_cast<uint32_t>(Utilities::RequiredExtensions.size()); ++i)
	{
		bool found = false;
		for (uint32_t j = 0; i < extensionPropertiesCount; ++j)
		{
			if (strcmp(Utilities::RequiredExtensions[i], availableInstanceExtensions[j].extensionName))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			assert(false);
			printf("Failed to find instance extension with name: %s\n", Utilities::RequiredExtensions[i]);
		}
	}

	/* Print requested instance-level layers */
	for (const char* requestedLayer : Utilities::RequestedLayers)
		printf("Requested layer with name %s\n", requestedLayer);

	/* Verify availibility of instance layers (requested, not required) */
	uint32_t instanceLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, 0);
	std::vector<VkLayerProperties> availableInstanceLayers(instanceLayerCount);
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, availableInstanceLayers.data());

	for (uint32_t i = 0; i < static_cast<uint32_t>(Utilities::RequestedLayers.size()); ++i)
	{
		bool found = false;
		for (uint32_t j = 0; j < instanceLayerCount; ++j)
		{
			if (strcmp(Utilities::RequestedLayers[i], availableInstanceLayers[j].layerName) == 0)
			{
				found = true;
				break;
			}
		}

		if (found)
			availableRequestedLayers.push_back(Utilities::RequestedLayers[i]);
		else
			printf("Failed to find a requested instance-level layer with given name: %s\n", Utilities::RequestedLayers[i]);
	}

	if (Utilities::RequestedLayers != availableRequestedLayers)
		printf("Found atleast one requested instance-level layer to be missing!\n");

	VkInstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(Utilities::RequiredExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = Utilities::RequiredExtensions.data();
	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(availableRequestedLayers.size());;
	instanceCreateInfo.ppEnabledLayerNames = availableRequestedLayers.data();
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pNext = 0;

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		printf("Failed to initialize vulkan instance. You may want to update your drivers\n");
		return false;
	}
#ifdef APP_DEBUG 
	auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugReportCallbackEXT");
	assert(vkCreateDebugReportCallbackEXT != nullptr);
	VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
	debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	debug_report_ci.pfnCallback = Utilities::VulkanDebugReportCallback;
	debug_report_ci.pUserData = nullptr;
	vkCreateDebugReportCallbackEXT(m_Instance, &debug_report_ci, nullptr, &m_DebugReportCallback);
#endif	
	return true;
}

bool VulkanApp::CreateSurface()
{
	const auto [windowHandle, hInstance] = m_Window->GetInternalState();
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hwnd = windowHandle;
	surfaceCreateInfo.hinstance = hInstance;
	surfaceCreateInfo.flags = 0;
	surfaceCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateWin32SurfaceKHR(
		m_Instance,
		&surfaceCreateInfo,
		nullptr,
		&m_Surface));

	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_QueueIndices.Graphics, m_Surface, &supported);
	return true;
}


typedef bool b8;
typedef uint32_t u32;

bool VulkanApp::CreateLogicalDevice()
{
	uint32_t physicalDeviceCount;
	VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, nullptr));
	assert(physicalDeviceCount > 0);
	std::vector<VkPhysicalDevice> availablePhysicalDevices(physicalDeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, availablePhysicalDevices.data()));

	for (const VkPhysicalDevice availablePhysicalDevice : availablePhysicalDevices)
	{
		vkGetPhysicalDeviceProperties(availablePhysicalDevice, &m_PhysicalDeviceProperties);
		vkGetPhysicalDeviceFeatures(availablePhysicalDevice, &m_PhysicalDeviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(availablePhysicalDevice, &m_PhysicalDeviceMemoryProperties);

		if (m_PhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			m_PhysicalDevice = availablePhysicalDevice;
			break;
		}
	}

	if (!m_PhysicalDevice)
	{
		m_PhysicalDevice = availablePhysicalDevices[0];

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_PhysicalDeviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_PhysicalDeviceMemoryProperties);
	}

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, 0);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());
	
	int32_t requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
	m_QueueIndices = GetQueueFamilyIndices(requestedQueueTypes);
	static const float defaultQueuePriority(0.0f);
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;

	// Graphics queue
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = m_QueueIndices.Graphics;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		deviceQueueCreateInfos.push_back(queueInfo);
	}

	// Dedicated compute queue
	if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
	{
		if (m_QueueIndices.Compute != m_QueueIndices.Graphics)
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = m_QueueIndices.Compute;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			deviceQueueCreateInfos.push_back(queueInfo);
		}
	}

	if (m_RenderMethod == ERenderMethod::Graphics)
		m_PresentQueue = m_GraphicsQueue;

	constexpr float defaultQueuePrority[1] = { 1.0f };
	VkPhysicalDeviceFeatures enabledFeatures = {};
	enabledFeatures.shaderFloat64 = m_PhysicalDeviceFeatures.shaderFloat64;
	
	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.enabledExtensionCount = Utilities::RequiredDeviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = Utilities::RequiredDeviceExtensions.data();
	deviceCreateInfo.enabledLayerCount = Utilities::RequestedDeviceLayers.size();
	deviceCreateInfo.ppEnabledLayerNames = Utilities::RequestedDeviceLayers.data();
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.pNext = nullptr;

	if (vkCreateDevice(
		m_PhysicalDevice,
		&deviceCreateInfo,
		nullptr,
		&m_LogicalDevice) != VK_SUCCESS)
	{
		printf("Failed to create vulkan logical device\n");
		return false;
	}

	vkGetDeviceQueue(
		m_LogicalDevice,
		m_QueueIndices.Graphics,
		0,
		&m_GraphicsQueue);

	vkGetDeviceQueue(
		m_LogicalDevice,
		m_QueueIndices.Compute,
		0,
		&m_ComputeQueue);

	VkCommandPoolCreateInfo graphicsCommandPoolCreateInfo;
	graphicsCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicsCommandPoolCreateInfo.queueFamilyIndex = m_QueueIndices.Graphics;
	graphicsCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	graphicsCommandPoolCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateCommandPool(
		m_LogicalDevice,
		&graphicsCommandPoolCreateInfo,
		nullptr,
		&m_GraphicsCommandPool));

	VkCommandPoolCreateInfo computeCommandPoolCreateInfo;
	computeCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	computeCommandPoolCreateInfo.queueFamilyIndex = m_QueueIndices.Compute;
	computeCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	computeCommandPoolCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateCommandPool(
		m_LogicalDevice,
		&computeCommandPoolCreateInfo,
		nullptr,
		&m_ComputeCommandPool));

	return true;
}

bool VulkanApp::CreateSwapchain()
{
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		m_PhysicalDevice,
		m_Surface,
		&m_SurfaceCapabilities));

	uint32_t surfaceFormatCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
		m_PhysicalDevice,
		m_Surface,
		&surfaceFormatCount,
		nullptr));
	assert(surfaceFormatCount > 0);

	std::vector<VkSurfaceFormatKHR> availableSurfaceFormats(surfaceFormatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
		m_PhysicalDevice,
		m_Surface,
		&surfaceFormatCount,
		availableSurfaceFormats.data()));

	m_SurfaceFormat = availableSurfaceFormats[0];
	for (const VkSurfaceFormatKHR surfaceFormat : availableSurfaceFormats)
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			m_SurfaceFormat = surfaceFormat;
			break;
		}

	uint32_t presentModeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
		m_PhysicalDevice,
		m_Surface,
		&presentModeCount,
		nullptr));
	assert(presentModeCount > 0);

	std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
		m_PhysicalDevice,
		m_Surface,
		&presentModeCount,
		availablePresentModes.data()));

	/* The only present mode guaranteed to be supported by the specification */
	m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const VkPresentModeKHR presentMode : availablePresentModes)
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			m_PresentMode = presentMode;
			break;
		}

	m_MaxFramesInFlight = m_PresentMode == VK_PRESENT_MODE_MAILBOX_KHR ? (3) : (2);

	const VkExtent2D minSwapchainImageExtent = m_SurfaceCapabilities.minImageExtent;
	const VkExtent2D maxSwapchainImageExtent = m_SurfaceCapabilities.maxImageExtent;

	/* Clamp */
	m_SwapchainExtent.width = APP_CLAMP(m_SwapchainExtent.width, minSwapchainImageExtent.width, maxSwapchainImageExtent.width);
	m_SwapchainExtent.height = APP_CLAMP(m_SwapchainExtent.height, minSwapchainImageExtent.height, maxSwapchainImageExtent.height);

	const uint32_t minImageCount = m_SurfaceCapabilities.minImageCount;
	const uint32_t maxImageCount = m_SurfaceCapabilities.maxImageCount;
	m_ImageCount = (minImageCount + 1) < maxImageCount ? (minImageCount + 1) : maxImageCount;
	
	VkSwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = m_Surface;
	swapchainCreateInfo.imageFormat = m_SurfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
	swapchainCreateInfo.presentMode = m_PresentMode;
	swapchainCreateInfo.imageExtent = m_SwapchainExtent;
	swapchainCreateInfo.minImageCount = m_ImageCount;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.queueFamilyIndexCount = 0; //queuesShared ? 0 : 2; /* One for graphics and one for present if not shared */
	swapchainCreateInfo.pQueueFamilyIndices = nullptr; //queuesShared ? nullptr : queueFamilyIndices;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // queuesShared ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.preTransform = m_SurfaceCapabilities.currentTransform;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.pNext = nullptr;

	if (vkCreateSwapchainKHR(
		m_LogicalDevice,
		&swapchainCreateInfo,
		nullptr,
		&m_Swapchain) != VK_SUCCESS)
	{
		printf("Failed to create swapchain\n");
		return false;
	}

	m_ImageCount = 0;
	m_SwapchainImages.clear();
	VK_CHECK(vkGetSwapchainImagesKHR(
		m_LogicalDevice,
		m_Swapchain,
		&m_ImageCount,
		nullptr));

	m_SwapchainImages.resize(m_ImageCount);
	m_SwapchainImageViews.resize(m_ImageCount);
	m_SwapchainFramebuffers.resize(m_ImageCount);

	VK_CHECK(vkGetSwapchainImagesKHR(
		m_LogicalDevice,
		m_Swapchain,
		&m_ImageCount,
		m_SwapchainImages.data()));

	VkAttachmentDescription colorAttachment;
	colorAttachment.format = m_SurfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.flags = 0;

	VkAttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = nullptr;
	subpassDescription.flags = 0;
	
	VkSubpassDependency subpassDependency;
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0; /* What subpass we are writing to */
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dependencyFlags = 0;

	const std::array<VkAttachmentDescription, 1> attachments = { colorAttachment };
	VkRenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateRenderPass(
		m_LogicalDevice,
		&renderPassCreateInfo,
		nullptr,
		&m_SwapchainRenderPass));

	uint32_t i = 0;
	for (const VkImage image : m_SwapchainImages)
	{
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.format = m_SurfaceFormat.format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.pNext = nullptr;

		VK_CHECK(vkCreateImageView(
			m_LogicalDevice,
			&imageViewCreateInfo,
			nullptr,
			&m_SwapchainImageViews[i]));

		VkFramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_SwapchainRenderPass;
		framebufferCreateInfo.width = m_SwapchainExtent.width;
		framebufferCreateInfo.height = m_SwapchainExtent.height;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &m_SwapchainImageViews[i];
		framebufferCreateInfo.layers = 1;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.pNext = nullptr;

		VK_CHECK(vkCreateFramebuffer(
			m_LogicalDevice,
			&framebufferCreateInfo,
			nullptr,
			&m_SwapchainFramebuffers[i]));

		++i;
	}

	VkSemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.flags = VK_SEMAPHORE_TYPE_BINARY;
	semaphoreCreateInfo.pNext = nullptr;

	VkFenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	fenceCreateInfo.pNext = nullptr;

	m_Semaphores.PresentComplete.resize(m_MaxFramesInFlight);
	m_Semaphores.RenderComplete.resize(m_MaxFramesInFlight);
	m_InFlightFences.resize(m_MaxFramesInFlight);
	for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i)
	{
		VK_CHECK(vkCreateSemaphore(
			m_LogicalDevice,
			&semaphoreCreateInfo,
			nullptr,
			&m_Semaphores.PresentComplete[i]));

		VK_CHECK(vkCreateSemaphore(
			m_LogicalDevice,
			&semaphoreCreateInfo,
			nullptr,
			&m_Semaphores.RenderComplete[i]));

		VK_CHECK(vkCreateFence(
			m_LogicalDevice,
			&fenceCreateInfo,
			nullptr,
			&m_InFlightFences[i]));
	}

	m_ImagesInFlight.resize(m_ImageCount, VK_NULL_HANDLE);
	return true;
}

bool VulkanApp::LoadAssets()
{
	m_ColorPaletteImage = new Image2D("assets/images/violetPalette.bmp");

	return true;
}

bool VulkanApp::CreateGraphicsBasedPipeline()
{
	constexpr VkDeviceSize __vbSize = sizeof(float) * 4 * 3;
	constexpr VkDeviceSize __ibSize = sizeof(uint32_t) * 6;
	const float fullscreenQuadVertices[4 * 3]
	{
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f
	};

	const uint32_t fullscreenQuadIndices[6]
	{
		0, 1, 2, 
		2, 3, 0
	};

	m_VertexBuffer.CPUData.Allocate(__vbSize);
	m_VertexBuffer.CPUData.Write(__vbSize, fullscreenQuadVertices);

	m_IndexBuffer.CPUData.Allocate(__ibSize);
	m_IndexBuffer.CPUData.Write(__ibSize, fullscreenQuadIndices);

	/* Vertex Staging Buffer */
	{
		VkBuffer vertexStagingBuffer;
		VkDeviceMemory vertexStagingBufferMemory;

		VkBufferCreateInfo vertexStagingBufferCreateInfo;
		vertexStagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexStagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		vertexStagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vertexStagingBufferCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
		vertexStagingBufferCreateInfo.pQueueFamilyIndices = nullptr;
		vertexStagingBufferCreateInfo.size = __vbSize;
		vertexStagingBufferCreateInfo.flags = 0;
		vertexStagingBufferCreateInfo.pNext = nullptr;

		VK_CHECK(vkCreateBuffer(
			m_LogicalDevice,
			&vertexStagingBufferCreateInfo,
			nullptr,
			&vertexStagingBuffer));

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(
			m_LogicalDevice,
			vertexStagingBuffer,
			&memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = RetrieveMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		allocateInfo.pNext = nullptr;

		vkAllocateMemory(
			m_LogicalDevice,
			&allocateInfo,
			nullptr,
			&vertexStagingBufferMemory);

		vkBindBufferMemory(
			m_LogicalDevice,
			vertexStagingBuffer,
			vertexStagingBufferMemory,
			0);

		void* data;
		vkMapMemory(m_LogicalDevice, vertexStagingBufferMemory, 0, __vbSize, 0, &data);
		memcpy(data, m_VertexBuffer.CPUData.Data(), __vbSize);
		vkUnmapMemory(m_LogicalDevice, vertexStagingBufferMemory);
		/* Vertex Staging Buffer */

		VkBufferCreateInfo vertexBufferCreateInfo;
		vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vertexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vertexBufferCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
		vertexBufferCreateInfo.pQueueFamilyIndices = nullptr;
		vertexBufferCreateInfo.size = __vbSize;
		vertexBufferCreateInfo.flags = 0;
		vertexBufferCreateInfo.pNext = nullptr;

		VK_CHECK(vkCreateBuffer(
			m_LogicalDevice,
			&vertexBufferCreateInfo,
			nullptr,
			&m_VertexBuffer.Handle));

		memoryRequirements;
		vkGetBufferMemoryRequirements(
			m_LogicalDevice,
			m_VertexBuffer.Handle,
			&memoryRequirements);

		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = RetrieveMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		allocateInfo.pNext = nullptr;

		vkAllocateMemory(
			m_LogicalDevice,
			&allocateInfo,
			nullptr,
			&m_VertexBuffer.DeviceMemory);

		vkBindBufferMemory(
			m_LogicalDevice,
			m_VertexBuffer.Handle,
			m_VertexBuffer.DeviceMemory,
			0);

		VkCommandBuffer commandBuffer = BeginRecordingSingleTimeUseCommands(false);
		VkBufferCopy bufferCopyRegion;
		bufferCopyRegion.size = __vbSize;
		bufferCopyRegion.srcOffset = 0;
		bufferCopyRegion.dstOffset = 0;

		vkCmdCopyBuffer(
			commandBuffer,
			vertexStagingBuffer,
			m_VertexBuffer.Handle,
			1,
			&bufferCopyRegion);
		EndRecordingSingleTimeUseCommands(commandBuffer, false);

		vkFreeMemory(
			m_LogicalDevice,
			vertexStagingBufferMemory,
			nullptr);

		vkDestroyBuffer(
			m_LogicalDevice,
			vertexStagingBuffer,
			nullptr);
	}

	/* Index staging buffer*/
	{
		VkBuffer stagingIndexBuffer;
		VkDeviceMemory stagingIndexBufferMemory;

		VkBufferCreateInfo stagingIndexBufferCreateInfo;
		stagingIndexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingIndexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingIndexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		stagingIndexBufferCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
		stagingIndexBufferCreateInfo.pQueueFamilyIndices = nullptr;
		stagingIndexBufferCreateInfo.size = __ibSize;
		stagingIndexBufferCreateInfo.flags = 0;
		stagingIndexBufferCreateInfo.pNext = nullptr;

		VK_CHECK(vkCreateBuffer(
			m_LogicalDevice,
			&stagingIndexBufferCreateInfo,
			nullptr,
			&stagingIndexBuffer));

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(
			m_LogicalDevice,
			stagingIndexBuffer,
			&memoryRequirements);

		VkMemoryAllocateInfo allocateInfo;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = RetrieveMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		allocateInfo.pNext = nullptr;

		VK_CHECK(vkAllocateMemory(
			m_LogicalDevice,
			&allocateInfo,
			nullptr,
			&stagingIndexBufferMemory));

		vkBindBufferMemory(
			m_LogicalDevice,
			stagingIndexBuffer,
			stagingIndexBufferMemory,
			0);

		void* data;
		vkMapMemory(m_LogicalDevice, stagingIndexBufferMemory, 0, __vbSize, 0, &data);
		memcpy(data, m_IndexBuffer.CPUData.Data(), __ibSize);
		vkUnmapMemory(m_LogicalDevice, stagingIndexBufferMemory);

		VkBufferCreateInfo indexBufferCreateInfo;
		indexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexBufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		indexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		indexBufferCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
		indexBufferCreateInfo.pQueueFamilyIndices = nullptr;
		indexBufferCreateInfo.size = __ibSize;
		indexBufferCreateInfo.flags = 0;
		indexBufferCreateInfo.pNext = nullptr;

		VK_CHECK(vkCreateBuffer(
			m_LogicalDevice,
			&indexBufferCreateInfo,
			nullptr,
			&m_IndexBuffer.Handle));

		vkGetBufferMemoryRequirements(
			m_LogicalDevice, 
			m_IndexBuffer.Handle, 
			&memoryRequirements);

		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = RetrieveMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		allocateInfo.pNext = nullptr;

		VK_CHECK(vkAllocateMemory(
			m_LogicalDevice,
			&allocateInfo,
			nullptr,
			&m_IndexBuffer.DeviceMemory));

		vkBindBufferMemory(
			m_LogicalDevice,
			m_IndexBuffer.Handle,
			m_IndexBuffer.DeviceMemory,
			0);

		VkCommandBuffer commandBuffer = BeginRecordingSingleTimeUseCommands(false);
		VkBufferCopy bufferCopyRegion;
		bufferCopyRegion.size = __ibSize;
		bufferCopyRegion.srcOffset = 0;
		bufferCopyRegion.dstOffset = 0;

		vkCmdCopyBuffer(
			commandBuffer,
			stagingIndexBuffer,
			m_IndexBuffer.Handle,
			1,
			&bufferCopyRegion);
		EndRecordingSingleTimeUseCommands(commandBuffer, false);

		vkFreeMemory(
			m_LogicalDevice,
			stagingIndexBufferMemory,
			nullptr);

		vkDestroyBuffer(
			m_LogicalDevice,
			stagingIndexBuffer,
			nullptr);
	}
	
	/* TODO: Add support for doubles */
	const bool deviceSupportsDoublePrecisionFloats = false; //m_PhysicalDeviceFeatures.shaderFloat64;
	m_VertexShaderModule = CreateShaderModule(deviceSupportsDoublePrecisionFloats ? "assets/shaders/vertexShaderDoublePrecision.spv" : "assets/shaders/vertexShader.spv");
	if (!m_VertexShaderModule)
	{
		printf("Failed to create vertex shader module\n");
		return false;
	}

	m_FragmentShaderModule = CreateShaderModule(deviceSupportsDoublePrecisionFloats ? "assets/shaders/fragmentShaderDoublePrecision.spv" : "assets/shaders/fragmentShader.spv");
	if (!m_FragmentShaderModule)
	{
		printf("Failed to create fragment shader module\n");
		return false;
	}

	VkPipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = m_VertexShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.pNext = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = m_FragmentShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;
	fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.pNext = nullptr;

	const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{ vertShaderStageInfo, fragShaderStageInfo };
	VkVertexInputBindingDescription vertexInputBindingDescription;
	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.stride = sizeof(float) * 3;
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	VkVertexInputAttributeDescription vertexInputAttributeDescription;
	vertexInputAttributeDescription.binding = 0;
	vertexInputAttributeDescription.location = 0;
	vertexInputAttributeDescription.offset = 0;
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = 1; 	
	vertexInputInfo.pVertexAttributeDescriptions = &vertexInputAttributeDescription;
	vertexInputInfo.flags = 0;
	vertexInputInfo.pNext = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	inputAssembly.flags = 0;
	inputAssembly.pNext = nullptr;

	VkViewport viewport;
	viewport.width = static_cast<float>(m_SwapchainExtent.width);
	viewport.height = static_cast<float>(m_SwapchainExtent.height);
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = m_SwapchainExtent;
	VkPipelineViewportStateCreateInfo viewportState;
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	viewportState.flags = 0;
	viewportState.pNext = nullptr;

	const std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateInfo.pDynamicStates = dynamicStates.data();
	dynamicStateInfo.flags = 0;
	dynamicStateInfo.pNext = nullptr;

	VkPipelineRasterizationStateCreateInfo rasterizerStateInfo{};
	rasterizerStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerStateInfo.depthClampEnable = VK_FALSE;
	rasterizerStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerStateInfo.lineWidth = 1.0f;
	rasterizerStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizerStateInfo.depthBiasEnable = VK_FALSE;
	rasterizerStateInfo.flags = 0;
	rasterizerStateInfo.pNext = nullptr;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	{
		VkDescriptorSetLayoutBinding uboBinding;
		uboBinding.binding = 0;
		uboBinding.descriptorCount = 1;
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboBinding.pImmutableSamplers = nullptr;

		const std::array<VkDescriptorSetLayoutBinding, 1> bindings{ uboBinding };
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		descriptorSetLayoutCreateInfo.pBindings = bindings.data();
		descriptorSetLayoutCreateInfo.flags = 0;
		descriptorSetLayoutCreateInfo.pNext = nullptr;

		VK_CHECK(vkCreateDescriptorSetLayout(
			m_LogicalDevice,
			&descriptorSetLayoutCreateInfo,
			nullptr,
			&m_GraphicsPipelineUBOBufferDescriptorSetLayout));
	}

	{
		VkDescriptorSetLayoutBinding colorPalleteBinding;
		colorPalleteBinding.binding = 0;
		colorPalleteBinding.descriptorCount = 1;
		colorPalleteBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		colorPalleteBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		colorPalleteBinding.pImmutableSamplers = nullptr;

		const std::array<VkDescriptorSetLayoutBinding, 1> bindings{ colorPalleteBinding };
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		descriptorSetLayoutCreateInfo.pBindings = bindings.data();
		descriptorSetLayoutCreateInfo.flags = 0;
		descriptorSetLayoutCreateInfo.pNext = nullptr;

		VK_CHECK(vkCreateDescriptorSetLayout(
			m_LogicalDevice,
			&descriptorSetLayoutCreateInfo,
			nullptr,
			&m_GraphicsPipelineColorPaletteDescriptorSetLayout));
	}

	const std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts{ m_GraphicsPipelineUBOBufferDescriptorSetLayout, m_GraphicsPipelineColorPaletteDescriptorSetLayout };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data(); 
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.pNext = nullptr;
	
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = m_ColorPaletteImage->GetImageView();
	imageInfo.sampler = m_ColorPaletteImage->GetImageSampler();

	VkDescriptorPoolSize uboBufferdescriptorPoolSize;
	uboBufferdescriptorPoolSize.descriptorCount = 1;
	uboBufferdescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	VkDescriptorPoolSize colorPalleteImagedescriptorPoolSize;
	colorPalleteImagedescriptorPoolSize.descriptorCount = 1;
	colorPalleteImagedescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	const std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes{ uboBufferdescriptorPoolSize, colorPalleteImagedescriptorPoolSize };
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
	descriptorPoolCreateInfo.maxSets = 10;
	descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorPoolCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateDescriptorPool(
		m_LogicalDevice,
		&descriptorPoolCreateInfo,
		nullptr,
		&m_GraphicsPipelineDescriptorPool));

	VkDescriptorSetAllocateInfo uboBufferDescriptorSetAllocateInfo;
	uboBufferDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	uboBufferDescriptorSetAllocateInfo.descriptorPool = m_GraphicsPipelineDescriptorPool;
	uboBufferDescriptorSetAllocateInfo.descriptorSetCount = 1;
	uboBufferDescriptorSetAllocateInfo.pSetLayouts = &m_GraphicsPipelineUBOBufferDescriptorSetLayout;
	uboBufferDescriptorSetAllocateInfo.pNext = nullptr;

	VK_CHECK(vkAllocateDescriptorSets(
		m_LogicalDevice,
		&uboBufferDescriptorSetAllocateInfo,
		&m_GraphicsPipelineUBOBufferDescriptorSet));

	VkDescriptorSetAllocateInfo colorPalleteImageDescriptorSetAllocateInfo;
	colorPalleteImageDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	colorPalleteImageDescriptorSetAllocateInfo.descriptorPool = m_GraphicsPipelineDescriptorPool;
	colorPalleteImageDescriptorSetAllocateInfo.descriptorSetCount = 1;
	colorPalleteImageDescriptorSetAllocateInfo.pSetLayouts = &m_GraphicsPipelineColorPaletteDescriptorSetLayout;
	colorPalleteImageDescriptorSetAllocateInfo.pNext = nullptr;

	VK_CHECK(vkAllocateDescriptorSets(
		m_LogicalDevice,
		&colorPalleteImageDescriptorSetAllocateInfo,
		&m_GraphicsPipelineColorPaletteDescriptorSet));

	VkBufferCreateInfo uboBufferCreateInfo;
	uboBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	uboBufferCreateInfo.size = sizeof(UBO);
	uboBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	uboBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	uboBufferCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
	uboBufferCreateInfo.pQueueFamilyIndices = nullptr;
	uboBufferCreateInfo.flags = 0;
	uboBufferCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateBuffer(
		m_LogicalDevice,
		&uboBufferCreateInfo,
		nullptr,
		&m_UBOBuffer.Handle));

	VkMemoryRequirements uboBufferMemoryRequirements;
	vkGetBufferMemoryRequirements(
		m_LogicalDevice,
		m_UBOBuffer.Handle,
		&uboBufferMemoryRequirements);

	VkMemoryAllocateInfo uboBufferMemoryAllocationInfo;
	uboBufferMemoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	uboBufferMemoryAllocationInfo.allocationSize = uboBufferMemoryRequirements.size;
	uboBufferMemoryAllocationInfo.memoryTypeIndex = RetrieveMemoryTypeIndex(uboBufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	uboBufferMemoryAllocationInfo.pNext = nullptr;

	VK_CHECK(vkAllocateMemory(
		m_LogicalDevice,
		&uboBufferMemoryAllocationInfo,
		nullptr,
		&m_UBOBuffer.DeviceMemory));

	vkBindBufferMemory(
		m_LogicalDevice,
		m_UBOBuffer.Handle,
		m_UBOBuffer.DeviceMemory,
		0);

	VkWriteDescriptorSet colorPalleteDescriptorSetWrite{};
	colorPalleteDescriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	colorPalleteDescriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	colorPalleteDescriptorSetWrite.dstBinding = 0;
	colorPalleteDescriptorSetWrite.dstArrayElement = 0;
	colorPalleteDescriptorSetWrite.descriptorCount = 1;
	colorPalleteDescriptorSetWrite.dstSet = m_GraphicsPipelineColorPaletteDescriptorSet;
	colorPalleteDescriptorSetWrite.pBufferInfo = nullptr;
	colorPalleteDescriptorSetWrite.pImageInfo = &imageInfo;
	colorPalleteDescriptorSetWrite.pTexelBufferView = nullptr;
	colorPalleteDescriptorSetWrite.pNext = nullptr;

	vkUpdateDescriptorSets(
		m_LogicalDevice,
		1,
		&colorPalleteDescriptorSetWrite,
		0,
		nullptr);

	glm::mat4 __temp = glm::mat4(1.0f);

	void* data;
	vkMapMemory(m_LogicalDevice, m_UBOBuffer.DeviceMemory, 0, sizeof(UBO), 0, &data);
	memcpy(data, &__temp, sizeof(UBO));
	vkUnmapMemory(m_LogicalDevice, m_UBOBuffer.DeviceMemory);

	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = m_UBOBuffer.Handle;
	bufferInfo.range = sizeof(UBO);
	bufferInfo.offset = 0;

	VkWriteDescriptorSet uboBufferDescriptorSetWrite{};
	uboBufferDescriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uboBufferDescriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBufferDescriptorSetWrite.dstBinding = 0;
	uboBufferDescriptorSetWrite.dstArrayElement = 0;
	uboBufferDescriptorSetWrite.descriptorCount = 1;
	uboBufferDescriptorSetWrite.dstSet = m_GraphicsPipelineUBOBufferDescriptorSet;
	uboBufferDescriptorSetWrite.pBufferInfo = &bufferInfo;
	uboBufferDescriptorSetWrite.pImageInfo = nullptr;
	uboBufferDescriptorSetWrite.pTexelBufferView = nullptr;
	uboBufferDescriptorSetWrite.pNext = nullptr;

	vkUpdateDescriptorSets(
		m_LogicalDevice,
		1,
		&uboBufferDescriptorSetWrite,
		0,
		nullptr);

	if (vkCreatePipelineLayout(
		m_LogicalDevice, 
		&pipelineLayoutInfo, 
		nullptr, 
		&m_GraphicsPipelineLayout) != VK_SUCCESS)
	{
		printf("Failed to create graphics pipeline layout\n");
		return false;
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizerStateInfo;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = m_GraphicsPipelineLayout;
	pipelineInfo.renderPass = m_SwapchainRenderPass;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(
		m_LogicalDevice, VK_NULL_HANDLE, 
		1, 
		&pipelineInfo, 
		nullptr, 
		&m_GraphicsPipeline) != VK_SUCCESS) 
	{
		printf("Failed to create graphics pipeline\n");
		return false;
	}

	vkDestroyShaderModule(
		m_LogicalDevice,
		m_FragmentShaderModule,
		nullptr);

	vkDestroyShaderModule(
		m_LogicalDevice,
		m_VertexShaderModule,
		nullptr);

	return true;
}

bool VulkanApp::CreateComputeBasedPipeline()
{
	constexpr uint32_t WORKGROUP_SIZE = 32U;

	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferCreateInfo.size = Utilities::ComputeBufferSize; 
	bufferCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateBuffer(
		m_LogicalDevice,
		&bufferCreateInfo,
		nullptr,
		&m_ComputePipelineStorageBuffer.Handle));

	VkMemoryRequirements storageBufferMemoryRequirements;
	vkGetBufferMemoryRequirements(
		m_LogicalDevice,
		m_ComputePipelineStorageBuffer.Handle,
		&storageBufferMemoryRequirements);

	VkMemoryAllocateInfo storageBufferMemoryAllocateInfo;
	storageBufferMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	storageBufferMemoryAllocateInfo.allocationSize = storageBufferMemoryRequirements.size;
	storageBufferMemoryAllocateInfo.memoryTypeIndex = RetrieveMemoryTypeIndex(storageBufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	storageBufferMemoryAllocateInfo.pNext = nullptr;

	VK_CHECK(vkAllocateMemory(
		m_LogicalDevice,
		&storageBufferMemoryAllocateInfo,
		nullptr,
		&m_ComputePipelineStorageBuffer.DeviceMemory));

	VK_CHECK(vkBindBufferMemory(
		m_LogicalDevice,
		m_ComputePipelineStorageBuffer.Handle,
		m_ComputePipelineStorageBuffer.DeviceMemory,
		0));

	m_ComputeShaderModule = CreateShaderModule("assets/shaders/computeShader.spv");
	if (!m_ComputeShaderModule)
	{
		printf("Failed to create compute shader\n");
		return false;
	}

	VkDescriptorSetLayoutBinding outImageBufferBinding;
	outImageBufferBinding.binding = 0;
	outImageBufferBinding.descriptorCount = 1;
	outImageBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	outImageBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	outImageBufferBinding.pImmutableSamplers = nullptr;

	const std::array<VkDescriptorSetLayoutBinding, 1> bindings{ outImageBufferBinding };
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetLayoutCreateInfo.pBindings = bindings.data();
	descriptorSetLayoutCreateInfo.flags = 0;
	descriptorSetLayoutCreateInfo.pNext = nullptr;

	if (vkCreateDescriptorSetLayout(
		m_LogicalDevice,
		&descriptorSetLayoutCreateInfo,
		nullptr,
		&m_ComputePipelineDescriptorSetLayout) != VK_SUCCESS)
	{
		printf("Failed to create compute pipeline descriptor set layout\n");
		return false;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_ComputePipelineDescriptorSetLayout;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.pNext = nullptr;
		
	if (vkCreatePipelineLayout(	
		m_LogicalDevice,
		&pipelineLayoutCreateInfo,
		nullptr,
		&m_ComputePipelineLayout) != VK_SUCCESS)
	{
		printf("Failed to create compute pipeline layout\n");
		return false;
	}

	VkDescriptorPoolSize storageBufferPoolSize;
	storageBufferPoolSize.descriptorCount = 1;
	storageBufferPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	const std::array<VkDescriptorPoolSize, 1> poolSizes{ storageBufferPoolSize };
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateDescriptorPool(
		m_LogicalDevice,
		&descriptorPoolCreateInfo,
		nullptr,
		&m_ComputePipelineDescriptorPool));

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &m_ComputePipelineDescriptorSetLayout;
	descriptorSetAllocateInfo.descriptorPool = m_ComputePipelineDescriptorPool;
	descriptorSetAllocateInfo.pNext = nullptr;

	VK_CHECK(vkAllocateDescriptorSets(
		m_LogicalDevice,
		&descriptorSetAllocateInfo,
		&m_ComputePipelineStorageBufferDescriptorSet));

	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = m_ComputePipelineStorageBuffer.Handle;
	bufferInfo.range = Utilities::ComputeBufferSize;
	bufferInfo.offset = 0;

	VkWriteDescriptorSet descriptorSetWrite;
	descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetWrite.dstBinding = 0;
	descriptorSetWrite.dstArrayElement = 0;
	descriptorSetWrite.descriptorCount = 1;
	descriptorSetWrite.dstSet = m_ComputePipelineStorageBufferDescriptorSet;
	descriptorSetWrite.pBufferInfo = &bufferInfo;
	descriptorSetWrite.pImageInfo = nullptr;
	descriptorSetWrite.pTexelBufferView = nullptr;
	descriptorSetWrite.pNext = nullptr;

	const std::array<VkWriteDescriptorSet, 1> descriptorSetWrites{ descriptorSetWrite };
	vkUpdateDescriptorSets(
		m_LogicalDevice,
		static_cast<uint32_t>(descriptorSetWrites.size()),
		descriptorSetWrites.data(),
		0,
		nullptr);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = m_ComputeShaderModule;
	computeShaderStageInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stage = computeShaderStageInfo;
	pipelineCreateInfo.layout = m_ComputePipelineLayout;
	pipelineCreateInfo.basePipelineIndex = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.pNext = nullptr;

	if (vkCreateComputePipelines(
		m_LogicalDevice, 
		VK_NULL_HANDLE, 
		1, 
		&pipelineCreateInfo,
		nullptr, 
		&m_ComputePipeline) != VK_SUCCESS) 
	{
		printf("Failed to create compute pipeline");
		return false;
	}

	vkDestroyShaderModule(
		m_LogicalDevice,
		m_ComputeShaderModule,
		nullptr);

	return true;
}

bool VulkanApp::AllocateGraphicsCommandBuffers()
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_GraphicsCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = m_ImageCount;
	commandBufferAllocateInfo.pNext = nullptr;

	m_GraphicsPipelineCommandBuffers.resize(m_ImageCount);
	VK_CHECK(vkAllocateCommandBuffers(
		m_LogicalDevice,
		&commandBufferAllocateInfo,
		m_GraphicsPipelineCommandBuffers.data()));

	return true;
}

bool VulkanApp::AllocateComputeCommandBuffers()
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_ComputeCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.pNext = nullptr;

	VK_CHECK(vkAllocateCommandBuffers(
		m_LogicalDevice,
		&commandBufferAllocateInfo,
		&m_ComputePipelineCommandBuffer));
	
	return true;
}

bool VulkanApp::RecordGraphicsCommandBuffers()
{
	for (uint32_t i = 0; i < m_ImageCount; ++i)
	{ 
		VkCommandBuffer& commandBuffer = m_GraphicsPipelineCommandBuffers[i];
		VkCommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;
		commandBufferBeginInfo.flags = 0;
		commandBufferBeginInfo.pNext = nullptr;

		VkClearValue colorClearValue = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		VkClearValue clearValues[1]{ colorClearValue };

		VkRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.framebuffer = m_SwapchainFramebuffers[i];
		renderPassBeginInfo.renderPass = m_SwapchainRenderPass;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.renderArea.extent = m_SwapchainExtent;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.pNext = nullptr;

		VkViewport viewport;
		viewport.width = static_cast<float>(m_SwapchainExtent.width);
		viewport.height = static_cast<float>(m_SwapchainExtent.height);
		viewport.x = 0;
		viewport.y = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.extent = m_SwapchainExtent;
		scissor.offset = { 0, 0 };

		VK_CHECK(vkBeginCommandBuffer(
			commandBuffer,
			&commandBufferBeginInfo));

		vkCmdSetViewport(
			commandBuffer,
			0,
			1,
			&viewport);

		vkCmdSetScissor(
			commandBuffer,
			0,
			1,
			&scissor);

		vkCmdBeginRenderPass(
			commandBuffer,
			&renderPassBeginInfo,
			VK_SUBPASS_CONTENTS_INLINE);
		
		constexpr VkDeviceSize offsets[1]{ 0 };
		vkCmdBindVertexBuffers(
			commandBuffer,
			0,
			1,
			&m_VertexBuffer.Handle,
			offsets);

		vkCmdBindIndexBuffer(
			commandBuffer,
			m_IndexBuffer.Handle,
			0,
			VK_INDEX_TYPE_UINT32);

		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_GraphicsPipeline);

		const std::array<VkDescriptorSet, 2> descriptorSets{ m_GraphicsPipelineUBOBufferDescriptorSet, m_GraphicsPipelineColorPaletteDescriptorSet };
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_GraphicsPipelineLayout,
			0,
			static_cast<uint32_t>(descriptorSets.size()),
			descriptorSets.data(),
			0,
			nullptr);

		vkCmdDrawIndexed(
			commandBuffer,
			6,
			1,
			0,
			0,
			0);

		vkCmdEndRenderPass(commandBuffer);

		VK_CHECK(vkEndCommandBuffer(commandBuffer));
	}

	return true;
}

bool VulkanApp::RecordComputeCommandBuffers()
{
	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	commandBufferBeginInfo.flags = 0;
	commandBufferBeginInfo.pNext = nullptr;

	VkCommandBuffer& commandBuffer = m_ComputePipelineCommandBuffer;

	VK_CHECK(vkBeginCommandBuffer(
		commandBuffer,
		&commandBufferBeginInfo));

	vkCmdBindPipeline(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		m_ComputePipeline);

	const std::array<VkDescriptorSet, 1> descriptorSets{ m_ComputePipelineStorageBufferDescriptorSet };
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		m_ComputePipelineLayout,
		0,
		static_cast<uint32_t>(descriptorSets.size()),
		descriptorSets.data(),
		0,
		nullptr);

	const int WORKGROUP_SIZE = 32; // Workgroup size in compute shader.

	vkCmdDispatch(
		commandBuffer,
		(uint32_t)ceil(Utilities::ComputeRenderWidth / float(WORKGROUP_SIZE)), (uint32_t)ceil(Utilities::ComputeRenderHeight / float(WORKGROUP_SIZE)), 1);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));
	
	return true;
}

void VulkanApp::UpdateFrameData(const double deltaTime)
{
	INTERNALSCOPE float zoomScale = 1.0f; 
	const auto [windowWidth, windowHeight] = m_Window->GetSize();

	if (windowWidth <= 0 || windowHeight <= 0)
		return;
	
	const float aspectRatio = (float)windowWidth / (float)windowHeight;
	INTERNALSCOPE UBO ubo = {
		aspectRatio,
		0.0f,
		-0.5f,
		zoomScale,
		800
	};
	
	constexpr float moveSpeedFactor = 0.25f;
	constexpr float zoomSpeedFactor = 1.0f;

	const float zoomSpeed = zoomSpeedFactor;
	const float moveSpeed = Input::IsKeyPressed(Key::KEY_SHIFT) ? moveSpeedFactor * 2.0f : moveSpeedFactor;
	/* Move */
	if (Input::IsKeyPressed(Key::KEY_Z))
		zoomScale += zoomScale * zoomSpeed * deltaTime;

	if (Input::IsKeyPressed(Key::KEY_X))
		zoomScale -= zoomScale * zoomSpeed * deltaTime;

	if (Input::IsKeyPressed(Key::KEY_W))
		ubo.CenterX -= moveSpeed * deltaTime * zoomScale;

	if (Input::IsKeyPressed(Key::KEY_S))
		ubo.CenterX += moveSpeed * deltaTime * zoomScale;

	if (Input::IsKeyPressed(Key::KEY_A))
		ubo.CenterY -= moveSpeed * deltaTime * zoomScale;

	if (Input::IsKeyPressed(Key::KEY_D))
		ubo.CenterY += moveSpeed * deltaTime * zoomScale;
	
	if (Input::IsKeyPressed(Key::KEY_UP))
		ubo.IterationCount += 1;

	if (Input::IsKeyPressed(Key::KEY_DOWN))
		ubo.IterationCount -= 1;

	/* Cap the zoom scale to avoid black border as we are rendering a quad */
	zoomScale = zoomScale > 1.0f * aspectRatio ? 1.0f * aspectRatio : fabs(zoomScale);
	/* Update uniform buffer block */
	ubo.ZoomScale = zoomScale;
	ubo.AspectRatio = aspectRatio;

	void* data;
	vkMapMemory(m_LogicalDevice, m_UBOBuffer.DeviceMemory, 0, sizeof(UBO), 0, &data);
	memcpy(data, &ubo, sizeof(UBO));
	vkUnmapMemory(m_LogicalDevice, m_UBOBuffer.DeviceMemory);

	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = m_UBOBuffer.Handle;
	bufferInfo.range = sizeof(UBO);
	bufferInfo.offset = 0;

	VkWriteDescriptorSet uboBufferDescriptorSetWrite{};
	uboBufferDescriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uboBufferDescriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBufferDescriptorSetWrite.dstBinding = 0;
	uboBufferDescriptorSetWrite.dstArrayElement = 0;
	uboBufferDescriptorSetWrite.descriptorCount = 1;
	uboBufferDescriptorSetWrite.dstSet = m_GraphicsPipelineUBOBufferDescriptorSet;
	uboBufferDescriptorSetWrite.pBufferInfo = &bufferInfo;
	uboBufferDescriptorSetWrite.pImageInfo = nullptr;
	uboBufferDescriptorSetWrite.pTexelBufferView = nullptr;
	uboBufferDescriptorSetWrite.pNext = nullptr;
}

#include "vendor/lodepng/lodepng.h"
#include <iostream>

void VulkanApp::DrawFrame()
{
	struct Pixel
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};

	if (m_RenderMethod == ERenderMethod::Compute)
	{
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_ComputePipelineCommandBuffer;

		VK_CHECK(vkQueueSubmit(m_ComputeQueue, 1, &submitInfo, VK_NULL_HANDLE));
		VK_CHECK(vkQueueWaitIdle(m_ComputeQueue));

		void* mappedMemory = nullptr;
		vkMapMemory(m_LogicalDevice, m_ComputePipelineStorageBuffer.DeviceMemory, 0, Utilities::ComputeBufferSize, 0, &mappedMemory);
		Pixel* pmappedMemory = reinterpret_cast<Pixel*>(mappedMemory);

		std::vector<uint8_t> image;
		/* To prevent unnecessary vector buffer reallocations */
		image.reserve(Utilities::RenderedImageSize);
		for (uint32_t i = 0; i < Utilities::RenderedImageSize; i += 4)
		{
			float pixelR = *(float*)&pmappedMemory[i + 0];
			float pixelG = *(float*)&pmappedMemory[i + 1];
			float pixelB = *(float*)&pmappedMemory[i + 2];
			float pixelA = *(float*)&pmappedMemory[i + 3];

			image.push_back(static_cast<uint8_t>(pixelR * 255.0f));
			image.push_back(static_cast<uint8_t>(pixelG * 255.0f));
			image.push_back(static_cast<uint8_t>(pixelB * 255.0f));
			image.push_back(static_cast<uint8_t>(pixelA * 255.0f));
		}

		vkUnmapMemory(m_LogicalDevice, m_ComputePipelineStorageBuffer.DeviceMemory);
		const auto error = lodepng::encode("mandelbrot.png", image, Utilities::ComputeRenderWidth, Utilities::ComputeRenderHeight, LodePNGColorType::LCT_RGBA, 8U);
		if (error)
			printf("encoder error %d: %s", error, lodepng_error_text(error));
		else
			printf("Sucessfully rendered image\n");

		return;
	}

	VkResult result = vkAcquireNextImageKHR(
		m_LogicalDevice,
		m_Swapchain,
		Utilities::MaxSwapchainTimeout,
		m_Semaphores.PresentComplete[m_FrameIndex],
		VK_NULL_HANDLE,
		&m_ImageIndex);

	if (result != VK_SUCCESS)
	{

		const auto [windowWidth, windowHeight] = m_Window->GetSize();
		RecreateSwapchain(windowWidth, windowHeight);
		return;
	}

	if (m_ImagesInFlight[m_ImageIndex] != VK_NULL_HANDLE) 
		vkWaitForFences(m_LogicalDevice, 1, &m_ImagesInFlight[m_ImageIndex], VK_TRUE, UINT64_MAX);
		
	m_ImagesInFlight[m_ImageIndex] = m_InFlightFences[m_FrameIndex];
	const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_GraphicsPipelineCommandBuffers[m_ImageIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_Semaphores.PresentComplete[m_FrameIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_Semaphores.RenderComplete[m_FrameIndex];
	submitInfo.pWaitDstStageMask = waitStages;

	VK_CHECK(vkResetFences(
		m_LogicalDevice, 
		1, 
		&m_InFlightFences[m_FrameIndex]));

	VK_CHECK(vkQueueSubmit(
		m_GraphicsQueue,
		1,
		&submitInfo,
		m_InFlightFences[m_FrameIndex]));

	VkPresentInfoKHR presentInfo;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.pImageIndices = &m_ImageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_Semaphores.RenderComplete[m_FrameIndex];
	presentInfo.pResults = nullptr;
	presentInfo.pNext = nullptr;

	result = vkQueuePresentKHR(
		m_GraphicsQueue,
		&presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		const auto [windowWidth, windowHeight] = m_Window->GetSize();
		RecreateSwapchain(windowWidth, windowHeight);
	}
	
	m_FrameIndex = (m_FrameIndex + 1) % m_MaxFramesInFlight;
}

void VulkanApp::RecreateSwapchain(const uint32_t width, const uint32_t height)
{
	m_SwapchainExtent.width = width;
	m_SwapchainExtent.height = height;

	while (m_SwapchainExtent.width == 0 || m_SwapchainExtent.height == 0)
	{
		m_Window->PollEvents();
		const auto [windowWidth, windowHeight] = m_Window->GetSize();

		m_SwapchainExtent.width = windowWidth;
		m_SwapchainExtent.height = windowHeight;
	}

	VK_CHECK(vkDeviceWaitIdle(m_LogicalDevice));
	CleanupSwapchain();
	CreateSwapchain();	
	RecordGraphicsCommandBuffers();
}

void VulkanApp::CleanupSwapchain()
{
	vkDestroyRenderPass(
		m_LogicalDevice,
		m_SwapchainRenderPass,
		nullptr);

	for (uint32_t i = 0; i < m_ImageCount; ++i)
	{
		vkDestroyFramebuffer(
			m_LogicalDevice,
			m_SwapchainFramebuffers[i],
			nullptr);

		vkDestroyImageView(
			m_LogicalDevice,
			m_SwapchainImageViews[i],
			nullptr);

		vkDestroyFence(
			m_LogicalDevice,
			m_InFlightFences[i],
			nullptr);
	}
	
	m_ImagesInFlight.clear();
	for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i)
	{
		if(m_Semaphores.PresentComplete.empty())
			return;

		if(m_Semaphores.PresentComplete[i])
			vkDestroySemaphore(
				m_LogicalDevice,
				m_Semaphores.PresentComplete[i],
				nullptr);

		if(m_Semaphores.RenderComplete[i])
			vkDestroySemaphore(
				m_LogicalDevice,
				m_Semaphores.RenderComplete[i],
				nullptr);
	}

	vkDestroySwapchainKHR(
		m_LogicalDevice,
		m_Swapchain,
		nullptr);
}

VkCommandBuffer VulkanApp::BeginRecordingSingleTimeUseCommands(const bool compute)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = compute ? m_ComputeCommandPool : m_GraphicsCommandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.pNext = nullptr;

	VkCommandBuffer commandBuffer;
	VK_CHECK(vkAllocateCommandBuffers(
		m_LogicalDevice,
		&commandBufferAllocateInfo,
		&commandBuffer));

	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	commandBufferBeginInfo.pNext = nullptr;

	VK_CHECK(vkBeginCommandBuffer(
		commandBuffer,
		&commandBufferBeginInfo));

	return commandBuffer;
}

void VulkanApp::EndRecordingSingleTimeUseCommands(VkCommandBuffer commandBuffer, const bool compute)
{
	VK_CHECK(vkEndCommandBuffer(commandBuffer));
	
	VkFence commandBufferFinishedExecutionFence;
	VkFenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;
	fenceCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateFence(
		m_LogicalDevice,
		&fenceCreateInfo,
		nullptr,
		&commandBufferFinishedExecutionFence));

	const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.pNext = nullptr;

	VK_CHECK(vkQueueSubmit(
		compute ? m_ComputeQueue : m_GraphicsQueue, 
		1, 
		&submitInfo, 
		commandBufferFinishedExecutionFence));

	VK_CHECK(vkWaitForFences(
		m_LogicalDevice,
		1,
		&commandBufferFinishedExecutionFence,
		VK_FALSE,
		UINT64_MAX));
	
	vkDestroyFence(
		m_LogicalDevice,
		commandBufferFinishedExecutionFence,
		nullptr);
}

void VulkanApp::InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange)
{
	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
}

void VulkanApp::SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	switch (oldImageLayout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
		{
			imageMemoryBarrier.srcAccessMask = 0;

			break;
		}

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;

			break;
		}

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			break;
		}

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			break;
		}

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			break;
		}

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			break;
		}

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

			break;
		}

		default:
		{
			assert(false);
			break;
		}
	}

	switch (newImageLayout)
	{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		{
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			break;
		}

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		{
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			break;
		}

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		{
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			break;
		}

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		{
			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			break;
		}

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		{
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			if (imageMemoryBarrier.srcAccessMask == 0)
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

			break;
		}
			
		default:
		{
			assert(false);
			break;
		}
	}

	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
}

VulkanApp::QueueFamilyIndices VulkanApp::GetQueueFamilyIndices(int32_t flags)
{
	QueueFamilyIndices indices;

	std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
	m_QueueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.data());

	if (flags & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
		{
			auto& queueFamilyProperties = m_QueueFamilyProperties[i];
			if ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
			{
				indices.Compute = i;
				break;
			}
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if (flags & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
		{
			auto& queueFamilyProperties = m_QueueFamilyProperties[i];
			if ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
			{
				indices.Transfer = i;
				break;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
	for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
	{
		if ((flags & VK_QUEUE_TRANSFER_BIT) && indices.Transfer == -1)
		{
			if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
				indices.Transfer = i;
		}

		if ((flags & VK_QUEUE_COMPUTE_BIT) && indices.Compute == -1)
		{
			if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
				indices.Compute = i;
		}

		if (flags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.Graphics = i;
		}
	}

	return indices;
}

VkShaderModule VulkanApp::CreateShaderModule(const std::string_view filepath) const
{
	std::ifstream file(filepath.data(), std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		printf("Failed to open file with given filepath: %s", filepath.data());
		return nullptr;
	}

	std::vector<char> code;
	const uint64_t fileSize = file.tellg();
	code.resize(fileSize);
	file.seekg(std::ios::beg);
	file.read(code.data(), fileSize);
	file.close();

	VkShaderModuleCreateInfo shaderModuleCreateInfo;
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = static_cast<uint32_t>(code.size());
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.pNext = nullptr;

	VkShaderModule module;
	if (vkCreateShaderModule(
		m_LogicalDevice,
		&shaderModuleCreateInfo,
		nullptr,
		&module) != VK_SUCCESS)
	{
		printf("Failed to create shader module from given filepath: %s\n", filepath.data());
		return nullptr;
	}

	return module;
}

uint32_t VulkanApp::RetrieveMemoryTypeIndex(VkMemoryPropertyFlags memoryPropertyFlags, uint32_t memoryTypeBits) const
{
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_PhysicalDeviceMemoryProperties);

	for (uint32_t i = 0; i < m_PhysicalDeviceMemoryProperties.memoryTypeCount; ++i) 
		if ((memoryPropertyFlags & (1 << i)) && (m_PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryTypeBits) == memoryTypeBits)
			return i;			
	
	assert(false);
	return 0;
}

namespace Utilities {
#ifdef APP_DEBUG
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		uint64_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData)
	{
		if (flags)
			printf("VulkanDebugCallback:\n  Object Type: %d\n  Message: %s", objectType, pMessage);

		return VK_FALSE;
	}
#endif
	const std::string_view RetrieveVendorFromID(const uint32_t vendorID)
	{
		switch (vendorID)
		{
			case 0x10DE: return "NVIDIA";
			case 0x1002: return "AMD";
			case 0x13B5: return "ARM";
			case 0x8086: return "INTEL";

			default:
			{
				printf("Failed to retrieve GPU vendor from ID\n");
				return "Unknown vendor";
			}
		}

		printf("Failed to retrieve GPU vendor from ID\n");
		return "Unknown vendor";
	}
}