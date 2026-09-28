#include "core/Application.h"
#include "core/Logger.h"

#include <windows.h>

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    ::aurevix::Logger::instance().init();
    ::aurevix::Application app;
    if (!app.initialize()) {
        AUR_ERROR("Application initialization failed.");
        return 1;
    }
    return app.run();
}