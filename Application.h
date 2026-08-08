#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include <X11/Xlib.h>

struct Application {
    unsigned width = 0, height = 0;
    unsigned minWidth = 0, minHeight = 0;
    const char *title = nullptr;
    bool isWindowOpen = false;

    Display *display = nullptr;
    Window window;
    Atom WM_DELETE_WINDOW;

    Application(unsigned width, unsigned height, unsigned minWidth, unsigned minHeight, const char *title);
    ~Application();
    void processEvents();
};



#endif //VULKAN_APPLICATION_H
