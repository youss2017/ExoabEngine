#include "EngineCore.hpp"
#include "GraphicsCardFeatureValidation.hpp"
#include <iostream>
#include <Utility/CppUtility.hpp>
#include <cassert>
#include <ranges>

static VkBool32 VKAPI_ATTR ApiDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
static VkBool32 DebugPrintfEXT_Callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData);

EGX_API egx::EngineCore::EngineCore(EngineCoreDebugFeatures debugFeatures, bool UsingRenderDOC)
	: Swapchain(nullptr), _CoreInterface(new VulkanCoreInterface()), UsingRenderDOC(UsingRenderDOC), _DebugCallbackHandle(nullptr)
{
	glfwInit();
	std::vector<const char*> layer_extensions;
	uint32_t extensions_count;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
	for (uint32_t i = 0; i < extensions_count; i++) layer_extensions.push_back(extensions[i]);
	std::vector<const char*> layers;
#ifdef _DEBUG
	layers.push_back("VK_LAYER_KHRONOS_validation");
	layer_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	uint32_t count = 0;
	vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &count, nullptr);
	std::vector<VkExtensionProperties> extProps(count);
	vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &count, extProps.data());
	if (std::ranges::count_if(extProps, [](VkExtensionProperties& l) {return strcmp(l.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME); })) {
		layer_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
#endif

	VkApplicationInfo appinfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appinfo.pApplicationName = "Application";
	appinfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	appinfo.pEngineName = "Application";
	appinfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	appinfo.apiVersion = VK_API_VERSION_1_3;
	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appinfo;
	createInfo.enabledLayerCount = (uint32_t)layers.size();
	createInfo.ppEnabledLayerNames = layers.data();
	createInfo.enabledExtensionCount = (uint32_t)layer_extensions.size();
	createInfo.ppEnabledExtensionNames = layer_extensions.data();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
#ifdef _DEBUG
	VkValidationFeatureEnableEXT enables[] =
	{
		(debugFeatures == EngineCoreDebugFeatures::GPUAssisted) ? VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT : VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
	};
	VkValidationFeaturesEXT validationFeatures{ VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = 1;
	validationFeatures.pEnabledValidationFeatures = enables;
	debugCreateInfo.pNext = &validationFeatures;
	debugCreateInfo = {};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugCreateInfo.pfnUserCallback = ApiDebugCallback;
	createInfo.pNext = &debugCreateInfo;
#endif

	vkCreateInstance(&createInfo, NULL, &_CoreInterface->Instance);
#if defined(VK_NO_PROTOTYPES)
	volkLoadInstance(instance);
#endif
#ifdef _DEBUG
	if (CreateDebugUtilsMessengerEXT(_CoreInterface->Instance, &debugCreateInfo, nullptr, &_CoreInterface->DebugMessenger) != VK_SUCCESS)
	{
		LOG(WARNING, "Failed to set up debug messenger!");
	}
	// From https://anki3d.org/debugprintf-vulkan/
	// Populate the VkDebugReportCallbackCreateInfoEXT
	VkDebugReportCallbackCreateInfoEXT ci = {};
	ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	ci.pfnCallback = DebugPrintfEXT_Callback;
	ci.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
	ci.pUserData = nullptr;

	// Create the callback handle
	PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_CoreInterface->Instance, "vkCreateDebugReportCallbackEXT");
	if (createDebugReportCallback)
		createDebugReportCallback(_CoreInterface->Instance, &ci, nullptr, &_DebugCallbackHandle);
#endif

#if defined(VK_NO_PROTOTYPES)
	volkLoadDevice(s_Context->defaultDevice);
#endif

}

EGX_API egx::EngineCore::~EngineCore()
{
	WaitIdle();
	if (_DebugCallbackHandle)
	{
		PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_CoreInterface->Instance, "vkDestroyDebugReportCallbackEXT");
		DestroyDebugReportCallbackEXT(_CoreInterface->Instance, _DebugCallbackHandle, nullptr);
	}
	if (Swapchain)
		delete Swapchain;
}

void EGX_API egx::EngineCore::_AssociateWindow(PlatformWindow* Window, uint32_t MaxFramesInFlight, bool VSync, bool SetupImGui)
{
	assert(Swapchain == nullptr);
	// [NOTE]: ImGui multi-viewport uses VK_FORMAT_B8G8R8A8_UNORM, if we use a different format
	// [NOTE]: there will be a mismatch of format between pipeline state objects and render pass
	_CoreInterface->MaxFramesInFlight = MaxFramesInFlight;
	Swapchain = new VulkanSwapchain(
		_CoreInterface,
		Window->GetWindow(),
		VSync,
		SetupImGui);
}

std::vector<egx::Device> EGX_API egx::EngineCore::EnumerateDevices()
{
	std::vector<egx::Device> list;
	uint32_t Count = 0;
	vkEnumeratePhysicalDevices(_CoreInterface->Instance, &Count, nullptr);
	std::vector<VkPhysicalDevice> Ids(Count);
	vkEnumeratePhysicalDevices(_CoreInterface->Instance, &Count, Ids.data());

	for (auto device : Ids) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);
		egx::Device d{};
		d.IsDedicated = !((properties.deviceType & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) |
			(properties.deviceType & VK_PHYSICAL_DEVICE_TYPE_CPU));
		d.Id = device;
		d.Properties = properties;
		d.VendorName = properties.deviceName;
		VkPhysicalDeviceMemoryProperties memProp;
		vkGetPhysicalDeviceMemoryProperties(device, &memProp);
		for (uint32_t j = 0; j < memProp.memoryHeapCount; j++) {
			if (memProp.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				d.VideoRam += memProp.memoryHeaps[j].size;
			}
			else {
				d.SharedSystemRam += memProp.memoryHeaps[j].size;
			}
		}
		list.push_back(d);
	}

	return list;
}

const egx::ref<egx::VulkanCoreInterface>& egx::EngineCore::EstablishDevice(const egx::Device& Device, const VkPhysicalDeviceFeatures2& features)
{
	using namespace std;
	VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };

	if (!egx::GraphicsCardFeatureValidation_Check(Device.Id, features)) {
		LOG(ERR, "{0} has failed feature validation check.", Device.VendorName);
	}
	else
		LOG(INFO, "{0} has passed feature validation check.", Device.VendorName);

	createInfo.pNext = &features;

	uint32_t QueueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device.Id, &QueueCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Device.Id, &QueueCount, QueueFamilies.data());

	int index = 0;
	bool set = false;
	for (const auto& queue : QueueFamilies)
	{
		if (queue.queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT))
		{
			set = true;
			break;
		}
		index++;
	}

	if (!set)
	{
		LOG(ERR, "Could not create logical device since the Queue Flags could not be found!");
	}

	float priorities = { 1.0f };
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &priorities;
	queueCreateInfo.queueFamilyIndex = index;

	std::vector<const char*> enabledExtensions;
	enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	enabledExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

#ifdef _DEBUG
	enabledExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	enabledExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
#endif

	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
	createInfo.ppEnabledExtensionNames = enabledExtensions.data();

	vkCreateDevice(Device.Id, &createInfo, NULL, &this->_CoreInterface->Device);
	this->_CoreInterface->QueueFamilyIndex = index;

	vkGetDeviceQueue(this->_CoreInterface->Device, index, 0, &this->_CoreInterface->Queue);

	this->_CoreInterface->MemoryContext = VkAlloc::CreateContext(_CoreInterface->Instance,
		this->_CoreInterface->Device, Device.Id, /* 64 mb*/ 64 * (1024 * 1024), !UsingRenderDOC);
	this->_CoreInterface->PhysicalDevice = Device;
	_CoreInterface->MaxFramesInFlight = 1;
	_CoreInterface->Features = features;
	return _CoreInterface;
}

ImGuiContext* egx::EngineCore::GetContext() const
{
	return ImGui::GetCurrentContext();
}

static VkBool32 VKAPI_ATTR ApiDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOG(WARNING, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOG(ERR, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		LOG(INFO, pCallbackData->pMessage);
	}
	else
	{
		LOG(INFOBOLD, pCallbackData->pMessage);
	}
	return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static VkBool32 DebugPrintfEXT_Callback(VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData)
{
	LOG(INFO, "{0:%s}({1:%d}):{2:%s}", pLayerPrefix, messageCode, pMessage);
	return false;
}

cpp::Logger* egx::EngineCore::GetEngineLogger()
{
	return &cpp::Logger::GetGlobalLogger();
}
