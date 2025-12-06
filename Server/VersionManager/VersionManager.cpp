// VersionManager.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "VersionManagerApp.h"

int main()
{
	logger::Logger logger(logger::VersionManager);
	return AppThread::main<VersionManagerApp>(logger);
}
