#pragma once
#include "include/Window.h"
#include "vendor/vulkan/include/vulkan.h"
#include "vendor/vulkan/include/vulkan_win32.h"
#include "include/Core.h"

#ifdef APP_DEBUG
#define VK_CHECK(x) if(x != VK_SUCCESS) \
assert(false)
#else
#define VK_CHECK(x) x
#endif

class Buffer
{
public:
	using Byte_t = uint8_t;
public:
	Buffer()
		:
		m_Data(nullptr),
		m_Size(0)
	{}

	~Buffer() 
	{
		if (m_Data)
			delete[] m_Data;

		m_Data = nullptr;
		m_Size = 0;
	}

	void Allocate(const std::size_t size)
	{
		if (m_Size && m_Size < size)
			delete[] m_Data;

		m_Size = size;
		m_Data = new Byte_t[size];
	}

	void Write(const std::size_t size, const void* data, const std::size_t offset = 0)
	{
		assert(m_Size >= size);
		memcpy(m_Data, data, size);
	}

	void Clear()
	{
		if (m_Data)
			delete[] m_Data;
		
		m_Size = 0;
	}

	void* Data() const
	{
		return m_Data;
	}

	std::size_t Size() const
	{
		return m_Size;
	}
private:
	void* m_Data;
	std::size_t m_Size;
};

struct VulkanBuffer
{
	VkBuffer Handle = VK_NULL_HANDLE;
	VkDeviceMemory DeviceMemory = VK_NULL_HANDLE;
	Buffer CPUData;
};

class VulkanApp
{
public:
	VulkanApp(HINSTANCE hInstance);
	~VulkanApp();

	bool Initialize();
	bool Run();
	bool Shutdown();	

	void OnEvent(Event& event);
public:
	static bool Close();
private:
	bool CreateInstance();
	bool CreateSurface();
	bool CreateLogicalDevice();
	bool CreateSwapchain();

	bool CreateGraphicsBasedPipeline();
	bool CreateComputeBasedPipeline();
	bool AllocateGraphicsCommandBuffers();
	bool AllocateComputeCommandBuffers();
	bool RecordGraphicsCommandBuffers();
	bool RecordComputeCommandBuffers();

	void DrawFrame();
	/* Swapchain */
	void RecreateSwapchain(const uint32_t width, const uint32_t height);
	void CleanupSwapchain();
	/* Pipeline */
	VkShaderModule CreateShaderModule(const std::string_view filepath) const;
	uint32_t RetrieveMemoryTypeIndex(VkMemoryPropertyFlags memoryPropertyFlags, uint32_t memoryTypeBits) const;
	
	VkCommandBuffer BeginRecordingSingleTimeUseCommands(const bool compute);
	void EndRecordingSingleTimeUseCommands(VkCommandBuffer commandBuffer, const bool compute);
private:
	bool m_Running;
	Window* m_Window;
	/* Vulkan API */
	/* Instance (loads the vulkan dll driver) */
	VkInstance m_Instance;

	/* Surface*/
	VkSurfaceKHR m_Surface;

	/* Physical Device */
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
	VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures;
	mutable VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
	VkPhysicalDevice m_PhysicalDevice;
	
	struct QueueIndices {
		enum EQueueFamily {
			InvalidFamily = -1,
			GraphicsFamily = 0,
			ComputeFamily = 1,
			PresentFamily = 2,

			Begin = GraphicsFamily,
			End = PresentFamily,
		};

		int32_t GraphicsQueueFamily = -1;
		int32_t ComputeQueueFamily = -1;
		int32_t PresentQueueFamily = -1;

		const bool AreQueueFamiliesShared(const EQueueFamily a, const EQueueFamily b)
		{
			return GetQueueFamilyIndex(a) == GetQueueFamilyIndex(b);
		}

		const uint32_t GetQueueFamilyIndex(const EQueueFamily queueFamily)
		{
			switch (queueFamily)
			{
				case EQueueFamily::GraphicsFamily:
				{
					return GraphicsQueueFamily;
				}

				case EQueueFamily::ComputeFamily:
				{
					return ComputeQueueFamily;
				}

				case EQueueFamily::PresentFamily:
				{
					return PresentFamily;
				}

				default:
				{
					printf("Unknown queue family!\n");
					assert(false);
				}
			}
		}
	private:
	} m_QueueIndices;

	/* Logical Device */
	VkDevice m_LogicalDevice;
	VkQueue m_GraphicsQueue;
	VkQueue m_ComputeQueue;
	VkQueue m_PresentQueue;
	VkCommandPool m_GraphicsCommandPool;
	VkCommandPool m_ComputeCommandPool;

	/* Swapchain */
	VkSwapchainKHR m_Swapchain;

	VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
	VkSurfaceFormatKHR m_SurfaceFormat;
	VkPresentModeKHR m_PresentMode;
	VkExtent2D m_SwapchainExtent;

	/* Swapchain sync */
	struct {
		std::vector<VkSemaphore> PresentComplete;
		std::vector<VkSemaphore> RenderComplete;
	} m_Semaphores;

	uint32_t m_MaxFramesInFlight;
	uint32_t m_ImageCount;
	uint32_t m_MinimalImageCount;

	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;
	std::vector<VkFramebuffer> m_SwapchainFramebuffers;

	/* Swapchain Compatible Renderpass */
	VkRenderPass m_SwapchainRenderPass;

	/* Pipelines*/
	/* Graphics Pipeline */
	VulkanBuffer m_VertexBuffer;
	VulkanBuffer m_IndexBuffer;
	VkShaderModule m_VertexShaderModule;
	VkShaderModule m_FragmentShaderModule;

	VkDescriptorSetLayout m_GraphicsPipelineDescriptorSetLayout;
	VkPipeline m_GraphicsPipeline;
	VkPipelineLayout m_GraphicsPipelineLayout;
	std::vector<VkCommandBuffer> m_GraphicsPipelineCommandBuffers;
	
	/* Compute Pipeline */
	VkShaderModule m_ComputeShaderModule;
	VkDescriptorSetLayout m_ComputePipelineDescriptorSetLayout;

	VkPipeline m_ComputePipeline;
	VkPipelineLayout m_ComputePipelineLayout;
	std::vector<VkCommandBuffer> m_ComputePipelineCommandBuffers;

	uint32_t m_ImageIndex;
	uint32_t m_FrameIndex;

	std::vector<VkFence> m_InFlightFences;;
	std::vector<VkFence> m_ImagesInFlight;
	/* Debug */
#ifdef APP_DEBUG 
	VkDebugReportCallbackEXT m_DebugReportCallback;
#endif	
private:
	static VulkanApp* s_ApplicationInstance;
};