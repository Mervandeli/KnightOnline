#pragma once

#include <Ebenezer/EbenezerApp.h>
#include <Ebenezer/EbenezerLogger.h>
#include <Ebenezer/Map.h>
#include <Ebenezer/Npc.h>

#include "TestMap.h"
#include "TestUser.h"

class TestApp : public Ebenezer::EbenezerApp
{
public:
	TestApp() : EbenezerApp(_logger)
	{
		spdlog::set_level(spdlog::level::off);

		InitSocketManager(Ebenezer::MAX_USER, 1);
	}

	void InitSocketManager(int serverSocketCount, int clientSocketCount)
	{
		_socketManager.InitSockets(serverSocketCount, clientSocketCount);

		for (int i = 0; i < serverSocketCount; i++)
			_socketManager._inactiveServerSocketArray[i] = new TestUser();
	}

	TestUser* AddUser()
	{
		TestUser* user = nullptr;
		int socketId   = -1;

		{
			std::lock_guard<std::recursive_mutex> lock(_socketManager._mutex);

			user = static_cast<TestUser*>(_socketManager.AcquireServerSocket(socketId));
			if (user == nullptr)
				return nullptr;
		}

		user->Initialize();

		return user;
	}

	TestMap* CreateMap(uint8_t zoneId, int mapSize = TestMap::DEFAULT_MAP_SIZE)
	{
		auto map = new TestMap(zoneId, mapSize);
		if (map == nullptr)
			return nullptr;

		m_ZoneArray.push_back(map);
		return map;
	}

	Ebenezer::CNpc* CreateNPC(uint16_t npcId)
	{
		auto npc = new Ebenezer::CNpc();
		if (npc == nullptr)
			return nullptr;

		npc->m_sNid = npcId;

		if (!m_NpcMap.PutData(npcId, npc))
		{
			delete npc;
			return nullptr;
		}

		return npc;
	}

	bool AddItemEntry(const Ebenezer::model::Item& itemModel)
	{
		auto modelForInsertion = new Ebenezer::model::Item { itemModel };
		if (modelForInsertion == nullptr)
			return false;

		if (!m_ItemTableMap.PutData(modelForInsertion->ID, modelForInsertion))
		{
			delete modelForInsertion;
			return false;
		}

		return true;
	}

	bool AddItemUpgradeEntry(const Ebenezer::model::ItemUpgrade& itemUpgradeModel)
	{
		auto modelForInsertion = new Ebenezer::model::ItemUpgrade { itemUpgradeModel };
		if (modelForInsertion == nullptr)
			return false;

		if (!m_ItemUpgradeTableMap.PutData(modelForInsertion->Index, modelForInsertion))
		{
			delete modelForInsertion;
			return false;
		}

		return true;
	}

protected:
	Ebenezer::EbenezerLogger _logger;
};
