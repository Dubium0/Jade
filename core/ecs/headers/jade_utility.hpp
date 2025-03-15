#pragma once
#include <unordered_map>
#include <vector>
#include <queue>
#include <set>
#include <exception>
#include <optional>
#include <tuple>
#include <string_view>
#include <type_traits>

#include <fmt/core.h>


namespace ecs::utilitiy {


	class PoolType {
	public:
		virtual ~PoolType() = default;
	};
	// a dyamic pool cannot guarantee the returned references since it may reallocate the whole array on resize
	// so we can only use an handle modify the value



	template<typename T>
	class ElementHandle {
	private:
		std::vector<T>& m_arrayRef;
		size_t m_elemIndex;
		
	public:
		ElementHandle(std::vector<T>& t_arrayRef, size_t t_elemIndex) :m_arrayRef(t_arrayRef),m_elemIndex(t_elemIndex){}
		
		T& getRW() { return  m_arrayRef[m_elemIndex]; }
		const T& getR() const{ return  m_arrayRef[m_elemIndex]; }

		operator T& () { return m_arrayRef[m_elemIndex];}

	};


	template<typename TKey, typename TValue>
	class Pool {
	private:

		std::vector<TValue> m_elements{};
		std::queue<size_t> m_dirtyElements{};
		std::unordered_map<TKey, size_t> m_keyValueMap{};
		std::unordered_map<size_t, TKey > m_valueKeyMap{};


	public:
		
		inline bool add(TKey t_key, TValue t_value) { // components should never be big that's why I just copy, store only handles of big objects inside the component
			auto it = m_keyValueMap.find(t_key);
			if ( it != m_keyValueMap.end() ) {
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
				valueIndex = m_elements.size()-1;
			}

			m_keyValueMap.insert({ t_key , valueIndex });
			m_valueKeyMap.insert({ valueIndex , t_key });
			return true;

		}
		
		inline bool remove(TKey t_key) {
			auto it = m_keyValueMap.find(t_key);
			if ( it == m_keyValueMap.end() ) {
				fmt::println("Key does not have a value of same type");
				return false;
			}
			size_t valueIndex = it->second;
			m_keyValueMap.erase(t_key);
			m_valueKeyMap.erase(valueIndex);
			//marking componentIndex dirty so I can use it 
			m_dirtyElements.push(valueIndex);
			return true;
		}

		inline const std::vector<TValue>& getElements() const { return m_elements; }

		inline TKey getKey(size_t t_value ) const {
			auto it = m_valueKeyMap.find(t_value);
			if (it == m_valueKeyMap.end()) {
				throw std::exception("The value is not present in valueKey map");
			}
			return it->second;
			
		}

		inline ElementHandle<TValue> getElementHandle(TKey t_key) {
			auto it = m_keyValueMap.find(t_key);
			if (it == m_keyValueMap.end()) {
				throw std::exception("The key is not present in keyValue map");
			}
			return ElementHandle(m_elements, it->second);
		}
		
		inline bool doKeyExists(TKey t_key) const{
			return m_keyValueMap.find(t_key) != m_keyValueMap.end();
		}
		inline bool doValueExists(size_t t_value) const {
			return m_valueKeyMap.find(t_value) != m_valueKeyMap.end();
		}
	
	};


	// Base case: depth 0 is just the type T
	//template<typename T, std::size_t Depth>
	//struct NestedVector {
	//	using type = std::vector<typename NestedVector<T, Depth - 1>::type>;
	//};

	//// Specialization for depth 1
	//template<typename T>
	//struct NestedVector<T, 1> {
	//	using type = std::vector<T>;
	//};
	//
	//template<typename T>
	//struct NestedVector<T, 0> {
	//	using type = T;
	//};
	//
	//template<typename T, std::size_t Depth>
	//using NestedVector_t = typename NestedVector<T, Depth>::type;
	//
	//template<typename T, std::size_t Depth>
	//using NestedTypeVectorPair_t = typename NestedVector<std::pair<T,std::vector<T>>, Depth>::type;

}

