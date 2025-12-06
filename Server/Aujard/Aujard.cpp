// Aujard.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "AujardApp.h"

int main()
{
	logger::Logger logger(logger::Aujard);
	return AppThread::main<AujardApp>(logger);
}
