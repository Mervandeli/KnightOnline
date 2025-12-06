#pragma once

#include <shared/Thread.h>

class AIServerApp;
class ZoneEventThread : public Thread
{
public:
	ZoneEventThread();
	void thread_loop() override;

protected:
	AIServerApp* _main;
};
