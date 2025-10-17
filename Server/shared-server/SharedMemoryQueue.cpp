﻿#include "stdafx.h"
#include "SharedMemoryQueue.h"

#include <boost/interprocess/ipc/message_queue.hpp>
#include <spdlog/spdlog.h>

using namespace boost::interprocess;

struct message_queue_impl : public message_queue
{
	using message_queue::message_queue;
};

SharedMemoryQueue::SharedMemoryQueue(int sendRetryCount /*= 0*/)
{
	_sendRetryCount = std::max(0, sendRetryCount);
}

bool SharedMemoryQueue::Create(const char* name, uint32_t maxMsgSize, uint32_t maxNumMsg)
{
	if (maxNumMsg < MinNumMsg)
	{
		spdlog::error("SharedMemoryQueue::Create: maxNumMsg too small. maxNumMsg={} name='{}'", maxNumMsg, name);
		return false;
	}

	try
	{
		_queue = std::make_unique<message_queue_impl>(create_only, name, maxNumMsg, maxMsgSize);

		// As with previous behaviour, as the expected 'creator' of the queue, flush it.
		FlushQueue();
		return true;
	}
	catch (const interprocess_exception& ex)
	{
		spdlog::error("SharedMemoryQueue::Create: failed to create shared memory. name='{}' ex='{}'", name, ex.what());
	}

	return false;
}

bool SharedMemoryQueue::OpenOrCreate(const char* name, uint32_t maxMsgSize, uint32_t maxNumMsg)
{
	if (maxNumMsg < MinNumMsg)
	{
		spdlog::error("SharedMemoryQueue::OpenOrCreate: maxNumMsg too small. maxNumMsg={} name='{}'", maxNumMsg, name);
		return false;
	}

	try
	{
		_queue = std::make_unique<message_queue_impl>(open_or_create, name, maxNumMsg, maxMsgSize);

		// As with previous behaviour, as the expected 'creator' of the queue, flush it, even if we just reopened it.
		FlushQueue();
		return true;
	}
	catch (const interprocess_exception& ex)
	{
		spdlog::error("SharedMemoryQueue::OpenOrCreate: failed to open or create shared memory. name='{}' ex='{}'", name, ex.what());
	}

	return false;
}

bool SharedMemoryQueue::Open(const char* name)
{
	try
	{
		_queue = std::make_unique<message_queue_impl>(open_only, name);
		return true;
	}
	catch (const interprocess_exception& ex)
	{
		spdlog::error("SharedMemoryQueue::Open: failed to open existing shared memory. name='{}' ex='{}'", name, ex.what());
	}

	return false;
}

int SharedMemoryQueue::PutData(const char* pBuf, int size)
{
	if (_queue == nullptr)
		return SMQ_GENERIC_ERROR;

	if (size > static_cast<int>(_queue->get_max_msg_size()))
	{
		spdlog::error("SharedMemoryQueue::PutData: data size overflow: {} bytes", size);
		return SMQ_PKTSIZEOVER;
	}
	
	const uint32_t priority = 0;
	int attemptCount = _sendRetryCount + 1;

	for (int i = 0; i < attemptCount; i++)
	{
		try
		{
			if (_queue->try_send(pBuf, size, priority))
				return SMQ_OK;
		}
		catch (interprocess_exception& ex)
		{
			spdlog::error("SharedMemoryQueue::PutData: fatal exception: {}", ex.what());
			return SMQ_GENERIC_ERROR;
		}

		std::this_thread::yield();
	}

	return SMQ_FULL;
}

int SharedMemoryQueue::GetData(char* pBuf)
{
	uint32_t receivedSize = 0;
	uint32_t priority = 0;

	try
	{
		if (!_queue->try_receive(pBuf, _queue->get_max_msg_size(), receivedSize, priority))
			return SMQ_EMPTY;
	}
	catch (interprocess_exception& ex)
	{
		spdlog::error("SharedMemoryQueue::GetData: fatal exception: {}", ex.what());
		return SMQ_GENERIC_ERROR;
	}

	// On success, return the number of bytes received
	return static_cast<int>(receivedSize);
}

void SharedMemoryQueue::FlushQueue()
{
	std::vector<char> buffer(_queue->get_max_msg_size());
	size_t received;
	uint32_t priority;

	while (true)
	{
		try
		{
			if (!_queue->try_receive(buffer.data(), buffer.size(), received, priority))
				break; // queue is now empty
		}
		catch (interprocess_exception&)
		{
			break;
		}
	}
}

SharedMemoryQueue::~SharedMemoryQueue()
{
}
