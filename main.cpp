#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdio>

int main() {
    Display *display = XOpenDisplay(nullptr);

    constexpr unsigned int windowWidth = 800, windowHeight = 600;
    const char *windowTitle = "X11 Window";

    constexpr int attributeMask = CWBackPixel | CWEventMask;
    XSetWindowAttributes windowAttributes = {
        .background_pixel = 0x00aade87,
        .event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask
    };

    Window window = XCreateWindow(
        display, DefaultRootWindow(display),
        0, 0, windowWidth, windowHeight,
        0, CopyFromParent, CopyFromParent, CopyFromParent,
        attributeMask, &windowAttributes);

    XMapWindow(display, window);
    XStoreName(display, window, windowTitle);

    Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
    if (!XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1)) {
        std::printf("XSetWMProtocols failed: Couldn't set WM_DELETE_WINDOW property\n");
        // std::exit(1);
    }
    bool isWindowOpen = true;
    while (isWindowOpen) {
        XEvent generalEvent;
        XNextEvent(display, &generalEvent);

        switch (generalEvent.type) {
            case Expose:
                std::printf("expose\n");
                break;
            case KeyPress: {
                std::printf("key press\n");
                auto *keyEvent = reinterpret_cast<XKeyPressedEvent *>(&generalEvent);
                if (keyEvent->keycode == XKeysymToKeycode(display, XK_Escape))
                    isWindowOpen = false;
                break;
            }
            case ClientMessage: {
                auto *event = reinterpret_cast<XClientMessageEvent *>(&generalEvent);
                std::printf("client message\n");
                if (static_cast<Atom>(event->data.l[0]) == WM_DELETE_WINDOW)
                    isWindowOpen = false;
                break;
            }
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
}
