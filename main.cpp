#include "Application.h"

int main() {
    Application app (1280, 720, 360, 640, "X11 Window", true);
    app.displayWindow();
    while (app.isWindowVisible()) {
        app.processEvents();
    }
}
