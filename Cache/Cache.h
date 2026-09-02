#pragma once
#include <mutex>

namespace Cache
{
	inline std::mutex Mutex;

	void Init();
	void Shutdown();
}