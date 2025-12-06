#pragma once

//#include "PartyUser.h"
//#include "STLMap.h"

//typedef CSTLMap <CPartyUser>			PartyUserArray;
class AIServerApp;
class CParty
{
public:
	//int		m_iSid;				// Party Number
	//int16_t	m_sCurUser;			// 파티 인원 수 
	//int16_t	m_sCurLevel;		// 파티원의 총 레벨 합
	AIServerApp* m_pMain;
	//PartyUserArray	m_arPartyUser;

public:
	CParty();
	virtual ~CParty();

	void PartyDelete(char* pBuf);
	void PartyRemove(char* pBuf);
	void PartyInsert(char* pBuf);
	void PartyCreate(char* pBuf);
	void PartyProcess(char* pBuf);
};
