// AIServer.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "AIServerApp.h"

int main()
{
	AIServerLogger logger;
	return AppThread::main<AIServerApp>(logger);
}
