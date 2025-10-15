﻿// AISocket.cpp: implementation of the CAISocket class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AISocket.h"
#include "EbenezerDlg.h"
#include "Define.h"
#include "Npc.h"
#include "User.h"
#include "Map.h"
#include "db_resources.h"

#include <shared/crc32.h>
#include <shared/lzf.h>
#include <shared/packets.h>

#include <spdlog/spdlog.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern CRITICAL_SECTION g_LogFile_critical;

CAISocket::CAISocket(SocketManager* socketManager, int zoneNum)
	: TcpClientSocket(socketManager)
{
	_zoneNum = zoneNum;
}

void CAISocket::Initialize()
{
	_main = (CEbenezerDlg*) AfxGetApp()->GetMainWnd();
	_magicProcess.m_pMain = _main;

	TcpClientSocket::Initialize();
}

bool CAISocket::PullOutCore(char*& data, int& length)
{
	uint8_t*	pTmp;
	int			len;
	bool		foundCore;
	MYSHORT		slen;

	len = _recvCircularBuffer.GetValidCount();

	if (len == 0
		|| len < 0)
		return false;

	pTmp = new uint8_t[len];

	_recvCircularBuffer.GetData((char*) pTmp, len);

	foundCore = false;

	int	sPos = 0, ePos = 0;

	for (int i = 0; i < len && !foundCore; i++)
	{
		if (i + 2 >= len)
			break;

		if (pTmp[i] == PACKET_START1
			&& pTmp[i + 1] == PACKET_START2)
		{
			sPos = i + 2;

			slen.b[0] = pTmp[sPos];
			slen.b[1] = pTmp[sPos + 1];

			length = slen.w;

			if (length < 0)
				goto cancelRoutine;

			if (length > len)
				goto cancelRoutine;

			ePos = sPos + length + 2;

			if ((ePos + 2) > len)
				goto cancelRoutine;
//			ASSERT(ePos+2 <= len);

			if (pTmp[ePos] == PACKET_END1
				&& pTmp[ePos + 1] == PACKET_END2)
			{
				data = new char[length + 1];
				memcpy(data, (pTmp + sPos + 2), length);
				data[length] = 0;
				foundCore = true;
//				int head = _recvCircularBuffer.GetHeadPos(), tail = _recvCircularBuffer.GetTailPos();
//				TRACE("data : %s, len : %d\n", data, length);
//				TRACE("head : %d, tail : %d\n", head, tail );
				break;
			}
			else
			{
				_recvCircularBuffer.HeadIncrease(3);
				break;
			}
		}
	}

	if (foundCore)
		_recvCircularBuffer.HeadIncrease(6 + length); // 6: header 2+ end 2+ length 2

cancelRoutine:
	delete[] pTmp;
	return foundCore;
}

int CAISocket::Send(char* pBuf, int length)
{
	constexpr int PacketHeaderSize = 6;

	_ASSERT(length >= 0);
	_ASSERT((length + PacketHeaderSize) <= MAX_PACKET_SIZE);

	if (length < 0
		|| (length + PacketHeaderSize) > MAX_PACKET_SIZE)
		return -1;

	char sendBuffer[MAX_PACKET_SIZE];
	int index = 0;
	SetByte(sendBuffer, PACKET_START1, index);
	SetByte(sendBuffer, PACKET_START2, index);
	SetShort(sendBuffer, length, index);
	SetString(sendBuffer, pBuf, length, index);
	SetByte(sendBuffer, PACKET_END1, index);
	SetByte(sendBuffer, PACKET_END2, index);
	return QueueAndSend(sendBuffer, index);
}

void CAISocket::Parsing(int len, char* pData)
{
	int index = 0;

	uint8_t command = GetByte(pData, index);

	//TRACE(_T("Parsing - command=%d, length = %d\n"), command, len);

	switch (command)
	{
		case AG_CHECK_ALIVE_REQ:
			RecvCheckAlive(pData + index);
			break;

		case AI_SERVER_CONNECT:
			LoginProcess(pData + index);
			break;

		case AG_SERVER_INFO:
			RecvServerInfo(pData + index);
			break;

		case NPC_INFO_ALL:
//			Sleep(730);
			RecvNpcInfoAll(pData + index);
			break;

		case MOVE_RESULT:
			RecvNpcMoveResult(pData + index);
			break;

		case MOVE_END_RESULT:
			break;

		case AG_ATTACK_RESULT:
			RecvNpcAttack(pData + index);
			break;

		case AG_MAGIC_ATTACK_RESULT:
			RecvMagicAttackResult(pData + index);
			break;

		case AG_NPC_INFO:
			RecvNpcInfo(pData + index);
			break;

		case AG_USER_SET_HP:
			RecvUserHP(pData + index);
			break;

		case AG_USER_EXP:
			RecvUserExp(pData + index);
			break;

		case AG_SYSTEM_MSG:
			RecvSystemMsg(pData + index);
			break;

		case AG_NPC_GIVE_ITEM:
			RecvNpcGiveItem(pData + index);
			break;

		case AG_USER_FAIL:
			RecvUserFail(pData + index);
			break;

		case AG_COMPRESSED_DATA:
			RecvCompressedData(pData + index);
			break;

		case AG_NPC_GATE_DESTORY:
			RecvGateDestroy(pData + index);
			break;

		case AG_DEAD:
			RecvNpcDead(pData + index);
			break;

		case AG_NPC_INOUT:
			RecvNpcInOut(pData + index);
			break;

		case AG_BATTLE_EVENT:
			RecvBattleEvent(pData + index);
			break;

		case AG_NPC_EVENT_ITEM:
			RecvNpcEventItem(pData + index);
			break;

		case AG_NPC_GATE_OPEN:
			RecvGateOpen(pData + index);
			break;
	}
}

void CAISocket::CloseProcess()
{
	spdlog::debug("AISocket::CloseProcess: closing socketID={}", _socketId);
	
	Initialize();

	TcpClientSocket::CloseProcess();
}

// sungyong 2002.05.23
void CAISocket::LoginProcess(char* pBuf)
{
	int index = 0;
	float fReConnectEndTime = 0.0f;
	uint8_t zone = GetByte(pBuf, index);
	// 0: first connect
	// 1: reconnect
	uint8_t byReConnect = GetByte(pBuf, index);
	
	// zone 틀리면 에러 
	if (zone == 0xff)
	{
		AfxMessageBox(_T("AI Server Version Fail!!"));
	}
	// 틀리면 에러 
	else
	{
		std::wstring logstr = std::format(L"AIServer zone connected: {}", zone);
		_main->AddOutputMessage(logstr);
		spdlog::info("AISocket::LoginProcess: AIServer zone={} connected", zone);

		if (byReConnect == 0)
		{
			_main->m_sSocketCount++;
			if (_main->m_sSocketCount == MAX_AI_SOCKET)
			{
				_main->m_bServerCheckFlag = true;
				_main->m_sSocketCount = 0;
				spdlog::debug("AISocket::LoginProcess: all AI sockets connected, sending all user info...");
				_main->SendAllUserInfo();
			}
		}
		else if (byReConnect == 1)
		{
			if (_main->m_sReSocketCount == 0)
				_main->m_fReConnectStart = TimeGet();

			_main->m_sReSocketCount++;

			spdlog::info("AISocket::LoginProcess: reconnect zone={} socketCount={}",
				zone, _main->m_sReSocketCount);

			fReConnectEndTime = TimeGet();
			if (fReConnectEndTime > _main->m_fReConnectStart + 120)
			{
				// all sockets reconnected within 2 minutes
				spdlog::info("AISocket::LoginProcess: sockets reconnected in under 2 minutes [sockets={}]",
					_main->m_sReSocketCount);
				_main->m_sReSocketCount = 0;
				_main->m_fReConnectStart = 0.0f;
			}

			if (_main->m_sReSocketCount == MAX_AI_SOCKET)
			{
				fReConnectEndTime = TimeGet();

				// 1분안에 모든 소켓이 재접됐다면...
				if (fReConnectEndTime < _main->m_fReConnectStart + 60)
				{
					spdlog::info("AISocket::LoginProcess: sockets reconnected in under a minute [sockets={}]",
					_main->m_sReSocketCount);
					_main->m_bServerCheckFlag = true;
					_main->m_sReSocketCount = 0;
					spdlog::debug("AISocket::LoginProcess: sending all user info...");
					_main->SendAllUserInfo();
				}
				// 하나의 떨어진 소켓이라면...
				else
				{
					_main->m_sReSocketCount = 0;
					_main->m_fReConnectStart = 0.0f;
				}
			}
		}
	}
}

void CAISocket::RecvServerInfo(char* pBuf)
{
	int index = 0;
	uint8_t type = GetByte(pBuf, index);
	uint8_t byZone = GetByte(pBuf, index);
	CString logstr;
	int size = static_cast<int>(_main->m_ZoneArray.size());

	if (type == SERVER_INFO_START)
	{
		spdlog::info("AISocket::RecvServerInfo: receiving NPC information for zoneId={}", byZone);
	}
	else if (type == SERVER_INFO_END)
	{
		int16_t sTotalMonster = GetShort(pBuf, index);
		std::wstring logStr = std::format(L"NPC info received for zoneId {}", byZone);
		_main->AddOutputMessage(logStr);
		//Sleep(100);

		_main->m_sZoneCount++;
		
		if (_main->m_sZoneCount == size)
		{
			_main->AddOutputMessage(_T("NPC info received for all zones"));
			if (!_main->m_bFirstServerFlag)
			{
				_main->UserAcceptThread();
				spdlog::info("AISocket::RecvServerInfo: accepting user connections...");
			}

			_main->m_sZoneCount = 0;
			_main->m_bFirstServerFlag = true;
			_main->m_bPointCheckFlag = true;
			// 여기에서 Event Monster의 포인터를 미리 할당 하도록 하장~~
			//InitEventMonster( sTotalMonster );
		}
	}
}

// ai server에 처음 접속시 npc의 모든 정보를 받아온다..
void CAISocket::RecvNpcInfoAll(char* pBuf)
{
	int index = 0;
	uint8_t		byCount = 0;	// 마리수
	uint8_t        byType;			// 0:처음에 등장하지 않는 몬스터, 1:등장
	int16_t		instanceId;			// NPC index
	int16_t		npcId;			// NPC index
	int16_t       sZone;			// Current zone number
	int16_t       sZoneIndex;		// Current zone index
	int16_t		pictureId;			// NPC Picture Number
	int16_t		sSize = 100;	// NPC Size
	int			iweapon_1;
	int			iweapon_2;
	char		npcName[MAX_NPC_NAME_SIZE + 1];
	uint8_t		byGroup;		// 소속 집단
	uint8_t		byLevel;		// level
	float		fPosX;			// X Position
	float		fPosZ;			// Z Position
	float		fPosY;			// Y Position
	uint8_t		byDirection;	// 
	uint8_t		tNpcType;		// 00	: Monster
								// 01	: NPC
	int			iSellingGroup;
	int			nMaxHP;			// 최대 HP
	int			nHP;			// 현재 HP
	uint8_t		byGateOpen;		// 성문일경우 열림과 닫힘 정보
	int16_t		sHitRate;
	uint8_t		byObjectType;	// 보통 : 0, 특수 : 1
	uint8_t		byTrapNumber;

	byCount = GetByte(pBuf, index);

	for (int i = 0; i < byCount; i++)
	{
		byType = GetByte(pBuf, index);
		instanceId = GetShort(pBuf, index);
		npcId = GetShort(pBuf, index);
		pictureId = GetShort(pBuf, index);
		sSize = GetShort(pBuf, index);
		iweapon_1 = GetDWORD(pBuf, index);
		iweapon_2 = GetDWORD(pBuf, index);
		sZone = GetShort(pBuf, index);
		sZoneIndex = GetShort(pBuf, index);
		int nLength = GetVarString(npcName, pBuf, sizeof(uint8_t), index);
		byGroup = GetByte(pBuf, index);
		byLevel = GetByte(pBuf, index);
		fPosX = Getfloat(pBuf, index);
		fPosZ = Getfloat(pBuf, index);
		fPosY = Getfloat(pBuf, index);
		byDirection = GetByte(pBuf, index);
		tNpcType = GetByte(pBuf, index);
		iSellingGroup = GetDWORD(pBuf, index);
		nMaxHP = GetDWORD(pBuf, index);
		nHP = GetDWORD(pBuf, index);
		byGateOpen = GetByte(pBuf, index);
		sHitRate = GetShort(pBuf, index);
		byObjectType = GetByte(pBuf, index);
		byTrapNumber = GetByte(pBuf, index);

		//TRACE(_T("RecvNpcInfoAll  : nid=%d, szName=%hs, count=%d\n"), nid, szName, byCount);

		if (nLength < 0
			|| nLength > MAX_NPC_NAME_SIZE)
		{
			spdlog::error("AISocket::RecvNpcInfoAll: npc name size out of bounds [npcId={} npcName={}]",
				npcId, npcName);
			continue;		// 잘못된 monster 아이디 
		}

		C3DMap* pMap = _main->GetMapByIndex(sZoneIndex);
		if (pMap == nullptr)
		{
			spdlog::error("AISocket::RecvNpcInfoAll: map not found for zoneIndex [serial={} npcId={} pictureId={} npcName={} zoneIndex={} x={} z={}]",
				instanceId, npcId, pictureId, npcName, sZoneIndex, fPosX, fPosZ);
			continue;
		}

		if (instanceId < 0)
		{
			spdlog::error("AISocket::RecvNpcInfoAll: invalid serial [serial={} npcId={} pictureId={} npcName={} zoneIndex={} x={} z={}]",
				instanceId, npcId, pictureId, npcName, sZoneIndex, fPosX, fPosZ);
			continue;
		}

		if (pictureId < 0)
		{
			spdlog::error("AISocket::RecvNpcInfoAll: invalid pictureId [serial={} npcId={} pictureId={} npcName={} zoneIndex={} x={} z={}]",
				instanceId, npcId, pictureId, npcName, sZoneIndex, fPosX, fPosZ);
			continue;
		}

		//TRACE(_T("Recv --> NpcUserInfo : uid = %d, x=%f, z=%f.. \n"), nid, fPosX, fPosZ);

		CNpc* pNpc = new CNpc();
		if (pNpc == nullptr)
		{
			spdlog::error("AISocket::RecvNpcInfoAll: new Npc() constructed as null.");
			continue;
		}

		pNpc->Initialize();

		pNpc->m_sNid = instanceId;
		pNpc->m_sSid = npcId;
		pNpc->m_sPid = pictureId;
		pNpc->m_sSize = sSize;
		pNpc->m_iWeapon_1 = iweapon_1;
		pNpc->m_iWeapon_2 = iweapon_2;
		strcpy(pNpc->m_strName, npcName);
		pNpc->m_byGroup = byGroup;
		pNpc->m_byLevel = byLevel;
		pNpc->m_sCurZone = sZone;
		pNpc->m_sZoneIndex = sZoneIndex;
		pNpc->m_fCurX = fPosX;
		pNpc->m_fCurZ = fPosZ;
		pNpc->m_fCurY = fPosY;
		pNpc->m_byDirection = byDirection;
		pNpc->m_NpcState = NPC_LIVE;
		pNpc->m_tNpcType = tNpcType;
		pNpc->m_iSellingGroup = iSellingGroup;
		pNpc->m_iMaxHP = nMaxHP;
		pNpc->m_iHP = nHP;
		pNpc->m_byGateOpen = byGateOpen;
		pNpc->m_sHitRate = sHitRate;
		pNpc->m_byObjectType = byObjectType;
		pNpc->m_NpcState = NPC_LIVE;
		pNpc->m_byTrapNumber = byTrapNumber;

		int nRegX = static_cast<int32_t>(fPosX / VIEW_DISTANCE);
		int nRegZ = static_cast<int32_t>(fPosZ / VIEW_DISTANCE);

		pNpc->m_sRegion_X = nRegX;
		pNpc->m_sRegion_Z = nRegZ;

		_OBJECT_EVENT* pEvent = nullptr;
		if (pNpc->m_byObjectType == SPECIAL_OBJECT)
		{
			pEvent = pMap->GetObjectEvent(pNpc->m_sSid);
			if (pEvent != nullptr)
				pEvent->byLife = 1;
		}

		if (nRegX < 0
			|| nRegZ < 0)
		{
			spdlog::error("AISocket::RecvNpcInfoAll: region out of bounds [serial={} npcId={} npcName={} zoneId={} x={} z={}]",
				instanceId, npcId, npcName, pMap->m_nZoneNumber, nRegX, nRegZ);
			delete pNpc;
			pNpc = nullptr;
			continue;
		}

		// TRACE(_T("Recv --> NpcUserInfoAll : uid=%d, sid=%d, name=%hs, x=%f, z=%f. gate=%d, objecttype=%d \n"), nid, sPid, szName, fPosX, fPosZ, byGateOpen, byObjectType);

		if (!_main->m_NpcMap.PutData(pNpc->m_sNid, pNpc))
		{
			spdlog::error("AISocket::RecvNpcInfoAll: NpcMap put failed [serial={} npcId={} npcName={} zoneId={} x={} z={}]",
				instanceId, npcId, npcName, pMap->m_nZoneNumber, fPosX, fPosZ);
			delete pNpc;
			pNpc = nullptr;
			continue;
		}

		if (byType == 0)
		{
			spdlog::error("AISocket::RecvNpcInfoAll: invalid byType={} [serial={} npcId={} npcName={} zoneId={} x={} z={}]",
				byType, instanceId, npcId, npcName, pMap->m_nZoneNumber, fPosX, fPosZ);
			continue;
		}

		pMap->RegionNpcAdd(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, pNpc->m_sNid);
	}
}
// ~sungyong 2002.05.23

void CAISocket::RecvNpcMoveResult(char* pBuf)
{
	// sungyong tw
	char send_buff[256];	memset(send_buff, 0x00, 256);
	int index = 0, send_index = 0;
	uint8_t		flag;			// 01(INFO_MODIFY)	: NPC 정보 변경
								// 02(INFO_DELETE)	: NPC 정보 삭제
	int16_t		nid;			// NPC index
	float		fPosX;			// X Position
	float		fPosZ;			// Z Position
	float		fPosY;			// Y Position
	float		fSecForMetor;	// Sec당 metor
	flag = GetByte(pBuf, index);
	nid = GetShort(pBuf, index);
	fPosX = Getfloat(pBuf, index);
	fPosZ = Getfloat(pBuf, index);
	fPosY = Getfloat(pBuf, index);
	fSecForMetor = Getfloat(pBuf, index);

	CNpc* pNpc = _main->m_NpcMap.GetData(nid);
	if (pNpc == nullptr)
		return;

	// Npc 상태 동기화 불량,, 재요청..
	if (pNpc->m_NpcState == NPC_DEAD
		|| pNpc->m_iHP <= 0)
	{
		SetByte(send_buff, AG_NPC_HP_REQ, send_index);
		SetShort(send_buff, nid, send_index);
		SetDWORD(send_buff, pNpc->m_iHP, send_index);
		Send(send_buff, send_index);
	}
	// ~sungyong tw

	pNpc->MoveResult(fPosX, fPosY, fPosZ, fSecForMetor);
}

void CAISocket::RecvNpcAttack(char* pBuf)
{
	int index = 0, send_index = 0;
	int sid = -1, tid = -1;
	uint8_t type, result;
	int16_t damage = 0;
	int nHP = 0;
	uint8_t  byAttackType = 0;
	CNpc* pNpc = nullptr;
	CNpc* pMon = nullptr;
	CUser* pUser = nullptr;
	char pOutBuf[1024] = {};
	_OBJECT_EVENT* pEvent = nullptr;

	int temp_damage = 0;

	type = GetByte(pBuf, index);
	result = GetByte(pBuf, index);
	sid = GetShort(pBuf, index);
	tid = GetShort(pBuf, index);
	damage = GetShort(pBuf, index);
	nHP = GetDWORD(pBuf, index);
	byAttackType = GetByte(pBuf, index);

	//TRACE(_T("CAISocket-RecvNpcAttack : sid=%hs, tid=%d, zone_num=%d\n"), sid, tid, m_iZoneNum);

	// user attack -> npc
	if (type == 0x01)
	{
		pNpc = _main->m_NpcMap.GetData(tid);
		if (pNpc == nullptr)
			return;

		pNpc->m_iHP -= damage;
		if (pNpc->m_iHP < 0)
			pNpc->m_iHP = 0;

		// 마법으로 죽는경우
		if (result == 0x04)
		{
			SetByte(pOutBuf, WIZ_DEAD, send_index);
			SetShort(pOutBuf, tid, send_index);
			_main->Send_Region(pOutBuf, send_index, pNpc->m_sCurZone, pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nullptr, false);
		}
		else
		{
			SetByte(pOutBuf, WIZ_ATTACK, send_index);
			SetByte(pOutBuf, byAttackType, send_index);		// 직접:1, 마법:2, 지속마법:3
			//if(result == 0x04)								// 마법으로 죽는경우
			//	SetByte( pOutBuf, 0x02, send_index );
			//else											// 단순공격으로 죽는경우
			SetByte(pOutBuf, result, send_index);
			SetShort(pOutBuf, sid, send_index);
			SetShort(pOutBuf, tid, send_index);

			_main->Send_Region(pOutBuf, send_index, pNpc->m_sCurZone, pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nullptr, false);
		}

		CUser* pSrcUser = _main->GetUserPtr(sid);
		if (pSrcUser != nullptr)
		{
			pSrcUser->SendTargetHP(0, tid, -damage);
			if (byAttackType != MAGIC_ATTACK && byAttackType != DURATION_ATTACK)
			{
				pSrcUser->ItemWoreOut(DURABILITY_TYPE_ATTACK, damage);

				// LEFT HAND!!! by Yookozuna
				temp_damage = damage * pSrcUser->m_bMagicTypeLeftHand / 100;

				// LEFT HAND!!!
				switch (pSrcUser->m_bMagicTypeLeftHand)
				{
					// HP Drain
					case ITEM_TYPE_HP_DRAIN:
						pSrcUser->HpChange(temp_damage, 0);
						// TRACE(_T("%d : 흡수 HP : %d  ,  현재 HP : %d"), sid, temp_damage, pSrcUser->m_pUserData->m_sHp);
						break;

					// MP Drain
					case ITEM_TYPE_MP_DRAIN:
						pSrcUser->MSpChange(temp_damage);
						break;
				}

				temp_damage = 0;	// reset data;

				// RIGHT HAND!!! by Yookozuna
				temp_damage = damage * pSrcUser->m_bMagicTypeRightHand / 100;

				// LEFT HAND!!!
				switch (pSrcUser->m_bMagicTypeRightHand)
				{
					// HP Drain
					case ITEM_TYPE_HP_DRAIN:
						pSrcUser->HpChange(temp_damage, 0);
						// TRACE(_T("%d : 흡수 HP : %d  ,  현재 HP : %d"), sid, temp_damage, pSrcUser->m_pUserData->m_sHp);
						break;

					// MP Drain
					case ITEM_TYPE_MP_DRAIN:
						pSrcUser->MSpChange(temp_damage);
						break;
				}
			}
		}

		// npc dead
		if (result == 0x02
			|| result == 0x04)
		{
			C3DMap* pMap = _main->GetMapByIndex(pNpc->m_sZoneIndex);
			if (pMap == nullptr)
				return;

			pMap->RegionNpcRemove(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, tid);

			// TRACE(_T("--- Npc Dead : Npc를 Region에서 삭제처리.. ,, region_x=%d, y=%d\n"), pNpc->m_sRegion_X, pNpc->m_sRegion_Z);
			pNpc->m_sRegion_X = 0;
			pNpc->m_sRegion_Z = 0;
			pNpc->m_NpcState = NPC_DEAD;

			if (pNpc->m_byObjectType == SPECIAL_OBJECT)
			{
				pEvent = pMap->GetObjectEvent(pNpc->m_sSid);
				if (pEvent != nullptr)
					pEvent->byLife = 0;
			}

			//	성용씨! 대만 재미있어요? --;
			if (pNpc->m_tNpcType == 2
				&& pSrcUser != nullptr)
				pSrcUser->GiveItem(900001000, 1);
		}
	}
	// npc attack -> user
	else if (type == 0x02)
	{
		pNpc = _main->m_NpcMap.GetData(sid);
		if (pNpc == nullptr)
			return;

		//TRACE(_T("CAISocket-RecvNpcAttack 222 : sid=%hs, tid=%d, zone_num=%d\n"), sid, tid, m_iZoneNum);

		if (tid >= USER_BAND
			&& tid < NPC_BAND)
		{
			pUser = _main->GetUserPtr(tid);
			if (pUser == nullptr)
				return;

			// sungyong 2002. 02.04
/*			if( sHP <= 0 && pUser->m_pUserData->m_sHp > 0 ) {
				TRACE(_T("Npc Attack : id=%hs, result=%d, AI_HP=%d, GM_HP=%d\n"), pUser->m_pUserData->m_id, result, sHP, pUser->m_pUserData->m_sHp);
				if(result == 0x02)
					pUser->HpChange(-1000, 1);
			}
			else
				pUser->HpChange(-damage, 1);
			*/

			// ~sungyong 2002. 02.04
			if (pUser->m_MagicProcess.m_bMagicState == MAGIC_STATE_CASTING)
				pUser->m_MagicProcess.IsAvailable(0, -1, -1, MAGIC_EFFECTING, 0, 0, 0);

			pUser->HpChange(-damage, 1, true);
			pUser->ItemWoreOut(DURABILITY_TYPE_DEFENCE, damage);

			SetByte(pOutBuf, WIZ_ATTACK, send_index);
			SetByte(pOutBuf, byAttackType, send_index);
			if (result == 0x03)
				SetByte(pOutBuf, 0x00, send_index);
			else
				SetByte(pOutBuf, result, send_index);
			SetShort(pOutBuf, sid, send_index);
			SetShort(pOutBuf, tid, send_index);

			_main->Send_Region(pOutBuf, send_index, pNpc->m_sCurZone, pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nullptr, false);

//			TRACE(_T("RecvNpcAttack : id=%hs, result=%d, AI_HP=%d, GM_HP=%d\n"), pUser->m_pUserData->m_id, result, sHP, pUser->m_pUserData->m_sHp);
			//TRACE(_T("RecvNpcAttack ==> sid = %d, tid = %d, result = %d\n"), sid, tid, result);

			// user dead
			if (result == 0x02)
			{
				if (pUser->m_bResHpType == USER_DEAD)
					return;

				// 유저에게는 바로 데드 패킷을 날림... (한 번 더 보냄, 유령을 없애기 위해서)
				pUser->Send(pOutBuf, send_index);

				pUser->m_bResHpType = USER_DEAD;

				spdlog::debug("AISocket::RecvNpcAttack: user is dead [charId={} result={} AI_HP={} GM_HP={} x={} z={}]",
					pUser->m_pUserData->m_id, result, nHP, pUser->m_pUserData->m_sHp,
					static_cast<int32_t>(pUser->m_pUserData->m_curx), static_cast<int32_t>(pUser->m_pUserData->m_curz));
				
				memset(pOutBuf, 0, sizeof(pOutBuf));
				send_index = 0;

				// 지휘권한이 있는 유저가 죽는다면,, 지휘 권한 박탈
				// If the user with command authority dies, revoke their command authority.
				if (pUser->m_pUserData->m_bFame == COMMAND_CAPTAIN)
				{
					pUser->m_pUserData->m_bFame = CHIEF;
					SetByte(pOutBuf, WIZ_AUTHORITY_CHANGE, send_index);
					SetByte(pOutBuf, COMMAND_AUTHORITY, send_index);
					SetShort(pOutBuf, pUser->GetSocketID(), send_index);
					SetByte(pOutBuf, pUser->m_pUserData->m_bFame, send_index);
					_main->Send_Region(pOutBuf, send_index, pUser->m_pUserData->m_bZone, pUser->m_RegionX, pUser->m_RegionZ);
					// sungyong tw
					pUser->Send(pOutBuf, send_index);
					// ~sungyong tw
					spdlog::debug("AISocket::RecvNpcAttack: Dead Captain Deprive [charId={}]", pUser->m_pUserData->m_id);
					if (pUser->m_pUserData->m_bNation == KARUS)
						_main->Announcement(KARUS_CAPTAIN_DEPRIVE_NOTIFY, KARUS);
					else if (pUser->m_pUserData->m_bNation == ELMORAD)
						_main->Announcement(ELMORAD_CAPTAIN_DEPRIVE_NOTIFY, ELMORAD);
				}

				// 경비병에게 죽는 경우라면..
				if (pNpc->m_tNpcType == NPC_PATROL_GUARD)
				{
					pUser->ExpChange(-pUser->m_iMaxExp / 100);
					//TRACE(_T("RecvNpcAttack : 경험치를 1%깍기 id = %hs\n"), pUser->m_pUserData->m_id);
				}
				else
				{
					if (pUser->m_pUserData->m_bZone != pUser->m_pUserData->m_bNation && pUser->m_pUserData->m_bZone < 3)
					{
						pUser->ExpChange(-pUser->m_iMaxExp / 100);
						//TRACE(_T("정말로 1%만 깍였다니까요 ㅠ.ㅠ"));
					}
					else
					{
						pUser->ExpChange(-pUser->m_iMaxExp / 20);
					}
					//TRACE(_T("RecvNpcAttack : 경험치를 5%깍기 id = %hs\n"), pUser->m_pUserData->m_id);
				}
			}
		}
		// npc attack -> monster
		else if (tid >= NPC_BAND)
		{
			pMon = _main->m_NpcMap.GetData(tid);
			if (pMon == nullptr)
				return;

			pMon->m_iHP -= damage;
			if (pMon->m_iHP < 0)
				pMon->m_iHP = 0;

			memset(pOutBuf, 0, sizeof(pOutBuf));
			send_index = 0;
			SetByte(pOutBuf, WIZ_ATTACK, send_index);
			SetByte(pOutBuf, byAttackType, send_index);
			SetByte(pOutBuf, result, send_index);
			SetShort(pOutBuf, sid, send_index);
			SetShort(pOutBuf, tid, send_index);

			// npc dead
			if (result == 0x02)
			{
				C3DMap* pMap = _main->GetMapByIndex(pMon->m_sZoneIndex);
				if (pMap == nullptr)
					return;

				pMap->RegionNpcRemove(pMon->m_sRegion_X, pMon->m_sRegion_Z, tid);
				// TRACE(_T("--- Npc Dead : Npc를 Region에서 삭제처리.. ,, region_x=%d, y=%d\n"), pMon->m_sRegion_X, pMon->m_sRegion_Z);
				pMon->m_sRegion_X = 0;
				pMon->m_sRegion_Z = 0;
				pMon->m_NpcState = NPC_DEAD;
				if (pNpc->m_byObjectType == SPECIAL_OBJECT)
				{
					pEvent = pMap->GetObjectEvent(pMon->m_sSid);
					if (pEvent != nullptr)
						pEvent->byLife = 0;
				}
			}

			_main->Send_Region(pOutBuf, send_index, pNpc->m_sCurZone, pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nullptr, false);
		}
	}
}

void CAISocket::RecvMagicAttackResult(char* pBuf)
{
	int index = 0, send_index = 1;
	int sid = -1, tid = -1, magicid = 0;
	uint8_t byCommand;
	int16_t data0, data1, data2, data3, data4, data5;

	CNpc* pNpc = nullptr;
	CUser* pUser = nullptr;
	char send_buff[1024] = {};

	//byType = GetByte(pBuf,index);				// who ( 1:mon->user 2:mon->mon )
	//byAttackType = GetByte(pBuf,index);			// attack type ( 1:long attack, 2:magic attack
	byCommand = GetByte(pBuf, index);			// magic type ( 1:casting, 2:flying, 3:effecting, 4:fail )
	magicid = GetDWORD(pBuf, index);
	sid = GetShort(pBuf, index);
	tid = GetShort(pBuf, index);
	data0 = GetShort(pBuf, index);
	data1 = GetShort(pBuf, index);
	data2 = GetShort(pBuf, index);
	data3 = GetShort(pBuf, index);
	data4 = GetShort(pBuf, index);
	data5 = GetShort(pBuf, index);

	SetByte(send_buff, byCommand, send_index);
	SetDWORD(send_buff, magicid, send_index);
	SetShort(send_buff, sid, send_index);
	SetShort(send_buff, tid, send_index);
	SetShort(send_buff, data0, send_index);
	SetShort(send_buff, data1, send_index);
	SetShort(send_buff, data2, send_index);
	SetShort(send_buff, data3, send_index);
	SetShort(send_buff, data4, send_index);
	SetShort(send_buff, data5, send_index);

	// casting
	if (byCommand == MAGIC_CASTING)
	{
		pNpc = _main->m_NpcMap.GetData(sid);
		if (pNpc == nullptr)
			return;

		index = 0;
		SetByte(send_buff, WIZ_MAGIC_PROCESS, index);
		_main->Send_Region(send_buff, send_index, pNpc->m_sCurZone, pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nullptr, false);
	}
	// effecting
	else if (byCommand == MAGIC_EFFECTING)
	{
		if (sid >= USER_BAND
			&& sid < NPC_BAND)
		{
			pUser = _main->GetUserPtr(sid);
			if (pUser == nullptr
				|| pUser->m_bResHpType == USER_DEAD)
				return;

			index = 0;
			SetByte(send_buff, WIZ_MAGIC_PROCESS, index);
			_main->Send_Region(send_buff, send_index, pUser->m_pUserData->m_bZone, pUser->m_RegionX, pUser->m_RegionZ, nullptr, false);
		}
		else if (sid >= NPC_BAND)
		{
			if (tid >= NPC_BAND)
			{
				pNpc = _main->m_NpcMap.GetData(tid);
				if (pNpc == nullptr)
					return;

				index = 0;
				SetByte(send_buff, WIZ_MAGIC_PROCESS, index);
				_main->Send_Region(send_buff, send_index, pNpc->m_sCurZone, pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nullptr, false);
				return;
			}

			memset(send_buff, 0, sizeof(send_buff));
			send_index = 0;
			SetByte(send_buff, byCommand, send_index);
			SetDWORD(send_buff, magicid, send_index);
			SetShort(send_buff, sid, send_index);
			SetShort(send_buff, tid, send_index);
			SetShort(send_buff, data0, send_index);
			SetShort(send_buff, data1, send_index);
			SetShort(send_buff, data2, send_index);
			SetShort(send_buff, data3, send_index);
			SetShort(send_buff, data4, send_index);
			SetShort(send_buff, data5, send_index);
			_magicProcess.MagicPacket(send_buff, send_index);
		}
	}
}

void CAISocket::RecvNpcInfo(char* pBuf)
{
	int index = 0;

	uint8_t		Mode;						// 01(INFO_MODIFY)	: NPC 정보 변경
											// 02(INFO_DELETE)	: NPC 정보 삭제
	int16_t		instanceId;						// NPC index
	int16_t		npcId;						// NPC index
	int16_t		pictureId;						// NPC Picture Number
	int16_t		sSize = 100;				// NPC Size
	int			iWeapon_1;					// 오른손 무기
	int			iWeapon_2;					// 왼손  무기
	int16_t		sZone;						// Current zone number
	int16_t		sZoneIndex;					// Current zone index
	char		npcName[MAX_NPC_NAME_SIZE + 1];	// NPC Name
	uint8_t		byGroup;					// 소속 집단
	uint8_t		byLevel;					// level
	float		fPosX;						// X Position
	float		fPosZ;						// Z Position
	float		fPosY;						// Y Position
	uint8_t		byDirection;				// 방향
	uint8_t		tState;						// NPC 상태
											// 00	: NPC Dead
											// 01	: NPC Live
	uint8_t		tNpcKind;					// 00	: Monster
											// 01	: NPC
	int			iSellingGroup;
	int			nMaxHP;						// 최대 HP
	int			nHP;						// 현재 HP
	uint8_t		byGateOpen;
	int16_t		sHitRate;					// 공격 성공률
	uint8_t		byObjectType;				// 보통 : 0, 특수 : 1
	uint8_t		byTrapNumber;

	Mode = GetByte(pBuf, index);
	instanceId = GetShort(pBuf, index);
	npcId = GetShort(pBuf, index);
	pictureId = GetShort(pBuf, index);
	sSize = GetShort(pBuf, index);
	iWeapon_1 = GetDWORD(pBuf, index);
	iWeapon_2 = GetDWORD(pBuf, index);
	sZone = GetShort(pBuf, index);
	sZoneIndex = GetShort(pBuf, index);
	int nLength = GetVarString(npcName, pBuf, sizeof(uint8_t), index);

	// 잘못된 monster 아이디 
	if (nLength < 0
		|| nLength > MAX_NPC_NAME_SIZE)
		return;

	byGroup = GetByte(pBuf, index);
	byLevel = GetByte(pBuf, index);
	fPosX = Getfloat(pBuf, index);
	fPosZ = Getfloat(pBuf, index);
	fPosY = Getfloat(pBuf, index);
	byDirection = GetByte(pBuf, index);
	tState = GetByte(pBuf, index);
	tNpcKind = GetByte(pBuf, index);
	iSellingGroup = GetDWORD(pBuf, index);
	nMaxHP = GetDWORD(pBuf, index);
	nHP = GetDWORD(pBuf, index);
	byGateOpen = GetByte(pBuf, index);
	sHitRate = GetShort(pBuf, index);
	byObjectType = GetByte(pBuf, index);
	byTrapNumber = GetByte(pBuf, index);

	CNpc* pNpc = _main->m_NpcMap.GetData(instanceId);
	if (pNpc == nullptr)
		return;

	pNpc->m_NpcState = NPC_DEAD;
	
	// 살아 있는데 또 정보를 받는 경우
	if (pNpc->m_NpcState == NPC_LIVE)
	{
		spdlog::get(logger::EbenezerRegion)->info("AISocket::RecvNpcInfo: npc regen check [state={} serial={} npcId={} npcName={} x={} z={} regionX={} regionZ={}]",
			pNpc->m_NpcState, instanceId, npcId, npcName,
			static_cast<int32_t>(pNpc->m_fCurX), static_cast<int32_t>(pNpc->m_fCurZ),
			pNpc->m_sRegion_X, pNpc->m_sRegion_Z);
	}

	pNpc->m_NpcState = NPC_LIVE;

	pNpc->m_sNid = instanceId;
	pNpc->m_sSid = npcId;
	pNpc->m_sPid = pictureId;
	pNpc->m_sSize = sSize;
	pNpc->m_iWeapon_1 = iWeapon_1;
	pNpc->m_iWeapon_2 = iWeapon_2;
	strcpy(pNpc->m_strName, npcName);
	pNpc->m_byGroup = byGroup;
	pNpc->m_byLevel = byLevel;
	pNpc->m_sCurZone = sZone;
	pNpc->m_sZoneIndex = sZoneIndex;
	pNpc->m_fCurX = fPosX;
	pNpc->m_fCurZ = fPosZ;
	pNpc->m_fCurY = fPosY;
	pNpc->m_byDirection = byDirection;
	pNpc->m_NpcState = tState;
	pNpc->m_tNpcType = tNpcKind;
	pNpc->m_iSellingGroup = iSellingGroup;
	pNpc->m_iMaxHP = nMaxHP;
	pNpc->m_iHP = nHP;
	pNpc->m_byGateOpen = byGateOpen;
	pNpc->m_sHitRate = sHitRate;
	pNpc->m_byObjectType = byObjectType;
	pNpc->m_byTrapNumber = byTrapNumber;

	int nRegX = static_cast<int32_t>(fPosX / VIEW_DISTANCE);
	int nRegZ = static_cast<int32_t>(fPosZ / VIEW_DISTANCE);

	pNpc->m_sRegion_X = nRegX;
	pNpc->m_sRegion_Z = nRegZ;

	C3DMap* pMap = _main->GetMapByIndex(pNpc->m_sZoneIndex);
	if (pMap == nullptr)
	{
		pNpc->m_NpcState = NPC_DEAD;
		spdlog::error("AISocket::RecvNpcInfo: map not found for zoneIndex [serial={} npcId={} pictureId={} npcName={} zoneIndex={} x={} z={}]",
				instanceId, npcId, pictureId, npcName, pNpc->m_sZoneIndex, fPosX, fPosZ);
		return;
	}

	_OBJECT_EVENT* pEvent = nullptr;
	if (pNpc->m_byObjectType == SPECIAL_OBJECT)
	{
		pEvent = pMap->GetObjectEvent(pNpc->m_sSid);
		if (pEvent != nullptr)
			pEvent->byLife = 1;
	}

	if (Mode == 0)
	{
		spdlog::error("AISocket::RecvNpcInfo: dead monster [serial={} npcId={} npcName={} zoneId={} x={} z={}]",
				instanceId, npcId, npcName, pMap->m_nZoneNumber, fPosX, fPosZ);
		return;
	}

	int send_index = 0;
	char pOutBuf[1024] = {};

	SetByte(pOutBuf, WIZ_NPC_INOUT, send_index);
	SetByte(pOutBuf, NPC_IN, send_index);
	SetShort(pOutBuf, pNpc->m_sNid, send_index);
	pNpc->GetNpcInfo(pOutBuf, send_index);

	_main->Send_Region(pOutBuf, send_index, pNpc->m_sCurZone, nRegX, nRegZ);

	pMap->RegionNpcAdd(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, pNpc->m_sNid);

//	int nTotMon = _main->m_NpcMap.GetSize();
//	TRACE(_T("Recv --> NpcUserInfo : uid = %d, x=%f, z=%f.. ,, tot = %d\n"), nid, fPosX, fPosZ, nTotMon);
}

void CAISocket::RecvUserHP(char* pBuf)
{
	int index = 0, send_index = 0;
	int nid = 0;
	int nHP = 0, nMaxHP = 0;
	char send_buff[256] = {};

	nid = GetShort(pBuf, index);
	nHP = GetDWORD(pBuf, index);
	nMaxHP = GetDWORD(pBuf, index);

	if (nid >= USER_BAND
		&& nid < NPC_BAND)
	{
		CUser* pUser = _main->GetUserPtr(nid);
		if (pUser == nullptr)
			return;

		pUser->m_pUserData->m_sHp = nHP;
	}
	else if (nid >= NPC_BAND)
	{
		CNpc* pNpc = _main->m_NpcMap.GetData(nid);
		if (pNpc == nullptr)
			return;

		int nOldHP = pNpc->m_iHP;
		pNpc->m_iHP = nHP;
		pNpc->m_iMaxHP = nMaxHP;
//		TRACE(_T("RecvNpcHP - (%d,%hs), %d->%d\n"), pNpc->m_sNid, pNpc->m_strName, nOldHP, pNpc->m_sHP);
	}
}

void CAISocket::RecvUserExp(char* pBuf)
{
	int index = 0;
	int userId = GetShort(pBuf, index);
	int16_t sExp = GetShort(pBuf, index);
	int16_t sLoyalty = GetShort(pBuf, index);

	CUser* pUser = _main->GetUserPtr(userId);
	if (pUser == nullptr)
	{
		spdlog::error("AISocket::RecvUserExp: attempting to grant exp or loyalty to invalid user [userId={}]",
			userId, pUser->m_pUserData->m_id, sExp, sLoyalty);
		return;
	}

	if (sExp < 0
		|| sLoyalty < 0)
	{
		spdlog::error("AISocket::RecvUserExp: invalid exp or loyalty amount granted [userId={} charId={} exp={} loyalty={}]",
			userId, pUser->m_pUserData->m_id, sExp, sLoyalty);
		return;
	}

	pUser->m_pUserData->m_iLoyalty += sLoyalty;
	pUser->ExpChange(sExp);

	if (sLoyalty > 0)
	{
		char send_buff[128] = {};
		int send_index = 0;
		SetByte(send_buff, WIZ_LOYALTY_CHANGE, send_index);
		SetDWORD(send_buff, pUser->m_pUserData->m_iLoyalty, send_index);
		pUser->Send(send_buff, send_index);
	}
}

void CAISocket::RecvSystemMsg(char* pBuf)
{
	int index = 0, send_index = 0;
	char send_buff[256] = {},
		strSysMsg[256] = {};

	uint8_t bType;
	int16_t sWho, sLength;

	bType = GetByte(pBuf, index);
	sWho = GetShort(pBuf, index);
	sLength = GetShort(pBuf, index);
	GetString(strSysMsg, pBuf, sLength, index);

	//TRACE(_T("RecvSystemMsg - type=%d, who=%d, len=%d, msg=%hs\n"), bType, sWho, sLength, strSysMsg);

	switch (sWho)
	{
		case SEND_ME:
			break;

		case SEND_REGION:
			break;

		case SEND_ALL:
			SetByte(send_buff, WIZ_CHAT, send_index);
			SetByte(send_buff, bType, send_index);
			SetByte(send_buff, 0x01, send_index);		// nation
			SetShort(send_buff, -1, send_index);		// sid
			SetByte(send_buff, 0, send_index);			// sender name length
			SetString2(send_buff, strSysMsg, sLength, send_index);
			_main->Send_All(send_buff, send_index);
			break;

		case SEND_ZONE:
			break;
	}

}

void CAISocket::RecvNpcGiveItem(char* pBuf)
{
	int index = 0, send_index = 0;
	char send_buff[1024] = {};
	int16_t sUid, sNid, sZone, regionx, regionz;
	float fX, fZ, fY;
	uint8_t byCount;
	int nItemNumber[NPC_HAVE_ITEM_LIST];
	int16_t sCount[NPC_HAVE_ITEM_LIST];
	_ZONE_ITEM* pItem = nullptr;
	C3DMap* pMap = nullptr;
	CUser* pUser = nullptr;

	sUid = GetShort(pBuf, index);	// Item을 가져갈 사람의 아이디... (이것을 참조해서 작업하셈~)
	sNid = GetShort(pBuf, index);
	sZone = GetShort(pBuf, index);
	regionx = GetShort(pBuf, index);
	regionz = GetShort(pBuf, index);
	fX = Getfloat(pBuf, index);
	fZ = Getfloat(pBuf, index);
	fY = Getfloat(pBuf, index);
	byCount = GetByte(pBuf, index);

	for (int i = 0; i < byCount; i++)
	{
		nItemNumber[i] = GetDWORD(pBuf, index);
		sCount[i] = GetShort(pBuf, index);
	}

	pUser = _main->GetUserPtr(sUid);
	if (pUser == nullptr)
		return;

	pMap = _main->GetMapByID(sZone);
	if (pMap == nullptr)
	{
		delete pItem;
		return;
	}

	pItem = new _ZONE_ITEM;
	for (int i = 0; i < 6; i++)
	{
		pItem->itemid[i] = 0;
		pItem->count[i] = 0;
	}

	pItem->bundle_index = pMap->m_wBundle;
	pItem->time = TimeGet();
	pItem->x = fX;
	pItem->z = fZ;
	pItem->y = fY;

	for (int i = 0; i < byCount; i++)
	{
		if (_main->m_ItemTableMap.GetData(nItemNumber[i]) != nullptr)
		{
			pItem->itemid[i] = nItemNumber[i];
			pItem->count[i] = sCount[i];
		}
	}

	if (!pMap->RegionItemAdd(regionx, regionz, pItem))
	{
		delete pItem;
		return;
	}

	send_index = 0;
	memset(send_buff, 0, sizeof(send_buff));

	SetByte(send_buff, WIZ_ITEM_DROP, send_index);
	SetShort(send_buff, sNid, send_index);
	SetDWORD(send_buff, pItem->bundle_index, send_index);
	if (pUser->m_sPartyIndex == -1)
		pUser->Send(send_buff, send_index);
	else
		_main->Send_PartyMember(pUser->m_sPartyIndex, send_buff, send_index);
}

void CAISocket::RecvUserFail(char* pBuf)
{
	int16_t instanceId = 0, npcId = 0;
	int index = 0, send_index = 0;
	char pOutBuf[1024] = {};

	instanceId = GetShort(pBuf, index);
	npcId = GetShort(pBuf, index);

	CUser* pUser = _main->GetUserPtr(instanceId);
	if (pUser == nullptr)
		return;

	// 여기에서 게임데이타의 정보를 AI서버에 보내보자...
	// 게임서버와 AI서버간의 데이타가 틀림..
/*	if (pUser->m_pUserData->m_sHp > 0
		&& pUser->m_bResHpType != USER_DEAD)
	{
		SetByte(pOutBuf, AG_USER_FAIL, send_index);
		SetShort( pOutBuf, nid, send_index );
		SetShort( pOutBuf, sid, send_index );
		SetShort( pOutBuf, pUser->m_pUserData->m_sHp, send_index);
		Send( pOutBuf, send_index);
	}	*/

	pUser->HpChange(-10000, 1);

	uint8_t type = 0x01;
	uint8_t result = 0x02;

	SetByte(pOutBuf, WIZ_ATTACK, send_index);
	SetByte(pOutBuf, type, send_index);
	SetByte(pOutBuf, result, send_index);
	SetShort(pOutBuf, npcId, send_index);
	SetShort(pOutBuf, instanceId, send_index);

	spdlog::trace("AISocket::RecvUserFail: [npcId={} serial={} charId={}]",
		npcId, instanceId, pUser->m_pUserData->m_id);

	_main->Send_Region(pOutBuf, send_index, pUser->m_pUserData->m_bZone, pUser->m_RegionX, pUser->m_RegionZ);
}

void CAISocket::RecvCompressedData(char* pBuf)
{
	int index = 0;
	int16_t sCompLen, sOrgLen, sCompCount;
	std::vector<uint8_t> decompressedBuffer;
	uint8_t* pCompressedBuffer = nullptr;

	uint32_t dwCrcValue = 0, dwActualCrcValue = 0;

	sCompLen = GetShort(pBuf, index);	// 압축된 데이타길이얻기...
	sOrgLen = GetShort(pBuf, index);	// 원래데이타길이얻기...
	dwCrcValue = GetDWORD(pBuf, index);	// CRC값 얻기...
	sCompCount = GetShort(pBuf, index);	// 압축 데이타 수 얻기...

	decompressedBuffer.resize(sOrgLen);

	pCompressedBuffer = reinterpret_cast<uint8_t*>(pBuf + index);
	index += sCompLen;

	uint32_t nDecompressedLength = lzf_decompress(
		pCompressedBuffer,
		sCompLen,
		&decompressedBuffer[0],
		sOrgLen);

	_ASSERT(nDecompressedLength == sOrgLen);

	if (nDecompressedLength != sOrgLen)
		return;

	dwActualCrcValue = crc32(&decompressedBuffer[0], sOrgLen);

	_ASSERT(dwCrcValue == dwActualCrcValue);

	if (dwCrcValue != dwActualCrcValue)
		return;

	Parsing(sOrgLen, reinterpret_cast<char*>(&decompressedBuffer[0]));
}

void CAISocket::InitEventMonster(int instanceId)
{
	int count = instanceId;
	if (count < 0
		|| count > NPC_BAND)
	{
		spdlog::error("AISocket::InitEventMonster: serial out of bounds [serial={}]", instanceId);
		return;
	}

	int max_eventmop = count + EVENT_MONSTER;
	for (int i = count; i < max_eventmop; i++)
	{
		CNpc* pNpc = new CNpc();
		if (pNpc == nullptr)
			return;

		pNpc->Initialize();

		pNpc->m_sNid = i + NPC_BAND;
		//TRACE(_T("InitEventMonster : uid = %d\n"), pNpc->m_sNid);

		if (!_main->m_NpcMap.PutData(pNpc->m_sNid, pNpc))
		{
			spdlog::error("AISocket::InitEventMonster: NpcMap Put failed for serial={}", pNpc->m_sNid);
			delete pNpc;
			pNpc = nullptr;
		}
	}
	
	spdlog::debug("AISocket::InitEventMonster: TotalMonster = {}", _main->m_NpcMap.GetSize());
}

void CAISocket::RecvCheckAlive(char* pBuf)
{
//	TRACE(_T("CAISocket-RecvCheckAlive : zone_num=%d\n"), m_iZoneNum);
	_main->m_sErrorSocketCount = 0;

	int len = 0;
	char pSendBuf[256] = {};
	SetByte(pSendBuf, AG_CHECK_ALIVE_REQ, len);
	Send(pSendBuf, len);
}

void CAISocket::RecvGateDestroy(char* pBuf)
{
	int index = 0, send_index = 0, cur_zone = 0, rx = 0, rz = 0;
	int instanceId = 0, gateStatus = 0;
	char send_buff[256] = {};

	instanceId = GetShort(pBuf, index);
	gateStatus = GetByte(pBuf, index);
	cur_zone = GetShort(pBuf, index);
	rx = GetShort(pBuf, index);
	rz = GetShort(pBuf, index);

	if (instanceId >= NPC_BAND)
	{
		CNpc* pNpc = _main->m_NpcMap.GetData(instanceId);
		if (pNpc == nullptr)
		{
			spdlog::error("AISocket::RecvGateDestroy: NPC not found serial={}", instanceId);
			return;
		}

		pNpc->m_byGateOpen = gateStatus;
		spdlog::debug("AISocket::RecvGateDestroy: [serial={} npcId={} npcName={} gateOpen={}]",
			pNpc->m_sNid, pNpc->m_sSid, pNpc->m_strName, pNpc->m_byGateOpen);
/*
		SetByte( send_buff, WIZ_OBJECT_EVENT, send_index );
		SetByte( send_buff, 1, send_index );					// type
		SetByte( send_buff, 1, send_index );
		SetShort( send_buff, nid, send_index );
		SetByte( send_buff, pNpc->m_byGateOpen, send_index );
		_main->Send_Region( send_buff, send_index, cur_zone, rx, rz );	*/
	}
}

void CAISocket::RecvNpcDead(char* pBuf)
{
	int index = 0, send_index = 0;
	int nid = 0;
	char send_buff[256] = {};
	_OBJECT_EVENT* pEvent = nullptr;

	nid = GetShort(pBuf, index);

	if (nid >= NPC_BAND)
	{
		CNpc* pNpc = _main->m_NpcMap.GetData(nid);
		if (pNpc == nullptr)
			return;

		C3DMap* pMap = _main->GetMapByIndex(pNpc->m_sZoneIndex);
		if (pMap == nullptr)
			return;

		if (pNpc->m_byObjectType == SPECIAL_OBJECT)
		{
			pEvent = pMap->GetObjectEvent(pNpc->m_sSid);
			if (pEvent != nullptr)
				pEvent->byLife = 0;
		}

		//pNpc->NpcInOut( NPC_OUT );
		//TRACE(_T("RecvNpcDead - (%d,%hs)\n"), pNpc->m_sNid, pNpc->m_strName);

		pMap->RegionNpcRemove(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nid);
		//TRACE(_T("--- RecvNpcDead : Npc를 Region에서 삭제처리.. ,, zone=%d, region_x=%d, y=%d\n"), pNpc->m_sZoneIndex, pNpc->m_sRegion_X, pNpc->m_sRegion_Z);

		SetByte(send_buff, WIZ_DEAD, send_index);
		SetShort(send_buff, nid, send_index);
		_main->Send_Region(send_buff, send_index, pNpc->m_sCurZone, pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nullptr, false);

		pNpc->m_sRegion_X = 0;
		pNpc->m_sRegion_Z = 0;
	}
}

void CAISocket::RecvNpcInOut(char* pBuf)
{
	int index = 0, send_index = 0;
	int nid = 0, nType = 0;
	float fx = 0.0f, fz = 0.0f, fy = 0.0f;
	char send_buff[256] = {};

	nType = GetByte(pBuf, index);
	nid = GetShort(pBuf, index);
	fx = Getfloat(pBuf, index);
	fz = Getfloat(pBuf, index);
	fy = Getfloat(pBuf, index);

	if (nid >= NPC_BAND)
	{
		CNpc* pNpc = _main->m_NpcMap.GetData(nid);
		if (pNpc == nullptr)
			return;

		pNpc->NpcInOut(nType, fx, fz, fy);
	}
}

void CAISocket::RecvBattleEvent(char* pBuf)
{
	int index = 0, send_index = 0, udp_index = 0, retvalue = 0;
	int nType = 0, nResult = 0, nLen = 0;
	char strMaxUserName[MAX_ID_SIZE + 1] = {},
		strKnightsName[MAX_ID_SIZE + 1] = {},
		send_buff[1024] = {},
		udp_buff[1024] = {};

	CUser* pUser = nullptr;
	CKnights* pKnights = nullptr;

	std::string buff;
	std::string buff2;

	nType = GetByte(pBuf, index);
	nResult = GetByte(pBuf, index);

	if (nType == BATTLE_EVENT_OPEN)
	{
	}
	else if (nType == BATTLE_MAP_EVENT_RESULT)
	{
		if (_main->m_byBattleOpen == NO_BATTLE)
		{
			spdlog::error("AISocket::RecvBattleEvent: No active battle [battleOpen={} type={}]",
				_main->m_byBattleOpen, nType);
			return;
		}

		if (nResult == KARUS)
		{
			//TRACE(_T("--> RecvBattleEvent : 카루스 땅으로 넘어갈 수 있어\n"));
			_main->m_byKarusOpenFlag = 1;		// 카루스 땅으로 넘어갈 수 있어
		}
		else if (nResult == ELMORAD)
		{
			//TRACE(_T("--> RecvBattleEvent : 엘모 땅으로 넘어갈 수 있어\n"));
			_main->m_byElmoradOpenFlag = 1;	// 엘모 땅으로 넘어갈 수 있어
		}

		SetByte(udp_buff, UDP_BATTLE_EVENT_PACKET, udp_index);
		SetByte(udp_buff, nType, udp_index);
		SetByte(udp_buff, nResult, udp_index);
	}
	else if (nType == BATTLE_EVENT_RESULT)
	{
		if (_main->m_byBattleOpen == NO_BATTLE)
		{
			spdlog::error("AISocket::RecvBattleEvent: No active battle [battleOpen={} type={}]",
				_main->m_byBattleOpen, nType);
			return;
		}

		if (nResult == KARUS)
		{
			//TRACE(_T("--> RecvBattleEvent : 카루스가 승리하였습니다.\n"));
		}
		else if (nResult == ELMORAD)
		{
			//TRACE(_T("--> RecvBattleEvent : 엘모라드가 승리하였습니다.\n"));
		}

		nLen = GetByte(pBuf, index);

		if (nLen > 0
			&& nLen < MAX_ID_SIZE + 1)
		{
			GetString(strMaxUserName, pBuf, nLen, index);
			if (_main->m_byBattleSave == 0)
			{
				// 승리국가를 sql에 저장
				memset(send_buff, 0, sizeof(send_buff));
				send_index = 0;
				SetByte(send_buff, WIZ_BATTLE_EVENT, send_index);
				SetByte(send_buff, nType, send_index);
				SetByte(send_buff, nResult, send_index);
				SetByte(send_buff, nLen, send_index);
				SetString(send_buff, strMaxUserName, nLen, send_index);
				retvalue = _main->m_LoggerSendQueue.PutData(send_buff, send_index);
				if (retvalue >= SMQ_FULL)
				{
					std::wstring logStr = std::format(L"WIZ_BATTLE_EVENT send fail [retValue={} type={}]",
						retvalue, nType);
					_main->AddOutputMessage(logStr);
					spdlog::error("AISocket::RecvBattleEvent: WIZ_BATTLE_EVENT send fail [retValue={} type={}]",
						retvalue, nType);
				}
				_main->m_byBattleSave = 1;
			}
		}

		_main->m_bVictory = nResult;
		_main->m_byOldVictory = nResult;
		_main->m_byKarusOpenFlag = 0;		// 카루스 땅으로 넘어갈 수 없도록
		_main->m_byElmoradOpenFlag = 0;	// 엘모 땅으로 넘어갈 수 없도록
		_main->m_byBanishFlag = 1;

		SetByte(udp_buff, UDP_BATTLE_EVENT_PACKET, udp_index);	// udp로 다른서버에 정보 전달
		SetByte(udp_buff, nType, udp_index);
		SetByte(udp_buff, nResult, udp_index);
	}
	else if (nType == BATTLE_EVENT_MAX_USER)
	{
		nLen = GetByte(pBuf, index);

		if (nLen > 0
			&& nLen < MAX_ID_SIZE + 1)
		{
			GetString(strMaxUserName, pBuf, nLen, index);
			pUser = _main->GetUserPtr(strMaxUserName, NameType::Character);
			if (pUser != nullptr)
			{
				pKnights = _main->m_KnightsMap.GetData(pUser->m_pUserData->m_bKnights);
				if (pKnights != nullptr)
					strcpy(strKnightsName, pKnights->m_strName);
			}

			//TRACE(_T("--> RecvBattleEvent : 적국의 대장을 죽인 유저이름은? %hs, len=%d\n"), strMaxUserName, nResult);

			std::string chatstr;
			if (nResult == 1)
			{
				chatstr = fmt::format_db_resource(IDS_KILL_CAPTAIN, strKnightsName, strMaxUserName);

		/*		if (_main->m_byBattleSave == 0)
				{
					// 승리국가를 sql에 저장
					memset(send_buff, 0, sizeof(send_buff));
					send_index = 0;
					SetByte(send_buff, WIZ_BATTLE_EVENT, send_index);
					SetByte(send_buff, nType, send_index);
					SetByte(send_buff, _main->m_bVictory, send_index);
					SetByte(send_buff, nLen, send_index);
					SetString(send_buff, strMaxUserName, nLen, send_index);
					retvalue = _main->m_LoggerSendQueue.PutData(send_buff, send_index);
					if (retvalue >= SMQ_FULL)
					{
						char logstr[256] = {};
						sprintf(logstr, "WIZ_BATTLE_EVENT Send Fail : %d, %d", retvalue, nType);
						_main->m_StatusList.AddString(logstr);
					}
					_main->m_byBattleSave = 1;
				}*/
			}
			else if (nResult == 2)
			{
				chatstr = fmt::format_db_resource(IDS_KILL_GATEKEEPER, strKnightsName, strMaxUserName);
			}
			else if (nResult == 3)
			{
				chatstr = fmt::format_db_resource(IDS_KILL_KARUS_GUARD1, strKnightsName, strMaxUserName);
			}
			else if (nResult == 4)
			{
				chatstr = fmt::format_db_resource(IDS_KILL_KARUS_GUARD2, strKnightsName, strMaxUserName);
			}
			else if (nResult == 5)
			{
				chatstr = fmt::format_db_resource(IDS_KILL_ELMO_GUARD1, strKnightsName, strMaxUserName);
			}
			else if (nResult == 6)
			{
				chatstr = fmt::format_db_resource(IDS_KILL_ELMO_GUARD2, strKnightsName, strMaxUserName);
			}
			else if (nResult == 7
				|| nResult == 8)
			{
				chatstr = fmt::format_db_resource(IDS_KILL_GATEKEEPER, strKnightsName, strMaxUserName);
			}

			memset(send_buff, 0, sizeof(send_buff));
			send_index = 0;
			chatstr = fmt::format_db_resource(IDP_ANNOUNCEMENT, chatstr);
			SetByte(send_buff, WIZ_CHAT, send_index);
			SetByte(send_buff, WAR_SYSTEM_CHAT, send_index);
			SetByte(send_buff, 1, send_index);
			SetShort(send_buff, -1, send_index);
			SetByte(send_buff, 0, send_index);			// sender name length
			SetString2(send_buff, chatstr, send_index);
			_main->Send_All(send_buff, send_index);

			memset(send_buff, 0, sizeof(send_buff));
			send_index = 0;
			SetByte(send_buff, WIZ_CHAT, send_index);
			SetByte(send_buff, PUBLIC_CHAT, send_index);
			SetByte(send_buff, 1, send_index);
			SetShort(send_buff, -1, send_index);
			SetByte(send_buff, 0, send_index);			// sender name length
			SetString2(send_buff, chatstr, send_index);
			_main->Send_All(send_buff, send_index);

			SetByte(udp_buff, UDP_BATTLE_EVENT_PACKET, udp_index);
			SetByte(udp_buff, nType, udp_index);
			SetByte(udp_buff, nResult, udp_index);
			SetByte(udp_buff, nLen, udp_index);
			SetString(udp_buff, strMaxUserName, nLen, udp_index);
		}
	}

	_main->Send_UDP_All(udp_buff, udp_index);
}

void CAISocket::RecvNpcEventItem(char* pBuf)
{
	int index = 0, send_index = 0, zoneindex = -1;
	char send_buff[1024] = {};
	int16_t sUid = 0, sNid = 0;
	int nItemNumber = 0, nCount = 0;
	CUser* pUser = nullptr;

	sUid = GetShort(pBuf, index);	// Item을 가져갈 사람의 아이디... (이것을 참조해서 작업하셈~)
	sNid = GetShort(pBuf, index);
	nItemNumber = GetDWORD(pBuf, index);
	nCount = GetDWORD(pBuf, index);

	pUser = _main->GetUserPtr(sUid);
	if (pUser == nullptr)
		return;

	pUser->EventMoneyItemGet(nItemNumber, nCount);
}

void CAISocket::RecvGateOpen(char* pBuf)
{
	int index = 0, send_index = 0, instanceId = 0, npcId = 0, nGateFlag = 0;
	char send_buff[256] = {};

	CNpc* pNpc = nullptr;
	_OBJECT_EVENT* pEvent = nullptr;

	instanceId = GetShort(pBuf, index);
	npcId = GetShort(pBuf, index);
	nGateFlag = GetByte(pBuf, index);

	pNpc = _main->m_NpcMap.GetData(instanceId);
	if (pNpc == nullptr)
	{
		spdlog::error("AISocket::RecvGateOpen: Npc not found [serial={} npcId={}]",
			instanceId, npcId);
		return;
	}

	pNpc->m_byGateOpen = nGateFlag;

	pEvent = _main->m_ZoneArray[pNpc->m_sZoneIndex]->GetObjectEvent(npcId);
	if (pEvent == nullptr)
	{
		spdlog::error("AISocket::RecvGateOpen: Npc ObjectEvent not found [serial={} npcId={}]",
			instanceId, npcId);
		return;
	}

	//TRACE(_T("---> RecvGateOpen Npc Object fail : nid=%d, sid=%d, nGateFlag = %d ####\n"), nNid, nSid, nGateFlag);

	if (pNpc->m_tNpcType == NPC_GATE
		|| pNpc->m_tNpcType == NPC_PHOENIX_GATE
		|| pNpc->m_tNpcType == NPC_SPECIAL_GATE)
	{
		SetByte(send_buff, WIZ_OBJECT_EVENT, send_index);
		SetByte(send_buff, static_cast<uint8_t>(pEvent->sType), send_index);
		SetByte(send_buff, 0x01, send_index);
		SetShort(send_buff, instanceId, send_index);
		SetByte(send_buff, pNpc->m_byGateOpen, send_index);
		_main->Send_Region(send_buff, send_index, pNpc->m_sCurZone, pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nullptr, false);
	}
}
