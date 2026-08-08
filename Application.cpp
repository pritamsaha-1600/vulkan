#include "Application.h"

#include <X11/Xutil.h>
#include <print>

Application::Application(unsigned width, unsigned height, unsigned minWidth, unsigned minHeight, const char *title) :
    width(width), height(height), minWidth(minWidth), minHeight(minHeight), title(title) {
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
        std::println(stderr,"%s", "XSetWMProtocols failed: Couldn't set WM_DELETE_WINDOW property");

    XMapWindow(display, window);
    isWindowOpen = true;
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
                    isWindowOpen = false;
                break;
            case ClientMessage:
                if (static_cast<Atom>(event.xclient.data.l[0]) == WM_DELETE_WINDOW)
                    isWindowOpen = false;
                break;
        }
    }
}