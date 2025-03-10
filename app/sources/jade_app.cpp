#include "jade_app.hpp"

#include <jade_engine.hpp>

int main() {
    jade::EngineCreateInfo createInfo{};
    createInfo.windowTitle = "Jade Engine";
    createInfo.windowWidth = 640;
    createInfo.windowHeight = 480;


    jade::initEngine(createInfo);

    jade::runEngine();

    jade::cleanupEngine();

    return 0;

}
