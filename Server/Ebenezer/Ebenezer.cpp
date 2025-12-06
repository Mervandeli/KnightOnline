// Ebenezer.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "EbenezerApp.h"

int main()
{
	EbenezerLogger logger;
	return AppThread::main<EbenezerApp>(logger);
}
