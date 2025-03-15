#include "jade_app.hpp"

#include <string>
#include <vector>
#include <format>

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
		auto& parentNode = m_nodeComponentPoolRef.getComponentRW(t_parent);
        auto& childNode = m_nodeComponentPoolRef.getComponentRW(t_child);

        if (parentNode.parentEntity == t_child) {
            fmt::println("Cannot add parent as child!");
            return; 
        }

        removeChild(t_child);// if child has a parent this detaches it, this also ensures we dont add twice

		if (parentNode.lastChild != ecs::InvalidEntity) {
			
            auto& lastChildNode = m_nodeComponentPoolRef.getComponentRW(parentNode.lastChild);
            lastChildNode.rightSibling = t_child;
            childNode.leftSibling = parentNode.lastChild;
            childNode.parentEntity = t_parent;
            parentNode.lastChild = t_child;
        }
        else {
            parentNode.lastChild = t_child;
            childNode.parentEntity = t_parent;
        }    
    }
    void removeChild( ecs::JadeEntity t_child) {
      
        auto& childNode = m_nodeComponentPoolRef.getComponentRW(t_child);

        if (childNode.parentEntity != ecs::InvalidEntity) {

            auto& oldParentNode = m_nodeComponentPoolRef.getComponentRW(childNode.parentEntity);

            if (childNode.leftSibling != ecs::InvalidEntity && childNode.rightSibling != ecs::InvalidEntity) {
                auto& leftSiblingNode = m_nodeComponentPoolRef.getComponentRW(childNode.leftSibling);
                auto& rightSiblingNode = m_nodeComponentPoolRef.getComponentRW(childNode.rightSibling);
                leftSiblingNode.rightSibling = childNode.rightSibling;
                rightSiblingNode.leftSibling = childNode.leftSibling;

            }
            else if (childNode.leftSibling == ecs::InvalidEntity && childNode.rightSibling != ecs::InvalidEntity) {

                auto& rightSiblingNode = m_nodeComponentPoolRef.getComponentRW(childNode.rightSibling);
                rightSiblingNode.leftSibling = ecs::InvalidEntity;

            }
            else if (childNode.leftSibling != ecs::InvalidEntity && childNode.rightSibling == ecs::InvalidEntity) {
                oldParentNode.lastChild = childNode.leftSibling;
                auto& leftSiblingNode = m_nodeComponentPoolRef.getComponentRW(childNode.leftSibling);
                leftSiblingNode.rightSibling = ecs::InvalidEntity;

            }
            else {
                oldParentNode.lastChild = ecs::InvalidEntity;
            }
            childNode.leftSibling = ecs::InvalidEntity;
            childNode.rightSibling = ecs::InvalidEntity;
            childNode.parentEntity = ecs::InvalidEntity;
        }
        
    }
    // caller should make sure that t_parent has node component
    std::vector<ecs::JadeEntity> getChildren(ecs::JadeEntity t_parent) {

        auto& parentNode = m_nodeComponentPoolRef.getComponentR(t_parent);
      
        std::vector<ecs::JadeEntity> children{};

        ecs::JadeEntity lastChild = parentNode.lastChild;

        while (lastChild != ecs::InvalidEntity) {
            children.push_back(lastChild);
            lastChild = m_nodeComponentPoolRef.getComponentR(lastChild).leftSibling;
        }
        return children;
       
        
    }

    void printTree(ecs::JadeEntity t_root) {
        
        std::string name{"Empty"};
        if (m_nameComponentPoolRef.doEntityExists(t_root)) {

            auto rootName = m_nameComponentPoolRef.getComponentR(t_root);
            name = rootName.name;

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

//
//auto& nameComponentPool = componentPoolManager.getComponentPool<NameComponent>();
//auto& nodeComponentPool = componentPoolManager.getComponentPool<NodeComponent>();
//
//TransformSystem transformSystem(nodeComponentPool, nameComponentPool);
//
//auto rootEntity = entityManager.createEntity();
//auto childEntity = entityManager.createEntity();
//
//nodeComponentPool.addPair(rootEntity, {});
//nodeComponentPool.addPair(childEntity, {});
//
//transformSystem.addChild(rootEntity, childEntity);
//auto& rootNode = nodeComponentPool.getComponentR(rootEntity);
//for (auto i = 2; i < UINT32_MAX; i++) {
//    auto entity = entityManager.createEntity();
//    nodeComponentPool.addPair(entity, {});
//    transformSystem.addChild(rootNode.lastChild, entity);
//}
//
//transformSystem.printTree(rootEntity);
    struct A {
        int x{0};
    };
    std::vector< A> intArray = { {.x = 1}, {.x = 2}, {.x = 3}, };
    ecs::utilitiy::ElementHandle handle ( intArray ,0);
    
    
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
