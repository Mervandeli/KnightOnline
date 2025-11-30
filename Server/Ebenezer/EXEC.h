#pragma once

class EXEC
{
public:
	void Init();
	uint8_t m_Exec;
	int m_ExecInt[MAX_EXEC_INT];

	void Parse(const char* line, const std::string& filename, int lineNumber);
	EXEC();
	virtual ~EXEC();
};
