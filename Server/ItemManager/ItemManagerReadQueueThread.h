#pragma once

#include <shared-server/ReadQueueThread.h>

class ItemManagerReadQueueThread : public ReadQueueThread
{
public:
	ItemManagerReadQueueThread();

protected:
	void process_packet(const char* buffer, int len) override;
};
