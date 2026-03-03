#include "app/App.hpp"
#include "platform/IPlatform.hpp"

namespace gv { IPlatform* createPlatform(); }

int main() {
    gv::App app;
    return app.run(*gv::createPlatform());
}