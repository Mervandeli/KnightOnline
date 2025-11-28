#pragma once

#include <shared-server/ReadQueueThread.h>

class EbenezerReadQueueThread : public ReadQueueThread
{
public:
	EbenezerReadQueueThread();

protected:
	void process_packet(const char* buffer, int len) override;
};
