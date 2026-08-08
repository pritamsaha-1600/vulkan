#include <X11/Xlib.h>
#include <unistd.h>

int main() {
    Display *display = XOpenDisplay(nullptr);
    Window rootWindow = DefaultRootWindow(display);
    Window mainWindow = XCreateSimpleWindow(display, rootWindow,
        0, 0, 800, 600, 0, 0, 0x00aade87);
    XMapWindow(display, mainWindow);
    XFlush(display);

    while (true)
        sleep(1);
}
