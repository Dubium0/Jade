#pragma once

#include <queue>
#include <fmt/core.h>

namespace ecs {

	using JadeEntity = uint32_t;

	const JadeEntity InvalidEntity = 0;

	// creating new entity == giving next available handle
	// control deletion and creation of an entity
	class EntityManager {
		private:
			JadeEntity m_tail{0}; // 0 is reserved for invalid entities
			uint32_t m_currentEntityCount{ 0 };
			std::queue<JadeEntity> m_availableQueue{};

		public:
			inline JadeEntity createEntity() {

				if (!m_availableQueue.empty()) {
					auto handle = m_availableQueue.front();
					m_availableQueue.pop();
					m_currentEntityCount++;
					return handle;
				}
				else {
					if (m_tail < UINT32_MAX) {
						m_tail++;
						m_currentEntityCount++;
						return m_tail;
					}
					else {
						fmt::println("Maximum number of entity allocated! ( Damn bro thats a lot try to change into 64 bit handles");
						return InvalidEntity;
					}

				}

			}
			inline bool deleteEntity(JadeEntity t_entityHandle) {

				//TODO: CLEAR COMPONENT REFERENCES

				if (t_entityHandle == m_tail) {
					m_tail--;
					
				}
				else {
					m_availableQueue.push(t_entityHandle);
					
				}
				m_currentEntityCount--;
				return true;

			}

	};



}

