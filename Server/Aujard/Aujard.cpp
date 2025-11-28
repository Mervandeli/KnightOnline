// Aujard.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "AujardInstance.h"

int main()
{
	logger::Logger logger(logger::Aujard);
	return AppThread::main<AujardInstance>(logger);
}
