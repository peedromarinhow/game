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

    vkDestroyInstance(Instance, NULL);
}

__declspec(dllexport) APP_UPDATE(AppUpdate) {
    VulkanTest();
}
