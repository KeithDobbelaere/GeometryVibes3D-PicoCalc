#include "app/App.hpp"
#include "platform/Platform.hpp"

namespace gv { IPlatform* createPlatform(); }

int main() {
    gv::App app;
    return app.run(*gv::createPlatform()); // keep your current 800-frame test
}