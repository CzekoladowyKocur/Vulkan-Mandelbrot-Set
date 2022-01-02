#pragma once
#include "include/Core.h"
#include "include/Window.h"
#include "include/VulkanTypes.h"
#include "include/Image2D.h"

class VulkanApp
{
public:
	VulkanApp(HINSTANCE hInstance, const bool showConsole);
	~VulkanApp();

	bool Initialize();
	bool Run();
	bool Shutdown();	

	void OnEvent(Event& event);
public:
	static bool Close();
	static VulkanApp* GetInstance();
private:
	/* Vulkan Context Initialization */
	bool CreateInstance();
	bool CreateSurface();
	bool CreateLogicalDevice();
	bool CreateSwapchain();
	bool LoadAssets();

	bool CreateGraphicsBasedPipeline();
	bool CreateComputeBasedPipeline();
	bool AllocateGraphicsCommandBuffers();
	bool AllocateComputeCommandBuffers();
	bool RecordGraphicsCommandBuffers();
	bool RecordComputeCommandBuffers();

	void UpdateFrameData(const double deltaTime);
	void DrawFrame();
	/* Swapchain */
	void RecreateSwapchain(const uint32_t width, const uint32_t height);
	void CleanupSwapchain();
	/* Pipeline */
	VkShaderModule CreateShaderModule(const std::string_view filepath) const;
	uint32_t RetrieveMemoryTypeIndex(VkMemoryPropertyFlags memoryPropertyFlags, uint32_t memoryTypeBits) const;
	
	VkCommandBuffer BeginRecordingSingleTimeUseCommands(const bool compute);
	void EndRecordingSingleTimeUseCommands(VkCommandBuffer commandBuffer, const bool compute);
	
	void InsertImageMemoryBarrier(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkImageSubresourceRange subresourceRange);

	void SetImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask);
private:
	struct UBO
	{
		float AspectRatio;
		float CenterX;
		float CenterY;
		float ZoomScale;
		int32_t IterationCount;
		float PADDING[3];
	};
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

			assert(false);
			return false;
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
	VulkanBuffer m_UBOBuffer;

	VkShaderModule m_VertexShaderModule;
	VkShaderModule m_FragmentShaderModule;

	VkDescriptorSetLayout m_GraphicsPipelineUBOBufferDescriptorSetLayout;
	VkDescriptorSetLayout m_GraphicsPipelineColorPaletteDescriptorSetLayout;
	VkPipeline m_GraphicsPipeline;
	VkPipelineLayout m_GraphicsPipelineLayout;
	std::vector<VkCommandBuffer> m_GraphicsPipelineCommandBuffers;
	
	/* Compute Pipeline */
	VkShaderModule m_ComputeShaderModule;
	VkDescriptorSetLayout m_ComputePipelineDescriptorSetLayout;
	VkDescriptorPool m_GraphicsPipelineDescriptorPool;
	VkDescriptorSet m_GraphicsPipelineUBOBufferDescriptorSet;
	VkDescriptorSet m_GraphicsPipelineColorPaletteDescriptorSet;

	VkPipeline m_ComputePipeline;
	VkPipelineLayout m_ComputePipelineLayout;
	std::vector<VkCommandBuffer> m_ComputePipelineCommandBuffers;

	uint32_t m_ImageIndex;
	uint32_t m_FrameIndex;

	std::vector<VkFence> m_InFlightFences;;
	std::vector<VkFence> m_ImagesInFlight;

	/* Assets */
	Image2D* m_ColorPaletteImage;

	/* Debug */
#ifdef APP_DEBUG 
	VkDebugReportCallbackEXT m_DebugReportCallback;
#endif	
private:
	static VulkanApp* s_ApplicationInstance;
private:
	friend class Image2D;
	friend class Input;
};