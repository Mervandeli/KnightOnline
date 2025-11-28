// AiServer.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "AiServerInstance.h"

int main()
{
	AIServerLogger logger;
	return AppThread::main<AiServerInstance>(logger);
}
