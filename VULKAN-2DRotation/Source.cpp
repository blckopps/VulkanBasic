#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdio.h>

//C++
#include<string>
#include<vector>
#include <fstream>      // std::ifstream

//vulkan header
#define VK_USE_PLATFORM_WIN32_KHR //It states what to include, changes as per platform.
#include<vulkan/vulkan.h>
#include <cassert>

//glm
#define GLM_FORCE_RADIANCE//Force GLM to take angles in radians.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE //Consider Depth as 0 to 1
#include"glm/glm.hpp" //.hpp is specifies that it is for C++ and not for C
#include"glm/gtc/matrix_transform.hpp" //gtc is glm for texture compression.

//vulkan related libraries
#pragma comment(lib,"vulkan-1.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"gdi32.lib")


#define WIN_WIDTH 800
#define WIN_HEIGHT 600

typedef std::vector<std::string> vectorString_t;
typedef std::vector<const char*> vectorConstChar_t;

//Windows callback function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//windows/WIN32 variables
DWORD dwStyle;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };
BOOL gbFullscreen = FALSE;
HWND gHwnd = NULL;
FILE* gpFile = NULL;
BOOL gbActiveWindow = FALSE;
BOOL bIsInitialized = FALSE;
BOOL bWindowMinimized = FALSE;

//vulkan global variables
const char* gpSzAppName = "VK-ROTATION";
const wchar_t* windowName = L"ROTATION";

struct Vertex
{
	float position[3];
	//float color[3];
};

//*************Shader attributes***************////////////////////////
typedef struct VertexData
{
	VkBuffer vkBuffer;
	VkDeviceMemory vkDeviceMemory;
};

VertexData vertexDataPosition{};	//Store position

//*******************Shader uniforms**********************///////////////
//This will be our struct data to store matrix
struct MyUniformData
{
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};

struct UniformData
{
	VkBuffer vkBuffer;
	VkDeviceMemory vkDeviceMemory;
};

UniformData uniformData{};


//store current index for swapchain image
uint32_t currentImageIndex = UINT32_MAX;

//vulkan instance
VkInstance vkInstance = VK_NULL_HANDLE;
int iEnabledInstanceCount = 2;		//We need two instances
vectorConstChar_t enabledInstanceExtNames =		//Add required instance names
{
	VK_KHR_SURFACE_EXTENSION_NAME,
#if defined (_WIN32)
		"VK_KHR_win32_surface",
#endif
};

//Win32 surface
VkSurfaceKHR vkSurfaceKHR = VK_NULL_HANDLE;

//Physical device data
//Device color space, format, presentation mode data.
typedef struct DeviceColorSpaceFormatPresentModeData
{
	VkFormat vkFormat_color = VK_FORMAT_UNDEFINED;
	VkColorSpaceKHR vkColorSpaceKHR = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	VkPresentModeKHR vkPresentModeKHR = VK_PRESENT_MODE_FIFO_KHR;
}SwapChainCreateData;


uint32_t physicalDeviceCount = 0;
std::vector<VkPhysicalDevice> physicalDeviceVector;
VkPhysicalDevice vkPhysicalDeviceSelected = VK_NULL_HANDLE;
VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProp;
VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;



//Queue info
uint32_t graphicsQueueFamilyIndex;
VkQueue vkQueue = VK_NULL_HANDLE;

//Vulkan device
VkDevice vkDevice = VK_NULL_HANDLE;
std::vector<const char*> vkEnabledDeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//swapchain
int winWidth = WIN_WIDTH;
int winHeight = WIN_HEIGHT;
VkExtent2D vkExtend2D_SwapChain;
VkSwapchainKHR vkSwapchainKHR;
uint32_t swapchainImageCount = 0;
std::vector<VkImage> swapchainImageVector;
std::vector<VkImageView> swapchainImageViewVector;
SwapChainCreateData vkColorSpaceFormatAndPresntModeData;

//viewport scissor
VkViewport vkViewPort;
VkRect2D vkRect2DScissor;

//Command pool & command buffer
VkCommandPool vkCommandBufferPool = VK_NULL_HANDLE;
std::vector<VkCommandBuffer> vkCommandBuffersVector;

//Render Pass
VkRenderPass vkRenderPass = VK_NULL_HANDLE;

//Frame buffers
std::vector<VkFramebuffer> vkFrameBuffersVector;

//Semaphores
VkSemaphore vkSemaphore_backbuffer = VK_NULL_HANDLE;
VkSemaphore vkSemaphore_renderComplete = VK_NULL_HANDLE;

//Fences
std::vector<VkFence> vkFencesVector;

//Shader data
VkShaderModule vkShaderModuleVertex = VK_NULL_HANDLE;
VkShaderModule vkShaderModuleFragment = VK_NULL_HANDLE;

//Descriptor set layout
VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;

//Destriptor pool
VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;

//Descriptor set
VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;

//Pipeline layout
VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;

//Graphics pipeline
VkPipeline vkGraphicsPipeline = VK_NULL_HANDLE;

//Rotation
float angle = 0.0f;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{

	WNDCLASSEX wndclassex;
	HWND hwnd;
	MSG msg;
	TCHAR szAppName[255];
	//LPCWSTR lpcw_windowName = windowName;
	int iResult = 0;
	BOOL bDone = FALSE;
	VkResult vkResult = VK_SUCCESS;

	VkResult Initialize(void); // changed return type.
	VkResult Display(void);
	void Update(void);
	void UnInitialize(void);

	fopen_s(&gpFile, "vkLog.txt", "w");

	if (NULL == gpFile)
	{
		MessageBox(NULL, TEXT("Cannot open file..."), TEXT("ERROR!!"), MB_OK);
		exit(EXIT_FAILURE);
	}
	else
	{
		wsprintf(szAppName, TEXT("%s"), gpSzAppName);//manaul change 2
		fprintf(gpFile, "Log file created and opened successfully!!!\n");
	}

	int x = GetSystemMetrics(SM_CXSCREEN);
	int y = GetSystemMetrics(SM_CYSCREEN);

	int xmid = x / 2;
	int ymid = y / 2;

	x = xmid - (800 / 2);
	y = ymid - (600 / 2);

	wndclassex.cbSize = sizeof(WNDCLASSEX);
	wndclassex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclassex.cbClsExtra = 0;
	wndclassex.cbWndExtra = 0;
	wndclassex.lpszClassName = szAppName;
	wndclassex.lpszMenuName = NULL;
	wndclassex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclassex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wndclassex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclassex.hInstance = hInstance;
	wndclassex.lpfnWndProc = WndProc;
	wndclassex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

	RegisterClassEx(&wndclassex);


	hwnd = CreateWindowEx(WS_EX_ACCEPTFILES, szAppName, windowName,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		x, y,
		WIN_WIDTH, WIN_HEIGHT, NULL, NULL, hInstance, NULL);

	gHwnd = hwnd;

	if (!hwnd)
	{
		MessageBox(NULL, TEXT("CreateWindowEx() failed!!!"), TEXT("!!ERROR!!"), MB_OK | MB_ICONSTOP | MB_TOPMOST);
		exit(EXIT_FAILURE);
	}

	vkResult = Initialize();

	if (VK_SUCCESS != vkResult)
	{
		fprintf(gpFile, "Initialize() is failed\n");
		//MessageBox(hwnd, TEXT("Initialize() failed!!!"), TEXT("Error"), MB_OK | MB_ICONERROR);
		DestroyWindow(hwnd);
		hwnd = NULL;
	}
	else
	{
		fprintf(gpFile, "Initialize() succeeded\n");
	}


	ShowWindow(hwnd, iCmdShow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);


	while (FALSE == bDone)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (WM_QUIT == msg.message)
			{
				bDone = TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			if (TRUE == gbActiveWindow)
			{
				if (bWindowMinimized == FALSE)
				{
					VkResult vkResult = Display();
					if (VK_FALSE != vkResult && VK_SUCCESS != vkResult &&
						VK_ERROR_OUT_OF_DATE_KHR == vkResult && VK_SUBOPTIMAL_KHR == vkResult)
					{
						bDone = TRUE;
					}
					Update();
				}
			}
		}

	}

	UnInitialize();

	return ((int)msg.wParam);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	VkResult Resize(int, int);
	void UnInitialize(void);
	void ToggleFullscreen(void);

	TCHAR str[] = TEXT("HELLO WORLD!!");

	switch (iMsg)
	{
	case WM_CREATE:
		fprintf(gpFile, "WndProc():-Program started successfully!\n");
		memset((void*)&wpPrev, 0, sizeof(WINDOWPLACEMENT));
		wpPrev.length = sizeof(WINDOWPLACEMENT);
		//memset karaicha WINDOWPLACEMENT wpprev sathi.
		//wpPrev.length=sizeof(WINDOWPLACEMENT);
		break;
	case WM_SETFOCUS:
		gbActiveWindow = TRUE;
		break;
	case WM_KILLFOCUS:
		gbActiveWindow = FALSE;
		break;
	case WM_SIZE:
		if (SIZE_MINIMIZED == wParam)
			bWindowMinimized = TRUE;
		else
		{
			bWindowMinimized = FALSE;
			Resize(LOWORD(lParam), HIWORD(lParam));
		}

		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(hwnd);
			break;
		case 0x46:
			if (FALSE == gbFullscreen)
			{
				ToggleFullscreen();
				gbFullscreen = TRUE;
			}
			else
			{
				ToggleFullscreen();
				gbFullscreen = FALSE;
			}
			break;
		default:
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	default:
		break;
	}
	return (DefWindowProc(hwnd, iMsg, wParam, lParam));
}

void ToggleFullscreen(void)
{
	MONITORINFO mi = { sizeof(MONITORINFO) };
	BOOL bGetWindowPlacement = FALSE;
	BOOL bGetMonitorInfo = FALSE;
	if (FALSE == gbFullscreen)
	{
		dwStyle = GetWindowLong(gHwnd, GWL_STYLE);
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			bGetWindowPlacement = GetWindowPlacement(gHwnd, &wpPrev);
			bGetMonitorInfo = GetMonitorInfo(MonitorFromWindow(gHwnd, MONITORINFOF_PRIMARY), &mi);
			if (bGetWindowPlacement && bGetMonitorInfo)
			{
				SetWindowLong(gHwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(gHwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
					mi.rcMonitor.right - mi.rcMonitor.left,
					mi.rcMonitor.bottom - mi.rcMonitor.top,
					SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}
		ShowCursor(FALSE);
	}
	else
	{
		SetWindowLong(gHwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(gHwnd, &wpPrev);
		SetWindowPos(gHwnd, HWND_TOP, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}
}

void LogData(const char* logs)
{
	if (gpFile)
	{
		fprintf(gpFile, logs);
		fprintf(gpFile, "\n");
	}
}

VkResult Initialize()
{
	//declare functions
	VkResult createVulkanInstance();
	VkResult getSupportedSurface();
	VkResult getPhysicalDeviceData();
	VkResult createVulkanDevice();
	VkResult createSwapChain();
	VkResult createImageAndImageViews();
	VkResult createCommandBufferPool();
	VkResult createCommandBuffers();
	//
	VkResult createVertexBuffer();
	VkResult createUniformBuffer();
	VkResult createShaders();
	VkResult createPipelineLayout();
	VkResult createDescriptorSetLayout();
	VkResult createDescriptorPool();
	VkResult createDescriptorSet();
	VkResult createRenderPass();
	VkResult createGraphicsPipeline();
	VkResult createFrameBuffers();
	VkResult createSemaphores();
	VkResult createFences();

	VkResult BuildCommandBuffers();

	//fn declaration end

	//code
	VkResult vkResult = VK_SUCCESS;

	//1.Create vulkan instance
	vkResult = createVulkanInstance();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createVulkanInstance failed!!!");
		return vkResult;
	}

	//2.create vulkan surface
	vkResult = getSupportedSurface();
	if (vkResult != VK_SUCCESS)
	{
		LogData("getSupportedSurface failed!!!");
		return vkResult;
	}

	//3.Select physical device and get data
	vkResult = getPhysicalDeviceData();
	if (vkResult != VK_SUCCESS)
	{
		LogData("getPhysicalDeviceData failed!!!");
		return vkResult;
	}

	//4.Create vulkan device
	vkResult = createVulkanDevice();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createVulkanDevice failed!!!");
		return vkResult;
	}

	//5. Get queue, Queue is already created along with vkDevice
	vkGetDeviceQueue(vkDevice, graphicsQueueFamilyIndex, 0, &vkQueue);
	if (vkQueue == VK_NULL_HANDLE)
	{
		LogData("GetDeviceQueue failed!!!");
		return VK_ERROR_UNKNOWN;
	}

	//6.Create swapchain
	vkResult = createSwapChain();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createSwapChain failed!!!");
		return vkResult;
	}

	//7. Vulkan images and views
	vkResult = createImageAndImageViews();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createImageAndImageViews failed!!!");
		return vkResult;
	}

	//8.Create command pool
	vkResult = createCommandBufferPool();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createCommandBufferPool failed!!!");
		return vkResult;
	}

	//9. Create command buffers
	vkResult = createCommandBuffers();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createCommandBuffers failed!!!");
		return vkResult;
	}

	//10.Create vertex buffer
	vkResult = createVertexBuffer();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createVertexBuffer failed!!!");
		return vkResult;
	}

	//10.1 Create uniform buffer
	vkResult = createUniformBuffer();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createUniformBuffer failed!!!");
		return vkResult;
	}

	//11.Create shaders
	vkResult = createShaders();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createShaders failed!!!");
		return vkResult;
	}

	vkResult = createDescriptorSetLayout();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createDescriptorSetLayout failed!!!");
		return vkResult;
	}

	//12.Create pipeline layout
	vkResult = createPipelineLayout();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createPipelineLayout failed!!!");
		return vkResult;
	}



	//create descriptor  pool
	vkResult = createDescriptorPool();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createDescriptorPool failed!!!");
		return vkResult;
	}

	//create descriptor set
	vkResult = createDescriptorSet();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createDescriptorSet failed!!!");
		return vkResult;
	}

	//13. Create render pass
	vkResult = createRenderPass();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createRenderPass failed!!!");
		return vkResult;
	}

	//14.Create pipeline
	vkResult = createGraphicsPipeline();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createGraphicsPipeline failed!!!");
		return vkResult;
	}

	//15. Create frame buffers
	vkResult = createFrameBuffers();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createFrameBuffers failed!!!");
		return vkResult;
	}

	//16.create semaphores
	vkResult = createSemaphores();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createSemaphores failed!!!");
		return vkResult;
	}

	//17.Create fences
	vkResult = createFences();
	if (vkResult != VK_SUCCESS)
	{
		LogData("createFences failed!!!");
		return vkResult;
	}

	//build command buffer
	vkResult = BuildCommandBuffers();
	if (vkResult != VK_SUCCESS)
	{
		LogData("BuildCommandBuffers failed!!!");
		return vkResult;
	}

	bIsInitialized = TRUE;
	return VK_SUCCESS;
}

VkResult Display()
{
	VkResult Resize(int width, int height);
	VkResult UpdateUniformBuffer();

	//Code

	VkResult vkResult = VK_SUCCESS;

	if (bIsInitialized == FALSE)
	{
		LogData("Initialization not done!!!");
		return static_cast<VkResult>(VK_FALSE);
	}

	/*
	* @UINT32_MAX: Timeout to get swapchain to get image or return VK_NOT_READY.
	* @semaphore: Semaphore is such a synchronization which is used for inter queue operation.
	* Semaphore is not waiting for the Swapchain to give image,it is waiting for another queue
	* to release the image held by another queue demanded by swapchain.
	*/
	vkResult = vkAcquireNextImageKHR(vkDevice, vkSwapchainKHR, UINT64_MAX, vkSemaphore_backbuffer, VK_NULL_HANDLE, &currentImageIndex);
	if (vkResult != VK_SUCCESS)
	{
		if (VK_ERROR_OUT_OF_DATE_KHR == vkResult || VK_SUBOPTIMAL_KHR == vkResult)
		{
			Resize(winWidth, winHeight);
		}
		else
		{
			LogData("vkAquireNextImage failed!!!");
			return vkResult;
		}
	}

	/*
	* Use fence to allow host to wait for completion of execution of previous command buffers.
	* Host wait for device- use Fences.
	*/
	vkResult = vkWaitForFences(vkDevice, 1, &vkFencesVector[currentImageIndex], VK_TRUE, UINT64_MAX);
	if (vkResult != VK_SUCCESS)
	{
		LogData("vkWaitForFences failed!!!");
		return vkResult;
	}

	vkResult = vkResetFences(vkDevice, 1, &vkFencesVector[currentImageIndex]);
	if (vkResult != VK_SUCCESS)
	{
		LogData("vkResetFences failed!!!");
		return vkResult;
	}

	//For which state we want to wait for command buffer/s to complete.
	//We have only one which is color attachment, still we need one member array.
	const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;


	//We can create multiple submit infoes and submit them.
	VkSubmitInfo vkSubmitInfo{};
	vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vkSubmitInfo.pNext = nullptr;
	vkSubmitInfo.pWaitDstStageMask = &waitDstStageMask;

	//For whoom command buffers should wait, There num and semaphores
	vkSubmitInfo.waitSemaphoreCount = 1;
	vkSubmitInfo.pWaitSemaphores = &vkSemaphore_backbuffer;

	vkSubmitInfo.commandBufferCount = 1;
	vkSubmitInfo.pCommandBuffers = &vkCommandBuffersVector[currentImageIndex];

	//After work complete, Whoom should we singaled, There num and semaphores
	vkSubmitInfo.signalSemaphoreCount = 1;
	vkSubmitInfo.pSignalSemaphores = &vkSemaphore_renderComplete;

	//Submit work to queue
	vkResult = vkQueueSubmit(vkQueue, 1, &vkSubmitInfo, vkFencesVector[currentImageIndex]);
	if (vkResult != VK_SUCCESS)
	{
		LogData("vkQueueSubmit failed!!!");
		return vkResult;
	}

	//Present image
	VkPresentInfoKHR vkPresentInfoKHR{};
	vkPresentInfoKHR.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	vkPresentInfoKHR.pNext = nullptr;
	vkPresentInfoKHR.swapchainCount = 1;
	vkPresentInfoKHR.pSwapchains = &vkSwapchainKHR;
	vkPresentInfoKHR.pImageIndices = &currentImageIndex;
	vkPresentInfoKHR.waitSemaphoreCount = 1;
	vkPresentInfoKHR.pWaitSemaphores = &vkSemaphore_renderComplete;

	vkResult = vkQueuePresentKHR(vkQueue, &vkPresentInfoKHR);
	if (vkResult != VK_SUCCESS)
	{
		if (VK_ERROR_OUT_OF_DATE_KHR == vkResult || VK_SUBOPTIMAL_KHR == vkResult)
		{
			Resize(winWidth, winHeight);
		}
		else
		{
			LogData("vkQueuePresentKHR failed!!!");
			return vkResult;
		}
	}

	vkResult = UpdateUniformBuffer();
	if (vkResult != VK_SUCCESS)
	{
		LogData("UpdateUniformBuffer failed!!!");
	}

	return vkResult;
}

VkResult Resize(int width, int height)
{
	VkResult createSwapChain();
	VkResult createImageAndImageViews();
	VkResult createRenderPass();
	VkResult createPipelineLayout();
	VkResult createGraphicsPipeline();
	VkResult createFrameBuffers();
	VkResult createCommandBuffers();
	VkResult BuildCommandBuffers();

	VkResult vkResult = VK_SUCCESS;

	if (0 >= height)
	{
		height = 1;
	}

	if (FALSE == bIsInitialized)
		return VK_ERROR_INITIALIZATION_FAILED;

	bIsInitialized = FALSE;

	winWidth = width;
	winHeight = height;

	//Wait for device to complete current taks
	if (vkDevice)
		vkDeviceWaitIdle(vkDevice);

	if (vkSwapchainKHR == VK_NULL_HANDLE)
		return VK_ERROR_INITIALIZATION_FAILED;

	//Destory
	{
		//destroy framebuffers
		for (auto& fb : vkFrameBuffersVector)
		{
			vkDestroyFramebuffer(vkDevice, fb, nullptr);
		}

		//destroy command buffers
		for (auto& commandBuffer : vkCommandBuffersVector)
		{
			vkFreeCommandBuffers(vkDevice, vkCommandBufferPool, 1, &commandBuffer);
		}

		//destroy pipeline
		if (vkGraphicsPipeline)
		{
			vkDestroyPipeline(vkDevice, vkGraphicsPipeline, nullptr);
			vkGraphicsPipeline = VK_NULL_HANDLE;
		}

		//destroy pipeline layout
		if (vkPipelineLayout)
		{
			vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
			vkPipelineLayout = VK_NULL_HANDLE;
		}

		//clean renderpass
		if (vkRenderPass)
		{
			vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);
			vkRenderPass = VK_NULL_HANDLE;
		}

		//We don't own swapchain images.....
		//destory swapchain image 
		/*for (auto& image : swapchainImageVector)
		{
			vkDestroyImage(vkDevice, image, nullptr);
		}*/


		//destory swapchain
		if (vkSwapchainKHR)
		{
			vkDestroySwapchainKHR(vkDevice, vkSwapchainKHR, nullptr);
			vkSwapchainKHR = VK_NULL_HANDLE;
		}
	}

	//Recreate
	{
		//swapchain
		vkResult = createSwapChain();
		if (vkResult != VK_SUCCESS)
		{
			LogData("resize createSwapChain failed!!!");
			return vkResult;
		}

		// Vulkan images and views
		vkResult = createImageAndImageViews();
		if (vkResult != VK_SUCCESS)
		{
			LogData("resize createImageAndImageViews failed!!!");
			return vkResult;
		}

		vkResult = createPipelineLayout();
		if (vkResult != VK_SUCCESS)
		{
			LogData("resize createPipelineLayout failed!!!");
			return vkResult;
		}

		// Create render pass
		vkResult = createRenderPass();
		if (vkResult != VK_SUCCESS)
		{
			LogData("resize createRenderPass failed!!!");
			return vkResult;
		}

		vkResult = createGraphicsPipeline();
		if (vkResult != VK_SUCCESS)
		{
			LogData("resize createGraphicsPipeline failed!!!");
			return vkResult;
		}

		vkResult = createFrameBuffers();
		if (vkResult != VK_SUCCESS)
		{
			LogData("resize createFrameBuffers failed!!!");
			return vkResult;
		}

		//Create command buffers
		vkResult = createCommandBuffers();
		if (vkResult != VK_SUCCESS)
		{
			LogData("resize createCommandBuffers failed!!!");
			return vkResult;
		}

		//build command buffers
		vkResult = BuildCommandBuffers();
		if (vkResult != VK_SUCCESS)
		{
			LogData("resize BuildCommandBuffers failed!!!");
			return vkResult;
		}
	}

	bIsInitialized = TRUE;
	return vkResult;
}

void Update()
{
	angle += 0.02f;
	if (angle >= 360.0f)
		angle = angle - 360.0f;
}

void UnInitialize(void)
{
	if (TRUE == gbFullscreen)
	{
		dwStyle = GetWindowLong(gHwnd, GWL_STYLE);
		SetWindowLong(gHwnd, GWL_STYLE, (dwStyle | WS_OVERLAPPEDWINDOW));
		SetWindowPlacement(gHwnd, &wpPrev);
		SetWindowPos(gHwnd, HWND_TOP, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}

	if (gHwnd)
	{
		DestroyWindow(gHwnd);
		gHwnd = NULL;
	}

	//Vulkan resource cleanup
	if (vkSwapchainKHR)
	{
		vkDestroySwapchainKHR(vkDevice, vkSwapchainKHR, nullptr);
		vkSwapchainKHR = VK_NULL_HANDLE;
	}

	//clean command buffers
	for (auto& commandBuffer : vkCommandBuffersVector)
	{
		vkFreeCommandBuffers(vkDevice, vkCommandBufferPool, 1, &commandBuffer);
	}

	//clear command buffer pool
	if (vkCommandBufferPool)
	{
		vkDestroyCommandPool(vkDevice, vkCommandBufferPool, nullptr);
		vkCommandBufferPool = VK_NULL_HANDLE;
	}

	//Clean uniform buffers
	if (uniformData.vkDeviceMemory)
	{
		vkFreeMemory(vkDevice, uniformData.vkDeviceMemory, nullptr);
		uniformData.vkDeviceMemory = VK_NULL_HANDLE;
	}

	if (uniformData.vkBuffer)
	{
		vkDestroyBuffer(vkDevice, uniformData.vkBuffer, nullptr);
		uniformData.vkBuffer = VK_NULL_HANDLE;
	}

	//clean vertex buffer's deivce memory
	if (vertexDataPosition.vkDeviceMemory)
	{
		vkFreeMemory(vkDevice, vertexDataPosition.vkDeviceMemory, nullptr);
		vertexDataPosition.vkDeviceMemory = VK_NULL_HANDLE;
	}

	if (vertexDataPosition.vkBuffer)
	{
		vkDestroyBuffer(vkDevice, vertexDataPosition.vkBuffer, nullptr);
		vertexDataPosition.vkBuffer = VK_NULL_HANDLE;
	}

	//clean shaders
	if (vkShaderModuleVertex)
	{
		vkDestroyShaderModule(vkDevice, vkShaderModuleVertex, nullptr);
		vkShaderModuleVertex = VK_NULL_HANDLE;
	}

	if (vkShaderModuleFragment)
	{
		vkDestroyShaderModule(vkDevice, vkShaderModuleFragment, nullptr);
		vkShaderModuleFragment = VK_NULL_HANDLE;
	}

	//Clean pipelinelayout
	if (vkPipelineLayout)
	{
		vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
		vkPipelineLayout = VK_NULL_HANDLE;
	}

	//Destroy desriptor pool
	//When pool is destroyed, Des set created by this pool will be destroyed
	if (vkDescriptorPool)
	{
		vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
		vkDescriptorPool = VK_NULL_HANDLE;
		vkDescriptorSet = VK_NULL_HANDLE;
	}

	if (vkDescriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);
		vkDescriptorSetLayout = VK_NULL_HANDLE;
	}

	//clean pipeline
	if (vkGraphicsPipeline)
	{
		vkDestroyPipeline(vkDevice, vkGraphicsPipeline, nullptr);
		vkGraphicsPipeline = VK_NULL_HANDLE;
	}

	//clean renderpass
	if (vkRenderPass)
	{
		vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);
		vkRenderPass = VK_NULL_HANDLE;
	}

	//clean fences
	for (auto& fence : vkFencesVector)
	{
		vkDestroyFence(vkDevice, fence, nullptr);
	}

	//semaphore cleanup
	if (vkSemaphore_backbuffer)
		vkDestroySemaphore(vkDevice, vkSemaphore_backbuffer, nullptr);
	if (vkSemaphore_renderComplete)
		vkDestroySemaphore(vkDevice, vkSemaphore_renderComplete, nullptr);

	for (auto& fb : vkFrameBuffersVector)
	{
		vkDestroyFramebuffer(vkDevice, fb, nullptr);
	}

	//device
	if (vkDevice)
	{
		vkDeviceWaitIdle(vkDevice);	//synchronization
		vkDestroyDevice(vkDevice, nullptr);
		vkDevice = VK_NULL_HANDLE;
	}
}

//Create vulkan instance
VkResult createVulkanInstance()
{
	VkResult checkInstanceExtNames();

	VkResult vkResult = VK_SUCCESS;

	//Check supported extensions names
	vkResult = checkInstanceExtNames();

	//Check required instance are filled
	assert(vkResult == VK_SUCCESS, "Required instance extensions not found");

	//Create vulkan instance
	VkApplicationInfo vkAppInfo{};
	vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vkAppInfo.pNext = nullptr;
	vkAppInfo.pEngineName = "SampleApp";
	vkAppInfo.pApplicationName = "SampleApp";
	vkAppInfo.engineVersion = 1;
	vkAppInfo.applicationVersion = 1;
	vkAppInfo.apiVersion = VK_API_VERSION_1_3;			//Instanned vulkan supported version..Important

	VkInstanceCreateInfo	vkCreateInfo{};
	vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkCreateInfo.pNext = nullptr;
	vkCreateInfo.pApplicationInfo = &vkAppInfo;
	vkCreateInfo.enabledExtensionCount = iEnabledInstanceCount;
	vkCreateInfo.ppEnabledExtensionNames = enabledInstanceExtNames.data();

	vkResult = vkCreateInstance(&vkCreateInfo, nullptr, &vkInstance);
	if (vkResult == VK_SUCCESS)
		LogData("vkCreateInstance success!!!");
	else
		LogData("vkCreateInstance failed!!!");

	return vkResult;
}

//Check instance extensions are supported
VkResult checkInstanceExtNames()
{
	VkResult vkResult = VK_SUCCESS;

	uint32_t iInstanceExtCount = 0;
	std::vector<VkExtensionProperties> vkExtensionProperties_vector;


	vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &iInstanceExtCount, nullptr);
	if (vkResult == VK_SUCCESS && iInstanceExtCount > 0)
	{
		vkExtensionProperties_vector.resize(iInstanceExtCount);
		vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &iInstanceExtCount, vkExtensionProperties_vector.data());
		if (vkResult == VK_SUCCESS && iInstanceExtCount > 0)
		{
			LogData("EnumerateInstanceExtensionProperties filled");
		}
	}
	else
		LogData("EnumerateInstanceExtensionProperties succed");

	//Find whether extension names contain our required two extensions
		//			1 ->  VK_KHR_SURFACE_EXTENSION_NAME
		//			2 ->  VK_KHR_WIN32_SURFACE_EXTENSION_NAME

	int allFound = 0;
	for (auto& instanceProp : vkExtensionProperties_vector)
	{
		if (iEnabledInstanceCount == allFound)
			break;
		for (auto& extNames : enabledInstanceExtNames)
		{
			if (strcmp(extNames, instanceProp.extensionName) == 0)
				allFound++;
		}
	}

	if (allFound == 2)
		LogData("VK_KHR_SURFACE_EXTENSION_NAME and VK_KHR_WIN32_SURFACE_EXTENSION_NAME extensions found");
	else
		LogData("One or more required instance extensions not found");

	return vkResult;
}

//create platform based surface object
VkResult getSupportedSurface()
{
	VkResult vkResult = VK_SUCCESS;
	VkWin32SurfaceCreateInfoKHR vkWin32SurfaceInfo{};

	vkWin32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	vkWin32SurfaceInfo.pNext = nullptr;
	vkWin32SurfaceInfo.flags = 0;
	vkWin32SurfaceInfo.hinstance = (HINSTANCE)GetWindowLongPtr(gHwnd, GWLP_HINSTANCE);;
	//or (HINSTANCE)GetModuleHandle(NULL);
	vkWin32SurfaceInfo.hwnd = gHwnd;

	vkResult = vkCreateWin32SurfaceKHR(vkInstance, &vkWin32SurfaceInfo, nullptr, &vkSurfaceKHR);
	if (vkResult != VK_SUCCESS)
		LogData("vkCreateWin32SurfaceKHR failed");

	return vkResult;
}

//Get physical device data
VkResult getPhysicalDeviceData()
{
	VkResult vkResult = VK_SUCCESS;
	bool phyDeviceFound = false;

	vkResult = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, nullptr);
	if (vkResult == VK_SUCCESS && physicalDeviceCount > 0)
	{
		physicalDeviceVector.resize(physicalDeviceCount);
		vkResult = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, physicalDeviceVector.data());
		if (vkResult == VK_SUCCESS)
		{
			//Iterate for device selection and supported queue family index.
			for (auto vkPhyDevice : physicalDeviceVector)
			{
				uint32_t queueCount = UINT32_MAX;
				std::vector<VkQueueFamilyProperties> vkQueueFamilyPropVector;
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhyDevice, &queueCount, NULL);

				vkQueueFamilyPropVector.resize(queueCount);
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhyDevice, &queueCount, vkQueueFamilyPropVector.data());

				std::vector<VkBool32> isQueueSurfaceSupportedVector(queueCount);

				for (int iQIndex = 0; iQIndex < queueCount; iQIndex++)
				{
					vkGetPhysicalDeviceSurfaceSupportKHR(vkPhyDevice, iQIndex, vkSurfaceKHR, &isQueueSurfaceSupportedVector[iQIndex]);
				}

				for (uint32_t j = 0; j < queueCount; j++)
				{
					if (vkQueueFamilyPropVector[j].queueFlags | VK_QUEUE_GRAPHICS_BIT)
					{
						if (VK_TRUE == isQueueSurfaceSupportedVector[j])
						{
							vkPhysicalDeviceSelected = vkPhyDevice;
							graphicsQueueFamilyIndex = j;
							phyDeviceFound = true;
							break;
						}
					}
				}

				if (phyDeviceFound)
					break;
			}

			VkPhysicalDeviceProperties vkPhyDeviceProp;
			vkGetPhysicalDeviceProperties(vkPhysicalDeviceSelected, &vkPhyDeviceProp);
			const char* deviceName = vkPhyDeviceProp.deviceName;

			//Memory Propeties

			vkGetPhysicalDeviceMemoryProperties(vkPhysicalDeviceSelected, &vkPhysicalDeviceMemoryProp);

			vkGetPhysicalDeviceFeatures(vkPhysicalDeviceSelected, &vkPhysicalDeviceFeatures);

			LogData("Selected device name *******");
			LogData(deviceName);
			LogData("Physical device selected......");
		}
	}
	return vkResult;
}

//Create vulkan device
VkResult createVulkanDevice()
{
	VkResult checkDeviceExtensions();

	VkResult vkResult = VK_SUCCESS;

	vkResult = checkDeviceExtensions();
	if (vkResult != VK_SUCCESS)
		LogData("checkDeviceExtensions failed...");


	float queuePriorities[1] = { 1.0f };
	VkDeviceQueueCreateInfo vkQCreateInfo{};
	vkQCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	vkQCreateInfo.pNext = nullptr;
	vkQCreateInfo.flags = 0;
	vkQCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	vkQCreateInfo.queueCount = 1;
	vkQCreateInfo.pQueuePriorities = queuePriorities;

	//device create info
	VkDeviceCreateInfo vkDeviceCreateInfo{};
	vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	vkDeviceCreateInfo.pNext = nullptr;
	vkDeviceCreateInfo.flags = 0;
	vkDeviceCreateInfo.enabledExtensionCount = vkEnabledDeviceExtensions.size();
	vkDeviceCreateInfo.ppEnabledExtensionNames = vkEnabledDeviceExtensions.data();
	vkDeviceCreateInfo.enabledLayerCount = 0;
	vkDeviceCreateInfo.ppEnabledLayerNames = nullptr;
	vkDeviceCreateInfo.pEnabledFeatures = nullptr;
	vkDeviceCreateInfo.queueCreateInfoCount = 1;
	vkDeviceCreateInfo.pQueueCreateInfos = &vkQCreateInfo;

	vkResult = vkCreateDevice(vkPhysicalDeviceSelected, &vkDeviceCreateInfo, nullptr, &vkDevice);
	if (vkResult == VK_SUCCESS)
	{
		LogData("Vulkan device created!!!!");
	}
	return vkResult;
}

//Check required device extensions are present
VkResult checkDeviceExtensions()
{
	VkResult vkResult = VK_SUCCESS;
	uint32_t extensionCount = 0;
	int extFound = 0;
	std::vector<VkExtensionProperties> vkExtensionPropertiesVector;
	vkResult = vkEnumerateDeviceExtensionProperties(vkPhysicalDeviceSelected, nullptr, &extensionCount, nullptr);
	if (vkResult == VK_SUCCESS && extensionCount > 0)
	{
		vkExtensionPropertiesVector.resize(extensionCount);
		vkResult = vkEnumerateDeviceExtensionProperties(vkPhysicalDeviceSelected, nullptr, &extensionCount, vkExtensionPropertiesVector.data());
		if (vkResult == VK_SUCCESS)
		{
			for (auto& extProp : vkExtensionPropertiesVector)
			{
				if (extFound == vkEnabledDeviceExtensions.size())
					break;
				for (auto& myExtProp : vkEnabledDeviceExtensions)
					if (strcmp(myExtProp, extProp.extensionName) == 0)
						extFound++;
			}
		}
	}
	if (extFound == vkEnabledDeviceExtensions.size())
		LogData("Required extensions are found");

	return vkResult;
}

//Create swapchain
VkResult createSwapChain()
{
	VkResult getPhyDeiceDataForSwapChain(SwapChainCreateData & outSwapChainData);

	VkResult vkResult = VK_SUCCESS;

	vkResult = getPhyDeiceDataForSwapChain(vkColorSpaceFormatAndPresntModeData);
	if (vkResult != VK_SUCCESS)
	{
		LogData("getPhyDeiceDataForSwapChain failed");
		return vkResult;
	}

	//Get physical device surface capabilities
	VkSurfaceCapabilitiesKHR vkSurfaceCapabilitiesKhr;
	vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDeviceSelected, vkSurfaceKHR, &vkSurfaceCapabilitiesKhr);
	if (vkResult != VK_SUCCESS)
	{
		LogData("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed!!!");
		return vkResult;
	}

	//Find desired number of swapchain images
	uint32_t testingNumOfSwapChainImages = vkSurfaceCapabilitiesKhr.minImageCount + 1;
	uint32_t desiredNumOfSwapChainImages = 0;

	if (0 < vkSurfaceCapabilitiesKhr.maxImageCount && testingNumOfSwapChainImages > vkSurfaceCapabilitiesKhr.maxImageCount)
	{
		desiredNumOfSwapChainImages = vkSurfaceCapabilitiesKhr.maxImageCount;
	}
	else
	{
		desiredNumOfSwapChainImages = vkSurfaceCapabilitiesKhr.minImageCount;
	}

	uint32_t cuurentExtWidth = vkSurfaceCapabilitiesKhr.currentExtent.width;
	uint32_t cuurentExtHeight = vkSurfaceCapabilitiesKhr.currentExtent.height;
	//Select size of swapchain image
	if (UINT32_MAX != cuurentExtWidth)
	{
		vkExtend2D_SwapChain.width = cuurentExtWidth;
		vkExtend2D_SwapChain.height = cuurentExtHeight;
	}
	else
	{
		VkExtent2D vkExtend2D;
		vkExtend2D.height = winHeight;
		vkExtend2D.width = winWidth;

		vkExtend2D_SwapChain.width = glm::max(cuurentExtWidth, glm::min(cuurentExtWidth, vkExtend2D.width));
		vkExtend2D_SwapChain.height = glm::max(cuurentExtHeight, glm::min(cuurentExtHeight, vkExtend2D.height));
	}

	//swapchain image usage flags
	VkImageUsageFlags vkImageUsageFlag = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	//Pretransform flipping
	VkSurfaceTransformFlagBitsKHR vkSurfaceTransformFlag;
	if (VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR & vkSurfaceCapabilitiesKhr.supportedTransforms)
	{
		vkSurfaceTransformFlag = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
		vkSurfaceTransformFlag = vkSurfaceCapabilitiesKhr.currentTransform;

	//Swapchain createinfo 
	VkSwapchainCreateInfoKHR vkSwapchainCreateInfo{};
	vkSwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	vkSwapchainCreateInfo.pNext = nullptr;
	vkSwapchainCreateInfo.flags = 0;
	vkSwapchainCreateInfo.surface = vkSurfaceKHR;
	vkSwapchainCreateInfo.minImageCount = desiredNumOfSwapChainImages;
	vkSwapchainCreateInfo.imageFormat = vkColorSpaceFormatAndPresntModeData.vkFormat_color;
	vkSwapchainCreateInfo.imageColorSpace = vkColorSpaceFormatAndPresntModeData.vkColorSpaceKHR;
	vkSwapchainCreateInfo.imageExtent.width = vkExtend2D_SwapChain.width;
	vkSwapchainCreateInfo.imageExtent.height = vkExtend2D_SwapChain.height;
	vkSwapchainCreateInfo.imageUsage = vkImageUsageFlag;
	vkSwapchainCreateInfo.preTransform = vkSurfaceTransformFlag;
	vkSwapchainCreateInfo.imageArrayLayers = 1;
	vkSwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkSwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	vkSwapchainCreateInfo.presentMode = vkColorSpaceFormatAndPresntModeData.vkPresentModeKHR;
	vkSwapchainCreateInfo.clipped = VK_TRUE;
	//vkSwapchainCreateInfo.queueFamilyIndexCount = 0;

	//OLD swap chain memebers will be used for resize

	//Create SwapChain
	vkResult = vkCreateSwapchainKHR(vkDevice, &vkSwapchainCreateInfo, NULL, &vkSwapchainKHR);
	if (vkResult == VK_SUCCESS)
		LogData("Swapchain created!!!");

	return vkResult;
}

VkResult getPhyDeiceDataForSwapChain(SwapChainCreateData& outSwapChainData)
{
	VkResult vkResult = VK_SUCCESS;

	uint32_t surfaceFormatCount = 0;
	std::vector<VkSurfaceFormatKHR> vkSurfaceFormatKHRDataVector;

	vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDeviceSelected, vkSurfaceKHR, &surfaceFormatCount, nullptr);
	if (vkResult == VK_SUCCESS && surfaceFormatCount > 0)
	{
		vkSurfaceFormatKHRDataVector.resize(surfaceFormatCount);
		vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDeviceSelected, vkSurfaceKHR, &surfaceFormatCount,
			vkSurfaceFormatKHRDataVector.data());

		if (vkResult == VK_SUCCESS)
		{
			if (surfaceFormatCount == 1 && VK_FORMAT_UNDEFINED == vkSurfaceFormatKHRDataVector[0].format)
			{
				outSwapChainData.vkFormat_color = VK_FORMAT_B8G8R8A8_UNORM;
				LogData("VkFormat selected as- VK_FORMAT_B8G8R8A8_UNORM");
			}
			else
			{
				outSwapChainData.vkFormat_color = vkSurfaceFormatKHRDataVector[0].format;
			}

			//set color space
			outSwapChainData.vkColorSpaceKHR = vkSurfaceFormatKHRDataVector[0].colorSpace;
		}
	}

	//Get presentation mode
	uint32_t surfacePresemtModeCount = 0;
	std::vector<VkPresentModeKHR> vkPresentModesVector;

	vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDeviceSelected,
		vkSurfaceKHR, &surfacePresemtModeCount, nullptr);
	if (vkResult == VK_SUCCESS && surfaceFormatCount > 0)
	{
		vkPresentModesVector.resize(surfacePresemtModeCount);
		vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDeviceSelected,
			vkSurfaceKHR, &surfacePresemtModeCount, vkPresentModesVector.data());
		if (vkResult != VK_SUCCESS)
		{
			LogData("Failed to get surface presenatation modes");
			return vkResult;
		}

		for (auto& presentMode : vkPresentModesVector)
		{
			if (VK_PRESENT_MODE_MAILBOX_KHR == presentMode)
			{
				outSwapChainData.vkPresentModeKHR = VK_PRESENT_MODE_MAILBOX_KHR;
				break;
			}
		}

		if (outSwapChainData.vkPresentModeKHR != VK_PRESENT_MODE_MAILBOX_KHR)
			outSwapChainData.vkPresentModeKHR = VK_PRESENT_MODE_FIFO_KHR;
	}
	else if (vkResult != VK_SUCCESS || surfacePresemtModeCount == 0)
	{
		LogData("failed to get surface present modes");
		vkResult = VK_ERROR_INITIALIZATION_FAILED;
		return vkResult;
	}
	return vkResult;
}

//Create image and image views
VkResult createImageAndImageViews()
{
	VkResult vkResult = VK_SUCCESS;

	vkResult = vkGetSwapchainImagesKHR(vkDevice, vkSwapchainKHR, &swapchainImageCount, nullptr);
	if (vkResult == VK_SUCCESS && swapchainImageCount > 0)
	{
		swapchainImageVector.resize(swapchainImageCount);
		swapchainImageViewVector.resize(swapchainImageCount);
		vkResult = vkGetSwapchainImagesKHR(vkDevice, vkSwapchainKHR, &swapchainImageCount, swapchainImageVector.data());
		if (vkResult == VK_SUCCESS)
		{
			VkImageViewCreateInfo vkImageViewCrateInfo{};
			vkImageViewCrateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			vkImageViewCrateInfo.pNext = nullptr;
			vkImageViewCrateInfo.flags = 0;
			vkImageViewCrateInfo.format = vkColorSpaceFormatAndPresntModeData.vkFormat_color;
			vkImageViewCrateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
			vkImageViewCrateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
			vkImageViewCrateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
			vkImageViewCrateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

			//aspectMask:--whch part of Image or whole of the Image is going to be affected by image barrier.
			//https://registry.khronos.org/vulkan/specs/latest/man/html/VkImageSubresourceRange.html
			vkImageViewCrateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			vkImageViewCrateInfo.subresourceRange.baseMipLevel = 0;
			vkImageViewCrateInfo.subresourceRange.levelCount = 1;
			vkImageViewCrateInfo.subresourceRange.baseArrayLayer = 0;
			vkImageViewCrateInfo.subresourceRange.layerCount = 1;

			vkImageViewCrateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

			//fill image view arrays
			for (uint32_t i = 0; i < swapchainImageCount; i++)
			{
				vkImageViewCrateInfo.image = swapchainImageVector[i];
				vkResult = vkCreateImageView(vkDevice, &vkImageViewCrateInfo, nullptr, &swapchainImageViewVector[i]);
				if (vkResult != VK_SUCCESS)
				{
					LogData("Faild to create image view!!!");
					return vkResult;
				}
			}

		}
	}

	return vkResult;
}

//Create command buffer pool
VkResult createCommandBufferPool()
{
	VkResult vkResult = VK_SUCCESS;

	VkCommandPoolCreateInfo vkCommandPoolCreateInfo{};
	vkCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	vkCommandPoolCreateInfo.pNext = nullptr;
	vkCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCommandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

	vkResult = vkCreateCommandPool(vkDevice, &vkCommandPoolCreateInfo, nullptr, &vkCommandBufferPool);
	if (vkResult == VK_SUCCESS)
	{
		LogData("Command pool created!!!");
	}

	return vkResult;
}

//create command buffers
VkResult createCommandBuffers()
{
	VkResult vkResult = VK_SUCCESS;

	VkCommandBufferAllocateInfo vkCommandBufferAllocInfo{};
	vkCommandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	vkCommandBufferAllocInfo.pNext = nullptr;
	vkCommandBufferAllocInfo.commandPool = vkCommandBufferPool;
	vkCommandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkCommandBufferAllocInfo.commandBufferCount = 1;

	//alloc command buffers same as swapchain image count
	vkCommandBuffersVector.resize(swapchainImageCount);

	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		vkResult = vkAllocateCommandBuffers(vkDevice, &vkCommandBufferAllocInfo, &vkCommandBuffersVector[i]);
		if (vkResult != VK_SUCCESS)
		{
			LogData("vkAllocateCommandBuffers failed!!!");
			return vkResult;
		}
	}

	return vkResult;
}

//Build command buffers
VkResult BuildCommandBuffers()
{
	VkResult vkResult = VK_SUCCESS;

	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		//1.Rest command buffer
		vkResult = vkResetCommandBuffer(vkCommandBuffersVector[i], 0);
		if (vkResult != VK_SUCCESS)
		{
			LogData("Failed to reset command Buffer!!!");
			return vkResult;
		}

		VkCommandBufferBeginInfo vkCommandBufferBeginInfo{};
		vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkCommandBufferBeginInfo.pNext = nullptr;
		vkCommandBufferBeginInfo.flags = 0;		// 0 indiacte 2 meanings , first, we will use only primarycommand buffer. second,
		//we are not going to use this simultaneously between multiple threads.

		//2. Begin command buffer
		vkResult = vkBeginCommandBuffer(vkCommandBuffersVector[i], &vkCommandBufferBeginInfo);
		if (vkResult != VK_SUCCESS)
		{
			LogData("Failed to begin command Buffer!!!");
			return vkResult;
		}


		VkClearColorValue vkClearColorValue{};
		vkClearColorValue.float32[0] = 0.0f;
		vkClearColorValue.float32[1] = 0.0f;
		vkClearColorValue.float32[2] = 1.0f;
		vkClearColorValue.float32[3] = 1.0f;
		//Record here
#pragma region RECORD COMMAND BUFFER
		std::vector<VkClearValue> vkClearValueVector(1);

		vkClearValueVector[0].color = vkClearColorValue;

		//Begin render pass
		VkRenderPassBeginInfo vkRenderPassBeginInfo{};
		vkRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkRenderPassBeginInfo.pNext = nullptr;
		vkRenderPassBeginInfo.renderPass = vkRenderPass;
		vkRenderPassBeginInfo.renderArea.offset.x = 0.0f;
		vkRenderPassBeginInfo.renderArea.offset.y = 0.0f;
		vkRenderPassBeginInfo.renderArea.extent.width = vkExtend2D_SwapChain.width;
		vkRenderPassBeginInfo.renderArea.extent.height = vkExtend2D_SwapChain.height;
		vkRenderPassBeginInfo.clearValueCount = vkClearValueVector.size();
		vkRenderPassBeginInfo.pClearValues = vkClearValueVector.data();
		vkRenderPassBeginInfo.framebuffer = vkFrameBuffersVector[i];

		vkCmdBeginRenderPass(vkCommandBuffersVector[i], &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		//here we should call vulkan drawing functions
		{
			//Bind with pipeline
			vkCmdBindPipeline(vkCommandBuffersVector[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);


			//Bind our descriptor set with pipeline
			vkCmdBindDescriptorSets(vkCommandBuffersVector[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 1, &vkDescriptorSet, 0, nullptr);

			//Bind with vertex buffer
			std::vector<VkDeviceSize> vkDeviceSizeOffsetVector(1);
			vkCmdBindVertexBuffers(vkCommandBuffersVector[i], 0, 1, &vertexDataPosition.vkBuffer, vkDeviceSizeOffsetVector.data());


			vkCmdDraw(vkCommandBuffersVector[i], 3, 1, 0, 0);
		}

		vkCmdEndRenderPass(vkCommandBuffersVector[i]);
#pragma endregion


		//End command buffer
		vkResult = vkEndCommandBuffer(vkCommandBuffersVector[i]);
		if (vkResult != VK_SUCCESS)
		{
			LogData("Failed to end command Buffer!!!");
			return vkResult;
		}
	}

	return vkResult;
}

//Format
/* createVertexBuffer();
createUniformBuffers();
createDescriptorSetLayout();
createDescriptorPool();
createDescriptorSets();
createPipelines();
*/


//Create render pass
VkResult createRenderPass()
{
	VkResult vkResult = VK_SUCCESS;

	//create attachment description
	std::vector<VkAttachmentDescription> vkAttachmentDesriptionVector(1);
	vkAttachmentDesriptionVector[0].flags = 0;
	vkAttachmentDesriptionVector[0].format = vkColorSpaceFormatAndPresntModeData.vkFormat_color;
	vkAttachmentDesriptionVector[0].samples = VK_SAMPLE_COUNT_1_BIT;
	vkAttachmentDesriptionVector[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; ////When we enter , what to do about data
	vkAttachmentDesriptionVector[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE; ////When we leave , what to do about data
	vkAttachmentDesriptionVector[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	vkAttachmentDesriptionVector[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	vkAttachmentDesriptionVector[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	////Image's data(arrangement) when we enter.
	vkAttachmentDesriptionVector[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; ////Image's data(arrangement) when we leave.

	//Attachment reference structure
	VkAttachmentReference vkAttachmentReference{};
	// 0 doesn't mean 0, but 0th attachment in above, we are refering the 0th attachment ,
	// i.e color attachment.
	vkAttachmentReference.attachment = 0;
	vkAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Subpass info
	VkSubpassDescription vkSubpassDescription{};
	vkSubpassDescription.flags = 0;
	vkSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	vkSubpassDescription.inputAttachmentCount = 0;
	vkSubpassDescription.pInputAttachments = nullptr;
	vkSubpassDescription.colorAttachmentCount = vkAttachmentDesriptionVector.size();
	vkSubpassDescription.pColorAttachments = &vkAttachmentReference;
	vkSubpassDescription.pResolveAttachments = nullptr;
	vkSubpassDescription.pDepthStencilAttachment = nullptr;
	vkSubpassDescription.preserveAttachmentCount = 0;
	vkSubpassDescription.pResolveAttachments = nullptr;



	//create iinfo for Render Pass
	VkRenderPassCreateInfo vkRenderPassCreateInfo{};
	vkRenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	vkRenderPassCreateInfo.pNext = nullptr;
	vkRenderPassCreateInfo.flags = 0;
	vkRenderPassCreateInfo.attachmentCount = vkAttachmentDesriptionVector.size();
	vkRenderPassCreateInfo.pAttachments = vkAttachmentDesriptionVector.data();
	vkRenderPassCreateInfo.subpassCount = 1;
	vkRenderPassCreateInfo.pSubpasses = &vkSubpassDescription;
	vkRenderPassCreateInfo.dependencyCount = 0;
	vkRenderPassCreateInfo.pDependencies = nullptr;

	vkResult = vkCreateRenderPass(vkDevice, &vkRenderPassCreateInfo, nullptr, &vkRenderPass);
	if (vkResult == VK_SUCCESS)
	{
		LogData("Render pass created!!!");
	}

	return vkResult;
}

//Create frame buffers
VkResult createFrameBuffers()
{
	VkResult vkResult = VK_SUCCESS;

	std::vector<VkImageView> vkImageViewTempVec(1);

	VkFramebufferCreateInfo vkFrameBufferCreateInfo{};
	vkFrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	vkFrameBufferCreateInfo.pNext = nullptr;
	vkFrameBufferCreateInfo.flags = 0;
	vkFrameBufferCreateInfo.renderPass = vkRenderPass;
	vkFrameBufferCreateInfo.attachmentCount = vkImageViewTempVec.size();
	vkFrameBufferCreateInfo.pAttachments = vkImageViewTempVec.data();
	vkFrameBufferCreateInfo.width = vkExtend2D_SwapChain.width;
	vkFrameBufferCreateInfo.height = vkExtend2D_SwapChain.height;
	vkFrameBufferCreateInfo.layers = 1;

	//resize vk framebuffers vector same as swapchain image count
	vkFrameBuffersVector.resize(swapchainImageCount);

	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		vkImageViewTempVec[0] = swapchainImageViewVector[i];
		vkResult = vkCreateFramebuffer(vkDevice, &vkFrameBufferCreateInfo, nullptr, &vkFrameBuffersVector[i]);
		if (vkResult != VK_SUCCESS)
		{
			LogData("Failed to create framebuffer!!!");
			return vkResult;
		}
	}
	LogData("Frame buffers created!!!");
	return vkResult;
}

//Create semaphores
VkResult createSemaphores()
{
	VkResult vkResult = VK_SUCCESS;

	//By default semaphore is BINARY, if not specified
	VkSemaphoreCreateInfo vkSemaphoreCreateInfo{};
	vkSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkSemaphoreCreateInfo.pNext = nullptr;
	vkSemaphoreCreateInfo.flags = 0;

	//create  semaphore for backbuffer
	vkResult = vkCreateSemaphore(vkDevice, &vkSemaphoreCreateInfo, nullptr, &vkSemaphore_backbuffer);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to create semaphore for back buffer!!!");
		return vkResult;
	}


	//create for render complete
	vkResult = vkCreateSemaphore(vkDevice, &vkSemaphoreCreateInfo, nullptr, &vkSemaphore_renderComplete);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to create semaphore for Render complete!!!");
		return vkResult;
	}

	return vkResult;
}

//Create fences
VkResult createFences()
{
	VkResult vkResult = VK_SUCCESS;

	VkFenceCreateInfo vkFenceCreateInfo{};
	vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkFenceCreateInfo.pNext = nullptr;
	vkFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //0 if signaled, if VK_FENCE_CREATE_SIGNALED_BIT then unsignaled.

	//resize fence vector same as swpachain image count;
	vkFencesVector.resize(swapchainImageCount);

	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		vkResult = vkCreateFence(vkDevice, &vkFenceCreateInfo, nullptr, &vkFencesVector[i]);
		if (vkResult != VK_SUCCESS)
		{
			LogData("Failed to create fence!!!");
			return vkResult;
		}
	}

	return vkResult;
}

//Create shaders
VkResult createShaders()
{
	VkShaderModule loadSPIRVShader(const std::string & shaderPath);

	VkResult vkResult = VK_SUCCESS;

	//Create vertex shader module
	vkShaderModuleVertex = loadSPIRVShader("Shaders\\Shader.vert.spv");
	if (vkShaderModuleVertex == VK_NULL_HANDLE)
	{
		LogData("Failed to create vertex shader module!!!");
		return VK_ERROR_INVALID_SHADER_NV;
	}

	//Create fragment shader module
	vkShaderModuleFragment = loadSPIRVShader("Shaders\\Shader.frag.spv");
	if (vkShaderModuleVertex == VK_NULL_HANDLE)
	{
		LogData("Failed to create fragment shader module!!!");
		return VK_ERROR_INVALID_SHADER_NV;
	}

	return vkResult;
}

//Load shader
VkShaderModule loadSPIRVShader(const std::string& shaderPath)
{
	size_t shaderSize;
	char* shaderCode{ nullptr };

	std::ifstream fileStream(shaderPath, std::ios::binary | std::ios::in | std::ios::ate);

	if (fileStream.is_open())
	{
		shaderSize = fileStream.tellg();
		fileStream.seekg(0, std::ios::beg);
		shaderCode = new char[shaderSize];
		fileStream.read(shaderCode, shaderSize);
		fileStream.close();
		assert(shaderSize > 0);
	}

	if (shaderCode != nullptr)
	{
		//Create shader module
		VkShaderModuleCreateInfo vkShaderModuleCreateInfo{};
		vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vkShaderModuleCreateInfo.pNext = nullptr;
		vkShaderModuleCreateInfo.flags = 0;
		vkShaderModuleCreateInfo.codeSize = shaderSize;
		vkShaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode);

		VkShaderModule vkShaderModule = VK_NULL_HANDLE;
		VkResult vkResult = vkCreateShaderModule(vkDevice, &vkShaderModuleCreateInfo, nullptr, &vkShaderModule);
		if (vkResult != VK_SUCCESS)
		{
			LogData("Failed to create shader module!!!");
			return VK_NULL_HANDLE;
		}
		return vkShaderModule;
	}
	return VK_NULL_HANDLE;
}

//INFO FOR SHADERS
// Shader attributes/Properties: Position, texcoords, colors
// Shader resources: Uniforms, Constant buffers, textures, Samplers
// Vulkan->Storage buffers, Push constant
// ***************************************************
// Resources used by shaders are called as Descriptors.
// set of descriptors called as descriptorset and there ordered arrangement
// accroding to there type is called descriptor set layuout.
// Using Descriptor set layout, Pipeline layout is formed.\
// ******************************************************
// Ordered type wise arrangement of descriptor set is called as descriptor set layuout.
// Arrangement of such descriptor set layout is called pipeline layout.
// ***************************************************
// Not all resouces are descriptors.
// "Textures, Samplers, UBO, Storage buffers, uniforms/UBO" are descriptors
// "Push constants, Storage buffers" are not descriptors
// ***************************************************
// DescriptorSetLayouts are different for different pipelines.
// You need to provide DescriptorSetLayout, desc set even if you don't 
// have any descriptors. Provide empty descriptor set.
// ***************************************************
// 
//Create vertex buffers
VkResult createVertexBuffer()
{
	// A note on memory management in Vulkan in general:
	//	This is a very complex topic and while it's fine for an example application to small individual memory allocations that is not
	//	what should be done a real-world application, where you should allocate large chunks of memory at once instead.


	VkResult vkResult = VK_SUCCESS;

	std::vector<Vertex> vertexBuffer
	{
		{0.0f, 1.0f, 0.0f},
		{-1.0f, -1.0f, 0.0f},
		{1.0f, -1.0f, 0.0f}
	};

	uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

	//1.Create buffer 
	VkBufferCreateInfo vkBufferCreateInfo{};
	vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferCreateInfo.pNext = nullptr;
	vkBufferCreateInfo.flags = 0;		//Valid flags are used in scatter buffer.(used in sparse buffers)
	vkBufferCreateInfo.size = vertexBufferSize;
	vkBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	vkResult = vkCreateBuffer(vkDevice, &vkBufferCreateInfo, nullptr, &vertexDataPosition.vkBuffer);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to create buffer!!!");
		return vkResult;
	}

	VkMemoryRequirements vkMemoryRequirements{};
	vkGetBufferMemoryRequirements(vkDevice, vertexDataPosition.vkBuffer, &vkMemoryRequirements);

	VkMemoryAllocateInfo vkMemoryAllocateInfo{};
	vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkMemoryAllocateInfo.pNext = nullptr;
	vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
	vkMemoryAllocateInfo.memoryTypeIndex = 0;

	for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProp.memoryTypeCount; i++)
	{
		if (1 == (vkMemoryRequirements.memoryTypeBits & 1))
		{
			if (vkPhysicalDeviceMemoryProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			{
				vkMemoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
		vkMemoryRequirements.memoryTypeBits >>= 1;
	}

	//2.Allocate device memory
	vkResult = vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, nullptr, &vertexDataPosition.vkDeviceMemory);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed create device memory!!!");
		return vkResult;
	}

	//3.Bind with device memory handle with vulkan buffer
	vkResult = vkBindBufferMemory(vkDevice, vertexDataPosition.vkBuffer, vertexDataPosition.vkDeviceMemory, 0);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed bind buffer memory!!!");
		return vkResult;
	}

	//4.Memory map
	void* data = nullptr;
	vkResult = vkMapMemory(vkDevice, vertexDataPosition.vkDeviceMemory, 0, vkMemoryAllocateInfo.allocationSize, 0, &data);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to map memory!!!");
		return vkResult;
	}

	//5.Copy vertex data 
	memcpy(data, vertexBuffer.data(), vertexBufferSize);

	//6.Unmap
	vkUnmapMemory(vkDevice, vertexDataPosition.vkDeviceMemory);

	return vkResult;
}

//Create uniform buffer
VkResult createUniformBuffer()
{
	VkResult UpdateUniformBuffer();

	//code
	VkResult vkResult = VK_SUCCESS;

	//1.Create buffer 
	VkBufferCreateInfo vkBufferCreateInfo{};
	vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferCreateInfo.pNext = nullptr;
	vkBufferCreateInfo.flags = 0;		//Valid flags are used in scatter buffer.(used in sparse buffers)
	vkBufferCreateInfo.size = sizeof(MyUniformData);			//Out uniform data structure size
	vkBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	vkResult = vkCreateBuffer(vkDevice, &vkBufferCreateInfo, nullptr, &uniformData.vkBuffer);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to create buffer for uniform data!!!");
		return vkResult;
	}

	VkMemoryRequirements vkMemoryRequirements{};
	vkGetBufferMemoryRequirements(vkDevice, uniformData.vkBuffer, &vkMemoryRequirements);

	VkMemoryAllocateInfo vkMemoryAllocateInfo{};
	vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkMemoryAllocateInfo.pNext = nullptr;
	vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
	vkMemoryAllocateInfo.memoryTypeIndex = 0;

	for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProp.memoryTypeCount; i++)
	{
		if (1 == (vkMemoryRequirements.memoryTypeBits & 1))
		{
			if (vkPhysicalDeviceMemoryProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			{
				vkMemoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
		vkMemoryRequirements.memoryTypeBits >>= 1;
	}

	//2.Allocate device memory
	vkResult = vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, nullptr, &uniformData.vkDeviceMemory);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed create device memory for uniform data!!!");
		return vkResult;
	}

	//3.Bind with device memory handle with vulkan buffer
	vkResult = vkBindBufferMemory(vkDevice, uniformData.vkBuffer, uniformData.vkDeviceMemory, 0);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed bind buffer memory for uniform data!!!");
		return vkResult;
	}

	//update unifrom data
	vkResult = UpdateUniformBuffer();
	if (vkResult != VK_SUCCESS)
	{
		LogData("UpdateUniformBuffer failed while creating uniform data!!!");
		return vkResult;
	}

	return vkResult;
}

//update uniform buffer
VkResult UpdateUniformBuffer()
{
	VkResult vkResult = VK_SUCCESS;

	MyUniformData myUniformData{};

	//update matrices
	myUniformData.modelMatrix = glm::mat4(1.0);
	
	glm::mat4 translationMatrix = glm::mat4(1.0);
	glm::mat4 rotationMatrix = glm::mat4(1.0);

	translationMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -4.0f));
	rotationMatrix = glm::rotate(glm::mat4(rotationMatrix), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

	myUniformData.modelMatrix = translationMatrix * rotationMatrix;


	myUniformData.viewMatrix = glm::mat4(1.0);
	//myUniformData.projectionMatrix = glm::mat4(1.0);
	glm::mat4 perspectiveProjectionMatrix = glm::mat4(1.0);

	perspectiveProjectionMatrix = glm::perspective(glm::radians(45.0f), (float)winWidth/(float)winHeight, 0.1f, 100.0f);
	perspectiveProjectionMatrix[1][1] = perspectiveProjectionMatrix[1][1] * (-1.0f);

	myUniformData.projectionMatrix = perspectiveProjectionMatrix;


	//map uniform buffer
	void* data = nullptr;

	vkResult = vkMapMemory(vkDevice, uniformData.vkDeviceMemory, 0, sizeof(MyUniformData), 0, &data);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to map uniform memory!!!");
		return vkResult;
	}

	//copy data
	memcpy(data, &myUniformData, sizeof(MyUniformData));

	//unmap memory
	vkUnmapMemory(vkDevice, uniformData.vkDeviceMemory);

	return vkResult;
}

//create vertex buffer using stage buffer
VkResult createVertexBuffer2()
{

	// Static data like vertex and index buffer should be stored on the device memory for optimal (and fastest) access by the GPU
	//
	// To achieve this we use so-called "staging buffers" :
	// - Create a buffer that's visible to the host (and can be mapped)
	// - Copy the data to this buffer
	// - Create another buffer that's local on the device (VRAM) with the same size
	// - Copy the data from the host to the device using a command buffer
	// - Delete the host visible (staging) buffer
	// - Use the device local buffers for rendering
	//
	// Note: On unified memory architectures where host (CPU) and GPU share the same memory, staging is not necessary
	// To keep this sample easy to follow, there is no check for that in place


	VkResult vkResult = VK_SUCCESS;

	//1.
	VertexData vertexData_stagingBuffer_position;

	std::vector<Vertex> vertexBuffer
	{
		{0.0f, 1.0f, 0.0f},
		{-1.0f, -1.0f, 0.0f},
		{1.0f, -1.0f, 0.0f}
	};

	uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

	VkBufferCreateInfo vkBufferCreateInfo_stagingBufferCI{};
	vkBufferCreateInfo_stagingBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferCreateInfo_stagingBufferCI.pNext = NULL;
	vkBufferCreateInfo_stagingBufferCI.flags = 0;
	vkBufferCreateInfo_stagingBufferCI.size = vertexBufferSize;
	vkBufferCreateInfo_stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;		//Buffer use as data transfer source
	vkBufferCreateInfo_stagingBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	//Concurrent use or multi thread


	vkResult = vkCreateBuffer(vkDevice, &vkBufferCreateInfo_stagingBufferCI, NULL, &vertexData_stagingBuffer_position.vkBuffer);
	if (VK_SUCCESS != vkResult)
	{
		fprintf(gpFile, "createVertexBufferStaging():-Call to VkCreateBuffer() is failed (%d)\n", vkResult);
		return vkResult;
	}

	VkMemoryRequirements vkMemoryRequirementsStagingBuffer{};
	vkGetBufferMemoryRequirements(vkDevice, vertexData_stagingBuffer_position.vkBuffer, &vkMemoryRequirementsStagingBuffer);

	VkMemoryAllocateInfo vkMemoryAllocateInfoStagingBuffer{};
	vkMemoryAllocateInfoStagingBuffer.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkMemoryAllocateInfoStagingBuffer.pNext = nullptr;
	vkMemoryAllocateInfoStagingBuffer.allocationSize = vkMemoryRequirementsStagingBuffer.size;
	vkMemoryAllocateInfoStagingBuffer.memoryTypeIndex = 0;

	for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProp.memoryTypeCount; i++)
	{
		if (1 == (vkMemoryRequirementsStagingBuffer.memoryTypeBits & 1))
		{
			//VK_MEMORY_PROPERTY_HOST_COHERENT_BIT- No need to manage vulkan cache mechanism of flush and mapping as we ordered vulkan to maintain coherancey
			if (vkPhysicalDeviceMemoryProp.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			{
				vkMemoryAllocateInfoStagingBuffer.memoryTypeIndex = i;
				break;
			}
		}
		vkMemoryRequirementsStagingBuffer.memoryTypeBits >>= 1;
	}


	vkResult = vkAllocateMemory(vkDevice, &vkMemoryAllocateInfoStagingBuffer, nullptr, &vertexData_stagingBuffer_position.vkDeviceMemory);
	if (vkResult != VK_SUCCESS)
	{
		LogData("staging buufer :Failed create device memory!!!");
		return vkResult;
	}

	vkResult = vkBindBufferMemory(vkDevice, vertexData_stagingBuffer_position.vkBuffer, vertexData_stagingBuffer_position.vkDeviceMemory, 0);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed bind buffer memory for staging buffer!!!");
		return vkResult;
	}

	void* data = nullptr;
	vkResult = vkMapMemory(vkDevice, vertexData_stagingBuffer_position.vkDeviceMemory, 0, vkMemoryAllocateInfoStagingBuffer.allocationSize, 0, &data);
	if (vkResult != VK_SUCCESS)
	{
		LogData("stage buffer-Failed to map memory!!!");
		return vkResult;
	}

	memcpy(data, vertexBuffer.data(), vertexBufferSize);

	vkUnmapMemory(vkDevice, vertexData_stagingBuffer_position.vkDeviceMemory);

	//2.
	VkBufferCreateInfo vkBufferCreateInfo{};
	vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferCreateInfo.pNext = nullptr;
	vkBufferCreateInfo.flags = 0;
	vkBufferCreateInfo.size = vertexBufferSize;
	vkBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkResult = vkCreateBuffer(vkDevice, &vkBufferCreateInfo, nullptr, &vertexDataPosition.vkBuffer);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to create buffer!!!");
		return vkResult;
	}

	VkMemoryRequirements vkMemoryRequirements{};
	vkGetBufferMemoryRequirements(vkDevice, vertexDataPosition.vkBuffer, &vkMemoryRequirements);

	VkMemoryAllocateInfo vkMemoryAllocateInfo{};
	vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkMemoryAllocateInfo.pNext = nullptr;
	vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
	vkMemoryAllocateInfo.memoryTypeIndex = 0;

	for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProp.memoryTypeCount; i++)
	{
		if (1 == (vkMemoryRequirements.memoryTypeBits & 1))
		{
			if (vkPhysicalDeviceMemoryProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)	//Device visible memory
			{
				vkMemoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
		vkMemoryRequirements.memoryTypeBits >>= 1;
	}

	//2.Allocate device memory
	vkResult = vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, nullptr, &vertexDataPosition.vkDeviceMemory);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed create device memory!!!");
		return vkResult;
	}

	vkResult = vkBindBufferMemory(vkDevice, vertexDataPosition.vkBuffer, vertexDataPosition.vkDeviceMemory, 0);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed bind buffer memory!!!");
		return vkResult;
	}

	//3.Copy data 
	//Create cmd buffer
	VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo{};
	vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	vkCommandBufferAllocateInfo.pNext = nullptr;
	vkCommandBufferAllocateInfo.commandPool = vkCommandBufferPool;
	vkCommandBufferAllocateInfo.commandBufferCount = 1;
	vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
	vkResult = vkAllocateCommandBuffers(vkDevice, &vkCommandBufferAllocateInfo, &vkCommandBuffer);
	if (vkResult != VK_SUCCESS)
	{
		LogData("vkAllocateCommandBuffers for buffer copy!!!");
		return vkResult;
	}

	//4
	VkCommandBufferBeginInfo vkCommandBufferBeginInfo{};
	vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkCommandBufferBeginInfo.pNext = nullptr;
	vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;		//We don't need reset call because of this flag. This will one time submit.

	// Begin command buffer
	vkResult = vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to begin command Buffer for copy!!!");
		return vkResult;
	}

	VkBufferCopy vkBufferCopy{};
	vkBufferCopy.srcOffset = 0;
	vkBufferCopy.dstOffset = 0;
	vkBufferCopy.size = vertexBufferSize;

	vkCmdCopyBuffer(vkCommandBuffer, vertexData_stagingBuffer_position.vkBuffer, vertexDataPosition.vkBuffer, 1, &vkBufferCopy);

	vkResult = vkEndCommandBuffer(vkCommandBuffer);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to end command Buffer for copy!!!");
		return vkResult;
	}

	//5.
	// We don't need synchronization between command buffers so We don't need dstmask as well.
	//In display we have 2 command buffers and we need synchronization between then per display call.
	VkSubmitInfo vkSubmitInfo{};
	vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vkSubmitInfo.pNext = nullptr;
	vkSubmitInfo.commandBufferCount = 1;
	vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;

	vkResult = vkQueueSubmit(vkQueue, 1, &vkSubmitInfo, VK_NULL_HANDLE);
	if (VK_SUCCESS != vkResult)
	{
		fprintf(gpFile, "vertex copy:- VkQueueSubmit():- Failed!!");
		return vkResult;
	}

	//wait for queue as we are in initialize
	vkResult = vkQueueWaitIdle(vkQueue);
	if (vkResult != VK_SUCCESS)
	{
		LogData("vkQueueWaitIdle failed!!!");
		return vkResult;
	}


	//6.free
	vkFreeCommandBuffers(vkDevice, vkCommandBufferPool, 1, &vkCommandBuffer);
	vkCommandBuffer = VK_NULL_HANDLE;

	//free local
	if (vertexData_stagingBuffer_position.vkDeviceMemory)
	{
		vkFreeMemory(vkDevice, vertexData_stagingBuffer_position.vkDeviceMemory, nullptr);
		vertexData_stagingBuffer_position.vkDeviceMemory = VK_NULL_HANDLE;
	}

	if (vertexData_stagingBuffer_position.vkBuffer)
	{
		vkDestroyBuffer(vkDevice, vertexData_stagingBuffer_position.vkBuffer, nullptr);
		vertexData_stagingBuffer_position.vkBuffer = VK_NULL_HANDLE;
	}

	return vkResult;
}

//Create descriptor set layout
// Descriptor set layouts define the interface between our application and the shader
// Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
// So every shader binding should map to one descriptor set layout binding
VkResult createDescriptorSetLayout()
{
	VkResult vkResult = VK_SUCCESS;

	VkDescriptorSetLayoutBinding vkDescriptorSetBinding{};
	vkDescriptorSetBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vkDescriptorSetBinding.binding = 0;		//Binding in shader "layout(binding=0) uniform mvpMatrix"
	vkDescriptorSetBinding.descriptorCount = 1;
	vkDescriptorSetBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;    //Add  more with | VK_SHADER_STAGE_VERTEX_BIT
	vkDescriptorSetBinding.pImmutableSamplers = nullptr;


	VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutInfo{};
	vkDescriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	vkDescriptorSetLayoutInfo.pNext = nullptr;
	vkDescriptorSetLayoutInfo.flags = 0;		//reserved
	vkDescriptorSetLayoutInfo.bindingCount = 1;
	vkDescriptorSetLayoutInfo.pBindings = &vkDescriptorSetBinding;	////pBindings is array of struct VkDescriptorSetLayoutBinding

	vkResult = vkCreateDescriptorSetLayout(vkDevice, &vkDescriptorSetLayoutInfo, nullptr, &vkDescriptorSetLayout);
	if (vkResult == VK_SUCCESS)
	{
		LogData("VK Descriptor set layout created!!!");
	}

	return vkResult;
}

//Create descriptor sets.
// Shaders access data using descriptor sets that "point" at our uniform buffers
// The descriptor sets make use of the descriptor set layouts created above 
//VkResult createDescriptorSets()
//{
//	VkResult vkResult = VK_SUCCESS;
//
//
//}

VkResult createPipelineLayout()
{
	VkResult vkResult = VK_SUCCESS;

	VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo{};
	vkPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	vkPipelineLayoutCreateInfo.pNext = nullptr;
	vkPipelineLayoutCreateInfo.flags = 0;
	vkPipelineLayoutCreateInfo.setLayoutCount = 1;
	vkPipelineLayoutCreateInfo.pSetLayouts = &vkDescriptorSetLayout;
	vkPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	vkPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	vkResult = vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutCreateInfo, nullptr, &vkPipelineLayout);
	if (vkResult == VK_SUCCESS)
	{
		LogData("Pipeline layout created!!!");
	}
	return vkResult;
}

//create descriptor pool
VkResult createDescriptorPool()
{
	VkResult vkResult = VK_SUCCESS;

	VkDescriptorPoolSize vkDescriptorPoolSize{};
	vkDescriptorPoolSize.descriptorCount = 1;
	vkDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;


	VkDescriptorPoolCreateInfo vkDescriptorPoolCreateInfo{};
	vkDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	vkDescriptorPoolCreateInfo.pNext = nullptr;
	vkDescriptorPoolCreateInfo.flags = 0;
	vkDescriptorPoolCreateInfo.poolSizeCount = 1;	//numof above struct count;
	vkDescriptorPoolCreateInfo.pPoolSizes = &vkDescriptorPoolSize;
	vkDescriptorPoolCreateInfo.maxSets = 1;			//How many sets you want to create like above

	vkResult = vkCreateDescriptorPool(vkDevice, &vkDescriptorPoolCreateInfo, nullptr, &vkDescriptorPool);
	if (vkResult != VK_SUCCESS)
	{
		LogData("vkCreateDescriptorPool failed!!!!");
		return vkResult;
	}

	return vkResult;
}

//create descriptor set
VkResult createDescriptorSet()
{
	VkResult vkResult = VK_SUCCESS;

	VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo{};
	vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	vkDescriptorSetAllocateInfo.pNext = nullptr;
	vkDescriptorSetAllocateInfo.descriptorPool = vkDescriptorPool;
	vkDescriptorSetAllocateInfo.descriptorSetCount = 1;  //How many structs
	vkDescriptorSetAllocateInfo.pSetLayouts = &vkDescriptorSetLayout;

	vkResult = vkAllocateDescriptorSets(vkDevice, &vkDescriptorSetAllocateInfo, &vkDescriptorSet);
	if (vkResult != VK_SUCCESS)
	{
		LogData("vkAllocateDescriptorSets failed!!!");
		return vkResult;
	}

	//Describe what we want..buufer or image
	VkDescriptorBufferInfo vkDescriptorBufferInfo{};
	vkDescriptorBufferInfo.buffer = uniformData.vkBuffer;
	vkDescriptorBufferInfo.offset = 0;
	vkDescriptorBufferInfo.range = sizeof(MyUniformData);

	//Update above descriptor set directly to the shader
	//We have two options, either write or copy. Same uniform but different shader then use copy.

	VkWriteDescriptorSet vkWriteDescriptorSet{};
	vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	vkWriteDescriptorSet.dstSet = vkDescriptorSet;
	vkWriteDescriptorSet.dstArrayElement = 0;
	vkWriteDescriptorSet.descriptorCount = 1;
	vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;
	vkWriteDescriptorSet.pImageInfo = nullptr;		//used in texture
	vkWriteDescriptorSet.pTexelBufferView = nullptr;
	vkWriteDescriptorSet.dstBinding = 0;		//Where do you want to bind...Our unfiform is at 'binding zero index in shader'

	vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);

	LogData("vkUpdateDescriptorSets called!!");

	return vkResult;
}

//Pipeline states:
// Pipeline state object
//1.VERTEX INPUT STATE
//2.INPUT ASSEMBLY STATE
//3.RASTERIZER STATE
//4.COLOR BLEND STATE
//5.VIEWPORT/SCISSOR STATE
//6.DEPTH/STENCILE STATE
//7.DYNAMIC STATE		-->You can skip
//8.MULTISAMPLE STATE
//9.SHADER STATE
//10.TESSELLATOR STATE	-->You can skip
VkResult createGraphicsPipeline()
{
	VkResult vkResult = VK_SUCCESS;

	std::vector<VkVertexInputBindingDescription> vkVertexInputBindingDescriptionVector(1);
	vkVertexInputBindingDescriptionVector[0].binding = 0;
	vkVertexInputBindingDescriptionVector[0].stride = sizeof(Vertex);
	vkVertexInputBindingDescriptionVector[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	// specifying the rate at which vertex attributes are pulled from buffers

	// These match the following shader layout (see shader.vert):
	//	layout (location = 0) in vec3 inPos;
	//	layout (location = 1) in vec3 inColor;
	std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescriptionVector(1);
	vkVertexInputAttributeDescriptionVector[0].binding = 0;
	vkVertexInputAttributeDescriptionVector[0].location = 0;		//layout(location=0) in vec4 vPosition; should be same on both sides
	vkVertexInputAttributeDescriptionVector[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vkVertexInputAttributeDescriptionVector[0].offset = 0;	//useed in interleaved, how much to jump

	//1.VERTEX INPUT STATE
	VkPipelineVertexInputStateCreateInfo vkPipelineVertexInputStateCreateInfo{};
	vkPipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vkPipelineVertexInputStateCreateInfo.pNext = nullptr;
	vkPipelineVertexInputStateCreateInfo.flags = 0;
	vkPipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vkVertexInputAttributeDescriptionVector.size());
	vkPipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vkVertexInputAttributeDescriptionVector.data();
	vkPipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vkVertexInputBindingDescriptionVector.size());;
	vkPipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vkVertexInputBindingDescriptionVector.data();

	//2.INPUT ASSEMBLY STATE
	VkPipelineInputAssemblyStateCreateInfo vkPipelineInputAssemblyStateCreateInfo{};
	vkPipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	vkPipelineInputAssemblyStateCreateInfo.pNext = nullptr;
	vkPipelineInputAssemblyStateCreateInfo.flags = 0;
	vkPipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	//primitiveRestartEnable controls whether a special vertex index value is treated as restarting the assembly of primitives.
	// This enable only applies to indexed draws (vkCmdDrawIndexed, vkCmdDrawMultiIndexedEXT, and vkCmdDrawIndexedIndirect),

	//3.RASTERIZER STATE
	VkPipelineRasterizationStateCreateInfo vkPipelineRasterizationStateCreateInfo{};
	vkPipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	vkPipelineRasterizationStateCreateInfo.pNext = nullptr;
	vkPipelineRasterizationStateCreateInfo.flags = 0;
	vkPipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	vkPipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
	vkPipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	vkPipelineRasterizationStateCreateInfo.lineWidth = 1.0;
	//depthBias and depthclamp is not required, but it is part of structure.

	//4.COLOR BLEND STATE
	// Color blend state describes how blend factors are calculated (if used)
	// We need one blend attachment state per color attachment (even if blending is not used)
	std::vector<VkPipelineColorBlendAttachmentState> vkPiepelineColorBlendAttachmentStateVector(1);
	vkPiepelineColorBlendAttachmentStateVector[0].colorWriteMask = 0xf;
	vkPiepelineColorBlendAttachmentStateVector[0].blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo vkPipelineColorBlendStateCreateInfo{};
	vkPipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	vkPipelineColorBlendStateCreateInfo.pNext = nullptr;
	vkPipelineColorBlendStateCreateInfo.flags = 0;
	vkPipelineColorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t>(vkPiepelineColorBlendAttachmentStateVector.size());
	vkPipelineColorBlendStateCreateInfo.pAttachments = vkPiepelineColorBlendAttachmentStateVector.data();

	//5.Viewport  scissor 
	vkViewPort = VkViewport{};
	vkViewPort.x = 0.0;
	vkViewPort.y = 0.0;
	vkViewPort.width = static_cast<float>(vkExtend2D_SwapChain.width);
	vkViewPort.height = static_cast<float>(vkExtend2D_SwapChain.height);
	vkViewPort.minDepth = 0.0f;
	vkViewPort.maxDepth = 1.0f;

	vkRect2DScissor = VkRect2D{};
	vkRect2DScissor.extent.width = vkExtend2D_SwapChain.width;
	vkRect2DScissor.extent.height = vkExtend2D_SwapChain.height;
	vkRect2DScissor.offset.x = 0;
	vkRect2DScissor.offset.y = 0;

	VkPipelineViewportStateCreateInfo vkPipelineViewportStateCreateInfo{};
	vkPipelineViewportStateCreateInfo.flags = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vkPipelineViewportStateCreateInfo.pNext = nullptr;
	vkPipelineViewportStateCreateInfo.flags = 0;
	vkPipelineViewportStateCreateInfo.viewportCount = 1;
	vkPipelineViewportStateCreateInfo.pViewports = &vkViewPort;
	vkPipelineViewportStateCreateInfo.pScissors = &vkRect2DScissor;

	//6.DEPTH STATE
	//Not used Rn

	//7.Dynamic state
	//We don;t have any rn

	//8.MS STATE
	VkPipelineMultisampleStateCreateInfo vkPipelineMultisampleStateCreateInfo{};
	vkPipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	vkPipelineMultisampleStateCreateInfo.pNext = nullptr;
	vkPipelineMultisampleStateCreateInfo.flags = 0;
	vkPipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	//vkPipelineMultisampleStateCreateInfo.pSampleMask = nullptr;

	//9.SHADER STATE
	std::vector<VkPipelineShaderStageCreateInfo> vkPipelineShaderStageCreateInfoVector(2);
	//Vertex module
	vkPipelineShaderStageCreateInfoVector[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vkPipelineShaderStageCreateInfoVector[0].pNext = nullptr;
	vkPipelineShaderStageCreateInfoVector[0].flags = 0;
	vkPipelineShaderStageCreateInfoVector[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	vkPipelineShaderStageCreateInfoVector[0].module = vkShaderModuleVertex;
	vkPipelineShaderStageCreateInfoVector[0].pName = "main";
	vkPipelineShaderStageCreateInfoVector[0].pSpecializationInfo = nullptr;

	//Fragment module
	vkPipelineShaderStageCreateInfoVector[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vkPipelineShaderStageCreateInfoVector[1].pNext = nullptr;
	vkPipelineShaderStageCreateInfoVector[1].flags = 0;
	vkPipelineShaderStageCreateInfoVector[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	vkPipelineShaderStageCreateInfoVector[1].module = vkShaderModuleFragment;
	vkPipelineShaderStageCreateInfoVector[1].pName = "main";
	vkPipelineShaderStageCreateInfoVector[1].pSpecializationInfo = nullptr;

	//10.TESSELLATOR STATE
	//We are not using rn

	//Pipeline cache
	VkPipelineCacheCreateInfo vkPipelineCacheCreateInfo{};
	vkPipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	vkPipelineCacheCreateInfo.pNext = nullptr;
	vkPipelineCacheCreateInfo.flags = 0;

	VkPipelineCache vkPipelineCache = VK_NULL_HANDLE;
	vkResult = vkCreatePipelineCache(vkDevice, &vkPipelineCacheCreateInfo, nullptr, &vkPipelineCache);
	if (vkResult != VK_SUCCESS)
	{
		LogData("Failed to create pipeline cache object!!!");
		return vkResult;
	}

	//Create pipeline
	VkGraphicsPipelineCreateInfo vkGraphicsPipelineCreateInfo{};
	vkGraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	vkGraphicsPipelineCreateInfo.pNext = nullptr;
	vkGraphicsPipelineCreateInfo.flags = 0;
	vkGraphicsPipelineCreateInfo.pVertexInputState = &vkPipelineVertexInputStateCreateInfo;
	vkGraphicsPipelineCreateInfo.pInputAssemblyState = &vkPipelineInputAssemblyStateCreateInfo;
	vkGraphicsPipelineCreateInfo.pRasterizationState = &vkPipelineRasterizationStateCreateInfo;
	vkGraphicsPipelineCreateInfo.pColorBlendState = &vkPipelineColorBlendStateCreateInfo;
	vkGraphicsPipelineCreateInfo.pViewportState = &vkPipelineViewportStateCreateInfo;
	vkGraphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	vkGraphicsPipelineCreateInfo.pDynamicState = nullptr;
	vkGraphicsPipelineCreateInfo.pMultisampleState = &vkPipelineMultisampleStateCreateInfo;
	vkGraphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(vkPipelineShaderStageCreateInfoVector.size());
	vkGraphicsPipelineCreateInfo.pStages = vkPipelineShaderStageCreateInfoVector.data();
	vkGraphicsPipelineCreateInfo.pTessellationState = nullptr;
	vkGraphicsPipelineCreateInfo.layout = vkPipelineLayout;
	vkGraphicsPipelineCreateInfo.renderPass = vkRenderPass;
	vkGraphicsPipelineCreateInfo.subpass = 0;
	vkGraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	vkResult = vkCreateGraphicsPipelines(vkDevice, vkPipelineCache, 1, &vkGraphicsPipelineCreateInfo, nullptr, &vkGraphicsPipeline);
	if (vkResult == VK_SUCCESS)
	{
		LogData("Graphics pipeline created!!!!!");
	}

	return vkResult;
}