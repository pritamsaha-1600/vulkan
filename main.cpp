#include "Application.h"
#include <vulkan/vulkan.h>
#include <print>
#include <span>
#include <ranges>
#include <vector>
#include <cstring>

size_t isExtensionSupported(std::span<const char*> extensions) {
    unsigned count;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, availableExtensions.data());

    for (auto [i, extension] : std::views::enumerate(extensions))
        if (std::ranges::find_if(availableExtensions, [extension](const auto &available) {
                return std::strcmp(available.extensionName, extension) == 0;
            }) == availableExtensions.end())
            return i;
    return extensions.size();
}

size_t isLayerSupported(std::span<const char*> layers) {
    unsigned count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> availableLayers(count);
    vkEnumerateInstanceLayerProperties(&count, availableLayers.data());

    for (auto [i, layer] : std::views::enumerate(layers))
        if (std::ranges::find_if(availableLayers, [layer](const auto &available) {
            return std::strcmp(available.layerName, layer) == 0;
        }) == availableLayers.end())
            return i;
    return layers.size();
}

int main() {
    // create vulkan instance
    std::vector<const char *> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        // VK_EXT_DEBUG_UTILS_EXTENSION_NAME,  // TODO: only if validation is turned on
    };
    if (auto i = isExtensionSupported(instanceExtensions); i != instanceExtensions.size()) {
        std::println(stderr, "Instance Extension {} is not supported", instanceExtensions[i]);
        std::exit(1);
    }

    // TODO: only if validation is turned on
    std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };
    if (auto i = isLayerSupported(validationLayers); i != validationLayers.size()) {
        std::println(stderr, "Validation Layer {} is not supported", validationLayers[i]);
        std::exit(1);
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan",
        .apiVersion = VK_API_VERSION_1_4
    };
    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data()
    };

    VkInstance instance;
    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        std::println(stderr, "vkCreateInstance failed");
        std::exit(1);
    }

    // TODO: if validation is turned on
    // auto messenger = setupDebugMessenger(instance);

    Application app (1280, 720, 360, 640, "X11 Window" );

    // create presentation surface
    VkXlibSurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = app.display,
        .window = app.window
    };
    VkSurfaceKHR surface;
    if (vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
        std::println(stderr, "vkCreateXlibSurfaceKHR failed");
        std::exit(1);
    }

    while (app.isWindowOpen) {
        app.processEvents();
    }
}
