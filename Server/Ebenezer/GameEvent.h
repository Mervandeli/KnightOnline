#pragma once

class CUser;
class CGameEvent
{
public:
	void RunEvent(CUser* pUser = nullptr);
	CGameEvent();
	virtual ~CGameEvent();

	int16_t	m_sIndex;
	uint8_t	m_bType;

	int		m_iExec[5];
};
