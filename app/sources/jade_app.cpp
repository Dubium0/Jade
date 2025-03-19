#include "jade_app.hpp"

#include <string>
#include <vector>
#include <format>
#include <chrono>
#include <fmt/core.h>

#include <jade_engine.hpp>
#include <jade_component.hpp>

struct NodeComponent {
    ecs::JadeEntity parentEntity{ 0 };
	ecs::JadeEntity lastChild{ 0 };
	ecs::JadeEntity leftSibling{ 0 };
	ecs::JadeEntity rightSibling{ 0 };
};

struct NameComponent {
    std::string name{};
};

struct TransformSystem {


    
	ecs::ComponentPool<NodeComponent>& m_nodeComponentPoolRef;
    ecs::ComponentPool<NameComponent>& m_nameComponentPoolRef;

    TransformSystem(ecs::ComponentPool<NodeComponent>& t_nodeComponentPoolRef, ecs::ComponentPool<NameComponent>& t_nameComponentPoolRef)
        : m_nodeComponentPoolRef(t_nodeComponentPoolRef), m_nameComponentPoolRef(t_nameComponentPoolRef){ }

    void addChild(ecs::JadeEntity t_parent, ecs::JadeEntity t_child) {
        if (t_parent == t_child) {
            fmt::println("Cannot add child to itself!");
            return;
        }
		auto parentHandle = m_nodeComponentPoolRef.getComponentHandle(t_parent);
        auto childHandle = m_nodeComponentPoolRef.getComponentHandle(t_child);

        if (parentHandle.getR().parentEntity == t_child) {
            fmt::println("Cannot add parent as child!");
            return; 
        }

        removeChild(t_child);// if child has a parent this detaches it, this also ensures we dont add twice

		if (parentHandle.getR().lastChild != ecs::InvalidEntity) {
			
            auto lastChildHandle = m_nodeComponentPoolRef.getComponentHandle(parentHandle.getR().lastChild);
            lastChildHandle.getRW().rightSibling = t_child;
            childHandle.getRW().leftSibling = parentHandle.getR().lastChild;
            childHandle.getRW().parentEntity = t_parent;
            parentHandle.getRW().lastChild = t_child;
        }
        else {
            parentHandle.getRW().lastChild = t_child;
            childHandle.getRW().parentEntity = t_parent;
        }    
    }
    void removeChild( ecs::JadeEntity t_child) {
      
        auto childHandle = m_nodeComponentPoolRef.getComponentHandle(t_child);

        if (childHandle.getR().parentEntity != ecs::InvalidEntity) {

            auto oldParentHandle = m_nodeComponentPoolRef.getComponentHandle(childHandle.getR().parentEntity);

            if (childHandle.getR().leftSibling != ecs::InvalidEntity && childHandle.getR().rightSibling != ecs::InvalidEntity) {
                auto leftSiblingHandle = m_nodeComponentPoolRef.getComponentHandle(childHandle.getR().leftSibling);
                auto rightSiblingHandle = m_nodeComponentPoolRef.getComponentHandle(childHandle.getR().rightSibling);
                leftSiblingHandle.getRW().rightSibling = childHandle.getR().rightSibling;
                rightSiblingHandle.getRW().leftSibling = childHandle.getR().leftSibling;

            }
            else if (childHandle.getR().leftSibling == ecs::InvalidEntity && childHandle.getR().rightSibling != ecs::InvalidEntity) {

                auto rightSiblingHandle= m_nodeComponentPoolRef.getComponentHandle(childHandle.getR().rightSibling);
                rightSiblingHandle.getRW().leftSibling = ecs::InvalidEntity;

            }
            else if (childHandle.getR().leftSibling != ecs::InvalidEntity && childHandle.getR().rightSibling == ecs::InvalidEntity) {
                oldParentHandle.getRW().lastChild = childHandle.getR().leftSibling;
                auto leftSiblingNode = m_nodeComponentPoolRef.getComponentHandle(childHandle.getR().leftSibling);
                leftSiblingNode.getRW().rightSibling = ecs::InvalidEntity;

            }
            else {
                oldParentHandle.getRW().lastChild = ecs::InvalidEntity;
            }
            childHandle.getRW().leftSibling = ecs::InvalidEntity;
            childHandle.getRW().rightSibling = ecs::InvalidEntity;
            childHandle.getRW().parentEntity = ecs::InvalidEntity;
            
        }
        
    }
    // caller should make sure that t_parent has node component
    std::vector<ecs::JadeEntity> getChildren(ecs::JadeEntity t_parent) {

        auto parentHandle = m_nodeComponentPoolRef.getComponentHandle(t_parent);
      
        std::vector<ecs::JadeEntity> children{};

        ecs::JadeEntity lastChild = parentHandle.getR().lastChild;

        while (lastChild != ecs::InvalidEntity) {
            children.push_back(lastChild);
            lastChild = m_nodeComponentPoolRef.getComponentHandle(lastChild).getR().leftSibling;
        }
        return children;
       
        
    }

    void printTree(ecs::JadeEntity t_root) {
        
        std::string name{"Empty"};
        if (m_nameComponentPoolRef.doEntityExists(t_root)) {

            auto rootHandle = m_nameComponentPoolRef.getComponentHandle(t_root);
            name = rootHandle.getR().name;

        }
        fmt::println("Entity : {} has name {}", t_root, name);

        auto childrenOfRoot = getChildren(t_root);
        // recursive code is bad :<
        for (auto children : childrenOfRoot) {
            printTree(children);
        }
        

    }
    
    

};



int main() {
    
	ecs::ComponentPoolManager componentPoolManager{};

	ecs::EntityManager entityManager{};

    
    auto& nameComponentPool = componentPoolManager.getComponentPool<NameComponent>();
    auto& nodeComponentPool = componentPoolManager.getComponentPool<NodeComponent>();
    
    TransformSystem transformSystem(nodeComponentPool, nameComponentPool);
    
    auto rootEntity = entityManager.createEntity();
    auto childEntity = entityManager.createEntity();
    
    nodeComponentPool.addPair(rootEntity, {});
    nodeComponentPool.addPair(childEntity, {});
    
    transformSystem.addChild(rootEntity, childEntity);
    auto rootHandle = nodeComponentPool.getComponentHandle(rootEntity);

    auto startCreateAndAdd = std::chrono::high_resolution_clock::now();
    for (auto i = 2; i < 1000000; i++) {
        auto entity = entityManager.createEntity();
        nodeComponentPool.addPair(entity, {});
        transformSystem.addChild(rootHandle.getR().lastChild, entity);
    }
    
    auto endCreateAndAdd = std::chrono::high_resolution_clock::now();

    uint32_t counter = 0;
    auto startPrint = std::chrono::high_resolution_clock::now();
    //transformSystem.printTree(rootEntity);
    for (const auto& elem : nodeComponentPool.getComponents()) {
        counter = elem.lastChild * elem.leftSibling * elem.parentEntity * elem.rightSibling;
    }

    auto endPrint = std::chrono::high_resolution_clock::now();



    auto elaspedCreateAndAdd = std::chrono::duration_cast<std::chrono::milliseconds>(endCreateAndAdd - startCreateAndAdd).count();
    auto elaspedPrint = std::chrono::duration_cast<std::chrono::milliseconds>(endPrint - startPrint).count();

    fmt::println("Elapsed create and add time {} in ms", elaspedCreateAndAdd);

    
    fmt::println("Elapsed print time {} in ms", elaspedPrint);

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
