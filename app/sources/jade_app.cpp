#include "jade_app.hpp"

#include <string>

#include <fmt/core.h>

#include <jade_engine.hpp>
#include <jade_utility.hpp>

int main() {
    
    

    ecs::utilitiy::Pool<uint32_t,std::string> intPool{};

    intPool.add(1,"RigidBody");
    intPool.add(2,"Health");

    for (auto& elem : intPool.getElements()) {
        fmt::println("Elem : {}", elem);
    }

    intPool.remove(2);

    for (auto& elem : intPool.getElements()) {
        fmt::println("Elem : {}", elem);
    }
    
    /*
    jade::EngineCreateInfo createInfo{};
    createInfo.windowTitle = "Jade Engine";
    createInfo.windowWidth = 640;
    createInfo.windowHeight = 480;

    if (jade::initEngine(createInfo) ){

        jade::runEngine();

        jade::cleanupEngine();

        return 0;
    }
    else {
        return -1;
    }


    */
  
}
