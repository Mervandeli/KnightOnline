// Ebenezer.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "EbenezerInstance.h"

int main()
{
	EbenezerLogger logger;
	return AppThread::main<EbenezerInstance>(logger);
}
