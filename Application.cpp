#include "Application.h"

#include <X11/Xutil.h>
#include <print>
#include <span>
#include <ranges>
#include <vector>
#include <cstring>

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
        std::println(stderr,"XSetWMProtocols failed: Couldn't set WM_DELETE_WINDOW property");
}

void Application::displayWindow() {
    XMapWindow(display, window);
    isWindowVisible = true;
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
    if (auto i = isExtensionSupported(instanceExtensions); i != instanceExtensions.size()) {
        std::println(stderr, "Instance Extension {} is not supported", instanceExtensions[i]);
        std::exit(1);
    }

    std::vector<const char *> validationLayers;
    if (requireValidationLayers) {
        validationLayers = {
            "VK_LAYER_KHRONOS_validation",
        };
        if (auto i = isLayerSupported(validationLayers); i != validationLayers.size()) {
            std::println(stderr, "Validation Layer {} is not supported", validationLayers[i]);
            std::exit(1);
        }
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

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        std::println(stderr, "vkCreateInstance failed: Couldn't create Vulkan instance");
        std::exit(1);
    }
    // auto messenger = setupDebugMessenger(instance);
}

void Application::createSurface() {
    const VkXlibSurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = display,
        .window = window
    };
    if (vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
        std::println(stderr, "vkCreateXlibSurfaceKHR failed: Couldn't create Vulkan Presentation Surface");
        std::exit(1);
    }
}

Application::Application(unsigned width, unsigned height, unsigned minWidth, unsigned minHeight,
                         const char *title, bool validationLayers) :
    width(width), height(height), minWidth(minWidth), minHeight(minHeight),
    title(title), requireValidationLayers(validationLayers) {
    createWindow();
    createVulkanInstance();
    createSurface();
}

Application::~Application() {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

void Application::processEvents() {
    XEvent event;
    while (XPending(display) > 0) {
        XNextEvent(display, &event);

        switch (event.type) {
            case KeyPress:
                if (event.xkey.keycode == XKeysymToKeycode(display, XK_Escape))
                    isWindowVisible = false;
                break;
            case ClientMessage:
                if (static_cast<Atom>(event.xclient.data.l[0]) == WM_DELETE_WINDOW)
                    isWindowVisible = false;
                break;
        }
    }
}