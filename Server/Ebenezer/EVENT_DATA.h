#pragma once

#include "EXEC.h"
#include "LOGIC_ELSE.h"

#include <list>

typedef	 std::list<EXEC*>				ExecArray;
typedef	 std::list<LOGIC_ELSE*>			LogicElseArray;

class EVENT_DATA
{
public:
	int					m_EventNum;
	ExecArray			m_arExec;
	LogicElseArray		m_arLogicElse;

	EVENT_DATA();
	virtual ~EVENT_DATA();
};
