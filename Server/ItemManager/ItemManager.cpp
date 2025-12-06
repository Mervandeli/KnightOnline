// ItemManager.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "ItemManagerApp.h"

int main()
{
	ItemManagerLogger logger;
	return AppThread::main<ItemManagerApp>(logger);
}
