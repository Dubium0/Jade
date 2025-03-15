#pragma once 

#include <vector>
#include <queue>
#include <unordered_map>
#include <array>
#include <set>
#include <memory>

#include <fmt/core.h>

#include "jade_entity.hpp"
#include "jade_utility.hpp"



namespace ecs {
	
	using ComponentIdType = uint16_t;
	inline ComponentIdType componentTypeIdSequence  = 0;
	template< typename T > inline const ComponentIdType componentTypeId = componentTypeIdSequence++;
	

	using ComponentPoolType = utilitiy::PoolType;

	
	template<typename T>
	class ComponentPool :public ComponentPoolType {
	private:
		utilitiy::Pool<JadeEntity, T> m_componentPool{};
	public:
		inline bool addPair(JadeEntity t_entity, T t_component) { return m_componentPool.add(t_entity, t_component); }
		inline bool removePair(JadeEntity t_entity) { return m_componentPool.remove(t_entity); }
		inline const std::vector<T>& getComponents() const { return m_componentPool.getElements(); }

		inline utilitiy::ElementHandle<T> getComponentHandle(JadeEntity t_entity) { return m_componentPool.getElementHandle(t_entity); }
		inline const JadeEntity getEntityOfComponent(size_t t_componentIndex) const { return m_componentPool.getKey(t_componentIndex); }

		inline bool doComponentExists(size_t t_componentIndex) const { return m_componentPool.doValueExists(t_componentIndex); }
		inline bool doEntityExists(JadeEntity t_entity) const { return m_componentPool.doKeyExists(t_entity); }

	};

	 
	class ComponentPoolManager {

	private:
	
		std::unordered_map<ComponentIdType,std::unique_ptr<ComponentPoolType>> m_componentPools{};
	public:

		template <typename T>
		ComponentPool<T>& getComponentPool() {
			auto typeIndex = componentTypeId<T>;
			//lazy init
			{

				auto it = m_componentPools.find(typeIndex);
				if (it == m_componentPools.end()) {
				
					auto pool = std::make_unique<ComponentPool<T>>();
				

					m_componentPools[typeIndex] = std::move(pool);

				}

			}

			return *static_cast<ComponentPool<T>*>(m_componentPools[typeIndex].get());
		}

	};


}





