#include "external/vulkan/vulkan.h"

#include "lingo.h"
#include "maths.h"
#include "platform.h"

void OutputSineWave(i16 *Samples, i32 SamplesPerSecond, i32 SampleCount,
                    i32 ToneFrequency, f32 tSine)
{
    i16 ToneVolume = 3000;
    i32 WavePeriod = SamplesPerSecond / ToneFrequency;

    i16* SampleOut = Samples;
    for (i32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex) {
        i16 SampleValue = 0;
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
        tSine += 2.0f * PI32 / (r32)WavePeriod;
        if (tSine > (2.0f * PI32)) {
            tSine -= 2.0f * PI32;
        }
    }
}

#if 0
internal b32 HasExtensions(VkPhysicalDevice *Device, char **RequiredExtensions, u32 ExtensionCount) {
    u32 DeviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(Device, NULL, &DeviceExtensionCount, NULL);
#define MAX_DEVICE_EXTENSION_COUNT 107
    //todo: dynamically allocate memory
    VkExtensionProperties Extensions[MAX_DEVICE_EXTENSION_COUNT/*DeviceExtensionCount*/] = {0};
#undef  MAX_DEVICE_EXTENSION_COUNT
    for (u32 ExtensionIndex;
                ExtensionIndex < ExtensionCount;
                ExtensionIndex++)
    {
        b32 HasExtension = 0;
        for (u32 DeviceExtensionIndex = 0;
                 DeviceExtensionIndex < DeviceExtensionCount;
                 DeviceExtensionIndex++)
        {
            //todo: actually compare the strings of the device and the required extensions
            //note: why are these strings?  
        }
    }
}

internal b32 GetQueuefamily(VkPhysicalDevice *Device, VkQueueFlags Flags, i32 QueueFamilyIndex) {
    b32 Succes = 0;
    u32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*Device, &QueueFamilyCount, NULL);
    VkQueueFamilyProperties QueueFamilies;
}

void VulkanTest(void) {
    VkInstance Instance = {0};
    VkApplicationInfo AppInfo = {0}; {
        AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pNext = NULL;
        AppInfo.pApplicationName = "app";
        AppInfo.pEngineName = NULL;
        AppInfo.engineVersion = 1;
        AppInfo.apiVersion = VK_API_VERSION_1_0;
    }
    VkInstanceCreateInfo InstanceInfo = {0}; {
        InstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        InstanceInfo.pNext = NULL;
        InstanceInfo.flags = 0;
        InstanceInfo.pApplicationInfo = &AppInfo;
        InstanceInfo.enabledLayerCount = 0;
        InstanceInfo.ppEnabledLayerNames = NULL;
        InstanceInfo.enabledExtensionCount = 0;
        InstanceInfo.ppEnabledExtensionNames = NULL;
    }

    VkResult Result = vkCreateInstance(&InstanceInfo, NULL, &Instance);
    if (Result != VK_SUCCESS) {
        //note: ERROR!! Failed to create vulkan instance
        //todo: logging
        Assert(!"NOOOOOOOOOO!!");
    }

    u32 DeviceCount;
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, NULL);
#define MAX_DEVICE_COUNT 1
    //todo: dynamically allocate memory
    VkPhysicalDevice Devices[MAX_DEVICE_COUNT/*DeviceCount*/] = {0};
#undef  MAX_DEVICE_COUNT
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices);

    const u32 ExtensionCount = 1;
    char *DeviceExtensions[ExtensionCount] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    for (u32 DeviceIndex = 0;
             DeviceIndex < DeviceCount;
             DeviceIndex++)
    {
        // if (HasRequiredEstensions(...))
        //note: assuming that my card has VK_KHR_SWAPCHAIN_EXTENSION_NAME
        {
            if (GetQueuefamily(Devices[DeviceIndex], 0, 0))

        }
    }

    vkDestroyInstance(Instance, NULL);
}
#endif
//note: vulkan is much harder than i expected

//todo: APP_ONLOAD function that runs when the app is loaded
//note:
//  this will require app memory, so that the vulkan stuff
//  can be stored in it and shared between the functions

__declspec(dllexport) APP_UPDATE(AppUpdate) {
    //
}
