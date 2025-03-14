#pragma once
#include <unordered_map>
#include <vector>
#include <queue>
#include <set>


#include <fmt/core.h>


namespace ecs::utilitiy {

	class PoolType {
	protected:
	public:
		virtual ~PoolType() = default;
	};

	
	template<typename TKey, typename Tvalue>
	class Pool {
	private:
		std::vector<Tvalue> m_elements{};
		std::queue<size_t> m_dirtyElements{};
		std::unordered_map<TKey, size_t> m_keyValueMap{};

	public:
		inline bool add(TKey t_key, Tvalue t_value) { // components should never be big that's why I just copy, store only handles of big objects inside the component

			if ( m_keyValueMap.find(t_key) != m_keyValueMap.end() ) {
				fmt::println("Key already has a value of same type");
				return false;
			}
			size_t valueIndex;

			if (!m_dirtyElements.empty()) {
				valueIndex = m_dirtyElements.front();
				m_dirtyElements.pop();
				m_elements[valueIndex] = t_value;
			}
			else {
				m_elements.push_back(t_value);
				valueIndex = m_elements.size();
			}

			m_keyValueMap.insert({ t_key , valueIndex });

			return true;

		}
		inline bool remove(TKey t_key) {
			if ( m_keyValueMap.find(t_key) != m_keyValueMap.end() ) {
				fmt::println("Key does not have a value of same type");
				return false;
			}
			size_t valueIndex = m_keyValueMap[t_key];
			m_keyValueMap.erase(t_key);
			//marking componentIndex dirty so I can use it 
			m_dirtyElements.push(valueIndex);
			return true;
		}

		inline const std::vector<Tvalue> getElements() const { return m_elements; }
	
	};

}