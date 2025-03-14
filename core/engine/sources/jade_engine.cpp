#include <iostream>

#include "jade_engine.hpp"
#include "vk_engine.hpp"


void sayHelloFromEngine() {
    std::cout << "Hello from Engine DLL!" << std::endl;
}

const char* getEngineMessage() {
    return "Greetings from the Engine!";
}



bool jade::initEngine(EngineCreateInfo createInfo){

    std::cout << "Hello from init engine DLL! title: " << createInfo.windowTitle << 
        " height: " << createInfo.windowHeight << " width: " << createInfo.windowWidth << std::endl;
    return VulkanEngine::getInstance()->init(createInfo);    
}
	
void jade::runEngine(){
    std::cout << "Hello from run engine DLL!" << std::endl;
    VulkanEngine::getInstance()->run();  
}

void jade::cleanupEngine(){
    std::cout << "Hello from cleanup engine DLL!" << std::endl;
    VulkanEngine::getInstance()->cleanup(); 
}