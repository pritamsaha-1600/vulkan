#include "Application.h"

#include <X11/Xutil.h>
#include <print>
#include <span>
#include <ranges>
#include <vector>
#include <cstring>

void print_error(const std::string& message, bool exit=true) {
    std::print(stderr, "{}\n", message);
    if (exit)
        std::exit(1);
}

void Application::createWindow() {
    display = XOpenDisplay(nullptr);

    constexpr int attributeMask = CWBackPixel | CWEventMask;
    XSetWindowAttributes windowAttributes = {
        .background_pixel = WhitePixel(display, DefaultScreen(display)),
        .event_mask = KeyPressMask | KeyReleaseMask
        // TODO: look into the usefulness of StructureNotifyMask and ExposureMask
    };
    XSizeHints sizeHint = {
        .flags = PMinSize, // TODO: look into min_(max)aspect
        .min_width = static_cast<int>(minWidth),
        .min_height = static_cast<int>(minHeight),
    };

    window = XCreateWindow(
        display, DefaultRootWindow(display),
        0, 0, width, height,
        0, CopyFromParent, CopyFromParent, CopyFromParent,
        attributeMask, &windowAttributes);

    XSetStandardProperties(display, window,
                           title, nullptr, None, nullptr, 0, &sizeHint);

    WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
    if (!XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1))
        print_error("XSetWMProtocols failed: Couldn't set WM_DELETE_WINDOW property", false);
}

void Application::displayWindow() {
    if (isWindowVisible())  return;
    XMapWindow(display, window);
    is_window_visible = true;
}

size_t isExtensionSupported(std::span<const char* const> extensions) {
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

size_t isLayerSupported(std::span<const char* const> layers) {
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

void Application::createVulkanInstance() {
    const std::vector<const char *> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        // VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    if (auto i = isExtensionSupported(instanceExtensions); i != instanceExtensions.size())
        return print_error(std::format("Instance Extension {} is not supported", instanceExtensions[i]),
            true);

    std::vector<const char *> validationLayers;
    if (requireValidationLayers) {
        validationLayers = {
            "VK_LAYER_KHRONOS_validation",
        };
        if (auto i = isLayerSupported(validationLayers); i != validationLayers.size())
            return print_error(std::format("Validation Layer {} is not supported", validationLayers[i]),
            true);
    }

    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = title,
        .apiVersion = VK_API_VERSION_1_4
    };
    const VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data()
    };

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS)
        print_error("vkCreateInstance failed: Couldn't create Vulkan instance", true);
    // auto messenger = setupDebugMessenger(instance);
}

void Application::createSurface() {
    const VkXlibSurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = display,
        .window = window
    };
    if (vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS)
        return print_error("vkCreateXlibSurfaceKHR failed: Couldn't create Vulkan Presentation Surface", true);
}

void Application::printDeviceProperties(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    std::string output = std::format("Device chosen: {}. Memory heap sizes: ", device_properties.deviceName);
    for (unsigned i = 0; i < memory_properties.memoryHeapCount; ++i)
        output += std::format("{} ", memory_properties.memoryHeaps->size);
    print_error(output, false);
}

void Application::createDevice() {
    unsigned count;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    physical_device = VK_NULL_HANDLE;
    unsigned queue_family_index = std::numeric_limits<unsigned>::max();
    for (const auto _device : devices) {
        unsigned extension_count;
        vkEnumerateDeviceExtensionProperties(_device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extension_count);
        vkEnumerateDeviceExtensionProperties(_device, nullptr, &extension_count, availableExtensions.data());

        bool all_extension_supported = true;
        for (const auto extension : deviceExtensions)
            if (std::ranges::find_if(availableExtensions, [extension](const auto &available) {
                    return std::strcmp(available.extensionName, extension) == 0;
            }) == availableExtensions.end()) {
                all_extension_supported = false;
                break;
            }
        if (!all_extension_supported)
            continue;

        unsigned queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(_device, &queue_family_count, queue_families.data());

        for (const auto [i, queue_family] : std::views::enumerate(queue_families)) {
            /* TODO: the same queue might not support both the surface and the graphics bit
                    implement logic for when these queues differ
            */
            VkBool32 supported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, surface, &supported);
            if (supported && queue_family.queueCount > 0 && (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                // printDeviceProperties(_device);

                queue_family_index = i;
                physical_device = _device;
                break;
            }
        }
        if (physical_device != VK_NULL_HANDLE)
            break;
    }
    if (physical_device == VK_NULL_HANDLE)
        return print_error("No suitable device found which supports the required extensions, the surface, "
            "and has a queue family with the desired properties.", true);

    constexpr float queue_priorities = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priorities,
    };
    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };
    if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
        return print_error("vkCreateDevice failed: Couldn't create Vulkan device", true);
    vkGetDeviceQueue(device, queue_create_info.queueFamilyIndex, 0, &queue);
}

void Application::createSwapchain() {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

    auto image_count = surface_capabilities.minImageCount + 1;

    VkExtent2D image_extent = surface_capabilities.currentExtent;
    // if currentExtent is set to invalid value
    if (image_extent.height == std::numeric_limits<unsigned>::max() ||
        image_extent.width == std::numeric_limits<unsigned>::max())
        image_extent = {.width = width, .height = height};
    else {
        print_error(std::format("Window width and height changed from ({},{}) to ({}, {})",
            width, height, image_extent.width, image_extent.height), false);
        width = image_extent.width;
        height = image_extent.height;
    }

    VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkImageUsageFlagBits image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkSurfaceFormatKHR image_format {.format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    VkSwapchainKHR old_swapchain = swapchain;

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = image_format.format,
        .imageColorSpace = image_format.colorSpace,
        .imageExtent = image_extent,
        .imageArrayLayers = 1,
        .imageUsage = image_usage,
        .imageSharingMode = sharing_mode,
        .preTransform = transform,
        .compositeAlpha = composite_alpha,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain
    };
    if (vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain) != VK_SUCCESS)
        return print_error("vkCreateSwapchainKHR failed: Couldn't create Vulkan swapchain", true);
    if (old_swapchain != VK_NULL_HANDLE)
        destroySwapchain(old_swapchain);

    unsigned images_count;
    vkGetSwapchainImagesKHR(device, swapchain, &images_count, nullptr);
    swapchain_images.resize(images_count);
    vkGetSwapchainImagesKHR(device, swapchain, &images_count, swapchain_images.data());

    swapchain_image_views.resize(images_count);
    VkComponentMapping component_mapping = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY
    };
    VkImageSubresourceRange subresource_range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };
    for (const auto [i, swapchain_image] : std::views::enumerate(swapchain_images)) {
        VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain_image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = image_format.format,
            .components = component_mapping,
            .subresourceRange = subresource_range
        };
        vkCreateImageView(device, &image_view_create_info, nullptr, &swapchain_image_views[i]);
    }
}

Application::Application(unsigned width, unsigned height, unsigned minWidth, unsigned minHeight,
                         const char *title, bool validationLayers) :
    width(width), height(height), minWidth(minWidth), minHeight(minHeight),
    title(title), requireValidationLayers(validationLayers) {
    createWindow();
    createVulkanInstance();
    createSurface();
    createDevice();
    if (requireValidationLayers)
        printDeviceProperties(physical_device);
    createSwapchain();
}

void Application::destroySwapchain(VkSwapchainKHR swapchain) {
    for (auto &swapchain_image_view : swapchain_image_views)
        vkDestroyImageView(device, swapchain_image_view, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

Application::~Application() {
    destroySwapchain(swapchain);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

void Application::processEvents() {
    while (XPending(display) > 0) {
        XEvent event;
        XNextEvent(display, &event);

        switch (event.type) {
            case KeyPress:
                if (event.xkey.keycode == XKeysymToKeycode(display, XK_Escape))
                    is_window_visible = false;
                break;
            case ClientMessage:
                // WM_DELETE_WINDOW might be None
                if (WM_DELETE_WINDOW != None && static_cast<Atom>(event.xclient.data.l[0]) == WM_DELETE_WINDOW)
                    is_window_visible = false;
                break;
        }
    }
}