#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <print>

struct WindowProperties {
    unsigned width = 0, height = 0;
    unsigned minWidth = 0, minHeight = 0;
    const char *title = nullptr;
};

int main() {
    constexpr WindowProperties window_properties {
        .width = 1280, .height = 720,
        .minWidth = 360, .minHeight = 640,
        .title = "X11 Window",
    };

    Display *display = XOpenDisplay(nullptr);

    constexpr int attributeMask = CWBackPixel | CWEventMask;
    XSetWindowAttributes windowAttributes = {
        .background_pixel = WhitePixel(display, DefaultScreen(display)),
        .event_mask = KeyPressMask | KeyReleaseMask
        // TODO: look into the usefulness of StructureNotifyMask and ExposureMask
    };
    XSizeHints sizeHint = {
        .flags = PMinSize, // TODO: look into min_(max)aspect
        .min_width = static_cast<int>(window_properties.minWidth),
        .min_height = static_cast<int>(window_properties.minHeight),
    };

    const Window window = XCreateWindow(
        display, DefaultRootWindow(display),
        0, 0, window_properties.width, window_properties.height,
        0, CopyFromParent, CopyFromParent, CopyFromParent,
        attributeMask, &windowAttributes);

    XSetStandardProperties(display, window,
        window_properties.title, nullptr, None, nullptr, 0, &sizeHint);

    Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
    if (!XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1))
        std::println(stderr,"%s", "XSetWMProtocols failed: Couldn't set WM_DELETE_WINDOW property");

    XMapWindow(display, window);

    bool isWindowOpen = true;
    while (isWindowOpen) {
        XEvent generalEvent;
        while (XPending(display) > 0) {
            XNextEvent(display, &generalEvent);

            switch (generalEvent.type) {
                case KeyPress:
                    if (generalEvent.xkey.keycode == XKeysymToKeycode(display, XK_Escape))
                        isWindowOpen = false;
                    break;
                case ClientMessage:
                    if (static_cast<Atom>(generalEvent.xclient.data.l[0]) == WM_DELETE_WINDOW)
                        isWindowOpen = false;
                    break;
            }
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
}
