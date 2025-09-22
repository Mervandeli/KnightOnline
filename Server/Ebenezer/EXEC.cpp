// EXEC.cpp: implementation of the EXEC class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Ebenezer.h"
#include "Define.h"
#include "EXEC.h"

#include <djb2/djb2_hasher.h>
#include <spdlog/spdlog.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EXEC::EXEC()
{
}

EXEC::~EXEC()
{
}

void EXEC::Parse(const char* line, const std::wstring& filename, int lineNumber)
{
	int index = 0, argsToParse = 0;
	char temp[1024];

	index += ParseSpace(temp, line + index);

	size_t opcode = hashing::djb2::hash(std::string_view(temp));
	switch (opcode)
	{
		// E SAY {'up' event ID} {'ok' event ID} {talk ID 1} {talk ID 2} {talk ID 3} {talk ID 4} {talk ID 5} {talk ID 6} {talk ID 7} {talk ID 8}
		// The talk ID refers to the ID in the Quest_Talk TBL in the client.
		case "SAY"_djb2:
			m_Exec = EXEC_SAY;
			argsToParse = 10;
			break;

		// E SELECT_MSG {npc prototype ID - unused} {talk ID} ...
		// ... {menu ID 1} {event ID 1} {menu ID 2} {event ID 2} {menu ID 3} {event ID 3} {menu ID 4} {event ID 4} {menu ID 5} {event ID 5}
		// ... {menu ID 6} {event ID 6} {menu ID 7} {event ID 7} {menu ID 8} {event ID 8} {menu ID 9} {event ID 9} {menu ID 10} {event ID 10}
		// The talk ID refers to the ID in the Quest_Talk TBL in the client. This is used for the dialogue text.
		// The menu ID refers to the ID in the Quest_Menu TBL in the client. This is the button text.
		// The event ID refers to the associated event ID to run on the server when this button is pressed in the client.
		case "SELECT_MSG"_djb2:
			m_Exec = EXEC_SELECT_MSG;
			argsToParse = 22;
			break;

		// E RUN_EVENT {event ID}
		case "RUN_EVENT"_djb2:
			m_Exec = EXEC_RUN_EVENT;
			argsToParse = 1;
			break;

		// E GIVE_ITEM {item ID} {item count}
		case "GIVE_ITEM"_djb2:
			m_Exec = EXEC_GIVE_ITEM;
			argsToParse = 2;
			break;

		// E ROB_ITEM {item ID} {item count}
		case "ROB_ITEM"_djb2:
			m_Exec = EXEC_ROB_ITEM;
			argsToParse = 2;
			break;

		// E OPEN_EDITBOX {npc prototype ID - unused} {input message} {next event}
		case "OPEN_EDITBOX"_djb2:
			m_Exec = EXEC_OPEN_EDITBOX;
			argsToParse = 3;
			break;

		// E GIVE_NOAH {amount}
		case "GIVE_NOAH"_djb2:
			m_Exec = EXEC_GIVE_NOAH;
			argsToParse = 1;
			break;

		// E LOG_COUPON_ITEM {item ID} {item count}
		case "LOG_COUPON_ITEM"_djb2:
			m_Exec = EXEC_LOG_COUPON_ITEM;
			argsToParse = 2;
			break;

		case "RETURN"_djb2:
			m_Exec = EXEC_RETURN;
			break;

		default:
			spdlog::warn("EXEC::Parse: unhandled opcode '{}' ({}:{})", temp, WideToUtf8(filename), lineNumber);
			break;
	}

	_ASSERT(argsToParse >= 0 && argsToParse <= MAX_EXEC_INT);
	for (int i = 0; i < argsToParse; i++)
	{
		index += ParseSpace(temp, line + index);
		m_ExecInt[i] = atoi(temp);
	}
}

void EXEC::Init()
{
	for (int i = 0; i < MAX_EXEC_INT; i++)
		m_ExecInt[i] = -1;
}
