// ItemManager.cpp : contains the main() function to start the server
//

#include "pch.h"
#include "ItemManagerInstance.h"

int main()
{
	ItemManagerLogger logger;
	return AppThread::main<ItemManagerInstance>(logger);
}
