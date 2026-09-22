#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include <vector>
#include <X11/Xlib.h>
#include <vulkan/vulkan.h>

struct Application {
public:
    Application(unsigned width, unsigned height, unsigned minWidth, unsigned minHeight,
        const char *title, bool validationLayers);
    ~Application();
    void displayWindow();
    [[nodiscard]] bool isWindowVisible() const { return is_window_visible; }
    void processEvents();

private:
    void createWindow();
    void createVulkanInstance();
    void createSurface();
    static void printDeviceProperties(VkPhysicalDevice physical_device);
    void createDevice();
    void createSwapchain();

    void destroySwapchain(VkSwapchainKHR);

    // config and settings
    unsigned width = 0, height = 0;
    unsigned minWidth = 0, minHeight = 0;
    const char *title = nullptr;
    bool requireValidationLayers = true;
    bool is_window_visible = false;

    // window data members
    Display *display = nullptr;
    Window window;
    Atom WM_DELETE_WINDOW = None;

    // vulkan data members
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue queue;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
};



#endif //VULKAN_APPLICATION_H
