#pragma once

#include <vector>
#include <queue>
#include <unordered_map>

#include <fmt/core.h>
namespace ecs {

	// there can be different entity pools
	// I dont think introducing an entity type makes sense
	// the thing that makes entities distinct things are the components they have
	// so having a monster entity type and human entity type can be look good practice for better readibiity, but instead  we can have a comoponent called Monster and Human 
	// then those components can be attached to generic entity type.
	// So that a lot of common operations such as transforming and drawing, can be done in one contigius array
	// So there will be only 1 entity pool
	// If you want to create groups or other stuff use component
	
	using JadeEntity = uint32_t;

	constexpr JadeEntity InvalidEntity = 0;


	
	// creating new entity == giving next available handle
	// control deletion and creation of an entity
	// our entity index is uint32_t but only the 24 bit used to keep ids other 8 bytes for group id 
	// so create entity function will get a optional parameter called group Id , as you can image the maximum value for distinct entity groups is 255 ( I dunno why woulld anyone need more)
	// a const component should now which entity group it assigned it to
	// so the constructor of const component should require entity group id as uint32_t ( only the first 8 byte will be used but for bit operations keeping at uint32 is bettter)
	// dynamic component does not require such a thing since it can be assigned to any entity group it will have internal mapping
	// whenever we ask for entities to const component, const component pool or the the index with group id and returns it
	constexpr uint32_t Max24BitNumber = 16777215;
	
	using EntityGroupIdType = uint32_t;

	inline EntityGroupIdType EntityGroupIdSequence = 0;

	template<typename T>
	inline const EntityGroupIdType EntityGroupId = EntityGroupIdSequence++;

	constexpr uint32_t EntityGroupMask =   0b11111111000000000000000000000000;

	constexpr uint32_t EntityLocalIDMask = 0b00000000111111111111111111111111;

	//inside everything is local but to outside it gives world id etc.
	

	template<typename T>
	class EntityGroup {	
	public:	
		EntityGroup(T t_groupHeader) : m_groupHeader(t_groupHeader){}
	private:
		JadeEntity m_tail{ 0 }; // local
		uint32_t m_currentEntityCount{ 0 };//local
		std::queue<JadeEntity> m_availableQueue{};//local
		T m_groupHeader;
	public:
		inline static const EntityGroupIdType getGroupId() { return EntityGroupId<T>; }
		inline const T& getHeader() const { return m_groupHeader; }

		inline static JadeEntity getLocalId(JadeEntity t_worldId){
			if (((EntityGroupMask & t_worldId) >> 24) != EntityGroupId<T>) {
				return InvalidEntity; // this entity does not belong to this entity group
			}
			else {
				return t_worldId & EntityLocalIDMask;
			}
		}
		//this does not have to be valid id
		inline static JadeEntity convertToWorldId(JadeEntity t_localId) {return t_localId | (EntityGroupId<T> >> 24); }


		inline JadeEntity createEntity() { // returns worldId, If you think you will need it store it or store the groupId and convert into worldID

			if (!m_availableQueue.empty()) {
				auto handle = m_availableQueue.front();
				m_availableQueue.pop();
				m_currentEntityCount++;
				handle |= (EntityGroupId<T> << 24); // attach the group ID when returning
				return handle;
			}
			else {
				if (m_tail < Max24BitNumber) {
					m_tail++;
					m_currentEntityCount++;
					auto shifted = EntityGroupId<T> << 24;
					return (m_tail | shifted );
				}
				else {
					fmt::println("Maximum number of entity allocated! ( Damn bro thats a lot try to change into 64 bit handles");
					return InvalidEntity;
				}
			}

		}

		inline bool deleteEntity(JadeEntity t_worldID) { //give this world id 

			//TODO: CLEAR COMPONENT REFERENCES

			if (((EntityGroupMask & t_worldID) >> 24) != EntityGroupId<T>) {
				return false; // this entity does not belong to this entity group
			}

			t_worldID &= EntityLocalIDMask; // removes group ID
			if (t_worldID == m_tail) {
				m_tail--;

			}
			else {
				m_availableQueue.push(t_worldID);

			}
			m_currentEntityCount--;
			return true;

		}

	};

}

